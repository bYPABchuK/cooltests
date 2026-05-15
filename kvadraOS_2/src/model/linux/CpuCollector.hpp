#pragma once
#include "../ICollector.hpp"
#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

namespace metrics {
    struct CpuTimes {
        std::uint64_t user = 0;
        std::uint64_t nice = 0;
        std::uint64_t system = 0;
        std::uint64_t idle = 0;
        std::uint64_t iowait = 0;
        std::uint64_t irq = 0;
        std::uint64_t softirq = 0;
        std::uint64_t steal = 0;
        std::uint64_t guest = 0;
        std::uint64_t guestNice = 0;

        std::uint64_t idleAll() const {
            return idle + iowait;
        }

        std::uint64_t nonIdle() const {
            return user + nice + system + irq + softirq + steal;
        }
    };

    class CpuCollector : public ICollector {
        public:
            CpuCollector() : _prevs() {}

            CollectorResult collect(MetricSnapshot &snap);
        private:
            std::vector<std::pair<std::uint64_t, std::uint64_t>> _prevs;
            std::filesystem::path procPath = "/proc";
            bool _hasPrev = false;
    };
}