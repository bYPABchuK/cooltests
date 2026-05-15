#pragma once
#include "../ICollector.hpp"
#include <filesystem>

namespace metrics {
    class MemCollector : public ICollector {
        public:
            CollectorResult collect(MetricSnapshot &snap);
        private:
            std::filesystem::path procPath = "/proc";
    };
}
