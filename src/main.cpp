/*
 * Copyright (c) 2019-2026, Rauli Laine
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
#include <cstring>
#include <format>
#include <iostream>

#include <selain/config.hpp>

#include <selain/core/command.hpp>
#include <selain/core/keyboard.hpp>
#include <selain/core/session.hpp>
#include <selain/core/setting.hpp>
#include <selain/core/theme.hpp>
#include <selain/ui/main-window.hpp>

/**
 * WebKitGTK on NVIDIA proprietary drivers SIGSEGVs in
 * AcceleratedBackingStore when entering accelerated compositing (reproduced
 * reliably on google.fi). Disable DMA-BUF and compositing before any
 * WebKitWebView is constructed, unless the user already overrode them.
 */
static void
maybe_workaround_nvidia_webkit()
{
  const auto* gl_vendor = std::getenv("GLX_VENDOR_LIBRARY_NAME");
  const auto* nvidia = std::getenv("__GLX_VENDOR_LIBRARY_NAME");
  const bool looks_nvidia =
    (gl_vendor && std::strstr(gl_vendor, "nvidia")) ||
    (nvidia && std::strstr(nvidia, "nvidia")) ||
    std::filesystem::exists("/proc/driver/nvidia/version") ||
    std::filesystem::exists("/dev/nvidia0");

  if (!looks_nvidia)
  {
    return;
  }

  // overwrite=0: keep any value the user already exported.
  ::setenv("WEBKIT_DISABLE_DMABUF_RENDERER", "1", 0);
  ::setenv("WEBKIT_DISABLE_COMPOSITING_MODE", "1", 0);
}

static selain::ui::MainWindow*
get_or_create_window(const Glib::RefPtr<Gtk::Application>& app)
{
  const auto& windows = app->get_windows();

  if (!windows.empty())
  {
    return static_cast<selain::ui::MainWindow*>(windows.front());
  }

  const auto window = Gtk::make_managed<selain::ui::MainWindow>(app);

  app->add_window(*window);

  return window;
}

static void
show_init_error(
  selain::ui::MainWindow* window,
  const Glib::ustring& path,
  const Glib::ustring& error
)
{
  if (!window)
  {
    return;
  }

  const auto dialog = Gtk::AlertDialog::create(
    std::format("Error in {}", path.raw())
  );

  dialog->set_detail(error);
  dialog->set_modal(true);
  dialog->choose(
    *window,
    [dialog](const Glib::RefPtr<Gio::AsyncResult>& result)
    {
      try
      {
        dialog->choose_finish(result);
      }
      catch (const Glib::Error&) {}
    }
  );
}

static void
run_init(selain::ui::MainWindow* window)
{
  const auto path = Glib::build_filename(
    Glib::get_user_config_dir(),
    "selain",
    "init.selain"
  );

  if (std::filesystem::exists(path))
  {
    window->run_script(
      path,
      [window, path](const std::optional<Glib::ustring>& error)
      {
        if (error)
        {
          show_init_error(window, path, *error);
        }
      }
    );
  }
}

static int
on_command_line(
  const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line,
  const Glib::RefPtr<Gtk::Application>& app
)
{
  static bool command_initialized = false;
  static bool keyboard_initialized = false;
  static bool theme_initialized = false;
  static bool init_run = false;
  int argc = 0;
  auto argv = command_line->get_arguments(argc);

  app->activate();

  const auto window = get_or_create_window(app);

  window->present();

  if (!command_initialized)
  {
    selain::command::initialize();
    command_initialized = true;
  }

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
    // Local files are resolved only for CLI arguments. In-app :open still
    // treats bare names as URLs/searches via resolve_navigation_uri.
    Glib::ustring target = argv[i];

    if (target.find("://") == Glib::ustring::npos)
    {
      const auto file = command_line->create_file_for_arg(target);

      if (file && file->query_exists())
      {
        target = file->get_uri();
      }
    }

    window->open_tab(target);
  }

  if (argc == 1 && !window->get_current_tab())
  {
    bool restored = false;

    if (selain::session::is_restore_enabled())
    {
      if (const auto session = selain::SessionStore::load())
      {
        restored = window->restore_session(*session);
      }
    }
    if (!restored)
    {
      if (const auto homepage = selain::setting::get(*window, "homepage"))
      {
        window->open_tab(*homepage);
      }
    }
  }

  return EXIT_SUCCESS;
}

int
main(int argc, char** argv)
{
  for (int i = 1; i < argc; ++i)
  {
    if (!std::strcmp(argv[i], "--version"))
    {
      std::cout << SELAIN_VERSION << std::endl;

      return EXIT_SUCCESS;
    }
  }

  maybe_workaround_nvidia_webkit();

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
