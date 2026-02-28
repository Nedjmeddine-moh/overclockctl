#pragma once
#include <gtkmm.h>
#include "../hardware/HwMonitor.h"
#include <vector>
#include <deque>

class MonitorPage : public Gtk::Box {
public:
    MonitorPage();
    void update(HwMonitor& hwmon);

private:
    // Temps section
    Gtk::Frame m_temp_frame;
    Gtk::Grid  m_temp_grid;
    std::vector<std::pair<Gtk::Label*, Gtk::ProgressBar*>> m_temp_rows;

    // Fan section
    Gtk::Frame m_fan_frame;
    Gtk::Box   m_fan_box{Gtk::Orientation::VERTICAL, 8};

    struct FanRow {
        Gtk::Box   row{Gtk::Orientation::HORIZONTAL, 8};
        Gtk::Label name_lbl;
        Gtk::Label rpm_lbl;
        Gtk::Scale pwm_scale;
        Gtk::Label pwm_lbl;
        Gtk::Switch auto_sw;
        FanReading  last;
        HwMonitor*  hwmon_ref = nullptr;
    };
    std::vector<std::unique_ptr<FanRow>> m_fan_rows;

    void rebuildTemps(const std::vector<TempReading>& temps);
    void rebuildFans(const std::vector<FanReading>& fans, HwMonitor& hwmon);
};
