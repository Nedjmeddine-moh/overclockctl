#include "CpuController.h"
#include "../utils/SysfsHelper.h"
#include <sstream>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
//  LINUX implementation
// ─────────────────────────────────────────────────────────────────────────────
#ifdef OC_PLATFORM_LINUX
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

void CpuController::probe() {
    m_core_count = 0;
    while (fs::exists("/sys/devices/system/cpu/cpu" + std::to_string(m_core_count)))
        m_core_count++;
    m_hw_max_khz = SysfsHelper::readLong(
        "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq").value_or(0);
    m_hw_min_khz = SysfsHelper::readLong(
        "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq").value_or(0);
}

CpuInfo CpuController::getInfo() {
    CpuInfo info;
    info.core_count   = m_core_count;
    info.thread_count = m_core_count;

    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.rfind("model name", 0) == 0) {
            auto pos = line.find(':');
            if (pos != std::string::npos) info.model_name = line.substr(pos + 2);
            break;
        }
    }

    info.max_freq_khz  = SysfsHelper::readLong("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq").value_or(0);
    info.min_freq_khz  = SysfsHelper::readLong("/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq").value_or(0);
    info.base_freq_khz = m_hw_max_khz;

    long sum = 0; int n = 0;
    for (int i = 0; i < m_core_count; ++i) {
        auto f = SysfsHelper::readLong(
            "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_cur_freq");
        if (f) { sum += *f; n++; }
    }
    info.cur_freq_khz = n > 0 ? sum / n : 0;

    info.governor = SysfsHelper::readFile(
        "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor").value_or("unknown");
    auto gs = SysfsHelper::readFile(
        "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors").value_or("");
    std::istringstream iss(gs);
    std::string g;
    while (iss >> g) info.available_governors.push_back(g);
    return info;
}

std::vector<long> CpuController::getCoreFrequencies() {
    std::vector<long> freqs;
    for (int i = 0; i < m_core_count; ++i) {
        auto f = SysfsHelper::readLong(
            "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_cur_freq");
        freqs.push_back(f.value_or(0));
    }
    return freqs;
}

bool CpuController::setMaxFreq(long khz) {
    bool ok = true;
    for (int i = 0; i < m_core_count; ++i)
        ok &= SysfsHelper::writeLong(
            "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_max_freq", khz);
    return ok;
}

bool CpuController::setMinFreq(long khz) {
    bool ok = true;
    for (int i = 0; i < m_core_count; ++i)
        ok &= SysfsHelper::writeLong(
            "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_min_freq", khz);
    return ok;
}

bool CpuController::setGovernor(const std::string& gov) {
    bool ok = true;
    for (int i = 0; i < m_core_count; ++i)
        ok &= SysfsHelper::writeFile(
            "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_governor", gov);
    return ok;
}
bool CpuController::setPowerPlan(const std::string&) { return false; }

#endif // OC_PLATFORM_LINUX


// ─────────────────────────────────────────────────────────────────────────────
//  WINDOWS implementation
// ─────────────────────────────────────────────────────────────────────────────
#ifdef OC_PLATFORM_WINDOWS
#include <array>

static std::string getCpuModelName() {
    char name[49] = {};
    std::array<int, 4> regs{};
    for (int i = 0; i < 3; ++i) {
        __cpuid(regs.data(), 0x80000002 + i);
        memcpy(name + i * 16, regs.data(), 16);
    }
    return std::string(name);
}

void CpuController::probe() {
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    m_core_count = (int)si.dwNumberOfProcessors;

    std::vector<PROCESSOR_POWER_INFORMATION> ppi(m_core_count);
    CallNtPowerInformation(ProcessorInformation, nullptr, 0,
                           ppi.data(),
                           (ULONG)(sizeof(PROCESSOR_POWER_INFORMATION) * m_core_count));
    m_hw_max_khz = (long)ppi[0].MaxMhz * 1000;
    m_hw_min_khz = 400000;
}

CpuInfo CpuController::getInfo() {
    CpuInfo info;
    info.model_name    = getCpuModelName();
    info.core_count    = m_core_count;
    info.thread_count  = m_core_count;
    info.base_freq_khz = m_hw_max_khz;
    info.max_freq_khz  = m_hw_max_khz;
    info.min_freq_khz  = m_hw_min_khz;

    std::vector<PROCESSOR_POWER_INFORMATION> ppi(m_core_count);
    CallNtPowerInformation(ProcessorInformation, nullptr, 0,
                           ppi.data(),
                           (ULONG)(sizeof(PROCESSOR_POWER_INFORMATION) * m_core_count));
    long sum = 0;
    for (int i = 0; i < m_core_count; ++i)
        sum += (long)ppi[i].CurrentMhz * 1000;
    info.cur_freq_khz = sum / m_core_count;
    info.power_plan   = "Use Windows Power Options";
    return info;
}

std::vector<long> CpuController::getCoreFrequencies() {
    std::vector<PROCESSOR_POWER_INFORMATION> ppi(m_core_count);
    CallNtPowerInformation(ProcessorInformation, nullptr, 0,
                           ppi.data(),
                           (ULONG)(sizeof(PROCESSOR_POWER_INFORMATION) * m_core_count));
    std::vector<long> freqs;
    for (int i = 0; i < m_core_count; ++i)
        freqs.push_back((long)ppi[i].CurrentMhz * 1000);
    return freqs;
}

bool CpuController::setMaxFreq(long)  { return false; }
bool CpuController::setMinFreq(long)  { return false; }
bool CpuController::setGovernor(const std::string&) { return false; }

bool CpuController::setPowerPlan(const std::string& guid) {
    GUID g{};
    CLSIDFromString(std::wstring(guid.begin(), guid.end()).c_str(), &g);
    return PowerSetActiveScheme(nullptr, &g) == ERROR_SUCCESS;
}
#endif // OC_PLATFORM_WINDOWS

CpuController::CpuController() { probe(); }
