CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2

GTEST_CFLAGS := $(shell pkg-config --cflags gtest 2>/dev/null)
GTEST_LIBS := $(shell pkg-config --libs gtest 2>/dev/null)

ifeq ($(strip $(GTEST_LIBS)),)
GTEST_LIBS := -lgtest -lgtest_main -pthread
endif

TEST_BIN := test/test_tape
TEST_SRC := test/test_tape.cpp

.PHONY: test clean

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) src/tape.hpp
	$(CXX) $(CXXFLAGS) $(GTEST_CFLAGS) $(TEST_SRC) -o $(TEST_BIN) $(GTEST_LIBS)

clean:
	rm -f $(TEST_BIN)
