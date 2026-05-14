#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <thread>

namespace tape {
    enum class TapeResult {
        Ok = 0,
        OpenError,
        SizeError,
        BadFormat,
        ReadError,
        WriteError,
        LeftBoundary,
        RightBoundary
    };

    class ITape {
    public:
        virtual ~ITape() = default;

        virtual std::optional<int> read() = 0;
        virtual TapeResult write(int input) = 0;

        virtual TapeResult moveLeft() = 0;
        virtual TapeResult moveRight() = 0;
        virtual TapeResult rewind() = 0;

        virtual size_t size() = 0;
    };

    class FileTape : public ITape {
    public:
        FileTape() = default;

        static std::optional<FileTape> open(const std::filesystem::path& path) {
            FileTape tape;

            TapeResult status = tape.openImpl(path);
            if (status != TapeResult::Ok) {
                return std::nullopt;
            }

            return tape;
        }

        static std::optional<FileTape> create(const std::filesystem::path& path, std::size_t size, int defVal) {
            FileTape tape;

            TapeResult status = tape.createImpl(path, size, defVal);
            if (status != TapeResult::Ok) {
                return std::nullopt;
            }

            return tape;
        }

        std::optional<int> read() override {
            if (!_file.is_open()) {
                return std::nullopt;
            }

            int value{};

            _file.clear();
            _file.seekg(_cursor);

            if (!_file.read(reinterpret_cast<char*>(&value), sizeof(value))) {
                return std::nullopt;
            }

            return value;
        }

        TapeResult write(int input) override {
            if (!_file.is_open()) {
                return TapeResult::OpenError;
            }

            _file.clear();
            _file.seekp(_cursor);

            if (!_file.write(reinterpret_cast<const char*>(&input), sizeof(input))) {
                return TapeResult::WriteError;
            }

            _file.flush();

            if (!_file) {
                return TapeResult::WriteError;
            }

            return TapeResult::Ok;
        }

        TapeResult moveLeft() override {
            if (_cursor < cellSize) {
                return TapeResult::LeftBoundary;
            }

            _cursor -= cellSize;
            return TapeResult::Ok;
        }

        TapeResult moveRight() override {
            if (_size == 0) {
                return TapeResult::RightBoundary;
            }

            const std::streamoff lastCell = static_cast<std::streamoff>(_size - 1) * cellSize;

            if (_cursor >= lastCell) {
                return TapeResult::RightBoundary;
            }

            _cursor += cellSize;
            return TapeResult::Ok;
        }

        TapeResult rewind() override {
            _cursor = 0;
            return TapeResult::Ok;
        }

        std::size_t size() override {
            return _size;
        }

        std::size_t positionIndex() const {
            return static_cast<std::size_t>(_cursor / cellSize);
        }

    private:
        std::fstream _file;
        std::streamoff _cursor = 0;
        std::size_t _size = 0;


        static constexpr std::streamoff cellSize = static_cast<std::streamoff>(sizeof(int));

        TapeResult openImpl(const std::filesystem::path& path) {
            _file.open(path, std::ios::in | std::ios::out | std::ios::binary);

            if (!_file.is_open()) {
                return TapeResult::OpenError;
            }

            _file.clear();
            _file.seekg(0, std::ios::end);

            const auto end = _file.tellg();

            if (end < 0) {
                return TapeResult::SizeError;
            }

            if (end % cellSize != 0) {
                return TapeResult::BadFormat;
            }

            _size = static_cast<std::size_t>(end / cellSize);
            _cursor = 0;

            _file.clear();
            _file.seekg(0, std::ios::beg);
            _file.seekp(0, std::ios::beg);

            return TapeResult::Ok;
        }

        TapeResult createImpl(const std::filesystem::path& path, std::size_t cellCount, int defaultValue) {
            _file.open(path, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
                            
            if (!_file.is_open()) {
                return TapeResult::OpenError;
            }
        
            for (std::size_t i = 0; i < cellCount; ++i) {
                _file.write(reinterpret_cast<const char*>(&defaultValue), sizeof(defaultValue));
            
                if (!_file) {
                    return TapeResult::WriteError;
                }
            }
        
            _file.flush();
            if (!_file) {
                return TapeResult::WriteError;
            }
        
            _size = cellCount;
            _cursor = 0;
        
            _file.clear();
            _file.seekg(0, std::ios::beg);
            _file.seekp(0, std::ios::beg);
        
            return TapeResult::Ok;
        }
    };

    using Delay = std::chrono::milliseconds;

    class SlowedTape : public ITape {
        public:
            SlowedTape(std::unique_ptr<ITape> impl, Delay readDelay, Delay writeDelay, Delay leftDelay, Delay rightDelay, Delay rewindDelay) :
                _impl(std::move(impl)), _readDelay(readDelay), _writeDelay(writeDelay),
                _leftDelay(leftDelay), _rightDelay(rightDelay), _rewindDelay(rewindDelay) {}

            std::optional<int> read() override {
                std::this_thread::sleep_for(_readDelay);
                return _impl->read();
            }

            TapeResult write(int input) override {
                std::this_thread::sleep_for(_writeDelay);
                return _impl->write(input);
            }

            TapeResult moveLeft() override {
                std::this_thread::sleep_for(_leftDelay);
                return _impl->moveLeft();
            }

            TapeResult moveRight() override {
                std::this_thread::sleep_for(_rightDelay);
                return _impl->moveRight();
            }

            TapeResult rewind() override {
                std::this_thread::sleep_for(_rewindDelay);
                return _impl->rewind();
            }

            std::size_t size() override {
                return _impl->size();
            }

        private:
            std::unique_ptr<ITape> _impl;
            Delay _readDelay;
            Delay _writeDelay;
            Delay _leftDelay;
            Delay _rightDelay;
            Delay _rewindDelay;
    };
}