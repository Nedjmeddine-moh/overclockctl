#pragma once
#include <gtkmm.h>
#include "../hardware/IntelGpuController.h"

class GpuPage : public Gtk::Box {
public:
    GpuPage();
    void update(IntelGpuController& gpu);

private:
    Gtk::Label m_name_label;
    Gtk::Label m_cur_freq_label;
    Gtk::Label m_temp_label;

    Gtk::Scale  m_max_freq_scale;
    Gtk::Label  m_max_freq_val_label;
    Gtk::Scale  m_min_freq_scale;
    Gtk::Label  m_min_freq_val_label;
    Gtk::Scale  m_boost_freq_scale;
    Gtk::Label  m_boost_freq_val_label;
    Gtk::Button m_apply_btn;

    Gtk::ProgressBar m_freq_bar;

    long m_hw_max_mhz = 0;
    long m_hw_min_mhz = 0;
    IntelGpuController* m_gpu_ref = nullptr;

    void buildInfoSection(Gtk::Box& parent);
    void buildControlSection(Gtk::Box& parent);
    void onApply();
    void addFreqRow(Gtk::Box& box, const char* label,
                    Gtk::Scale& scale, Gtk::Label& val_lbl);
};
