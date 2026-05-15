#include "Webserver.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <fstream>
#include <sstream>

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace control {
    boost::json::object Webserver::_toJsonObject(const metrics::MetricSnapshot& snap) const {
        boost::json::object root;
        root["timestamp"] = static_cast<std::int64_t>(snap.timestamp);

        boost::json::object cpu;
        cpu["totalUsage"] = snap.cpu.totalUsage;
        cpu["loadAvg1"] = snap.cpu.loadAvg1;
        cpu["loadAvg5"] = snap.cpu.loadAvg5;
        cpu["loadAvg15"] = snap.cpu.loadAvg15;
        boost::json::array perCore;
        for (double v : snap.cpu.perCoreUsage) {
            perCore.push_back(v);
        }
        cpu["perCoreUsage"] = std::move(perCore);
        root["cpu"] = std::move(cpu);

        boost::json::object mem;
        mem["total"] = snap.mem.total;
        mem["used"] = snap.mem.used;
        mem["free"] = snap.mem.free;
        mem["available"] = snap.mem.available;
        root["mem"] = std::move(mem);

        boost::json::array disks;
        for (const auto& d : snap.disks) {
            boost::json::object obj;
            obj["mountPoint"] = d.mountPoint;
            obj["total"] = d.total;
            obj["used"] = d.used;
            obj["readBytesPerSec"] = d.readBytesPerSec;
            obj["writeBytesPerSec"] = d.writeBytesPerSec;
            disks.push_back(std::move(obj));
        }
        root["disks"] = std::move(disks);

        boost::json::array processes;
        for (const auto& p : snap.processes) {
            boost::json::object obj;
            obj["pid"] = p.pid;
            obj["nice"] = p.nice;
            obj["command"] = p.command;
            obj["threads"] = p.threads;
            obj["uid"] = p.uid;
            obj["memPercent"] = p.memPercent;
            obj["cpuPercent"] = p.cpuPercent;
            obj["rssBytes"] = p.rssBytes;
            processes.push_back(std::move(obj));
        }
        root["processes"] = std::move(processes);

        return root;
    }

    boost::json::array Webserver::_toJsonArray(const std::vector<metrics::MetricSnapshot>& snaps) const {
        boost::json::array arr;
        for (const auto& snap : snaps) {
            arr.push_back(_toJsonObject(snap));
        }
        return arr;
    }

    std::string Webserver::_readTextFile(const std::string& path) const {
        std::ifstream file(path);
        if (!file.good()) {
            return "";
        }
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
            if (snap.has_value()) {
                res.body() = boost::json::serialize(_toJsonObject(*snap));
            } else {
                res.body() = "{}";
            }
        });

        _router.addGet("/api/history", [this](const HttpRouter::Request&, HttpRouter::Response& res) {
            auto snaps = _storage->snapshots();
            res.result(http::status::ok);
            res.set(http::field::content_type, "application/json");
            res.body() = boost::json::serialize(_toJsonArray(snaps));
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
