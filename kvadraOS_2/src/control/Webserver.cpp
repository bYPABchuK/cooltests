#include "Webserver.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <fstream>
#include <iomanip>
#include <sstream>

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace control {
    std::string Webserver::_toJson(const metrics::MetricSnapshot& snap) const {
        std::ostringstream ss;
        ss << "{";
        ss << "\"timestamp\":" << snap.timestamp << ",";

        ss << "\"cpu\":{";
        ss << "\"totalUsage\":" << snap.cpu.totalUsage << ",";
        ss << "\"loadAvg1\":" << snap.cpu.loadAvg1 << ",";
        ss << "\"loadAvg5\":" << snap.cpu.loadAvg5 << ",";
        ss << "\"loadAvg15\":" << snap.cpu.loadAvg15 << ",";
        ss << "\"perCoreUsage\":[";
        for (std::size_t i = 0; i < snap.cpu.perCoreUsage.size(); ++i) {
            if (i) ss << ",";
            ss << snap.cpu.perCoreUsage[i];
        }
        ss << "]},";

        ss << "\"mem\":{";
        ss << "\"total\":" << snap.mem.total << ",";
        ss << "\"used\":" << snap.mem.used << ",";
        ss << "\"free\":" << snap.mem.free << ",";
        ss << "\"available\":" << snap.mem.available;
        ss << "},";

        ss << "\"disks\":[";
        for (std::size_t i = 0; i < snap.disks.size(); ++i) {
            if (i) ss << ",";
            const auto& d = snap.disks[i];
            ss << "{";
            ss << "\"mountPoint\":\"" << _jsonEscape(d.mountPoint) << "\",";
            ss << "\"total\":" << d.total << ",";
            ss << "\"used\":" << d.used << ",";
            ss << "\"readBytesPerSec\":" << d.readBytesPerSec << ",";
            ss << "\"writeBytesPerSec\":" << d.writeBytesPerSec;
            ss << "}";
        }
        ss << "],";

        ss << "\"processes\":[";
        for (std::size_t i = 0; i < snap.processes.size(); ++i) {
            if (i) ss << ",";
            const auto& p = snap.processes[i];
            ss << "{";
            ss << "\"pid\":" << p.pid << ",";
            ss << "\"nice\":" << p.nice << ",";
            ss << "\"command\":\"" << _jsonEscape(p.command) << "\",";
            ss << "\"threads\":" << p.threads << ",";
            ss << "\"uid\":" << p.uid << ",";
            ss << "\"memPercent\":" << p.memPercent << ",";
            ss << "\"cpuPercent\":" << p.cpuPercent << ",";
            ss << "\"rssBytes\":" << p.rssBytes;
            ss << "}";
        }
        ss << "]";

        ss << "}";
        return ss.str();
    }

    std::string Webserver::_toJson(const std::vector<metrics::MetricSnapshot>& snaps) const {
        std::ostringstream ss;
        ss << "[";
        for (std::size_t i = 0; i < snaps.size(); ++i) {
            if (i) ss << ",";
            ss << _toJson(snaps[i]);
        }
        ss << "]";
        return ss.str();
    }

    std::string Webserver::_jsonEscape(const std::string& input) const {
        std::ostringstream out;
        for (unsigned char ch : input) {
            switch (ch) {
                case '"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default:
                    if (ch < 0x20) {
                        out << "\\u" << std::hex << std::uppercase << std::setw(4)
                            << std::setfill('0') << static_cast<int>(ch) << std::dec;
                    } else {
                        out << static_cast<char>(ch);
                    }
            }
        }
        return out.str();
    }

    std::string Webserver::_readTextFile(const std::string& path) const {
        std::ifstream file(path);
        if (!file.good()) return "";
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    void Webserver::run() {
        _router.addGet("/", [this](const HttpRouter::Request&, HttpRouter::Response& res) {
            auto html = _readTextFile(_uiRoot + "/index.html");
            if (html.empty()) {
                res.result(http::status::not_found);
                res.set(http::field::content_type, "text/plain");
                res.body() = "index.html not found";
            } else {
                res.result(http::status::ok);
                res.set(http::field::content_type, "text/html; charset=utf-8");
                res.body() = html;
            }
        });

        _router.addGet("/style.css", [this](const HttpRouter::Request&, HttpRouter::Response& res) {
            auto css = _readTextFile(_uiRoot + "/style.css");
            if (css.empty()) {
                res.result(http::status::not_found);
                res.set(http::field::content_type, "text/plain");
                res.body() = "style.css not found";
            } else {
                res.result(http::status::ok);
                res.set(http::field::content_type, "text/css; charset=utf-8");
                res.body() = css;
            }
        });

        _router.addGet("/script.js", [this](const HttpRouter::Request&, HttpRouter::Response& res) {
            auto js = _readTextFile(_uiRoot + "/script.js");
            if (js.empty()) {
                res.result(http::status::not_found);
                res.set(http::field::content_type, "text/plain");
                res.body() = "script.js not found";
            } else {
                res.result(http::status::ok);
                res.set(http::field::content_type, "application/javascript; charset=utf-8");
                res.body() = js;
            }
        });

        _router.addGet("/api/metrics", [this](const HttpRouter::Request&, HttpRouter::Response& res) {
            auto snap = _storage->lastSnap();
            res.result(http::status::ok);
            res.set(http::field::content_type, "application/json");
            res.body() = snap.has_value() ? _toJson(*snap) : "{}";
        });

        _router.addGet("/api/history", [this](const HttpRouter::Request&, HttpRouter::Response& res) {
            auto snaps = _storage->snapshots();
            res.result(http::status::ok);
            res.set(http::field::content_type, "application/json");
            res.body() = _toJson(snaps);
        });

        net::io_context ioc{1};
        tcp::acceptor acceptor(ioc, {net::ip::make_address(_host), _port});

        for (;;) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);

            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            http::read(socket, buffer, req);

            http::response<http::string_body> res;
            res.version(req.version());
            res.keep_alive(false);

            if (!_router.route(req, res)) {
                if (req.method() != http::verb::get) {
                    res.result(http::status::bad_request);
                    res.set(http::field::content_type, "text/plain");
                    res.body() = "Only GET is supported";
                } else {
                    res.result(http::status::not_found);
                    res.set(http::field::content_type, "text/plain");
                    res.body() = "Not found";
                }
            }

            res.prepare_payload();
            http::write(socket, res);

            beast::error_code ec;
            socket.shutdown(tcp::socket::shutdown_send, ec);
        }
    }
}
