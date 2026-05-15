#include "DiskCollector.hpp"
#include <fstream>
#include <sstream>

namespace metrics {
    CollectorResult DiskCollector::collect(MetricSnapshot &snap) {
        std::ifstream diskstats(procPath / "diskstats");
        if (!diskstats.good()) {
            return CollectorResult::SourceError;
        }

        std::unordered_map<std::string, std::pair<std::uint64_t, std::uint64_t>> nowStats;
        std::string line;
        while (getline(diskstats, line)) {
            std::istringstream ss(line);
            std::uint64_t major = 0;
            std::uint64_t minor = 0;
            std::string dev;
            std::uint64_t readsCompleted = 0;
            std::uint64_t readsMerged = 0;
            std::uint64_t sectorsRead = 0;
            std::uint64_t readMs = 0;
            std::uint64_t writesCompleted = 0;
            std::uint64_t writesMerged = 0;
            std::uint64_t sectorsWritten = 0;
            std::uint64_t writeMs = 0;

            ss >> major >> minor >> dev
               >> readsCompleted >> readsMerged >> sectorsRead >> readMs
               >> writesCompleted >> writesMerged >> sectorsWritten >> writeMs;
            if (dev.empty()) {
                continue;
            }

            nowStats[dev] = {sectorsRead * 512, sectorsWritten * 512};
        }

        std::ifstream mounts(procPath / "mounts");
        if (!mounts.good()) {
            return CollectorResult::SourceError;
        }

        snap.disks.clear();

        while (getline(mounts, line)) {
            std::istringstream ss(line);
            std::string device;
            std::string mountPoint;
            std::string fsType;
            std::string options;
            int dump = 0;
            int pass = 0;

            ss >> device >> mountPoint >> fsType >> options >> dump >> pass;
            if (device.rfind("/dev/", 0) != 0) {
                continue;
            }
            if (_allowedFs.find(fsType) == _allowedFs.end()) {
                continue;
            }

            std::string devName = device.substr(5);
            auto statIt = nowStats.find(devName);
            if (statIt == nowStats.end()) {
                continue;
            }

            std::error_code ec;
            auto fs = std::filesystem::space(mountPoint, ec);
            if (ec) {
                continue;
            }

            DiskMetrics disk;
            disk.mountPoint = mountPoint;
            disk.total = fs.capacity;
            disk.used = fs.capacity - fs.available;

            if (_hasPrev) {
                auto prevIt = _prevs.find(devName);
                if (prevIt != _prevs.end()) {
                    disk.readBytesPerSec = statIt->second.first - prevIt->second.first;
                    disk.writeBytesPerSec = statIt->second.second - prevIt->second.second;
                }
            }

            snap.disks.push_back(disk);
            _prevs[devName] = statIt->second;
        }

        _hasPrev = true;
        return CollectorResult::ok;
    }
}