#pragma once
#include "MetricSnapshot.hpp"

namespace metrics {
    enum CollectorResult {
        ok,
        SourceError,
    };

    class ICollector {
        public:
        virtual ~ICollector() = default;

        virtual CollectorResult collect(MetricSnapshot& snap) = 0;
    };
}