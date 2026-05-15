#include "CpuCollector.hpp"
#include <fstream>

namespace metrics {
    CollectorResult CpuCollector::collect(MetricSnapshot &snap) {
        std::ifstream cpustats( procPath / "stat");
        if (!cpustats.good()) {
            return CollectorResult::SourceError;
        }
        if (_hasPrev) {
            snap.cpu.perCoreUsage.clear();
        }

        std::string line; bool first = 1; int i = 0; std::string cpuLabel;
        for(;getline(cpustats, line) && !(line.rfind("intr", 0) == 0); i++) {
            std::istringstream ss(line);
            CpuTimes times;
            ss >> cpuLabel;
            ss >> times.user
                >> times.nice
                >> times.system
                >> times.idle
                >> times.iowait
                >> times.irq
                >> times.softirq
                >> times.steal
                >> times.guest
                >> times.guestNice;

            std::uint64_t _idleNow = times.idleAll();
            std::uint64_t _totalNow = times.nonIdle() + _idleNow;
            if (_prevs.size() <= static_cast<size_t>(i)) {
                _prevs.resize(i + 1, {0, 0});
            }

            std::uint64_t totalDelta = _totalNow - _prevs[i].first;
            std::uint64_t idleDelta = _idleNow - _prevs[i].second;
            double usage = totalDelta == 0 ? 0.0 : static_cast<double>(totalDelta - idleDelta) 
                    / static_cast<double>(totalDelta) * 100;

            if (_hasPrev) {
                if (first) {
                    snap.cpu.totalUsage = usage;
                    first = 0;
                } else {
                    snap.cpu.perCoreUsage.push_back(usage);
                }
            }

            _prevs[i].first = _totalNow;
            _prevs[i].second = _idleNow;
        }
        
        std::ifstream loadAvgStat(procPath / "loadavg");
        if (!loadAvgStat.good()) {
            return CollectorResult::SourceError;
        }
        if (!getline(loadAvgStat, line)) {
            return CollectorResult::SourceError;
        }

        std::istringstream ss(line);
        if (_hasPrev) {
            ss >> snap.cpu.loadAvg1
                >> snap.cpu.loadAvg5
                >> snap.cpu.loadAvg15;
        }
        
        _hasPrev = true;
        return CollectorResult::ok;
    }
}