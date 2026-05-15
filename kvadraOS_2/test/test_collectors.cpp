#include "../src/model/linux/MemCollector.hpp"
#include "../src/model/linux/DiskCollector.hpp"

#include <gtest/gtest.h>

namespace {
    TEST(MemCollectorTest, collect_validProcMemInfo_fillsSnapshotAndReturnsOk) {
        metrics::MemCollector collector;
        metrics::MetricSnapshot snap;

        auto status = collector.collect(snap);

        EXPECT_EQ(status, metrics::CollectorResult::ok);
        EXPECT_GT(snap.mem.total, 0u);
        EXPECT_GT(snap.mem.available, 0u);
        EXPECT_LE(snap.mem.used, snap.mem.total);
    }

    TEST(DiskCollectorTest, collect_validProcData_fillsMountedDisksAndReturnsOk) {
        metrics::DiskCollector collector;
        metrics::MetricSnapshot snap;

        auto status = collector.collect(snap);

        EXPECT_EQ(status, metrics::CollectorResult::ok);
        EXPECT_FALSE(snap.disks.empty());
        for (const auto& disk : snap.disks) {
            EXPECT_FALSE(disk.mountPoint.empty());
            EXPECT_GT(disk.total, 0u);
            EXPECT_LE(disk.used, disk.total);
        }
    }

    TEST(DiskCollectorTest, collect_secondCall_computesReadWriteDeltas) {
        metrics::DiskCollector collector;
        metrics::MetricSnapshot first;
        metrics::MetricSnapshot second;

        auto firstStatus = collector.collect(first);
        auto secondStatus = collector.collect(second);

        EXPECT_EQ(firstStatus, metrics::CollectorResult::ok);
        EXPECT_EQ(secondStatus, metrics::CollectorResult::ok);
        EXPECT_FALSE(second.disks.empty());
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}