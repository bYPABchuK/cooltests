#include "sort.hpp"

#include <algorithm>
#include <queue>
#include <string>
#include <system_error>
#include <utility>

namespace tape {
    bool HeapNodeGreater::operator()(const HeapNode& lhs, const HeapNode& rhs) const {
        return lhs.value > rhs.value;
    }

    TapeSorter::TapeSorter(std::size_t memoryLimitBytes, std::filesystem::path tmpDir)
        : _memoryLimitBytes(memoryLimitBytes), _tmpDir(std::move(tmpDir)) {
    }

    SortResult TapeSorter::sort(ITape& input, ITape& output) {
        if (_memoryLimitBytes < sizeof(int) * 2) {
            return SortResult::NotEnoughMemory;
        }

        const std::size_t inputSize = input.size();
        const std::size_t outputSize = output.size();

        if (inputSize != outputSize) {
            return SortResult::OutputSizeMismatch;
        }

        std::error_code ec;
        std::filesystem::create_directories(_tmpDir, ec);
        if (ec) {
            return SortResult::TempTapeError;
        }

        auto runsOpt = createInitialRuns(input);
        if (!runsOpt.has_value()) {
            return SortResult::TempTapeError;
        }

        auto runs = std::move(*runsOpt);

        while (runs.size() > 1) {
            auto mergedOpt = mergePass(runs);
            if (!mergedOpt.has_value()) {
                removeRuns(runs);
                return SortResult::TempTapeError;
            }

            removeRuns(runs);
            runs = std::move(*mergedOpt);
        }

        if (runs.empty()) {
            return SortResult::Ok;
        }

        auto copyResult = copyRunToOutput(runs.front(), output);
        removeRuns(runs);
        return copyResult;
    }

    std::size_t TapeSorter::chunkCapacity() const {
        return std::max<std::size_t>(1, (_memoryLimitBytes / 2) / sizeof(int));
    }

    std::size_t TapeSorter::mergeFanIn() const {
        return std::max<std::size_t>(2, (_memoryLimitBytes / 2) / sizeof(HeapNode));
    }

    std::filesystem::path TapeSorter::makeTmpPath() {
        return _tmpDir / ("run_" + std::to_string(_tmpCounter++) + ".bin");
    }

    void TapeSorter::removeRuns(const std::vector<RunInfo>& runs) {
        for (const auto& run : runs) {
            std::error_code ec;
            std::filesystem::remove(run.path, ec);
        }
    }

    std::optional<std::vector<RunInfo>> TapeSorter::createInitialRuns(ITape& input) {
        std::vector<RunInfo> runs;
        std::vector<int> buffer;
        buffer.reserve(chunkCapacity());

        if (input.rewind() != TapeResult::Ok) {
            return std::nullopt;
        }

        const std::size_t total = input.size();
        std::size_t processed = 0;

        while (processed < total) {
            buffer.clear();

            while (buffer.size() < chunkCapacity() && processed < total) {
                auto value = input.read();
                if (!value.has_value()) {
                    return std::nullopt;
                }

                buffer.push_back(*value);
                ++processed;

                if (processed < total && input.moveRight() != TapeResult::Ok) {
                    return std::nullopt;
                }
            }

            std::sort(buffer.begin(), buffer.end());

            auto runPath = makeTmpPath();
            auto runTapeOpt = FileTape::create(runPath, buffer.size(), 0);
            if (!runTapeOpt.has_value()) {
                return std::nullopt;
            }

            auto& runTape = *runTapeOpt;
            for (std::size_t i = 0; i < buffer.size(); ++i) {
                if (runTape.write(buffer[i]) != TapeResult::Ok) {
                    return std::nullopt;
                }

                if (i + 1 < buffer.size() && runTape.moveRight() != TapeResult::Ok) {
                    return std::nullopt;
                }
            }

            runs.push_back({runPath, buffer.size()});
        }

        return runs;
    }

    std::optional<std::vector<RunInfo>> TapeSorter::mergePass(const std::vector<RunInfo>& runs) {
        std::vector<RunInfo> result;
        const std::size_t fanIn = mergeFanIn();

        for (std::size_t begin = 0; begin < runs.size(); begin += fanIn) {
            const std::size_t end = std::min(begin + fanIn, runs.size());
            std::vector<RunInfo> batch(runs.begin() + static_cast<std::ptrdiff_t>(begin),
                                       runs.begin() + static_cast<std::ptrdiff_t>(end));

            if (batch.size() == 1) {
                auto copied = copyRunToNewRun(batch.front());
                if (!copied.has_value()) {
                    return std::nullopt;
                }

                result.push_back(*copied);
                continue;
            }

            auto merged = mergeBatch(batch);
            if (!merged.has_value()) {
                return std::nullopt;
            }

            result.push_back(*merged);
        }

        return result;
    }

    std::optional<RunInfo> TapeSorter::copyRunToNewRun(const RunInfo& run) {
        auto srcOpt = FileTape::open(run.path);
        if (!srcOpt.has_value()) {
            return std::nullopt;
        }

        auto dstPath = makeTmpPath();
        auto dstOpt = FileTape::create(dstPath, run.size, 0);
        if (!dstOpt.has_value()) {
            return std::nullopt;
        }

        auto& src = *srcOpt;
        auto& dst = *dstOpt;

        for (std::size_t i = 0; i < run.size; ++i) {
            auto value = src.read();
            if (!value.has_value()) {
                return std::nullopt;
            }

            if (dst.write(*value) != TapeResult::Ok) {
                return std::nullopt;
            }

            if (i + 1 < run.size) {
                if (src.moveRight() != TapeResult::Ok || dst.moveRight() != TapeResult::Ok) {
                    return std::nullopt;
                }
            }
        }

        return RunInfo{dstPath, run.size};
    }

    std::optional<RunInfo> TapeSorter::mergeBatch(const std::vector<RunInfo>& runs) {
        std::vector<FileTape> tapes;
        tapes.reserve(runs.size());

        std::vector<std::size_t> positions(runs.size(), 0);
        std::size_t totalSize = 0;

        for (const auto& run : runs) {
            totalSize += run.size;
            auto tapeOpt = FileTape::open(run.path);
            if (!tapeOpt.has_value()) {
                return std::nullopt;
            }

            tapes.push_back(std::move(*tapeOpt));
        }

        auto outPath = makeTmpPath();
        auto outOpt = FileTape::create(outPath, totalSize, 0);
        if (!outOpt.has_value()) {
            return std::nullopt;
        }
        auto& outTape = *outOpt;

        std::priority_queue<HeapNode, std::vector<HeapNode>, HeapNodeGreater> heap;

        for (std::size_t i = 0; i < tapes.size(); ++i) {
            if (runs[i].size == 0) {
                continue;
            }

            auto value = tapes[i].read();
            if (!value.has_value()) {
                return std::nullopt;
            }

            heap.push({*value, i});
        }

        std::size_t written = 0;
        while (!heap.empty()) {
            HeapNode node = heap.top();
            heap.pop();

            if (outTape.write(node.value) != TapeResult::Ok) {
                return std::nullopt;
            }

            ++written;
            if (written < totalSize && outTape.moveRight() != TapeResult::Ok) {
                return std::nullopt;
            }

            const std::size_t idx = node.runIndex;
            ++positions[idx];

            if (positions[idx] < runs[idx].size) {
                if (tapes[idx].moveRight() != TapeResult::Ok) {
                    return std::nullopt;
                }

                auto nextValue = tapes[idx].read();
                if (!nextValue.has_value()) {
                    return std::nullopt;
                }

                heap.push({*nextValue, idx});
            }
        }

        return RunInfo{outPath, totalSize};
    }

    SortResult TapeSorter::copyRunToOutput(const RunInfo& run, ITape& output) {
        auto inOpt = FileTape::open(run.path);
        if (!inOpt.has_value()) {
            return SortResult::TempTapeError;
        }

        auto& in = *inOpt;

        if (output.rewind() != TapeResult::Ok) {
            return SortResult::TapeError;
        }

        for (std::size_t i = 0; i < run.size; ++i) {
            auto value = in.read();
            if (!value.has_value()) {
                return SortResult::TapeError;
            }

            if (output.write(*value) != TapeResult::Ok) {
                return SortResult::TapeError;
            }

            if (i + 1 < run.size) {
                if (in.moveRight() != TapeResult::Ok || output.moveRight() != TapeResult::Ok) {
                    return SortResult::TapeError;
                }
            }
        }

        return SortResult::Ok;
    }

    SortResult sort(ITape& in, ITape& out, std::size_t memLim, std::filesystem::path tmpDir) {
        TapeSorter sorter(memLim, std::move(tmpDir));
        return sorter.sort(in, out);
    }
}
