#pragma once
#include <gtkmm.h>
#include <adwaita.h>
#include "../hardware/CpuController.h"
#include "../hardware/IntelGpuController.h"
#include "../hardware/HwMonitor.h"
#include "CpuPage.h"
#include "GpuPage.h"
#include "MonitorPage.h"
#include "BenchmarkPage.h"

class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow();
    ~MainWindow() override = default;

private:
    // Hardware backends
    CpuController      m_cpu;
    IntelGpuController m_gpu;
    HwMonitor          m_hwmon;

    // UI
    Gtk::Box           m_root_box{Gtk::Orientation::VERTICAL};
    Gtk::StackSwitcher m_switcher;
    Gtk::Stack         m_stack;

    CpuPage       m_cpu_page;
    GpuPage       m_gpu_page;
    MonitorPage   m_monitor_page;
    BenchmarkPage m_benchmark_page;

    // Live update timer
    sigc::connection m_timer;
    bool onTick();

    // Theme
    bool m_dark_mode = false;
    void setupHeaderBar();
    void toggleTheme();
    Gtk::Button* m_theme_btn = nullptr;
};
