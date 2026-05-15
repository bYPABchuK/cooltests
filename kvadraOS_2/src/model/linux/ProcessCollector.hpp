#pragma once

#include "../ICollector.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace metrics {
    class ProcessCollector : public ICollector {
        public:
            CollectorResult collect(MetricSnapshot& snap);

        private:
            struct ProcRaw {
                int pid = 0;
                int nice = 0;
                int threads = 0;
                std::uint32_t uid = 0;
                std::string command;
                std::uint64_t rssBytes = 0;
                std::uint64_t cpuTime = 0;
            };

            static bool _isPidDirName(const std::string& name);
            std::uint64_t _readTotalCpuTime() const;
            std::optional<ProcRaw> _readProcess(int pid) const;
            std::optional<ProcRaw> _readStat(int pid) const;
            void _readStatus(int pid, ProcRaw& proc) const;
            void _readCmdline(int pid, ProcRaw& proc) const;

            std::filesystem::path procPath = "/proc";
            std::unordered_map<int, std::uint64_t> _prevProcTimes;
            std::uint64_t _prevTotalCpuTime = 0;
            bool _hasPrev = false;
            std::size_t _limit = 50;
    };
}
