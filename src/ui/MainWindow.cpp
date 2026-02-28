#include "MainWindow.h"
#include "../utils/Platform.h"

MainWindow::MainWindow()
    : m_cpu_page(), m_gpu_page(), m_monitor_page(), m_benchmark_page()
{
    set_title("OverclockCTL");
    set_default_size(900, 680);

    setupHeaderBar();

    // Stack pages
    m_stack.set_vexpand(true);
    m_stack.set_transition_type(Gtk::StackTransitionType::SLIDE_LEFT_RIGHT);
    m_stack.add(m_cpu_page,       "cpu",       "⚙ CPU");
    m_stack.add(m_gpu_page,       "gpu",       "🖥 GPU");
    m_stack.add(m_monitor_page,   "monitor",   "📊 Monitor");
    m_stack.add(m_benchmark_page, "benchmark", "🏁 Benchmark");

    m_switcher.set_stack(m_stack);
    m_switcher.set_halign(Gtk::Align::CENTER);

    m_root_box.append(m_switcher);
    m_root_box.append(m_stack);
    set_child(m_root_box);

    // Set initial theme based on system preference
    auto* style_mgr = adw_style_manager_get_default();
    m_dark_mode = (adw_style_manager_get_dark(style_mgr) == TRUE);

    onTick();
    m_timer = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &MainWindow::onTick), 1000);
}

void MainWindow::setupHeaderBar() {
    auto* header = Gtk::make_managed<Gtk::HeaderBar>();

    auto* title_lbl = Gtk::make_managed<Gtk::Label>("OverclockCTL");
    title_lbl->add_css_class("title");
    header->set_title_widget(*title_lbl);

    // Theme toggle button
    m_theme_btn = Gtk::make_managed<Gtk::Button>();
    m_theme_btn->set_icon_name("weather-clear-night-symbolic"); // dark icon
    m_theme_btn->set_tooltip_text("Toggle Dark / Light mode");
    m_theme_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::toggleTheme));
    header->pack_end(*m_theme_btn);

#ifdef OC_PLATFORM_LINUX
    // Root warning on Linux
    if (geteuid() != 0) {
        auto* warn = Gtk::make_managed<Gtk::Button>("⚠ Not Root");
        warn->add_css_class("warning");
        warn->set_tooltip_text("Run as root to apply overclocking settings");
        warn->set_sensitive(false);
        header->pack_end(*warn);
    }
#endif

#ifdef OC_PLATFORM_WINDOWS
    // Windows UAC note
    auto* warn = Gtk::make_managed<Gtk::Button>("ℹ UAC");
    warn->set_tooltip_text("Some settings require elevated privileges (run as Administrator)");
    warn->set_sensitive(false);
    header->pack_end(*warn);
#endif

    set_titlebar(*header);
}

void MainWindow::toggleTheme() {
    m_dark_mode = !m_dark_mode;
    auto* style_mgr = adw_style_manager_get_default();
    adw_style_manager_set_color_scheme(style_mgr,
        m_dark_mode ? ADW_COLOR_SCHEME_FORCE_DARK
                    : ADW_COLOR_SCHEME_FORCE_LIGHT);
    if (m_theme_btn) {
        m_theme_btn->set_icon_name(m_dark_mode
            ? "weather-clear-symbolic"          // light icon (switch to light)
            : "weather-clear-night-symbolic");   // dark icon (switch to dark)
        m_theme_btn->set_tooltip_text(m_dark_mode
            ? "Switch to Light mode"
            : "Switch to Dark mode");
    }
}

bool MainWindow::onTick() {
    m_cpu_page.update(m_cpu);
    m_gpu_page.update(m_gpu);
    m_monitor_page.update(m_hwmon);
    return true;
}
