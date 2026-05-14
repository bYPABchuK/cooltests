#pragma once

#include "tape.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <vector>

namespace tape {
    enum class SortResult {
        Ok = 0,
        NotEnoughMemory,
        TapeError,
        TempTapeError,
        OutputSizeMismatch
    };

    struct RunInfo {
        std::filesystem::path path;
        std::size_t size = 0;
    };

    struct HeapNode {
        int value = 0;
        std::size_t runIndex = 0;
    };

    struct HeapNodeGreater {
        bool operator()(const HeapNode& lhs, const HeapNode& rhs) const;
    };

    class TapeSorter {
    public:
        TapeSorter(std::size_t memoryLimitBytes, std::filesystem::path tmpDir);
        SortResult sort(ITape& input, ITape& output);

    private:
        std::size_t _memoryLimitBytes = 0;
        std::filesystem::path _tmpDir = "tmp";
        std::size_t _tmpCounter = 0;

        std::size_t chunkCapacity() const;
        std::size_t mergeFanIn() const;
        std::filesystem::path makeTmpPath();

        static void removeRuns(const std::vector<RunInfo>& runs);
        std::optional<std::vector<RunInfo>> createInitialRuns(ITape& input);
        std::optional<std::vector<RunInfo>> mergePass(const std::vector<RunInfo>& runs);
        std::optional<RunInfo> copyRunToNewRun(const RunInfo& run);
        std::optional<RunInfo> mergeBatch(const std::vector<RunInfo>& runs);
        static SortResult copyRunToOutput(const RunInfo& run, ITape& output);
    };

    SortResult sort(ITape& in, ITape& out, std::size_t memLim, std::filesystem::path tmpDir);

    inline SortResult sort(ITape& in, ITape& out, std::size_t memLim) {
        return sort(in, out, memLim, "tmp");
    }
}