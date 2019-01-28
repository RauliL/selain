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
#include <selain/main-window.hpp>
#include <selain/utils.hpp>

namespace selain
{
  static void cmd_hint_mode(MainWindow&, View&, const Glib::ustring&);
  static void cmd_insert_mode(MainWindow&, View&, const Glib::ustring&);
  static void cmd_open(MainWindow&, View&, const Glib::ustring&);
  static void cmd_open_tab(MainWindow&, View&, const Glib::ustring&);
  static void cmd_quit(MainWindow&, View&, const Glib::ustring&);
  static void cmd_quit_all(MainWindow&, View&, const Glib::ustring&);
  static void cmd_reload(MainWindow&, View&, const Glib::ustring&);
  static void cmd_force_reload(MainWindow&, View&, const Glib::ustring&);
  static void cmd_stop(MainWindow&, View&, const Glib::ustring&);
  static void cmd_tab_next(MainWindow&, View&, const Glib::ustring&);
  static void cmd_tab_prev(MainWindow&, View&, const Glib::ustring&);

  static const std::vector<Command> command_list =
  {
    { "hint", "h", cmd_hint_mode },
    { "insert", "i", cmd_insert_mode },
    { "open", "o", cmd_open },
    { "open-tab", "ot", cmd_open_tab },
    { "quit", "q", cmd_quit },
    { "quit-all", "qa", cmd_quit_all },
    { "reload", "r", cmd_reload },
    { "reload!", "r!", cmd_force_reload },
    { "stop", "s", cmd_stop },
    { "tab-next", "tn", cmd_tab_next },
    { "tab-prev", "tp", cmd_tab_prev },
  };

  void
  MainWindow::initialize_commands()
  {
    for (const auto& command : command_list)
    {
      if (command.name && *command.name)
      {
        m_command_mapping[command.name] = command;
      }
      if (command.name_shortcut && *command.name_shortcut)
      {
        m_command_mapping[command.name_shortcut] = command;
      }
    }
  }

  static void
  cmd_hint_mode(MainWindow& window, View&, const Glib::ustring&)
  {
    window.set_mode(Mode::HINT);
  }

  static void
  cmd_insert_mode(MainWindow& window, View&, const Glib::ustring&)
  {
    window.set_mode(Mode::INSERT);
  }

  static void
  cmd_open(MainWindow&, View& view, const Glib::ustring& args)
  {
    view.load_uri(args);
  }

  static void
  cmd_open_tab(MainWindow& window, View&, const Glib::ustring& args)
  {
    window.open_tab(args);
  }

  static void
  cmd_quit(MainWindow&, View& view, const Glib::ustring&)
  {
    view.close();
  }

  static void
  cmd_quit_all(MainWindow&, View&, const Glib::ustring&)
  {
    // TODO: Close the main window instead.
    std::exit(EXIT_SUCCESS);
  }

  static void
  cmd_reload(MainWindow&, View& view, const Glib::ustring&)
  {
    view.reload();
  }

  static void
  cmd_force_reload(MainWindow&, View& view, const Glib::ustring&)
  {
    view.reload(true);
  }

  static void
  cmd_stop(MainWindow&, View& view, const Glib::ustring&)
  {
    view.stop_loading();
  }

  static void
  cmd_tab_prev(MainWindow& window, View&, const Glib::ustring&)
  {
    window.prev_tab();
  }

  static void
  cmd_tab_next(MainWindow& window, View&, const Glib::ustring&)
  {
    window.next_tab();
  }
}
