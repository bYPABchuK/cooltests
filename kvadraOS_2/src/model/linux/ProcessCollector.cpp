#include "ProcessCollector.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace metrics {
    bool ProcessCollector::_isPidDirName(const std::string& name) {
        if (name.empty()) {
            return false;
        }

        for (char ch : name) {
            if (!std::isdigit(static_cast<unsigned char>(ch))) {
                return false;
            }
        }
        return true;
    }

    std::uint64_t ProcessCollector::_readTotalCpuTime() const {
        std::ifstream file(procPath / "stat");
        if (!file.good()) {
            return 0;
        }

        std::string label;
        file >> label;
        if (label != "cpu") {
            return 0;
        }

        std::uint64_t total = 0;
        std::uint64_t value = 0;
        while (file >> value) {
            total += value;
        }
        return total;
    }

    std::optional<ProcessCollector::ProcRaw> ProcessCollector::_readStat(int pid) const {
        std::ifstream file(procPath / std::to_string(pid) / "stat");
        if (!file.good()) {
            return std::nullopt;
        }

        std::string line;
        getline(file, line);
        if (line.empty()) {
            return std::nullopt;
        }

        auto openPos = line.find('(');
        auto closePos = line.rfind(')');
        if (openPos == std::string::npos || closePos == std::string::npos || closePos <= openPos) {
            return std::nullopt;
        }

        ProcRaw proc;
        proc.pid = pid;
        proc.command = line.substr(openPos + 1, closePos - openPos - 1);

        std::string rest = line.substr(closePos + 2);
        std::istringstream ss(rest);
        std::vector<std::string> fields;
        std::string field;
        while (ss >> field) {
            fields.push_back(field);
        }

        if (fields.size() <= 17) {
            return std::nullopt;
        }

        try {
            std::uint64_t utime = static_cast<std::uint64_t>(std::stoull(fields[11]));
            std::uint64_t stime = static_cast<std::uint64_t>(std::stoull(fields[12]));
            proc.cpuTime = utime + stime;
            proc.nice = std::stoi(fields[16]);
            proc.threads = std::stoi(fields[17]);
        } catch (...) {
            return std::nullopt;
        }

        return proc;
    }

    void ProcessCollector::_readStatus(int pid, ProcRaw& proc) const {
        std::ifstream file(procPath / std::to_string(pid) / "status");
        if (!file.good()) {
            return;
        }

        std::string key;
        while (file >> key) {
            if (key == "Uid:") {
                file >> proc.uid;
                std::string rest;
                getline(file, rest);
            } else if (key == "VmRSS:") {
                std::uint64_t kb = 0;
                std::string unit;
                file >> kb >> unit;
                proc.rssBytes = kb * 1024;
            } else if (key == "Threads:") {
                int threads = 0;
                file >> threads;
                proc.threads = threads;
            } else {
                std::string rest;
                getline(file, rest);
            }
        }
    }

    void ProcessCollector::_readCmdline(int pid, ProcRaw& proc) const {
        std::ifstream file(procPath / std::to_string(pid) / "cmdline", std::ios::binary);
        if (!file.good()) {
            return;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (content.empty()) {
            return;
        }

        for (char& ch : content) {
            if (ch == '\0') {
                ch = ' ';
            }
        }

        while (!content.empty() && std::isspace(static_cast<unsigned char>(content.back()))) {
            content.pop_back();
        }

        if (!content.empty()) {
            proc.command = std::move(content);
        }
    }

    std::optional<ProcessCollector::ProcRaw> ProcessCollector::_readProcess(int pid) const {
        auto proc = _readStat(pid);
        if (!proc.has_value()) {
            return std::nullopt;
        }

        _readStatus(pid, *proc);
        _readCmdline(pid, *proc);
        return proc;
    }

    CollectorResult ProcessCollector::collect(MetricSnapshot& snap) {
        std::uint64_t totalCpuNow = _readTotalCpuTime();
        if (totalCpuNow == 0) {
            return CollectorResult::SourceError;
        }

        std::uint64_t totalMem = snap.mem.total;
        std::vector<ProcessMetrics> processes;
        std::unordered_map<int, std::uint64_t> nowProcTimes;

        double totalCpuDelta = _hasPrev && totalCpuNow > _prevTotalCpuTime
            ? static_cast<double>(totalCpuNow - _prevTotalCpuTime)
            : 0.0;
        double cpuCount = snap.cpu.perCoreUsage.empty()
            ? 1.0
            : static_cast<double>(snap.cpu.perCoreUsage.size());

        for (const auto& entry : std::filesystem::directory_iterator(procPath)) {
            if (!entry.is_directory()) {
                continue;
            }

            std::string name = entry.path().filename().string();
            if (!_isPidDirName(name)) {
                continue;
            }

            int pid = 0;
            try {
                pid = std::stoi(name);
            } catch (...) {
                continue;
            }

            auto raw = _readProcess(pid);
            if (!raw.has_value()) {
                continue;
            }

            nowProcTimes[pid] = raw->cpuTime;

            ProcessMetrics proc;
            proc.pid = raw->pid;
            proc.nice = raw->nice;
            proc.command = std::move(raw->command);
            proc.threads = raw->threads;
            proc.uid = raw->uid;
            proc.rssBytes = raw->rssBytes;

            if (totalMem > 0) {
                proc.memPercent = static_cast<double>(raw->rssBytes)
                    / static_cast<double>(totalMem) * 100.0;
            }

            if (_hasPrev && totalCpuDelta > 0.0) {
                auto prevIt = _prevProcTimes.find(pid);
                if (prevIt != _prevProcTimes.end() && raw->cpuTime >= prevIt->second) {
                    std::uint64_t procDelta = raw->cpuTime - prevIt->second;
                    proc.cpuPercent = static_cast<double>(procDelta)
                        / totalCpuDelta * 100.0 * cpuCount;
                }
            }

            processes.push_back(std::move(proc));
        }

        std::sort(processes.begin(), processes.end(), [](const ProcessMetrics& a, const ProcessMetrics& b) {
            if (a.cpuPercent == b.cpuPercent) {
                return a.memPercent > b.memPercent;
            }
            return a.cpuPercent > b.cpuPercent;
        });

        if (processes.size() > _limit) {
            processes.resize(_limit);
        }

        snap.processes = std::move(processes);
        _prevProcTimes = std::move(nowProcTimes);
        _prevTotalCpuTime = totalCpuNow;
        _hasPrev = true;

        return CollectorResult::ok;
    }
}
