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

#include <glibmm/ustring_hash.h>

namespace selain
{
  using callback = std::function<bool(
    MainWindow&,
    const Glib::ustring&
  )>;

  using tab_callback = std::function<bool(
    MainWindow&,
    Tab&,
    const Glib::ustring&
  )>;

  static callback switch_mode(Mode);
  static callback require_tab(const tab_callback&);

  static bool cmd_back(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_command(MainWindow&, const Glib::ustring&);
  static bool cmd_force_reload(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_forward(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_inspector(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_js(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_only(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_open(MainWindow&, const Glib::ustring&);
  static bool cmd_open_tab(MainWindow&, const Glib::ustring&);
  static bool cmd_paste(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_paste_tab(MainWindow&, const Glib::ustring&);
  static bool cmd_quit(MainWindow&, const Glib::ustring&);
  static bool cmd_quit_all(MainWindow&, const Glib::ustring&);
  static bool cmd_reload(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_scroll(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_search_next(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_search_prev(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_set(MainWindow&, const Glib::ustring&);
  static bool cmd_split(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_split_focus(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_stop(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_tab_next(MainWindow&, const Glib::ustring&);
  static bool cmd_tab_prev(MainWindow&, const Glib::ustring&);
  static bool cmd_vsplit(MainWindow&, Tab&, const Glib::ustring&);
  static bool cmd_yank(MainWindow&, Tab&, const Glib::ustring&);

  static std::unordered_map<Glib::ustring, callback> mapping =
  {
    { "b", require_tab(cmd_back) },
    { "back", require_tab(cmd_back) },
    { "c", cmd_command },
    { "command", cmd_command },
    { "f", require_tab(cmd_forward) },
    { "forward", require_tab(cmd_forward) },
    { "h", switch_mode(Mode::HINT) },
    { "hint", switch_mode(Mode::HINT) },
    { "hint-tab", switch_mode(Mode::HINT_NEW_TAB) },
    { "ht", switch_mode(Mode::HINT_NEW_TAB) },
    { "i", switch_mode(Mode::INSERT) },
    { "insert", switch_mode(Mode::INSERT) },
    { "insp", require_tab(cmd_inspector) },
    { "inspector", require_tab(cmd_inspector) },
    { "js", require_tab(cmd_js) },
    { "o", cmd_open },
    { "on", require_tab(cmd_only) },
    { "only", require_tab(cmd_only) },
    { "open", cmd_open },
    { "open-tab", cmd_open_tab },
    { "ot", cmd_open_tab },
    { "p", require_tab(cmd_paste) },
    { "paste", require_tab(cmd_paste) },
    { "paste-tab", cmd_paste_tab },
    { "pt", cmd_paste_tab },
    { "q", cmd_quit },
    { "qa", cmd_quit_all },
    { "quit", cmd_quit },
    { "quit-all", cmd_quit_all },
    { "r!", require_tab(cmd_force_reload) },
    { "r", require_tab(cmd_reload) },
    { "reload!", require_tab(cmd_force_reload) },
    { "reload", require_tab(cmd_reload) },
    { "s", require_tab(cmd_stop) },
    { "sc", require_tab(cmd_scroll) },
    { "scroll", require_tab(cmd_scroll) },
    { "se", cmd_set },
    { "search-next", require_tab(cmd_search_next) },
    { "search-prev", require_tab(cmd_search_prev) },
    { "sen", require_tab(cmd_search_next) },
    { "sep", require_tab(cmd_search_prev) },
    { "set", cmd_set },
    { "sp", require_tab(cmd_split) },
    { "spf", require_tab(cmd_split_focus) },
    { "split", require_tab(cmd_split) },
    { "split-focus", require_tab(cmd_split_focus) },
    { "stop", require_tab(cmd_stop) },
    { "tab-next", cmd_tab_next },
    { "tab-prev", cmd_tab_prev },
    { "tn", cmd_tab_next },
    { "tp", cmd_tab_prev },
    { "vs", require_tab(cmd_vsplit) },
    { "vsp", require_tab(cmd_vsplit) },
    { "vsplit", require_tab(cmd_vsplit) },
    { "y", require_tab(cmd_yank) },
    { "yank", require_tab(cmd_yank) },
  };

  namespace command
  {
    std::vector<Glib::ustring>
    names()
    {
      std::vector<Glib::ustring> result;

      result.reserve(mapping.size());
      for (const auto& [key, _] : mapping)
      {
        result.push_back(key);
      }
      std::sort(std::begin(result), std::end(result));

      return result;
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

    const auto entry = mapping.find(name);

    if (entry != std::end(mapping))
    {
      return entry->second(*this, args);
    }

    show_error("Unknown command: " + command);

    return false;
  }

  static callback
  switch_mode(Mode mode)
  {
    return [mode](MainWindow& window, const Glib::ustring&)
    {
      window.set_mode(mode);

      return true;
    };
  }

  static callback
  require_tab(const tab_callback& callback)
  {
    return [callback](MainWindow& window, const Glib::ustring& args)
    {
      if (const auto tab = window.get_current_tab())
      {
        return callback(window, *tab, args);
      }
      window.show_error("No tab open.");

      return false;
    };
  }

  static bool
  cmd_inspector(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    // TODO: Add support for argument such as on/off.
    tab.toggle_inspector();

    return true;
  }

  static void
  js_command_callback(
    ::GObject* web_view_object,
    ::GAsyncResult* result,
    ::gpointer tab_data
  )
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
      const auto exception = ::jsc_context_get_exception(
        ::jsc_value_get_context(value)
      );

      if (exception)
      {
        window->show_error(::jsc_exception_get_message(exception));
      } else {
        window->show_info(utils::js_value_to_text(value));
      }
    }
    ::g_object_unref(value);
  }

  static bool
  cmd_js(MainWindow& window, Tab& tab, const Glib::ustring& args)
  {
    if (utils::is_blank(args))
    {
      window.show_error("Usage: :js <script>");

      return false;
    }
    tab.execute_script(
      args,
      nullptr,
      js_command_callback,
      static_cast<void*>(&tab)
    );

    return true; // TODO: Try to get JavaScript execution status.
  }

  static bool
  cmd_back(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    tab.go_back();

    return true;
  }

  static bool
  cmd_command(MainWindow& window, const Glib::ustring& args)
  {
    if (!args.empty())
    {
      window.get_command_entry().set_text(args);
    }
    window.set_mode(Mode::COMMAND);

    return true;
  }

  static bool
  cmd_forward(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    tab.go_forward();

    return true;
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
  cmd_only(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    tab.close_other_views();

    return true;
  }

  static bool
  do_paste(
    MainWindow& window,
    const std::function<void(const Glib::ustring&)>& callback
  )
  {
    const auto clipboard = window.get_clipboard();

    clipboard->read_text_async(
      [callback, clipboard](const Glib::RefPtr<Gio::AsyncResult>& result)
      {
        try
        {
          const auto uri = clipboard->read_text_finish(result);

          // TODO: Do not even attempt if it doesn't look like an URI.
          if (!uri.empty())
          {
            callback(uri);
          }
        }
        catch (const Glib::Error&) {}
      }
    );

    return true;
  }

  static bool
  cmd_paste(MainWindow& window, Tab& tab, const Glib::ustring&)
  {
    return do_paste(
      window,
      [&tab](const Glib::ustring& uri)
      {
        tab.load_uri(uri);
      }
    );
  }

  static bool
  cmd_paste_tab(MainWindow& window, const Glib::ustring&)
  {
    return do_paste(
      window,
      [&window](const Glib::ustring& uri)
      {
        window.open_tab(uri);
      }
    );
  }

  static bool
  cmd_quit(MainWindow& window, const Glib::ustring&)
  {
    if (const auto tab = window.get_current_tab())
    {
      tab->close_active_view();
    } else {
      window.schedule_close();
    }

    return true;
  }

  static bool
  cmd_quit_all(MainWindow& window, const Glib::ustring&)
  {
    window.schedule_close();

    return true;
  }

  static bool
  do_reload(Tab& tab, bool force)
  {
    tab.reload(force);

    return true;
  }

  static bool
  cmd_reload(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    return do_reload(tab, false);
  }

  static bool
  cmd_force_reload(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    return do_reload(tab, true);
  }

  static bool
  cmd_scroll(MainWindow& window, Tab& tab, const Glib::ustring& args)
  {
    // TODO: Allow finer control by accepting direction _and_ amount as
    // arguments.
    const auto arg = utils::trim(args);
    Glib::ustring method = "scrollBy";
    Glib::ustring direction;
    Glib::ustring value;

    if (arg.empty())
    {
      window.show_error("Missing direction/amount.");

      return false;
    }
    else if (arg == "top")
    {
      method = "scrollTo";
      direction = "top";
      value = "0";
    }
    else if (arg == "bottom")
    {
      method = "scrollTo";
      direction = "top";
      value = "document.body.scrollHeight";
    }
    else if (arg == "left")
    {
      direction = "left";
      value = "-100";
    }
    else if (arg == "right")
    {
      direction = "left";
      value = "100";
    }
    else if (arg == "up")
    {
      direction = "top";
      value = "-100";
    }
    else if (arg == "down")
    {
      direction = "top";
      value = "100";
    }
    else if (arg == "page-up")
    {
      direction = "top";
      value = "-(window.innerHeight / 2)";
    }
    else if (arg == "page-down")
    {
      direction = "top";
      value = "window.innerHeight / 2";
    } else {
      window.show_error("Unrecognized direction.");

      return false;
    }
    tab.execute_script(std::format(
      "window.{}({{ {}: {} }});",
      method.raw(),
      direction.raw(),
      value.raw()
    ));

    return true;
  }

  static bool
  cmd_search_next(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    tab.search_next();

    return true;
  }

  static bool
  cmd_search_prev(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    tab.search_prev();

    return true;
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
  do_split(Tab& tab, bool vertical, const Glib::ustring& uri)
  {
    tab.split(vertical, uri);

    return true;
  }

  static bool
  cmd_split(MainWindow&, Tab& tab, const Glib::ustring& args)
  {
    return do_split(tab, false, args);
  }

  static bool
  cmd_split_focus(MainWindow& window, Tab& tab, const Glib::ustring& args)
  {
    FocusDirection direction;
    const auto arg = utils::trim(args);

    if (arg.empty())
    {
      window.show_error("Missing direction.");

      return false;
    }
    else if (arg == "left")
    {
      direction = FocusDirection::LEFT;
    }
    else if (arg == "right")
    {
      direction = FocusDirection::RIGHT;
    }
    else if (arg == "up")
    {
      direction = FocusDirection::UP;
    }
    else if (arg == "down")
    {
      direction = FocusDirection::DOWN;
    }
    else if (arg == "next")
    {
      tab.cycle_view(false);

      return true;
    }
    else if (arg == "prev")
    {
      tab.cycle_view(true);

      return true;
    } else {
      window.show_error("Unrecognized direction.");

      return false;
    }
    tab.focus_view(direction);

    return true;
  }

  static bool
  cmd_stop(MainWindow&, Tab& tab, const Glib::ustring&)
  {
    tab.stop_loading();

    return true;
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
  cmd_vsplit(MainWindow&, Tab& tab, const Glib::ustring& args)
  {
    return do_split(tab, true, args);
  }

  static bool
  cmd_yank(MainWindow& window, Tab& tab, const Glib::ustring&)
  {
    const auto clipboard = window.get_clipboard();
    const auto uri = tab.get_uri();

    if (uri.empty())
    {
      return false;
    }
    clipboard->set_text(uri);
    window.show_info("Yanked " + uri.raw());

    return true;
  }
}
