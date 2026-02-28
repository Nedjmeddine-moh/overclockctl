#include "CpuPage.h"
#include <iomanip>
#include <sstream>

static std::string freqStr(long khz) {
    std::ostringstream ss;
    if (khz >= 1000000)
        ss << std::fixed << std::setprecision(2) << khz / 1000000.0 << " GHz";
    else
        ss << khz / 1000 << " MHz";
    return ss.str();
}

CpuPage::CpuPage() : Gtk::Box(Gtk::Orientation::VERTICAL, 12) {
    set_margin(20);

    auto* scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroll->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    scroll->set_vexpand(true);

    auto* inner = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 16);
    inner->set_margin(4);

    buildInfoSection(*inner);
    buildControlSection(*inner);
    buildCoreSection(*inner);

    scroll->set_child(*inner);
    append(*scroll);
}

void CpuPage::buildInfoSection(Gtk::Box& parent) {
    auto* frame = Gtk::make_managed<Gtk::Frame>("CPU Information");
    auto* grid  = Gtk::make_managed<Gtk::Grid>();
    grid->set_margin(12);
    grid->set_row_spacing(6);
    grid->set_column_spacing(16);

    auto addRow = [&](int row, const char* key, Gtk::Label& val) {
        auto* lbl = Gtk::make_managed<Gtk::Label>(key);
        lbl->set_xalign(0.0f);
        lbl->add_css_class("dim-label");
        val.set_xalign(0.0f);
        val.set_hexpand(true);
        grid->attach(*lbl, 0, row);
        grid->attach(val,  1, row);
    };

    addRow(0, "Model:",    m_model_label);
    addRow(1, "Cores:",    m_cores_label);
    addRow(2, "Cur Freq:", m_cur_freq_label);
    addRow(3, "Governor:", m_governor_label);

    frame->set_child(*grid);
    parent.append(*frame);
}

void CpuPage::buildControlSection(Gtk::Box& parent) {
    auto* frame = Gtk::make_managed<Gtk::Frame>("Frequency & Governor Control");
    auto* box   = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
    box->set_margin(12);

    // Max freq row
    auto* max_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto* max_lbl = Gtk::make_managed<Gtk::Label>("Max Freq:");
    max_lbl->set_xalign(0.0f);
    max_lbl->set_size_request(100, -1);
    m_max_freq_scale.set_hexpand(true);
    m_max_freq_scale.set_draw_value(false);
    m_max_freq_label.set_size_request(90, -1);
    m_max_freq_label.set_xalign(1.0f);
    max_row->append(*max_lbl);
    max_row->append(m_max_freq_scale);
    max_row->append(m_max_freq_label);
    box->append(*max_row);

    // Min freq row
    auto* min_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto* min_lbl = Gtk::make_managed<Gtk::Label>("Min Freq:");
    min_lbl->set_xalign(0.0f);
    min_lbl->set_size_request(100, -1);
    m_min_freq_scale.set_hexpand(true);
    m_min_freq_scale.set_draw_value(false);
    m_min_freq_label.set_size_request(90, -1);
    m_min_freq_label.set_xalign(1.0f);
    min_row->append(*min_lbl);
    min_row->append(m_min_freq_scale);
    min_row->append(m_min_freq_label);
    box->append(*min_row);

    // Governor row
    auto* gov_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto* gov_lbl = Gtk::make_managed<Gtk::Label>("Governor:");
    gov_lbl->set_xalign(0.0f);
    gov_lbl->set_size_request(100, -1);
    m_governor_combo.set_hexpand(true);
    gov_row->append(*gov_lbl);
    gov_row->append(m_governor_combo);
    box->append(*gov_row);

    // Apply button
    m_apply_btn.set_label("Apply Settings");
    m_apply_btn.add_css_class("suggested-action");
    m_apply_btn.set_halign(Gtk::Align::END);
    m_apply_btn.signal_clicked().connect(sigc::mem_fun(*this, &CpuPage::onApply));
    box->append(m_apply_btn);

    // Scale value labels
    m_max_freq_scale.signal_value_changed().connect([this]{
        m_max_freq_label.set_text(freqStr((long)m_max_freq_scale.get_value()));
    });
    m_min_freq_scale.signal_value_changed().connect([this]{
        m_min_freq_label.set_text(freqStr((long)m_min_freq_scale.get_value()));
    });

    frame->set_child(*box);
    parent.append(*frame);
}

void CpuPage::buildCoreSection(Gtk::Box& parent) {
    auto* frame = Gtk::make_managed<Gtk::Frame>("Per-Core Frequency");
    m_core_grid.set_margin(12);
    m_core_grid.set_row_spacing(6);
    m_core_grid.set_column_spacing(10);
    frame->set_child(m_core_grid);
    parent.append(*frame);
}

void CpuPage::update(CpuController& cpu) {
    m_cpu_ref = &cpu;
    auto info = cpu.getInfo();

    m_model_label.set_text(info.model_name);
    m_cores_label.set_text(std::to_string(info.core_count) + " cores / " +
                            std::to_string(info.thread_count) + " threads");
    m_cur_freq_label.set_text(freqStr(info.cur_freq_khz));
    m_governor_label.set_text(info.governor);

    // Init scales if needed
    if (m_hw_max_khz == 0 && cpu.getHardwareMaxFreq() > 0) {
        m_hw_max_khz = cpu.getHardwareMaxFreq();
        m_hw_min_khz = cpu.getHardwareMinFreq();

        m_max_freq_scale.set_range(m_hw_min_khz, m_hw_max_khz);
        m_max_freq_scale.set_increments(100000, 500000);
        m_min_freq_scale.set_range(m_hw_min_khz, m_hw_max_khz);
        m_min_freq_scale.set_increments(100000, 500000);

        // Governor combo
        m_governor_combo.remove_all();
        for (auto& g : info.available_governors)
            m_governor_combo.append(g, g);
    }

    m_max_freq_scale.set_value(info.max_freq_khz);
    m_min_freq_scale.set_value(info.min_freq_khz);
    m_governor_combo.set_active_id(info.governor);

    // Per-core bars
    auto freqs = cpu.getCoreFrequencies();
    // Rebuild grid if core count changed
    if ((int)m_core_bars.size() != (int)freqs.size()) {
        // Remove old children
        while (auto* child = m_core_grid.get_first_child())
            m_core_grid.remove(*child);
        m_core_bars.clear();
        m_core_bars.resize(freqs.size());
        for (int i = 0; i < (int)freqs.size(); ++i) {
            auto* lbl = Gtk::make_managed<Gtk::Label>("Core " + std::to_string(i) + ":");
            lbl->set_xalign(0.0f);
            lbl->set_size_request(70, -1);
            m_core_bars[i].set_hexpand(true);
            m_core_bars[i].set_show_text(true);
            m_core_grid.attach(*lbl, 0, i);
            m_core_grid.attach(m_core_bars[i], 1, i);
        }
    }

    for (int i = 0; i < (int)freqs.size(); ++i) {
        double frac = m_hw_max_khz > 0 ? (double)freqs[i] / m_hw_max_khz : 0.0;
        m_core_bars[i].set_fraction(std::clamp(frac, 0.0, 1.0));
        m_core_bars[i].set_text(freqStr(freqs[i]));
    }
}

void CpuPage::onApply() {
    if (!m_cpu_ref) return;
    long max_khz = (long)m_max_freq_scale.get_value();
    long min_khz = (long)m_min_freq_scale.get_value();
    if (min_khz > max_khz) min_khz = max_khz;
    m_cpu_ref->setMaxFreq(max_khz);
    m_cpu_ref->setMinFreq(min_khz);
    auto gov = m_governor_combo.get_active_id();
    if (!gov.empty()) m_cpu_ref->setGovernor(gov);
}
