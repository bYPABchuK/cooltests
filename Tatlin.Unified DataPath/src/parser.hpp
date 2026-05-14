#pragma once

#include <charconv>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

struct AppConfig {
    std::size_t memoryLimitBytes = 1024 * 1024;
    std::filesystem::path tmpDir = "/tmp/";

    std::chrono::milliseconds readDelay{0};
    std::chrono::milliseconds writeDelay{0};
    std::chrono::milliseconds moveLeftDelay{0};
    std::chrono::milliseconds moveRightDelay{0};
    std::chrono::milliseconds rewindDelay{0};
};

enum class ConfigResult {
    Ok,
    OpenError,
    BadLine,
    BadNumber,
    BadValue
};

static std::string_view trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }

    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }

    return s;
}

static bool parseSize(std::string_view s, std::size_t& out) {
    s = trim(s);

    if (s.empty()) {
        return false;
    }

    std::size_t value = 0;

    auto* begin = s.data();
    auto* end = s.data() + s.size();

    auto result = std::from_chars(begin, end, value);

    if (result.ec != std::errc{} || result.ptr != end) {
        return false;
    }

    out = value;
    return true;
}

static bool parseInt64(std::string_view s, std::int64_t& out) {
    s = trim(s);

    if (s.empty()) {
        return false;
    }

    std::int64_t value = 0;

    auto* begin = s.data();
    auto* end = s.data() + s.size();

    auto result = std::from_chars(begin, end, value);

    if (result.ec != std::errc{} || result.ptr != end) {
        return false;
    }

    out = value;
    return true;
}

static ConfigResult applyConfigLine(std::string_view key, std::string_view value, AppConfig& config) {
    key = trim(key);
    value = trim(value);

    if (key == "memory_limit_bytes") {
        std::size_t parsed = 0;

        if (!parseSize(value, parsed)) {
            return ConfigResult::BadNumber;
        }

        if (parsed < sizeof(int)) {
            return ConfigResult::BadValue;
        }

        config.memoryLimitBytes = parsed;
        return ConfigResult::Ok;
    }

    if (key == "tmp_dir") {
        if (value.empty()) {
            return ConfigResult::BadValue;
        }

        config.tmpDir = std::filesystem::path(std::string(value));
        return ConfigResult::Ok;
    }

    auto parseDelay = [&](std::chrono::milliseconds& target) -> ConfigResult {
        std::int64_t ms = 0;

        if (!parseInt64(value, ms)) {
            return ConfigResult::BadNumber;
        }

        if (ms < 0) {
            return ConfigResult::BadValue;
        }

        target = std::chrono::milliseconds(ms);
        return ConfigResult::Ok;
    };

    if (key == "read_delay_ms") {
        return parseDelay(config.readDelay);
    }

    if (key == "write_delay_ms") {
        return parseDelay(config.writeDelay);
    }

    if (key == "move_left_delay_ms") {
        return parseDelay(config.moveLeftDelay);
    }

    if (key == "move_right_delay_ms") {
        return parseDelay(config.moveRightDelay);
    }

    if (key == "rewind_delay_ms") {
        return parseDelay(config.rewindDelay);
    }
    return ConfigResult::BadLine;
}

static ConfigResult loadConfig(const std::filesystem::path& path, AppConfig& config) {
    std::ifstream in(path);

    if (!in.is_open()) {
        return ConfigResult::OpenError;
    }

    std::string line;

    while (std::getline(in, line)) {
        std::string_view view(line);

        if (auto comment = view.find('#'); comment != std::string_view::npos) {
            view = view.substr(0, comment);
        }

        view = trim(view);

        if (view.empty()) {
            continue;
        }

        auto eq = view.find('=');

        if (eq == std::string_view::npos) {
            return ConfigResult::BadLine;
        }

        auto key = view.substr(0, eq);
        auto value = view.substr(eq + 1);

        auto result = applyConfigLine(key, value, config);

        if (result != ConfigResult::Ok) {
            return result;
        }
    }

    return ConfigResult::Ok;
}