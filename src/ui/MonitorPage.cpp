#include "MonitorPage.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

MonitorPage::MonitorPage() : Gtk::Box(Gtk::Orientation::VERTICAL, 12) {
    set_margin(20);

    auto* scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroll->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    scroll->set_vexpand(true);
    auto* inner = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 16);
    inner->set_margin(4);

    // Temp frame
    m_temp_frame.set_label("Temperatures");
    m_temp_grid.set_margin(12);
    m_temp_grid.set_row_spacing(6);
    m_temp_grid.set_column_spacing(12);
    m_temp_frame.set_child(m_temp_grid);
    inner->append(m_temp_frame);

    // Fan frame
    m_fan_frame.set_label("Fan Control");
    m_fan_box.set_margin(12);
    m_fan_frame.set_child(m_fan_box);
    inner->append(m_fan_frame);

    scroll->set_child(*inner);
    append(*scroll);
}

void MonitorPage::rebuildTemps(const std::vector<TempReading>& temps) {
    while (auto* c = m_temp_grid.get_first_child())
        m_temp_grid.remove(*c);
    m_temp_rows.clear();

    for (int i = 0; i < (int)temps.size(); ++i) {
        auto* lbl = Gtk::make_managed<Gtk::Label>(temps[i].label + ":");
        lbl->set_xalign(0.0f);
        lbl->add_css_class("dim-label");
        lbl->set_size_request(160, -1);

        auto* bar = Gtk::make_managed<Gtk::ProgressBar>();
        bar->set_hexpand(true);
        bar->set_show_text(true);

        auto* val_lbl = Gtk::make_managed<Gtk::Label>();
        val_lbl->set_size_request(70, -1);
        val_lbl->set_xalign(1.0f);

        m_temp_grid.attach(*lbl,    0, i);
        m_temp_grid.attach(*bar,    1, i);
        m_temp_grid.attach(*val_lbl,2, i);
        m_temp_rows.push_back({val_lbl, bar});
    }
}

void MonitorPage::rebuildFans(const std::vector<FanReading>& fans, HwMonitor& hwmon) {
    while (auto* c = m_fan_box.get_first_child())
        m_fan_box.remove(*c);
    m_fan_rows.clear();

    if (fans.empty()) {
        auto* lbl = Gtk::make_managed<Gtk::Label>("No controllable fans detected.");
        lbl->add_css_class("dim-label");
        m_fan_box.append(*lbl);
        return;
    }

    for (auto& fan : fans) {
        auto fr = std::make_unique<FanRow>();
        fr->last = fan;
        fr->hwmon_ref = &hwmon;

        fr->name_lbl.set_text("Fan " + std::to_string(fan.index) + ":");
        fr->name_lbl.set_size_request(60, -1);
        fr->name_lbl.set_xalign(0.0f);

        fr->rpm_lbl.set_size_request(80, -1);
        fr->rpm_lbl.set_xalign(0.0f);

        if (fan.has_pwm) {
            fr->pwm_scale.set_range(0, 255);
            fr->pwm_scale.set_increments(1, 10);
            fr->pwm_scale.set_value(fan.pwm);
            fr->pwm_scale.set_hexpand(true);
            fr->pwm_scale.set_draw_value(false);
            fr->pwm_scale.set_size_request(150, -1);

            fr->pwm_lbl.set_size_request(50, -1);
            fr->pwm_lbl.set_xalign(0.0f);
            fr->pwm_lbl.set_text(std::to_string(fan.pwm));

            fr->auto_sw.set_active(fan.pwm_enable == 2);
            auto* auto_lbl = Gtk::make_managed<Gtk::Label>("Auto");

            // Wire up
            FanRow* fr_ptr = fr.get();
            fr->auto_sw.property_active().signal_changed().connect([fr_ptr, &hwmon]{
                int mode = fr_ptr->auto_sw.get_active() ? 2 : 1;
                hwmon.setFanMode(fr_ptr->last, mode);
                fr_ptr->pwm_scale.set_sensitive(!fr_ptr->auto_sw.get_active());
            });
            fr->pwm_scale.signal_value_changed().connect([fr_ptr, &hwmon]{
                if (!fr_ptr->auto_sw.get_active()) {
                    int pwm = (int)fr_ptr->pwm_scale.get_value();
                    hwmon.setFanPwm(fr_ptr->last, pwm);
                    fr_ptr->pwm_lbl.set_text(std::to_string(pwm));
                }
            });
            fr->pwm_scale.set_sensitive(fan.pwm_enable != 2);

            fr->row.append(fr->name_lbl);
            fr->row.append(fr->rpm_lbl);
            fr->row.append(fr->pwm_scale);
            fr->row.append(fr->pwm_lbl);
            fr->row.append(*auto_lbl);
            fr->row.append(fr->auto_sw);
        } else {
            fr->row.append(fr->name_lbl);
            fr->row.append(fr->rpm_lbl);
            auto* note = Gtk::make_managed<Gtk::Label>("(no PWM control)");
            note->add_css_class("dim-label");
            fr->row.append(*note);
        }

        m_fan_box.append(fr->row);
        m_fan_rows.push_back(std::move(fr));
    }
}

void MonitorPage::update(HwMonitor& hwmon) {
    auto temps = hwmon.getTemperatures();
    auto fans  = hwmon.getFans();

    if ((int)temps.size() != (int)m_temp_rows.size())
        rebuildTemps(temps);

    for (int i = 0; i < (int)temps.size(); ++i) {
        double frac = temps[i].celsius / 100.0; // assume 100°C = full
        m_temp_rows[i].second->set_fraction(std::clamp(frac, 0.0, 1.0));
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << temps[i].celsius << " °C";
        m_temp_rows[i].first->set_text(ss.str());
        m_temp_rows[i].second->set_text(ss.str());
    }

    if ((int)fans.size() != (int)m_fan_rows.size())
        rebuildFans(fans, hwmon);

    for (int i = 0; i < (int)fans.size() && i < (int)m_fan_rows.size(); ++i) {
        m_fan_rows[i]->rpm_lbl.set_text(std::to_string(fans[i].rpm) + " RPM");
        m_fan_rows[i]->last = fans[i];
    }
}
