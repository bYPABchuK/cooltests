#include "control/Webserver.hpp"
#include "model/MetricService.hpp"
#include "model/MetricStorage.hpp"
#include "model/linux/CpuCollector.hpp"
#include "model/linux/MemCollector.hpp"
#include "model/linux/DiskCollector.hpp"
#include "model/linux/ProcessCollector.hpp"

#include <iostream>
#include <memory>
#include <thread>

int main(int argc, char** argv) {
    auto storage = std::make_shared<metrics::MetricStorage>(256);

    metrics::CpuCollector cpuCollector;
    metrics::MemCollector memCollector;
    metrics::DiskCollector diskCollector;
    metrics::ProcessCollector processCollector;

    metrics::MetricService metricService(storage);
    metricService.addCollector(&cpuCollector);
    metricService.addCollector(&memCollector);
    metricService.addCollector(&diskCollector);
    metricService.addCollector(&processCollector);

    std::thread metricsThread([&metricService]() {
        metricService.start();
    });
    metricsThread.detach();

    std::size_t port = 8080;
    
    if (argc >= 2) {
        char* endptr;
        unsigned long val = std::strtoul(argv[1], &endptr, 10);
        
        if (*endptr != '\0') {
            std::cerr << "Invalid port number: " << argv[1] << std::endl;
            return 1;
        }
        
        if (val > 65535) {
            std::cerr << "Port out of range (0-65535): " << val << std::endl;
            return 1;
        }
        
        port = static_cast<std::size_t>(val);
    }

    control::Webserver webserver(storage, port);
    std::cout << "Server launch on 127.0.0.1:" << port << '\n';
    webserver.run();

    return 0;
}