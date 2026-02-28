#include "BenchmarkPage.h"
#include <iomanip>
#include <sstream>
#include <cmath>

static std::string fmt1(double v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << v;
    return ss.str();
}
static std::string fmtScore(double v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(0) << v;
    return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
BenchmarkPage::BenchmarkPage()
    : Gtk::Box(Gtk::Orientation::VERTICAL, 12)
{
    set_margin(20);

    auto* scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroll->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    scroll->set_vexpand(true);
    auto* inner = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 16);
    inner->set_margin(4);

    buildCpuSection(*inner);
    buildGpuSection(*inner);
    buildResultsSection(*inner);

    scroll->set_child(*inner);
    append(*scroll);

    // Wire dispatchers
    m_cpu_dispatcher.connect(sigc::mem_fun(*this, &BenchmarkPage::onCpuUpdate));
    m_gpu_dispatcher.connect(sigc::mem_fun(*this, &BenchmarkPage::onGpuUpdate));
}

BenchmarkPage::~BenchmarkPage() {
    if (m_cpu_thread && m_cpu_thread->joinable()) m_cpu_thread->detach();
    if (m_gpu_thread && m_gpu_thread->joinable()) m_gpu_thread->detach();
}

// ─────────────────────────────────────────────────────────────────────────────
void BenchmarkPage::buildCpuSection(Gtk::Box& parent) {
    m_cpu_frame.set_label("🖥 CPU Benchmark  (Mandelbrot FP64 — multi-threaded)");
    m_cpu_box.set_margin(14);

    // Info row
    auto* info_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 24);
    m_cpu_threads_label.set_text("Threads: detecting…");
    m_cpu_threads_label.set_xalign(0);
    m_cpu_gflops_label.set_text("GFLOPS: —");
    m_cpu_gflops_label.set_xalign(0);
    info_row->append(m_cpu_threads_label);
    info_row->append(m_cpu_gflops_label);
    m_cpu_box.append(*info_row);

    // Progress
    m_cpu_progress.set_fraction(0.0);
    m_cpu_progress.set_show_text(true);
    m_cpu_progress.set_text("Idle");
    m_cpu_box.append(m_cpu_progress);

    m_cpu_status.set_xalign(0);
    m_cpu_status.add_css_class("dim-label");
    m_cpu_box.append(m_cpu_status);

    // Score row
    auto* score_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 32);
    auto addScore = [&](Gtk::Label& lbl, const char* title) {
        auto* vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
        auto* tlbl = Gtk::make_managed<Gtk::Label>(title);
        tlbl->add_css_class("dim-label");
        tlbl->set_xalign(0.5f);
        lbl.set_text("—");
        lbl.add_css_class("title-2");
        lbl.set_xalign(0.5f);
        vbox->append(*tlbl);
        vbox->append(lbl);
        score_row->append(*vbox);
    };
    addScore(m_cpu_single_score, "Single-Core Score");
    addScore(m_cpu_multi_score,  "Multi-Core Score");
    m_cpu_box.append(*score_row);

    // Button
    m_cpu_run_btn.set_label("▶  Run CPU Benchmark");
    m_cpu_run_btn.add_css_class("suggested-action");
    m_cpu_run_btn.set_halign(Gtk::Align::START);
    m_cpu_run_btn.signal_clicked().connect(sigc::mem_fun(*this, &BenchmarkPage::onRunCpu));
    m_cpu_box.append(m_cpu_run_btn);

    m_cpu_frame.set_child(m_cpu_box);
    parent.append(m_cpu_frame);

    // Show thread count immediately
    int n = (int)std::thread::hardware_concurrency();
    m_cpu_threads_label.set_text("Threads: " + std::to_string(n));
}

void BenchmarkPage::buildGpuSection(Gtk::Box& parent) {
    m_gpu_frame.set_label("⚡ GPU Benchmark  (Vulkan Compute — FP32 MAC)");
    m_gpu_box.set_margin(14);

    // Info row
    auto* info_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 16);
    m_gpu_name_label.set_text(m_gpu_bench.isVulkanAvailable()
        ? "Vulkan: available" : "Vulkan: not detected");
    m_gpu_name_label.set_xalign(0);
    m_gpu_api_label.set_xalign(0);
    m_gpu_gflops_label.set_text("GFLOPS: —");
    m_gpu_gflops_label.set_xalign(0);
    info_row->append(m_gpu_name_label);
    info_row->append(m_gpu_gflops_label);
    m_gpu_box.append(*info_row);
    m_gpu_box.append(m_gpu_api_label);

    // Progress
    m_gpu_progress.set_fraction(0.0);
    m_gpu_progress.set_show_text(true);
    m_gpu_progress.set_text("Idle");
    m_gpu_box.append(m_gpu_progress);

    m_gpu_status.set_xalign(0);
    m_gpu_status.add_css_class("dim-label");
    m_gpu_box.append(m_gpu_status);

    // Score
    auto* score_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
    auto* score_title = Gtk::make_managed<Gtk::Label>("GPU Score");
    score_title->add_css_class("dim-label");
    score_title->set_xalign(0.5f);
    m_gpu_score_label.set_text("—");
    m_gpu_score_label.add_css_class("title-2");
    m_gpu_score_label.set_xalign(0.5f);
    score_box->append(*score_title);
    score_box->append(m_gpu_score_label);
    m_gpu_box.append(*score_box);

    // Button
    m_gpu_run_btn.set_label("▶  Run GPU Benchmark");
    m_gpu_run_btn.add_css_class("suggested-action");
    m_gpu_run_btn.set_halign(Gtk::Align::START);
    if (!m_gpu_bench.isVulkanAvailable()) {
        m_gpu_run_btn.set_tooltip_text("Vulkan not available — will run CPU fallback");
    }
    m_gpu_run_btn.signal_clicked().connect(sigc::mem_fun(*this, &BenchmarkPage::onRunGpu));
    m_gpu_box.append(m_gpu_run_btn);

    m_gpu_frame.set_child(m_gpu_box);
    parent.append(m_gpu_frame);
}

void BenchmarkPage::buildResultsSection(Gtk::Box& parent) {
    m_results_frame.set_label("📊 Results Comparison");
    m_results_box.set_margin(14);
    auto* placeholder = Gtk::make_managed<Gtk::Label>(
        "Run both benchmarks to see a comparison.");
    placeholder->add_css_class("dim-label");
    m_results_box.append(*placeholder);
    m_results_frame.set_child(m_results_box);
    parent.append(m_results_frame);
}

// ─────────────────────────────────────────────────────────────────────────────
void BenchmarkPage::onRunCpu() {
    if (m_cpu_running) return;
    m_cpu_running = true;
    m_cpu_run_btn.set_sensitive(false);
    m_cpu_run_btn.set_label("Running…");
    m_cpu_progress.set_fraction(0.0);
    m_cpu_single_score.set_text("—");
    m_cpu_multi_score.set_text("—");
    m_cpu_gflops_label.set_text("GFLOPS: —");

    m_cpu_thread = std::make_unique<std::thread>([this] {
        auto result = m_cpu_bench.run([this](double p, const std::string& msg) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cpu_update = {p, msg, {}, false};
            m_cpu_dispatcher.emit();
        });
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cpu_update = {1.0, "Done!", result, true};
        }
        m_cpu_dispatcher.emit();
    });
    m_cpu_thread->detach();
}

void BenchmarkPage::onRunGpu() {
    if (m_gpu_running) return;
    m_gpu_running = true;
    m_gpu_run_btn.set_sensitive(false);
    m_gpu_run_btn.set_label("Running…");
    m_gpu_progress.set_fraction(0.0);
    m_gpu_score_label.set_text("—");
    m_gpu_gflops_label.set_text("GFLOPS: —");

    m_gpu_thread = std::make_unique<std::thread>([this] {
        auto result = m_gpu_bench.run([this](double p, const std::string& msg) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_gpu_update = {p, msg, {}, false};
            m_gpu_dispatcher.emit();
        });
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_gpu_update = {1.0, "Done!", result, true};
        }
        m_gpu_dispatcher.emit();
    });
    m_gpu_thread->detach();
}

void BenchmarkPage::onCpuUpdate() {
    CpuUpdate upd;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        upd = m_cpu_update;
    }
    m_cpu_progress.set_fraction(std::clamp(upd.progress, 0.0, 1.0));
    m_cpu_progress.set_text(upd.msg);
    m_cpu_status.set_text(upd.msg);

    if (upd.done && upd.result.completed) {
        m_last_cpu_result = upd.result;
        m_cpu_single_score.set_text(fmtScore(upd.result.single_core_score));
        m_cpu_multi_score.set_text(fmtScore(upd.result.multi_core_score));
        m_cpu_gflops_label.set_text("GFLOPS: " + fmt1(upd.result.gflops_multi));
        m_cpu_run_btn.set_sensitive(true);
        m_cpu_run_btn.set_label("▶  Run CPU Benchmark");
        m_cpu_running = false;
        updateResultsChart();
    }
}

void BenchmarkPage::onGpuUpdate() {
    GpuUpdate upd;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        upd = m_gpu_update;
    }
    m_gpu_progress.set_fraction(std::clamp(upd.progress, 0.0, 1.0));
    m_gpu_progress.set_text(upd.msg);
    m_gpu_status.set_text(upd.msg);

    if (upd.done) {
        m_last_gpu_result = upd.result;
        if (!upd.result.gpu_name.empty())
            m_gpu_name_label.set_text("GPU: " + upd.result.gpu_name);
        if (!upd.result.api_version.empty())
            m_gpu_api_label.set_text("Vulkan API: " + upd.result.api_version +
                                      "  Driver: " + upd.result.driver_version);
        if (upd.result.completed) {
            m_gpu_score_label.set_text(fmtScore(upd.result.score));
            m_gpu_gflops_label.set_text("GFLOPS: " + fmt1(upd.result.gflops));
            if (!upd.result.error_msg.empty())
                m_gpu_status.set_text(upd.result.error_msg);
        } else if (!upd.result.error_msg.empty()) {
            m_gpu_status.set_text("Error: " + upd.result.error_msg);
        }
        m_gpu_run_btn.set_sensitive(true);
        m_gpu_run_btn.set_label("▶  Run GPU Benchmark");
        m_gpu_running = false;
        updateResultsChart();
    }
}

void BenchmarkPage::updateResultsChart() {
    bool cpu_done = m_last_cpu_result.completed;
    bool gpu_done = m_last_gpu_result.completed;
    if (!cpu_done && !gpu_done) return;

    // Rebuild results box
    while (auto* c = m_results_box.get_first_child())
        m_results_box.remove(*c);

    auto* grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_row_spacing(8);
    grid->set_column_spacing(16);

    int row = 0;
    auto addRow = [&](const std::string& metric,
                      const std::string& cpu_val,
                      const std::string& gpu_val) {
        auto* m_lbl = Gtk::make_managed<Gtk::Label>(metric);
        m_lbl->set_xalign(0); m_lbl->add_css_class("dim-label");
        m_lbl->set_size_request(180, -1);
        auto* c_lbl = Gtk::make_managed<Gtk::Label>(cpu_val);
        c_lbl->set_xalign(1); c_lbl->set_hexpand(true);
        auto* g_lbl = Gtk::make_managed<Gtk::Label>(gpu_val);
        g_lbl->set_xalign(1); g_lbl->set_hexpand(true);
        grid->attach(*m_lbl, 0, row);
        grid->attach(*c_lbl, 1, row);
        grid->attach(*g_lbl, 2, row);
        ++row;
    };

    // Header
    auto* h1 = Gtk::make_managed<Gtk::Label>(""); h1->set_size_request(180,-1);
    auto* h2 = Gtk::make_managed<Gtk::Label>("CPU"); h2->set_xalign(1); h2->add_css_class("heading"); h2->set_hexpand(true);
    auto* h3 = Gtk::make_managed<Gtk::Label>("GPU (Vulkan)"); h3->set_xalign(1); h3->add_css_class("heading"); h3->set_hexpand(true);
    grid->attach(*h1, 0, row); grid->attach(*h2, 1, row); grid->attach(*h3, 2, row);
    ++row;

    // Separator
    auto* sep = Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL);
    grid->attach(*sep, 0, row, 3, 1); ++row;

    if (cpu_done) {
        addRow("Single-Core Score", fmtScore(m_last_cpu_result.single_core_score), gpu_done ? "" : "—");
        addRow("Multi-Core Score",  fmtScore(m_last_cpu_result.multi_core_score),  gpu_done ? fmtScore(m_last_gpu_result.score) : "—");
        addRow("GFLOPS",            fmt1(m_last_cpu_result.gflops_multi) + " GF",  gpu_done ? fmt1(m_last_gpu_result.gflops) + " GF" : "—");
        addRow("Duration",          fmt1(m_last_cpu_result.duration_sec) + " s",   gpu_done ? fmt1(m_last_gpu_result.duration_sec) + " s" : "—");
    }

    // Score bars
    if (cpu_done && gpu_done) {
        auto* sep2 = Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL);
        m_results_box.append(*sep2);

        double max_score = std::max({m_last_cpu_result.multi_core_score,
                                     m_last_gpu_result.score, 1.0});

        auto addBar = [&](const std::string& label, double score, double max_s) {
            auto* lbl = Gtk::make_managed<Gtk::Label>(label);
            lbl->set_xalign(0);
            lbl->set_size_request(200, -1);
            auto* bar = Gtk::make_managed<Gtk::ProgressBar>();
            bar->set_fraction(score / max_s);
            bar->set_hexpand(true);
            bar->set_show_text(true);
            bar->set_text(fmtScore(score) + " pts");
            auto* row_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
            row_box->append(*lbl);
            row_box->append(*bar);
            m_results_box.append(*row_box);
        };

        addBar("CPU Single-Core", m_last_cpu_result.single_core_score, max_score);
        addBar("CPU Multi-Core",  m_last_cpu_result.multi_core_score,  max_score);
        addBar("GPU (Vulkan)",    m_last_gpu_result.score,             max_score);
    }

    m_results_box.append(*grid);
}
