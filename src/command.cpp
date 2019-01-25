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
  static void cmd_hint_mode(MainWindow&, Tab&, const Glib::ustring&);
  static void cmd_insert_mode(MainWindow&, Tab&, const Glib::ustring&);
  static void cmd_open(MainWindow&, Tab&, const Glib::ustring&);
  static void cmd_open_tab(MainWindow&, Tab&, const Glib::ustring&);
  static void cmd_quit(MainWindow&, Tab&, const Glib::ustring&);
  static void cmd_quit_all(MainWindow&, Tab&, const Glib::ustring&);
  static void cmd_reload(MainWindow&, Tab&, const Glib::ustring&);
  static void cmd_force_reload(MainWindow&, Tab&, const Glib::ustring&);
  static void cmd_stop(MainWindow&, Tab&, const Glib::ustring&);
  static void cmd_tab_next(MainWindow&, Tab&, const Glib::ustring&);
  static void cmd_tab_prev(MainWindow&, Tab&, const Glib::ustring&);

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

  void
  Tab::execute_command(const Glib::ustring& command)
  {
    const auto length = command.length();
    const auto window = get_main_window();

    // Skip empty commands.
    if (!length || std::all_of(command.begin(), command.end(), ::isspace))
    {
      return;
    }

    // Do not allow execution of commands on tabs that do not have a window.
    if (!window)
    {
      return;
    }

    if (command[0] == '/')
    {
      search(command.substr(1));
      return;
    }
    else if (command[0] == '?')
    {
      search(command.substr(1), false);
      return;
    }
    else if (command[0] == ':' && length > 1)
    {
      const auto subcommand = command.substr(1);
      const auto pos = subcommand.find(' ');
      Glib::ustring command_name;
      Glib::ustring command_args;
      const auto& mapping = window->get_command_mapping();
      MainWindow::command_mapping_type::const_iterator entry;

      if (pos == Glib::ustring::npos)
      {
        command_name = utils::string_trim(subcommand);
      } else {
        command_name = utils::string_trim(subcommand.substr(0, pos));
        command_args = utils::string_trim(subcommand.substr(pos + 1));
      }
      entry = mapping.find(command_name.c_str());
      if (entry != std::end(mapping))
      {
        entry->second.callback(*window, *this, command_args);
        return;
      }
    }

    window->get_command_entry().show_notification(
      "Error: Unknown command: " + command,
      NotificationType::ERROR
    );
  }

  static void
  cmd_hint_mode(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    tab.set_mode(Mode::HINT);
  }

  static void
  cmd_insert_mode(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    tab.set_mode(Mode::INSERT);
  }

  static void
  cmd_open(MainWindow&, Tab& tab, const Glib::ustring& args)
  {
    tab.load_uri(args);
  }

  static void
  cmd_open_tab(MainWindow& window, Tab&, const Glib::ustring& args)
  {
    window.open_tab(args);
  }

  static void
  cmd_quit(MainWindow& window, Tab& tab, const Glib::ustring&)
  {
    window.close_tab(tab);
  }

  static void
  cmd_quit_all(MainWindow&, Tab&, const Glib::ustring&)
  {
    // TODO: Close the main window instead.
    std::exit(EXIT_SUCCESS);
  }

  static void
  cmd_reload(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    tab.reload();
  }

  static void
  cmd_force_reload(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    tab.reload(true);
  }

  static void
  cmd_stop(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    tab.stop_loading();
  }

  static void
  cmd_tab_prev(MainWindow& window, Tab&, const Glib::ustring&)
  {
    window.prev_tab();
  }

  static void
  cmd_tab_next(MainWindow& window, Tab&, const Glib::ustring&)
  {
    window.next_tab();
  }
}
