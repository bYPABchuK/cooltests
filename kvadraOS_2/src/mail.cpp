#include "control/Webserver.hpp"
#include "model/MetricService.hpp"
#include "model/MetricStorage.hpp"
#include "model/linux/CpuCollector.hpp"
#include "model/linux/MemCollector.hpp"
#include "model/linux/DiskCollector.hpp"
#include "model/linux/ProcessCollector.hpp"

#include <memory>
#include <thread>

int main() {
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

    control::Webserver webserver(storage, "0.0.0.0", 8080);
    webserver.run();

    return 0;
}