CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2

GTEST_CFLAGS := $(shell pkg-config --cflags gtest 2>/dev/null)
GTEST_LIBS := $(shell pkg-config --libs gtest 2>/dev/null)

ifeq ($(strip $(GTEST_LIBS)),)
GTEST_LIBS := -lgtest -lgtest_main -pthread
endif

TEST_BIN := test/test_tape
TEST_SRC := test/test_tape.cpp
TEST_EXTRA_SRC := src/sort.cpp
APP_BIN := bin/tape_sort
APP_SRC := src/main.cpp
APP_EXTRA_SRC := src/sort.cpp

.PHONY: test app clean

test: $(TEST_BIN)
	./$(TEST_BIN)

app: $(APP_BIN)

$(APP_BIN): $(APP_SRC) $(APP_EXTRA_SRC) src/tape.hpp src/sort.hpp src/parser.hpp
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(APP_SRC) $(APP_EXTRA_SRC) -o $(APP_BIN)

$(TEST_BIN): $(TEST_SRC) $(TEST_EXTRA_SRC) src/tape.hpp src/sort.hpp src/parser.hpp
	$(CXX) $(CXXFLAGS) $(GTEST_CFLAGS) $(TEST_SRC) $(TEST_EXTRA_SRC) -o $(TEST_BIN) $(GTEST_LIBS)

clean:
	rm -f $(TEST_BIN) $(APP_BIN)
