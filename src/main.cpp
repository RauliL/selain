/*
 * Copyright (c) 2019, Rauli Laine
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <filesystem>
#include <fstream>

#include <selain/keyboard.hpp>
#include <selain/main-window.hpp>
#include <selain/setting.hpp>
#include <selain/theme.hpp>

static selain::MainWindow*
get_or_create_window(const Glib::RefPtr<Gtk::Application>& app)
{
  const auto& windows = app->get_windows();

  if (!windows.empty())
  {
    return static_cast<selain::MainWindow*>(windows.front());
  }

  const auto window = Gtk::make_managed<selain::MainWindow>(app);

  app->add_window(*window);

  return window;
}

static void
run_init(selain::MainWindow* window)
{
  const auto path = Glib::build_filename(
    Glib::get_user_config_dir(),
    "selain",
    "init.selain"
  );

  if (std::filesystem::exists(path))
  {
    std::ifstream is(path);
    std::string line;

    while (std::getline(is, line))
    {
      window->execute_command(Glib::ustring(line));
    }
    is.close();
  }
}

static int
on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line,
                const Glib::RefPtr<Gtk::Application>& app)
{
  int argc = 0;
  auto argv = command_line->get_arguments(argc);
  static bool keyboard_initialized = false;
  static bool theme_initialized = false;
  static bool init_run = false;

  app->activate();

  const auto window = get_or_create_window(app);

  window->present();

  if (!keyboard_initialized)
  {
    selain::keyboard::initialize();
    keyboard_initialized = true;
  }

  if (!theme_initialized)
  {
    selain::theme::initialize();
    theme_initialized = true;
  }

  if (!init_run)
  {
    run_init(window);
    init_run = true;
  }

  for (int i = 1; i < argc; ++i)
  {
    window->open_tab(argv[i]);
  }

  if (argc == 1 && !window->get_current_tab())
  {
    if (const auto homepage = selain::setting::get(*window, "homepage"))
    {
      window->open_tab(*homepage);
    }
  }

  return EXIT_SUCCESS;
}

int
main(int argc, char** argv)
{
  const auto app = Gtk::Application::create(
    "dev.rauli.selain",
    Gio::Application::Flags::HANDLES_COMMAND_LINE
  );

  app->signal_activate().connect([app]() {
    get_or_create_window(app)->present();
  });
  app->signal_command_line().connect(
    sigc::bind(sigc::ptr_fun(&on_command_line), app),
    false
  );

  return app->run(argc, argv);
}
