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

#include <functional>
#include <unordered_map>

namespace selain
{
  using command_callback = std::function<void(Tab*, const Glib::ustring&)>;
  using command_mapping = std::unordered_map<std::string, command_callback>;

  static void
  cmd_hint_mode(Tab* tab, const Glib::ustring&)
  {
    if (const auto window = tab->get_main_window())
    {
      window->set_mode(Mode::HINT);
    }
  }

  static void
  cmd_insert_mode(Tab* tab, const Glib::ustring&)
  {
    if (const auto window = tab->get_main_window())
    {
      window->set_mode(Mode::INSERT);
    }
  }

  static void
  cmd_open(Tab* tab, const Glib::ustring& args)
  {
    tab->load_uri(args);
  }

  static void
  cmd_open_tab(Tab* tab, const Glib::ustring& args)
  {
    if (const auto window = tab->get_main_window())
    {
      window->open_tab(args);
    }
  }

  static void
  cmd_quit(Tab* tab, const Glib::ustring&)
  {
    if (const auto window = tab->get_main_window())
    {
      window->close_tab(tab);
    }
  }

  static void
  cmd_quit_all(Tab*, const Glib::ustring&)
  {
    std::exit(EXIT_SUCCESS);
  }

  static void
  cmd_reload(Tab* tab, const Glib::ustring&)
  {
    tab->reload();
  }

  static void
  cmd_reload_bang(Tab* tab, const Glib::ustring&)
  {
    tab->reload(true);
  }

  static void
  cmd_stop(Tab* tab, const Glib::ustring&)
  {
    tab->stop_loading();
  }

  static void
  cmd_tabprevious(Tab* tab, const Glib::ustring&)
  {
    if (const auto window = tab->get_main_window())
    {
      window->prev_tab();
    }
  }

  static void
  cmd_tabnext(Tab* tab, const Glib::ustring&)
  {
    if (const auto window = tab->get_main_window())
    {
      window->next_tab();
    }
  }

  static const command_mapping commands =
  {
    { "h", cmd_hint_mode },
    { "hint", cmd_hint_mode },
    { "i", cmd_insert_mode },
    { "insert", cmd_insert_mode },
    { "o", cmd_open },
    { "open", cmd_open },
    { "ot", cmd_open_tab },
    { "open-tab", cmd_open_tab },
    { "q", cmd_quit },
    { "qa", cmd_quit_all },
    { "qall", cmd_quit_all },
    { "quit", cmd_quit },
    { "r", cmd_reload },
    { "r!", cmd_reload_bang },
    { "reload", cmd_reload },
    { "reload!", cmd_reload_bang },
    { "s", cmd_stop },
    { "stop", cmd_stop },
    { "tn", cmd_tabnext },
    { "tabnext", cmd_tabnext },
    { "tp", cmd_tabprevious },
    { "tabprev", cmd_tabprevious },
  };

  void
  Tab::execute_command(const Glib::ustring& command)
  {
    const auto length = command.length();

    // Skip empty commands.
    if (!length || std::all_of(command.begin(), command.end(), ::isspace))
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
      command_mapping::const_iterator entry;

      if (pos == Glib::ustring::npos)
      {
        command_name = utils::string_trim(subcommand);
      } else {
        command_name = utils::string_trim(subcommand.substr(0, pos));
        command_args = utils::string_trim(subcommand.substr(pos + 1));
      }
      entry = commands.find(command_name.c_str());
      if (entry != std::end(commands))
      {
        entry->second(this, command_args);
        return;
      }
    }
    if (const auto window = get_main_window())
    {
      window->add_notification(
        "Error: Unknown command: " + command,
        NotificationType::ERROR
      );
    }
  }
}
