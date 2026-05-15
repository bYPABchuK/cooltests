#pragma once

#include "MetricSnapshot.hpp"
#include "../../include/ringbuffer.hpp"
#include <cstddef>
#include <mutex>
#include <optional>
#include <shared_mutex>

namespace metrics {
    class MetricStorage {
        public:
            MetricStorage(std::size_t bufSize) : _buff(bufSize) {}

            void push(const MetricSnapshot& snapshot) {
                std::unique_lock lock(_mutex);
                _buff.push(snapshot);
            }

            std::optional<MetricSnapshot> lastSnap() const {
                std::shared_lock lock(_mutex);
                return _buff.last();
            }
        
            std::vector<MetricSnapshot> snapshots() const {
                std::shared_lock lock(_mutex);
                return _buff.values();
            }
        private:
            mutable std::shared_mutex _mutex;
            algorithm::RingBuffer<MetricSnapshot> _buff;
    };
}