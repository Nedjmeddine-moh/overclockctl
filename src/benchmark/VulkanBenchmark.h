#pragma once
#include <string>
#include <functional>

struct GpuBenchResult {
    double gflops       = 0;
    double score        = 0;      // normalized score
    double duration_sec = 0;
    std::string gpu_name;
    std::string driver_version;
    std::string api_version;
    bool   completed    = false;
    std::string error_msg;
};

using ProgressCb = std::function<void(double, const std::string&)>;

class VulkanBenchmark {
public:
    VulkanBenchmark();
    ~VulkanBenchmark();

    bool isVulkanAvailable() const { return m_available; }
    GpuBenchResult run(ProgressCb progress = nullptr);

private:
    bool m_available = false;
    std::string m_gpu_name;
    std::string m_driver_ver;
    std::string m_api_ver;
    void probe();
};
