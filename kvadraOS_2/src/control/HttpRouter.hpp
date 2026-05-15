#pragma once

#include <boost/beast/http.hpp>

#include <functional>
#include <string>
#include <unordered_map>

namespace control {
    namespace http = boost::beast::http;

    class HttpRouter {
        public:
            using Request = http::request<http::string_body>;
            using Response = http::response<http::string_body>;
            using Handler = std::function<void(const Request&, Response&)>;

            void addGet(const std::string& path, Handler handler) {
                _get[path] = std::move(handler);
            }

            bool route(const Request& req, Response& res) const {
                if (req.method() != http::verb::get) {
                    return false;
                }

                std::string target = std::string(req.target());
                auto it = _get.find(target);
                if (it == _get.end()) {
                    return false;
                }

                it->second(req, res);
                return true;
            }

        private:
            std::unordered_map<std::string, Handler> _get;
    };
}
