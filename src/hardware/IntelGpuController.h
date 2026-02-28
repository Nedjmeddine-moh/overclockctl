#pragma once
#include <string>
#include <optional>

struct IntelGpuInfo {
    std::string name;
    long cur_freq_mhz;
    long min_freq_mhz;
    long max_freq_mhz;
    long boost_freq_mhz;
    long hw_max_freq_mhz;
    long hw_min_freq_mhz;
    std::optional<double> temp_celsius;
};

class IntelGpuController {
public:
    IntelGpuController();
    bool isAvailable() const { return m_available; }
    IntelGpuInfo getInfo();
    bool setMaxFreq(long mhz);
    bool setMinFreq(long mhz);
    bool setBoostFreq(long mhz);

private:
    bool m_available = false;
    std::string m_drm_path;   // e.g. /sys/class/drm/card0
    std::string m_hwmon_path; // for temp
    void probe();
};
