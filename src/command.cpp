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
  cmd_open(Tab* tab, const Glib::ustring& args)
  {
    tab->load_uri(args);
  }

  static void
  cmd_tabnew(Tab* tab, const Glib::ustring& args)
  {
    const auto container = tab->get_toplevel();

    if (!container)
    {
      return;
    }
    static_cast<MainWindow*>(container)->open_tab(args);
  }

  static void
  cmd_quit(Tab* tab, const Glib::ustring&)
  {
    const auto container = tab->get_toplevel();

    if (!container)
    {
      return;
    }
    static_cast<MainWindow*>(container)->close_tab(tab);
  }

  static void
  cmd_quit_all(Tab*, const Glib::ustring&)
  {
    std::exit(EXIT_SUCCESS);
  }

  static const command_mapping commands =
  {
    { "o", cmd_open },
    { "open", cmd_open },
    { "q", cmd_quit },
    { "qa", cmd_quit_all },
    { "qall", cmd_quit_all },
    {" quit", cmd_quit },
    { "tabnew", cmd_tabnew },
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
      // TODO: Implement forwards search.
      return;
    }
    else if (command[0] == '?')
    {
      // TODO: Implement backwards search.
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

    // TODO: Display "Unknown command" error message to the user.
  }
}
