#include "HwMonitor.h"
#include "../utils/SysfsHelper.h"
#include <algorithm>

#ifdef OC_PLATFORM_LINUX
#include <filesystem>
namespace fs = std::filesystem;

void HwMonitor::discoverHwmon() {
    m_hwmon_paths.clear();
    for (auto& p : SysfsHelper::glob("/sys/class/hwmon/hwmon*"))
        m_hwmon_paths.push_back(p);
    std::sort(m_hwmon_paths.begin(), m_hwmon_paths.end());
}

HwMonitor::HwMonitor() { discoverHwmon(); }

std::vector<TempReading> HwMonitor::getTemperatures() {
    std::vector<TempReading> readings;
    for (auto& base : m_hwmon_paths) {
        auto name = SysfsHelper::readFile(base + "/name").value_or("unknown");
        for (int i = 1; i <= 10; ++i) {
            auto val = SysfsHelper::readLong(base + "/temp" + std::to_string(i) + "_input");
            if (!val) break;
            auto label = SysfsHelper::readFile(base + "/temp" + std::to_string(i) + "_label")
                             .value_or(name + " temp" + std::to_string(i));
            readings.push_back({label, *val / 1000.0});
        }
    }
    return readings;
}

std::vector<FanReading> HwMonitor::getFans() {
    std::vector<FanReading> fans;
    for (auto& base : m_hwmon_paths) {
        for (int i = 1; i <= 5; ++i) {
            auto rpm = SysfsHelper::readLong(base + "/fan" + std::to_string(i) + "_input");
            if (!rpm) continue;
            FanReading fan;
            fan.path_base  = base;
            fan.index      = i;
            fan.rpm        = (int)*rpm;
            std::string pp = base + "/pwm" + std::to_string(i);
            auto pwm       = SysfsHelper::readLong(pp);
            fan.has_pwm    = pwm.has_value();
            fan.pwm        = (int)pwm.value_or(255);
            fan.pwm_enable = (int)SysfsHelper::readLong(pp + "_enable").value_or(2);
            fans.push_back(fan);
        }
    }
    return fans;
}

bool HwMonitor::setFanPwm(const FanReading& fan, int pwm) {
    return SysfsHelper::writeLong(fan.path_base + "/pwm" + std::to_string(fan.index),
                                   std::clamp(pwm, 0, 255));
}

bool HwMonitor::setFanMode(const FanReading& fan, int mode) {
    return SysfsHelper::writeLong(fan.path_base + "/pwm" + std::to_string(fan.index) + "_enable", mode);
}
#endif // OC_PLATFORM_LINUX

#ifdef OC_PLATFORM_WINDOWS
// Windows: fan control not available without vendor SDKs; return empty/stub
HwMonitor::HwMonitor() {}

std::vector<TempReading> HwMonitor::getTemperatures() {
    // WMI MSAcpi_ThermalZoneTemperature could be used here;
    // returning a stub to keep the UI functional without WMI boilerplate
    return {};
}

std::vector<FanReading> HwMonitor::getFans() { return {}; }
bool HwMonitor::setFanPwm(const FanReading&, int) { return false; }
bool HwMonitor::setFanMode(const FanReading&, int) { return false; }
#endif // OC_PLATFORM_WINDOWS
