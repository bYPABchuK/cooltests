#include "MemCollector.hpp"
#include <fstream>

namespace metrics {
    CollectorResult MemCollector::collect(MetricSnapshot &snap) {
        std::ifstream meminfo(procPath / "meminfo");
        if (!meminfo.good()) {
            return CollectorResult::SourceError;
        }

        std::string key;
        std::uint64_t value = 0;
        std::string kb;
        std::uint64_t total = 0;
        std::uint64_t free = 0;
        std::uint64_t available = 0;

        while (meminfo >> key >> value >> kb) {
            if (key == "MemTotal:") {
                total = value * 1024;
            } else if (key == "MemFree:") {
                free = value * 1024;
            } else if (key == "MemAvailable:") {
                available = value * 1024;
            }
        }

        if (total == 0 || available == 0) {
            return CollectorResult::SourceError;
        }

        snap.mem.total = total;
        snap.mem.free = free;
        snap.mem.available = available;
        snap.mem.used = total - available;

        return CollectorResult::ok;
    }
}