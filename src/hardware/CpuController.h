#pragma once
#include "../utils/Platform.h"
#include <string>
#include <vector>
#include <optional>

struct CpuInfo {
    std::string model_name;
    int core_count       = 0;
    int thread_count     = 0;
    long base_freq_khz   = 0;
    long max_freq_khz    = 0;
    long min_freq_khz    = 0;
    long cur_freq_khz    = 0;
    std::string governor;                         // Linux only
    std::vector<std::string> available_governors; // Linux only
    std::string power_plan;                       // Windows only
    std::optional<double> voltage_v;
};

class CpuController {
public:
    CpuController();
    CpuInfo getInfo();
    std::vector<long> getCoreFrequencies();

    bool setMaxFreq(long khz);
    bool setMinFreq(long khz);
    bool setGovernor(const std::string& gov);     // Linux
    bool setPowerPlan(const std::string& guid);   // Windows

    long getHardwareMaxFreq() const { return m_hw_max_khz; }
    long getHardwareMinFreq() const { return m_hw_min_khz; }

private:
    int  m_core_count  = 0;
    long m_hw_max_khz  = 0;
    long m_hw_min_khz  = 0;
    void probe();
};
