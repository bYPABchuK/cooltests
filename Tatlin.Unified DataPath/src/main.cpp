#include "sort.hpp"
#include "parser.hpp"
#include "tape.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Error: need more tapes\n";
        return -1;
    }
    AppConfig c;

    if (argc < 4) {
        std::cerr << "Warning: will be used default values\n";
    } else {
        loadConfig(argv[3], c);
    }

    auto inOpt  = tape::FileTape::open(argv[1]);
    auto outOpt = tape::FileTape::open(argv[2]);
    if (!inOpt || !outOpt) {
        std::cerr << "Error: incorrect tapes\n";
        return -1;
    }

    tape::SlowedTape in(std::make_unique<tape::FileTape>(std::move(*inOpt)),
        c.readDelay, c.writeDelay, c.moveLeftDelay, c.moveRightDelay, c.rewindDelay);
    tape::SlowedTape out(std::make_unique<tape::FileTape>(std::move(*outOpt)),
        c.readDelay, c.writeDelay, c.moveLeftDelay, c.moveRightDelay, c.rewindDelay);

    auto result = tape::sort(in, out, c.memoryLimitBytes);

    switch (result) {
        case tape::SortResult::Ok:
            std::cout << "Sort completed successfully\n" << std::endl;
            break;

        case tape::SortResult::NotEnoughMemory:
            std::cerr << "Sort error: not enough memory limit for external sort\n";
            return (int)tape::SortResult::NotEnoughMemory;

        case tape::SortResult::OutputSizeMismatch:
            std::cerr << "Sort error: input and output tape sizes differ\n";
            return (int)tape::SortResult::OutputSizeMismatch;

        case tape::SortResult::TapeError:
            std::cerr << "Sort error: tape read/write/move operation failed\n";
            return (int)tape::SortResult::TapeError;

        case tape::SortResult::TempTapeError:
            std::cerr << "Sort error: temporary tape operation failed\n";
            return (int)tape::SortResult::TempTapeError;
        default:
            std::cerr << "Sort error: unknown result\n";
            return -1;
    }

    out.rewind();
    for (std::size_t i = 0; i < out.size(); ++i) {
        std::cout << out.read().value() << ' ';
        if (i + 1 < out.size()) {
            out.moveRight();
        }
    }

    return 0;
}