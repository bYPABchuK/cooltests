#pragma once

#include "HttpRouter.hpp"
#include "../model/MetricStorage.hpp"
#include <boost/json.hpp>
#include <memory>
#include <string>

namespace control {
    class Webserver {
        public:
            Webserver(std::shared_ptr<metrics::MetricStorage> storage,
                      std::string host = "0.0.0.0",
                      unsigned short port = 8080)
                : _storage(std::move(storage)), _host(std::move(host)), _port(port) {}

            void run();

        private:
            boost::json::object _toJsonObject(const metrics::MetricSnapshot& snap) const;
            boost::json::array _toJsonArray(const std::vector<metrics::MetricSnapshot>& snaps) const;
            std::string _readTextFile(const std::string& path) const;

            std::shared_ptr<metrics::MetricStorage> _storage;
            std::string _host;
            unsigned short _port;
            std::string _uiRoot = "src/UI";
            HttpRouter _router;
    };
}