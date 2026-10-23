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

#include <format>

namespace selain
{
  static void cmd_hint_mode(MainWindow&, Tab*, const Glib::ustring&);
  static void cmd_insert_mode(MainWindow&, Tab*, const Glib::ustring&);
  static void cmd_open(MainWindow&, Tab*, const Glib::ustring&);
  static void cmd_open_tab(MainWindow&, Tab*, const Glib::ustring&);
  static void cmd_quit(MainWindow&, Tab*, const Glib::ustring&);
  static void cmd_quit_all(MainWindow&, Tab*, const Glib::ustring&);
  static void cmd_reload(MainWindow&, Tab*, const Glib::ustring&);
  static void cmd_force_reload(MainWindow&, Tab*, const Glib::ustring&);
  static void cmd_stop(MainWindow&, Tab*, const Glib::ustring&);
  static void cmd_tab_next(MainWindow&, Tab*, const Glib::ustring&);
  static void cmd_tab_prev(MainWindow&, Tab*, const Glib::ustring&);

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
  MainWindow::execute_command(const Glib::ustring& command)
  {
    const auto tab = get_current_tab();

    // Skip empty commands.
    if (utils::is_blank(command))
    {
      return;
    }

    // Activate search mode.
    if (command[0] == '/')
    {
      if (tab)
      {
        tab->search(command.substr(1));
      }
      return;
    }

    // Activate backwards search mode.
    if (command[0] == '?')
    {
      if (tab)
      {
        tab->search(command.substr(1), false);
      }
      return;
    }

    if (command[0] == ':' && command.length() > 1)
    {
      Glib::ustring command_name;
      Glib::ustring command_args;
      const auto sub = command.substr(1);
      const auto pos = sub.find(' ');
      command_mapping_type::const_iterator entry;

      if (pos == Glib::ustring::npos)
      {
        command_name = utils::trim(sub);
      } else {
        command_name = utils::trim(sub.substr(0, pos));
        command_args = utils::trim(sub.substr(pos + 1));
      }
      entry = m_command_mapping.find(command_name.raw());
      if (entry != std::end(m_command_mapping))
      {
        entry->second.callback(*this, tab, command_args);
        return;
      }
    }

    m_command_entry.show_notification(
      std::format("Error: Unknown command: {}", command.raw()),
      Notification::Type::ERROR
    );
  }

  static void
  cmd_hint_mode(MainWindow& window, Tab*, const Glib::ustring&)
  {
    window.set_mode(Mode::HINT);
  }

  static void
  cmd_insert_mode(MainWindow& window, Tab*, const Glib::ustring&)
  {
    window.set_mode(Mode::INSERT);
  }

  static void
  cmd_open(MainWindow& window, Tab* tab, const Glib::ustring& args)
  {
    if (tab)
    {
      tab->load_uri(args);
    } else {
      window.open_tab(args);
    }
  }

  static void
  cmd_open_tab(MainWindow& window, Tab*, const Glib::ustring& args)
  {
    window.open_tab(args);
  }

  static void
  cmd_quit(MainWindow& window, Tab* tab, const Glib::ustring&)
  {
    if (tab)
    {
      window.close_tab(*tab);
    } else {
      window.close();
    }
  }

  static void
  cmd_quit_all(MainWindow& window, Tab*, const Glib::ustring&)
  {
    window.close();
  }

  static void
  cmd_reload(MainWindow&, Tab* tab, const Glib::ustring&)
  {
    if (tab)
    {
      tab->reload();
    }
  }

  static void
  cmd_force_reload(MainWindow&, Tab* tab, const Glib::ustring&)
  {
    if (tab)
    {
      tab->reload(true);
    }
  }

  static void
  cmd_stop(MainWindow&, Tab* tab, const Glib::ustring&)
  {
    if (tab)
    {
      tab->stop_loading();
    }
  }

  static void
  cmd_tab_prev(MainWindow& window, Tab*, const Glib::ustring&)
  {
    window.prev_tab();
  }

  static void
  cmd_tab_next(MainWindow& window, Tab*, const Glib::ustring&)
  {
    window.next_tab();
  }
}
