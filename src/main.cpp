#include <gtkmm.h>
#include <adwaita.h>
#include "ui/MainWindow.h"
#include <iostream>

int main(int argc, char* argv[]) {
    adw_init();

    auto app = Gtk::Application::create("com.github.overclockctl");

    app->signal_activate().connect([&]() {
        auto* win = new MainWindow();
        app->add_window(*win);
        win->present();
    });

    return app->run(argc, argv);
}
