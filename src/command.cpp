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
#include <selain/setting.hpp>
#include <selain/utils.hpp>

#include <format>

namespace selain
{
  static bool cmd_hint_mode(MainWindow&, const Glib::ustring&);
  static bool cmd_insert_mode(MainWindow&, const Glib::ustring&);
  static bool cmd_js(MainWindow&, const Glib::ustring&);
  static bool cmd_only(MainWindow&, const Glib::ustring&);
  static bool cmd_open(MainWindow&, const Glib::ustring&);
  static bool cmd_open_tab(MainWindow&, const Glib::ustring&);
  static bool cmd_quit(MainWindow&, const Glib::ustring&);
  static bool cmd_quit_all(MainWindow&, const Glib::ustring&);
  static bool cmd_reload(MainWindow&, const Glib::ustring&);
  static bool cmd_force_reload(MainWindow&, const Glib::ustring&);
  static bool cmd_set(MainWindow&, const Glib::ustring&);
  static bool cmd_split(MainWindow&, const Glib::ustring&);
  static bool cmd_stop(MainWindow&, const Glib::ustring&);
  static bool cmd_tab_next(MainWindow&, const Glib::ustring&);
  static bool cmd_tab_prev(MainWindow&, const Glib::ustring&);
  static bool cmd_vsplit(MainWindow&, const Glib::ustring&);
  static void js_command_callback(::GObject*, ::GAsyncResult*, ::gpointer);

  static const std::vector<Command> command_list =
  {
    { "hint", "h", cmd_hint_mode },
    { "insert", "i", cmd_insert_mode },
    { "js", nullptr, cmd_js },
    { "only", "on", cmd_only },
    { "open", "o", cmd_open },
    { "open-tab", "ot", cmd_open_tab },
    { "quit", "q", cmd_quit },
    { "quit-all", "qa", cmd_quit_all },
    { "reload", "r", cmd_reload },
    { "reload!", "r!", cmd_force_reload },
    { "set", "se", cmd_set },
    { "split", "sp", cmd_split },
    { "stop", "s", cmd_stop },
    { "tab-next", "tn", cmd_tab_next },
    { "tab-prev", "tp", cmd_tab_prev },
    { "vsplit", "vs", cmd_vsplit },
    { "vsp", nullptr, cmd_vsplit },
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
  split_command(
    const Glib::ustring& input,
    Glib::ustring& name,
    Glib::ustring& args
  )
  {
    const auto pos = input.find(' ');

    if (pos == Glib::ustring::npos)
    {
      name = utils::trim(input);
    } else {
      name = utils::trim(input.substr(0, pos));
      args = utils::trim(input.substr(pos + 1));
    }
  }

  bool
  MainWindow::run_command(const Glib::ustring& command, bool interactive)
  {
    Glib::ustring name;
    Glib::ustring args;
    command_mapping_type::const_iterator entry;

    // Skip empty commands and comments.
    if (
      utils::is_blank(command) ||
      utils::starts_with(utils::trim_start(command), '"')
    )
    {
      return true;
    }

    if (interactive)
    {
      // Activate search mode.
      if (command[0] == '/')
      {
        if (const auto tab = get_current_tab())
        {
          tab->search(command.substr(1));
        }

        return true;
      }
      // Activate backwards search mode.
      else if (command[0] == '?')
      {
        if (const auto tab = get_current_tab())
        {
          tab->search(command.substr(1), false);
        }

        return true;
      }
    }

    if (command[0] == ':' && command.length() > 1)
    {
      split_command(command.substr(1), name, args);
    }
    else if (!interactive)
    {
      split_command(command, name, args);
    }

    if (name.empty())
    {
      show_error("Missing command.");

      return false;
    }

    entry = m_command_mapping.find(name.raw());
    if (entry != std::end(m_command_mapping))
    {
      return entry->second.callback(*this, args);
    }

    show_error("Unknown command: " + command);

    return false;
  }

  static bool
  cmd_hint_mode(MainWindow& window, const Glib::ustring&)
  {
    window.set_mode(Mode::HINT);

    return true;
  }

  static bool
  cmd_insert_mode(MainWindow& window, const Glib::ustring&)
  {
    window.set_mode(Mode::INSERT);

    return true;
  }

  static bool
  cmd_js(MainWindow& window, const Glib::ustring& args)
  {
    if (utils::is_blank(args))
    {
      window.show_error("Usage: :js <script>");

      return false;
    }
    if (const auto tab = window.get_current_tab())
    {
      tab->execute_script(
        args,
        nullptr,
        js_command_callback,
        static_cast<void*>(tab)
      );

      return true; // TODO: Try to get JavaScript execution status.
    }

    window.show_error("No tab to run JavaScript in.");

    return false;
  }

  static void
  js_command_callback(::GObject* web_view_object,
                      ::GAsyncResult* result,
                      ::gpointer tab_data)
  {
    const auto tab = static_cast<Tab*>(tab_data);
    const auto window = tab ? tab->get_main_window() : nullptr;
    ::GError* error = nullptr;
    const auto value = ::webkit_web_view_evaluate_javascript_finish(
      WEBKIT_WEB_VIEW(web_view_object),
      result,
      &error
    );

    if (!value)
    {
      if (window && error)
      {
        window->show_error(error->message);
      }
      if (error)
      {
        ::g_error_free(error);
      }
      return;
    }

    if (window)
    {
      if (const auto exception = ::jsc_context_get_exception(
            ::jsc_value_get_context(value)
          ))
      {
        window->show_error(::jsc_exception_get_message(exception));
      } else {
        window->show_info(utils::js_value_to_text(value));
      }
    }
    ::g_object_unref(value);
  }

  static bool
  cmd_open(MainWindow& window, const Glib::ustring& args)
  {
    if (const auto tab = window.get_current_tab())
    {
      tab->load_uri(args);
    } else {
      window.open_tab(args);
    }

    return true;
  }

  static bool
  cmd_open_tab(MainWindow& window, const Glib::ustring& args)
  {
    window.open_tab(args);

    return true;
  }

  static bool
  cmd_only(MainWindow& window, const Glib::ustring&)
  {
    if (const auto tab = window.get_current_tab())
    {
      tab->close_other_views();

      return true;
    }
    window.show_error("No tab to display.");

    return false;
  }

  static bool
  cmd_quit(MainWindow& window, const Glib::ustring&)
  {
    if (const auto tab = window.get_current_tab())
    {
      tab->close_active_view();
    } else {
      window.close();
    }

    return true;
  }

  static bool
  cmd_quit_all(MainWindow& window, const Glib::ustring&)
  {
    window.close();

    return true;
  }

  static bool
  do_reload(MainWindow& window, bool force)
  {
    if (const auto tab = window.get_current_tab())
    {
      tab->reload(force);

      return true;
    }
    window.show_error("No tab to reload.");

    return false;
  }

  static bool
  cmd_reload(MainWindow& window, const Glib::ustring&)
  {
    return do_reload(window, false);
  }

  static bool
  cmd_force_reload(MainWindow& window, const Glib::ustring&)
  {
    return do_reload(window, true);
  }

  static bool
  cmd_set(MainWindow& window, const Glib::ustring& args)
  {
    const auto index = args.find('=');

    if (index != Glib::ustring::npos)
    {
      const auto name = utils::trim(args.substr(0, index));
      const auto value = utils::trim(args.substr(index + 1));

      if (!setting::set(window, name, value))
      {
        window.show_error("Unknown setting: " + name);

        return false;
      }

      return true;
    }
    else if (!utils::is_blank(args))
    {
      const auto name = utils::trim(args);

      if (const auto value = setting::get(window, name))
      {
        window.show_info(std::format("  {}={}", name.raw(), value->raw()));

        return true;
      }
      window.show_error("Unknown setting: " + name.raw());

      return false;
    }
    window.show_error("Missing setting name.");

    return false;
  }

  static bool
  do_split(MainWindow& window, bool vertical, const Glib::ustring& uri)
  {
    if (const auto tab = window.get_current_tab())
    {
      tab->split(vertical, uri);

      return true;
    }
    window.show_error("No tab to split.");

    return false;
  }

  static bool
  cmd_split(MainWindow& window, const Glib::ustring& args)
  {
    return do_split(window, false, args);
  }

  static bool
  cmd_stop(MainWindow& window, const Glib::ustring&)
  {
    if (const auto tab = window.get_current_tab())
    {
      tab->stop_loading();

      return true;
    }
    window.show_error("No tab to stop loading of.");

    return false;
  }

  static bool
  cmd_tab_prev(MainWindow& window, const Glib::ustring&)
  {
    window.prev_tab();

    return true;
  }

  static bool
  cmd_tab_next(MainWindow& window, const Glib::ustring&)
  {
    window.next_tab();

    return true;
  }

  static bool
  cmd_vsplit(MainWindow& window, const Glib::ustring& args)
  {
    return do_split(window, true, args);
  }
}
