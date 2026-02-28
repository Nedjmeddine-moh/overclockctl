#include "IntelGpuController.h"
#include "../utils/SysfsHelper.h"
#include <filesystem>

namespace fs = std::filesystem;

IntelGpuController::IntelGpuController() {
    probe();
}

void IntelGpuController::probe() {
    // Find an Intel GPU DRM card (i915 driver)
    for (auto& card : SysfsHelper::glob("/sys/class/drm/card[0-9]")) {
        auto driver_link = card + "/device/driver";
        if (fs::exists(driver_link)) {
            auto target = fs::read_symlink(driver_link).filename().string();
            if (target == "i915") {
                m_drm_path = card;
                m_available = true;
                break;
            }
        }
    }
    if (!m_available) return;

    // Find hwmon for GPU temp (i915 exposes one)
    auto card_dev = m_drm_path + "/device";
    for (auto& hwmon : SysfsHelper::glob(card_dev + "/hwmon/hwmon*")) {
        m_hwmon_path = hwmon;
        break;
    }
}

IntelGpuInfo IntelGpuController::getInfo() {
    IntelGpuInfo info;
    info.name = "Intel Integrated GPU (i915)";

    auto read_mhz = [&](const std::string& file) -> long {
        return SysfsHelper::readLong(m_drm_path + "/" + file).value_or(0);
    };

    info.cur_freq_mhz   = read_mhz("gt_cur_freq_mhz");
    info.min_freq_mhz   = read_mhz("gt_min_freq_mhz");
    info.max_freq_mhz   = read_mhz("gt_max_freq_mhz");
    info.boost_freq_mhz = read_mhz("gt_boost_freq_mhz");
    info.hw_max_freq_mhz= read_mhz("gt_RP0_freq_mhz");  // RP0 = max hardware turbo
    info.hw_min_freq_mhz= read_mhz("gt_RPn_freq_mhz");  // RPn = min hardware freq

    if (!m_hwmon_path.empty()) {
        auto t = SysfsHelper::readLong(m_hwmon_path + "/temp1_input");
        if (t) info.temp_celsius = *t / 1000.0;
    }

    return info;
}

bool IntelGpuController::setMaxFreq(long mhz) {
    return SysfsHelper::writeLong(m_drm_path + "/gt_max_freq_mhz", mhz);
}

bool IntelGpuController::setMinFreq(long mhz) {
    return SysfsHelper::writeLong(m_drm_path + "/gt_min_freq_mhz", mhz);
}

bool IntelGpuController::setBoostFreq(long mhz) {
    return SysfsHelper::writeLong(m_drm_path + "/gt_boost_freq_mhz", mhz);
}
