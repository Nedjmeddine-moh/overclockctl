#pragma once
#include "../utils/Platform.h"
#include <string>
#include <vector>
#include <optional>

struct TempReading {
    std::string label;
    double celsius;
};

struct FanReading {
    std::string path_base;
    int index;
    int rpm;
    bool has_pwm;
    int pwm;
    int pwm_enable;
};

class HwMonitor {
public:
    HwMonitor();
    std::vector<TempReading> getTemperatures();
    std::vector<FanReading>  getFans();
    bool setFanPwm(const FanReading& fan, int pwm);
    bool setFanMode(const FanReading& fan, int mode);

private:
#ifdef OC_PLATFORM_LINUX
    std::vector<std::string> m_hwmon_paths;
    void discoverHwmon();
#endif
};
