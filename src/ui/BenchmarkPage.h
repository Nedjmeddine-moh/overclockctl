#pragma once
#include <gtkmm.h>
#include "../benchmark/CpuBenchmark.h"
#include "../benchmark/VulkanBenchmark.h"
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>

class BenchmarkPage : public Gtk::Box {
public:
    BenchmarkPage();
    ~BenchmarkPage() override;

private:
    // ── CPU bench UI ─────────────────────────────────────────────────────────
    Gtk::Frame      m_cpu_frame;
    Gtk::Box        m_cpu_box{Gtk::Orientation::VERTICAL, 8};
    Gtk::Label      m_cpu_status;
    Gtk::ProgressBar m_cpu_progress;
    Gtk::Label      m_cpu_single_score;
    Gtk::Label      m_cpu_multi_score;
    Gtk::Label      m_cpu_gflops_label;
    Gtk::Label      m_cpu_threads_label;
    Gtk::Button     m_cpu_run_btn;

    // ── GPU bench UI ─────────────────────────────────────────────────────────
    Gtk::Frame      m_gpu_frame;
    Gtk::Box        m_gpu_box{Gtk::Orientation::VERTICAL, 8};
    Gtk::Label      m_gpu_status;
    Gtk::ProgressBar m_gpu_progress;
    Gtk::Label      m_gpu_score_label;
    Gtk::Label      m_gpu_gflops_label;
    Gtk::Label      m_gpu_name_label;
    Gtk::Label      m_gpu_api_label;
    Gtk::Button     m_gpu_run_btn;

    // ── Results comparison ────────────────────────────────────────────────────
    Gtk::Frame      m_results_frame;
    Gtk::Box        m_results_box{Gtk::Orientation::VERTICAL, 6};

    // ── Backends ─────────────────────────────────────────────────────────────
    CpuBenchmark    m_cpu_bench;
    VulkanBenchmark m_gpu_bench;

    std::atomic<bool> m_cpu_running{false};
    std::atomic<bool> m_gpu_running{false};
    std::unique_ptr<std::thread> m_cpu_thread;
    std::unique_ptr<std::thread> m_gpu_thread;

    CpuBenchResult  m_last_cpu_result{};
    GpuBenchResult  m_last_gpu_result{};

    // Dispatcher for thread→UI updates
    Glib::Dispatcher m_cpu_dispatcher;
    Glib::Dispatcher m_gpu_dispatcher;

    // Shared state for dispatcher
    struct CpuUpdate { double progress; std::string msg; CpuBenchResult result; bool done; };
    struct GpuUpdate { double progress; std::string msg; GpuBenchResult result; bool done; };
    std::mutex m_mutex;
    CpuUpdate m_cpu_update{};
    GpuUpdate m_gpu_update{};

    void buildCpuSection(Gtk::Box& parent);
    void buildGpuSection(Gtk::Box& parent);
    void buildResultsSection(Gtk::Box& parent);

    void onRunCpu();
    void onRunGpu();
    void onCpuUpdate();
    void onGpuUpdate();
    void updateResultsChart();

    Gtk::Label* scoreLabel(const std::string& title, const std::string& val);
};
