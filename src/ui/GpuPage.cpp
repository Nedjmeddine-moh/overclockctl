#include "GpuPage.h"
#include <sstream>
#include <iomanip>

static std::string mhzStr(long mhz) {
    return std::to_string(mhz) + " MHz";
}

GpuPage::GpuPage() : Gtk::Box(Gtk::Orientation::VERTICAL, 12) {
    set_margin(20);
    auto* scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroll->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    scroll->set_vexpand(true);
    auto* inner = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 16);
    inner->set_margin(4);
    buildInfoSection(*inner);
    buildControlSection(*inner);
    scroll->set_child(*inner);
    append(*scroll);
}

void GpuPage::buildInfoSection(Gtk::Box& parent) {
    auto* frame = Gtk::make_managed<Gtk::Frame>("GPU Information");
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

    addRow(0, "GPU:",         m_name_label);
    addRow(1, "Current Freq:",m_cur_freq_label);
    addRow(2, "Temperature:", m_temp_label);

    // Frequency bar
    auto* bar_lbl = Gtk::make_managed<Gtk::Label>("GPU Load:");
    bar_lbl->set_xalign(0.0f);
    bar_lbl->add_css_class("dim-label");
    m_freq_bar.set_hexpand(true);
    m_freq_bar.set_show_text(true);
    grid->attach(*bar_lbl, 0, 3);
    grid->attach(m_freq_bar, 1, 3);

    frame->set_child(*grid);
    parent.append(*frame);
}

void GpuPage::addFreqRow(Gtk::Box& box, const char* label,
                          Gtk::Scale& scale, Gtk::Label& val_lbl) {
    auto* row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto* lbl = Gtk::make_managed<Gtk::Label>(label);
    lbl->set_xalign(0.0f);
    lbl->set_size_request(110, -1);
    scale.set_hexpand(true);
    scale.set_draw_value(false);
    val_lbl.set_size_request(80, -1);
    val_lbl.set_xalign(1.0f);
    row->append(*lbl);
    row->append(scale);
    row->append(val_lbl);
    box.append(*row);

    scale.signal_value_changed().connect([&scale, &val_lbl]{
        val_lbl.set_text(mhzStr((long)scale.get_value()));
    });
}

void GpuPage::buildControlSection(Gtk::Box& parent) {
    auto* frame = Gtk::make_managed<Gtk::Frame>("Frequency Control (i915)");
    auto* box   = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
    box->set_margin(12);

    addFreqRow(*box, "Max Freq:",   m_max_freq_scale,   m_max_freq_val_label);
    addFreqRow(*box, "Min Freq:",   m_min_freq_scale,   m_min_freq_val_label);
    addFreqRow(*box, "Boost Freq:", m_boost_freq_scale, m_boost_freq_val_label);

    auto* note = Gtk::make_managed<Gtk::Label>(
        "⚠ Writing GPU frequencies requires root privileges.");
    note->add_css_class("dim-label");
    note->set_xalign(0.0f);
    box->append(*note);

    m_apply_btn.set_label("Apply GPU Settings");
    m_apply_btn.add_css_class("suggested-action");
    m_apply_btn.set_halign(Gtk::Align::END);
    m_apply_btn.signal_clicked().connect(sigc::mem_fun(*this, &GpuPage::onApply));
    box->append(m_apply_btn);

    frame->set_child(*box);
    parent.append(*frame);
}

void GpuPage::update(IntelGpuController& gpu) {
    m_gpu_ref = &gpu;
    if (!gpu.isAvailable()) {
        m_name_label.set_text("Not detected");
        return;
    }
    auto info = gpu.getInfo();
    m_name_label.set_text(info.name);
    m_cur_freq_label.set_text(mhzStr(info.cur_freq_mhz));
    if (info.temp_celsius)
        m_temp_label.set_text(std::to_string((int)*info.temp_celsius) + " °C");
    else
        m_temp_label.set_text("N/A");

    double frac = info.hw_max_freq_mhz > 0
        ? (double)info.cur_freq_mhz / info.hw_max_freq_mhz : 0.0;
    m_freq_bar.set_fraction(std::clamp(frac, 0.0, 1.0));
    m_freq_bar.set_text(mhzStr(info.cur_freq_mhz) + " / " + mhzStr(info.hw_max_freq_mhz));

    // Init sliders once
    if (m_hw_max_mhz == 0 && info.hw_max_freq_mhz > 0) {
        m_hw_max_mhz = info.hw_max_freq_mhz;
        m_hw_min_mhz = info.hw_min_freq_mhz;
        for (auto* s : {&m_max_freq_scale, &m_min_freq_scale, &m_boost_freq_scale}) {
            s->set_range(m_hw_min_mhz, m_hw_max_mhz);
            s->set_increments(25, 100);
        }
    }
    m_max_freq_scale.set_value(info.max_freq_mhz);
    m_min_freq_scale.set_value(info.min_freq_mhz);
    m_boost_freq_scale.set_value(info.boost_freq_mhz);
}

void GpuPage::onApply() {
    if (!m_gpu_ref || !m_gpu_ref->isAvailable()) return;
    m_gpu_ref->setMaxFreq((long)m_max_freq_scale.get_value());
    m_gpu_ref->setMinFreq((long)m_min_freq_scale.get_value());
    m_gpu_ref->setBoostFreq((long)m_boost_freq_scale.get_value());
}
