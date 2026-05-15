#pragma once
#include "../ICollector.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace metrics {
    class DiskCollector : public ICollector {
        public:
            CollectorResult collect(MetricSnapshot &snap);
        private:
            std::filesystem::path procPath = "/proc";
            std::unordered_map<std::string, std::pair<std::uint64_t, std::uint64_t>> _prevs;
            std::unordered_set<std::string> _allowedFs{
                "ext2", "ext3", "ext4", "xfs", "btrfs", "f2fs", "jfs", "reiserfs", "zfs"
            };
            bool _hasPrev = false;
    };
}