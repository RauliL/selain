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
#include <selain/keyboard.hpp>
#include <selain/main-window.hpp>

#include <iostream>

static int on_command_line(
  const Glib::RefPtr<Gio::ApplicationCommandLine>&,
  const Glib::RefPtr<Gtk::Application>&,
  selain::MainWindow*
);

int
main(int argc, char** argv)
{
  const auto app = Gtk::Application::create(
    "pw.rauli.selain",
    Gio::APPLICATION_HANDLES_COMMAND_LINE
  );
  selain::MainWindow window(app);

  app->signal_command_line().connect(
    sigc::bind(sigc::ptr_fun(&on_command_line), app, &window),
    false
  );

  return app->run(window, argc, argv);
}

static int
on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line,
                const Glib::RefPtr<Gtk::Application>& app,
                selain::MainWindow* window)
{
  int argc;
  auto argv = command_line->get_arguments(argc);
  Glib::OptionContext context;
  Glib::OptionGroup gtk_group(::gtk_get_option_group(true));

  context.add_group(gtk_group);

  if (!context.parse(argc, argv))
  {
    std::exit(EXIT_FAILURE);
  }

  app->activate();

  selain::keyboard::initialize();

  for (int i = 1; i < argc; ++i)
  {
    window->open_tab(argv[i]);
  }

  if (argc == 1)
  {
    // TODO: Make the initial home page URL customizable through settings.
    window->open_tab("https://duckduckgo.com");
  }

  return EXIT_SUCCESS;
}
