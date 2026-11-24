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
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include <selain/core/command.hpp>
#include <selain/core/keyboard.hpp>
#include <selain/core/setting.hpp>
#include <selain/core/theme.hpp>
#include <selain/ui/main-window.hpp>
#include <selain/version.hpp>

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
show_init_errors(
  selain::ui::MainWindow* window,
  const Glib::ustring& path,
  const std::vector<Glib::ustring>& errors
)
{
  if (!window || errors.empty())
  {
    return;
  }

  Glib::ustring detail;

  for (std::size_t i = 0; i < errors.size(); ++i)
  {
    if (i > 0)
    {
      detail += '\n';
    }
    detail += errors[i];
  }

  const auto dialog = Gtk::AlertDialog::create(
    errors.size() == 1
      ? std::format("Error in {}", path.raw())
      : std::format("Errors in {}", path.raw())
  );

  dialog->set_detail(detail);
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
run_init_lines(
  selain::ui::MainWindow* window,
  const Glib::ustring& path,
  std::shared_ptr<std::vector<std::pair<std::size_t, Glib::ustring>>> lines,
  std::shared_ptr<std::vector<Glib::ustring>> errors,
  std::size_t index
)
{
  if (!window || !lines || !errors)
  {
    return;
  }

  if (index >= lines->size())
  {
    show_init_errors(window, path, *errors);
    return;
  }

  const auto line_number = (*lines)[index].first;
  const auto& command = (*lines)[index].second;

  window->run_command(
    command,
    false,
    [window, path, lines, errors, index, line_number](
      const std::optional<Glib::ustring>& error
    )
    {
      if (error)
      {
        errors->push_back(
          std::format("Line {}: {}", line_number, error->raw())
        );
        // Stop further init script execution on the first error.
        show_init_errors(window, path, *errors);
        return;
      }

      run_init_lines(window, path, lines, errors, index + 1);
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

  if (!std::filesystem::exists(path))
  {
    return;
  }

  std::ifstream is(path);
  auto lines = std::make_shared<std::vector<std::pair<std::size_t, Glib::ustring>>>();
  auto errors = std::make_shared<std::vector<Glib::ustring>>();
  std::string line;
  std::size_t line_number = 0;

  while (std::getline(is, line))
  {
    ++line_number;
    lines->emplace_back(line_number, Glib::ustring(line));
  }

  run_init_lines(window, path, std::move(lines), std::move(errors), 0);
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
