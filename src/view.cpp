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
#include <selain/theme.hpp>
#include <selain/utils.hpp>

namespace selain
{
  View::View()
    : Gtk::Bin()
  {
    set_can_focus(true);
    override_background_color(theme::window_background);
  }

  MainWindow*
  View::get_main_window()
  {
    const auto container = get_toplevel();

    return container ? static_cast<MainWindow*>(container) : nullptr;
  }

  const MainWindow*
  View::get_main_window() const
  {
    const auto container = get_toplevel();

    return container ? static_cast<const MainWindow*>(container) : nullptr;
  }

  void
  View::close()
  {
    signal_close().emit(*this);
  }

  void
  View::execute_command(const Glib::ustring& command)
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
      find(command.substr(1), FindDirection::FORWARD);
      return;
    }
    else if (command[0] == '?')
    {
      find(command.substr(1), FindDirection::BACKWARD);
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
}
