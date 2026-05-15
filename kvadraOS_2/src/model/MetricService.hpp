#pragma once

#include "ICollector.hpp"
#include "MetricSnapshot.hpp"
#include "MetricStorage.hpp"
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace metrics {
    class MetricService {
        public:
            MetricService(std::shared_ptr<MetricStorage> storage) : _storage(std::move(storage)) {}

            void addCollector(ICollector* collector) {
                _collectors.push_back(collector);
            }

            void start() {
                MetricSnapshot snap;
                if (!_started) {
                    _started = 1;
                }

                while (_started) {
                    for (const auto& c : _collectors) {
                        c->collect(snap);
                    }

                    _storage->push(snap);
                    std::this_thread::sleep_for(sampleInterval_);
                }
            }

            void stop() {
                if (_started) {
                    _started = 0;
                }
            }

        private:
            std::atomic_bool _started = 0;
            std::chrono::milliseconds sampleInterval_{1000};

            std::vector<ICollector*> _collectors;
            std::shared_ptr<MetricStorage> _storage;
    };
}