#include "../src/tape.hpp"

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
    struct TempFile {
        std::filesystem::path path;

        explicit TempFile(const std::string& name)
            : path(std::filesystem::temp_directory_path() / name) {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }

        ~TempFile() {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
    };

    void writeIntsBinary(const std::filesystem::path& path, const std::vector<int>& values) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(out.is_open());

        for (int value : values) {
            out.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }
        out.close();
    }

    void writeRawBytes(const std::filesystem::path& path, const std::vector<unsigned char>& bytes) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        out.close();
    }
}

TEST(FileTapeTest, open_validBinaryFile_returnsTape) {
    TempFile file("tape_test_open_success.bin");
    writeIntsBinary(file.path, {10, 20, 30});

    auto tape = tape::FileTape::open(file.path);

    ASSERT_TRUE(tape.has_value());
    EXPECT_EQ(tape->size(), 3u);
    EXPECT_EQ(tape->positionIndex(), 0u);
}

TEST(FileTapeTest, open_nonExistingFile_returnsNullopt) {
    TempFile file("tape_test_open_missing.bin");

    auto tape = tape::FileTape::open(file.path);

    EXPECT_FALSE(tape.has_value());
}

TEST(FileTapeTest, open_malformedFileSize_returnsNullopt) {
    TempFile file("tape_test_bad_format.bin");
    writeRawBytes(file.path, {0x01, 0x02, 0x03});

    auto tape = tape::FileTape::open(file.path);

    EXPECT_FALSE(tape.has_value());
}

TEST(FileTapeTest, read_afterOpenAtFirstCell_returnsFirstValue) {
    TempFile file("tape_test_read_first.bin");
    writeIntsBinary(file.path, {111, 222});
    auto tape = tape::FileTape::open(file.path);
    ASSERT_TRUE(tape.has_value());

    auto value = tape->read();

    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 111);
}

TEST(FileTapeTest, moveRight_thenRead_returnsNextCellValue) {
    TempFile file("tape_test_move_right_read.bin");
    writeIntsBinary(file.path, {7, 8, 9});
    auto tape = tape::FileTape::open(file.path);
    ASSERT_TRUE(tape.has_value());

    auto moveStatus = tape->moveRight();
    auto value = tape->read();

    EXPECT_EQ(moveStatus, tape::TapeResult::Ok);
    EXPECT_EQ(tape->positionIndex(), 1u);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 8);
}

TEST(FileTapeTest, moveLeft_atLeftBoundary_returnsLeftBoundary) {
    TempFile file("tape_test_left_boundary.bin");
    writeIntsBinary(file.path, {5, 6});
    auto tape = tape::FileTape::open(file.path);
    ASSERT_TRUE(tape.has_value());

    auto status = tape->moveLeft();

    EXPECT_EQ(status, tape::TapeResult::LeftBoundary);
    EXPECT_EQ(tape->positionIndex(), 0u);
}

TEST(FileTapeTest, moveRight_atRightBoundary_returnsRightBoundary) {
    TempFile file("tape_test_right_boundary.bin");
    writeIntsBinary(file.path, {1, 2});
    auto tape = tape::FileTape::open(file.path);
    ASSERT_TRUE(tape.has_value());
    ASSERT_EQ(tape->moveRight(), tape::TapeResult::Ok);
    ASSERT_EQ(tape->positionIndex(), 1u);

    auto status = tape->moveRight();

    EXPECT_EQ(status, tape::TapeResult::RightBoundary);
    EXPECT_EQ(tape->positionIndex(), 1u);
}

TEST(FileTapeTest, moveRight_onEmptyTape_returnsRightBoundary) {
    TempFile file("tape_test_empty.bin");
    writeIntsBinary(file.path, {});
    auto tape = tape::FileTape::open(file.path);
    ASSERT_TRUE(tape.has_value());
    ASSERT_EQ(tape->size(), 0u);

    auto status = tape->moveRight();

    EXPECT_EQ(status, tape::TapeResult::RightBoundary);
    EXPECT_EQ(tape->positionIndex(), 0u);
}

TEST(FileTapeTest, write_validValueAtCurrentCell_returnsOkAndUpdatesValue) {
    TempFile file("tape_test_write.bin");
    writeIntsBinary(file.path, {10, 20, 30});
    auto tape = tape::FileTape::open(file.path);
    ASSERT_TRUE(tape.has_value());
    ASSERT_EQ(tape->moveRight(), tape::TapeResult::Ok);

    auto writeStatus = tape->write(999);
    auto value = tape->read();

    EXPECT_EQ(writeStatus, tape::TapeResult::Ok);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 999);
}

TEST(FileTapeTest, readWrite_defaultConstructedTape_readNulloptWriteOpenError) {
    tape::FileTape tape;

    auto readValue = tape.read();
    auto writeStatus = tape.write(123);

    EXPECT_FALSE(readValue.has_value());
    EXPECT_EQ(writeStatus, tape::TapeResult::OpenError);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
