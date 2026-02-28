#pragma once
#include <gtkmm.h>
#include "../hardware/CpuController.h"

class CpuPage : public Gtk::Box {
public:
    CpuPage();
    void update(CpuController& cpu);

private:
    // Info labels
    Gtk::Label m_model_label;
    Gtk::Label m_cores_label;
    Gtk::Label m_cur_freq_label;
    Gtk::Label m_governor_label;

    // Controls
    Gtk::Scale       m_max_freq_scale;
    Gtk::Label       m_max_freq_label;
    Gtk::Scale       m_min_freq_scale;
    Gtk::Label       m_min_freq_label;
    Gtk::ComboBoxText m_governor_combo;
    Gtk::Button       m_apply_btn;

    // Per-core frequency bars
    std::vector<Gtk::ProgressBar> m_core_bars;
    Gtk::Grid m_core_grid;

    long m_hw_max_khz = 0;
    long m_hw_min_khz = 0;
    CpuController* m_cpu_ref = nullptr;

    void buildInfoSection(Gtk::Box& parent);
    void buildControlSection(Gtk::Box& parent);
    void buildCoreSection(Gtk::Box& parent);
    void onApply();
};
