CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I./src -I./include
LDFLAGS := -lgtest -pthread
WEB_LDFLAGS := -pthread

TEST_BIN := bin/test_collectors
TEST_SRCS := \
	test/test_collectors.cpp \
	src/model/linux/MemCollector.cpp \
	src/model/linux/DiskCollector.cpp

.PHONY: all test clean

WEB_BIN := bin/webui
WEB_SRCS := \
	src/mail.cpp \
	src/control/Webserver.cpp \
	src/model/linux/CpuCollector.cpp \
	src/model/linux/MemCollector.cpp \
	src/model/linux/DiskCollector.cpp \
	src/model/linux/ProcessCollector.cpp

ui:
	npx tsc -p src/UI/tsconfig.json

$(WEB_BIN): $(WEB_SRCS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(WEB_SRCS) -o $(WEB_BIN) $(WEB_LDFLAGS)

webui: ui $(WEB_BIN)

$(TEST_BIN): $(TEST_SRCS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $(TEST_BIN) $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf bin