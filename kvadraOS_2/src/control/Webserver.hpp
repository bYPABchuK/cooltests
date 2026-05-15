#pragma once

#include "HttpRouter.hpp"
#include "../model/MetricStorage.hpp"
#include <memory>
#include <string>

namespace control {
    class Webserver {
        public:
            Webserver(std::shared_ptr<metrics::MetricStorage> storage,
                      unsigned short port = 8080)
                : _storage(std::move(storage)), _host("0.0.0.0"), _port(port) {}

            void run();

        private:
            std::string _toJson(const metrics::MetricSnapshot& snap) const;
            std::string _toJson(const std::vector<metrics::MetricSnapshot>& snaps) const;
            std::string _jsonEscape(const std::string& input) const;
            std::string _readTextFile(const std::string& path) const;

            std::shared_ptr<metrics::MetricStorage> _storage;
            std::string _host;
            unsigned short _port;
            std::string _uiRoot = "src/UI";
            HttpRouter _router;
    };
}