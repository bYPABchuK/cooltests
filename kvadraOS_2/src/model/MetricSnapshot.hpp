#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <ctime>

namespace metrics {
    struct CpuMetrics {
        double totalUsage = 0.0;
        std::vector<double> perCoreUsage;
        double loadAvg1 = 0.0;
        double loadAvg5 = 0.0;
        double loadAvg15 = 0.0;
    };
    
    struct MemMetrics {
        std::uint64_t total = 0;
        std::uint64_t used = 0;
        std::uint64_t free = 0;
        std::uint64_t available = 0;
    };
    
    struct DiskMetrics {
        std::string mountPoint;
        std::uint64_t total = 0;
        std::uint64_t used = 0;
        std::uint64_t readBytesPerSec = 0;
        std::uint64_t writeBytesPerSec = 0;
    };

    struct ProcessMetrics {
        int pid = 0;
        int nice = 0;
        std::string command;
        int threads = 0;
        std::uint32_t uid = 0;

        double memPercent = 0.0;
        double cpuPercent = 0.0;

        std::uint64_t rssBytes = 0;
    };
    
    struct MetricSnapshot {
        time_t timestamp = time(NULL);
    
        CpuMetrics cpu;
        MemMetrics mem;
        std::vector<DiskMetrics> disks;
        std::vector<ProcessMetrics> processes;
    };
}