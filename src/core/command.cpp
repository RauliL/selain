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
#include <format>
#include <type_traits>

#include <glibmm/ustring_hash.h>

#include <selain/core/command.hpp>
#include <selain/core/setting.hpp>
#include <selain/keyboard.hpp>
#include <selain/ui/main-window.hpp>
#include <selain/ui/tab.hpp>
#include <selain/utils.hpp>

using tab_callback_type = std::function<bool(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring
)>;
using paste_callback_type = std::function<void(const Glib::ustring&)>;

static selain::command::callback_type switch_mode(selain::Mode);
static selain::command::callback_type require_tab(const tab_callback_type&);

// Commands that do not require tab.
static bool cmd_bind(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_command(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_open(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_open_tab(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_paste_tab(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_quit(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_quit_all(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_set(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_tab(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_tab_next(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_tab_prev(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_unbind(selain::ui::MainWindow&, Glib::ustring);

// Commands that require tab.
static bool cmd_back(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_force_reload(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_forward(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_inspector(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_js(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_only(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_paste(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_reload(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_scroll(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_search_next(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_search_prev(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_split(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_split_focus(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_stop(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_vsplit(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_yank(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);

namespace selain::command
{
  static const std::vector<Info> list =
  {
    {
      "back", { "b" },
      "Goes back in browsing history.",
      require_tab(cmd_back)
    },
    {
      "bind", {},
      "Add keyboard binding.",
      cmd_bind
    },
    {
      "command", { "c" },
      "Switches to command mode.",
      cmd_command
    },
    {
      "forward", { "f" },
      "Goes forward in browsing history.",
      require_tab(cmd_forward)
    },
    {
      "hint", { "h" },
      "Switches to hint mode.",
      switch_mode(selain::Mode::HINT)
    },
    {
      "hint-tab", { "ht" },
      "Switches to open new tab hint mode.",
      switch_mode(selain::Mode::HINT_NEW_TAB)
    },
    {
      "insert", { "i" },
      "Switches to insert mode.",
      switch_mode(selain::Mode::INSERT)
    },
    {
      "inspector", { "insp" },
      "Toggles developer tools.",
      require_tab(cmd_inspector)
    },
    {
      "js", { "j" },
      "Runs JavaScript in the current tab.",
      require_tab(cmd_js)
    },
    {
      "only", { "on" },
      "Closes all splits except the current.",
      require_tab(cmd_only),
    },
    {
      "open", { "o" },
      "Opens URI given as argument.",
      cmd_open
    },
    {
      "open-tab", { "ot" },
      "Opens URI given as argument in new tab.",
      cmd_open_tab
    },
    {
      "paste", { "p" },
      "Opens URI from clipboard.",
      require_tab(cmd_paste)
    },
    {
      "paste-tab", { "pt" },
      "Opens URI from cliboard in new tab.",
      cmd_paste_tab
    },
    {
      "qall", { "qa" },
      "Closes all tabs.",
      cmd_quit_all
    },
    {
      "quit", { "q" },
      "Closes current split, or tab if alone.",
      cmd_quit
    },
    {
      "reload", { "r" },
      "Reloads page.",
      require_tab(cmd_reload)
    },
    {
      "reload!", { "r!" },
      "Reloads page bypassing the cache.",
      require_tab(cmd_force_reload)
    },
    {
      "scroll", { "sc" },
      "Scrolls the current tab.",
      require_tab(cmd_scroll)
    },
    {
      "search-next", { "sen" },
      "Goes to next search result.",
      require_tab(cmd_search_next)
    },
    {
      "search-prev", { "sep" },
      "Goes to previous search result.",
      require_tab(cmd_search_prev)
    },
    {
      "set", { "se" },
      "Views/changes an setting.",
      cmd_set
    },
    {
      "split", { "sp" },
      "Splits the current view horizontally.",
      require_tab(cmd_split)
    },
    {
      "split-focus", { "spf" },
      "Changes focus of splitted view.",
      require_tab(cmd_split_focus)
    },
    {
      "stop", { "s" },
      "Stops page from loading content.",
      require_tab(cmd_stop)
    },
    {
      "tab", { "t" },
      "Switches to tab given as 1-based index.",
      cmd_tab
    },
    {
      "tab-next", { "tn" },
      "Switches to next tab.",
      cmd_tab_next
    },
    {
      "tab-prev", { "tp" },
      "Switches to previous tab.",
      cmd_tab_prev
    },
    {
      "unbind", {},
      "Remove keyboard binding.",
      cmd_unbind
    },
    {
      "vsplit", { "vs", "vsp" },
      "Splits the current view vertically.",
      require_tab(cmd_vsplit)
    },
    {
      "yank", { "y" },
      "Copies current URI to clipboard.",
      require_tab(cmd_yank)
    },
  };

  static std::unordered_map<Glib::ustring, callback_type> mapping;

  std::vector<Glib::ustring>
  names()
  {
    std::vector<Glib::ustring> result;

    result.reserve(mapping.size());
    for (const auto& entry : mapping)
    {
      result.push_back(entry.first);
    }
    std::sort(std::begin(result), std::end(result));

    return result;
  }

  void
  initialize()
  {
    for (const auto& info : list)
    {
      mapping[info.name] = info.callback;
      for (const auto& alias : info.aliases)
      {
        mapping[alias] = info.callback;
      }
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
  using selain::utils::trim;

  const auto pos = input.find(' ');

  if (pos == Glib::ustring::npos)
  {
    name = trim(input);
  } else {
    name = trim(input.substr(0, pos));
    args = trim(input.substr(pos + 1));
  }
}

namespace selain::ui
{
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

    const auto entry = command::mapping.find(name);

    if (entry != std::end(command::mapping))
    {
      return entry->second(*this, args);
    }

    show_error("Unknown command: " + command);

    return false;
  }
}

static selain::command::callback_type
switch_mode(selain::Mode mode)
{
  return [mode](selain::ui::MainWindow& window, Glib::ustring)
  {
    window.set_mode(mode);

    return true;
  };
}

static selain::command::callback_type
require_tab(const tab_callback_type& callback)
{
  return [callback](selain::ui::MainWindow& window, Glib::ustring args)
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
do_paste(selain::ui::MainWindow& window, const paste_callback_type& callback)
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
do_reload(selain::ui::Tab& tab, bool force)
{
  tab.reload(force);

  return true;
}

static bool
do_split(selain::ui::Tab& tab, bool vertical, Glib::ustring uri)
{
  tab.split(vertical, uri);

  return true;
}

static bool
require_arg(
  selain::ui::MainWindow& window,
  const char* what,
  Glib::ustring& args,
  Glib::ustring& slot
)
{
  args = selain::utils::trim_start(args);

  if (args.empty())
  {
    window.show_error(std::format("Missing {}.", what));

    return false;
  }

  const auto length = args.length();
  std::size_t i;

  for (i = 0; i < length; ++i)
  {
    if (std::isspace(args[i]))
    {
      break;
    }
  }

  slot = args.substr(0, i);
  args = selain::utils::trim_start(args.substr(i));

  return true;
}

static bool
require_int_arg(
  selain::ui::MainWindow& window,
  const char* what,
  Glib::ustring& args,
  int& slot
)
{
  Glib::ustring arg;

  if (!require_arg(window, what, args, arg))
  {
    return false;
  }
  try
  {
    slot = std::atoi(arg.raw().c_str());

    return true;
  }
  catch (const std::exception&)
  {
    window.show_error(std::format("Invalid {}: {}", what, arg.raw()));

    return false;
  }
}

template<
  typename T,
  typename = typename std::enable_if<std::is_enum<T>::value>::type
>
static bool
require_enum_arg(
  selain::ui::MainWindow& window,
  const char* what,
  Glib::ustring& args,
  T& slot,
  const std::unordered_map<Glib::ustring, T>& mapping
)
{
  Glib::ustring arg;

  if (require_arg(window, what, args, arg))
  {
    const auto it = mapping.find(arg);

    if (it != std::end(mapping))
    {
      slot = it->second;

      return true;
    }
    window.show_error(std::format("Unrecognized {}: {}", what, arg.raw()));
  }

  return false;
}

// Commands that do not require tab.

static bool
cmd_bind(selain::ui::MainWindow& window, Glib::ustring args)
{
  Glib::ustring sequence;

  if (!require_arg(window, "sequence", args, sequence))
  {
    return false;
  }
  else if (selain::utils::is_blank(args))
  {
    window.show_error("Missing command.");

    return false;
  }
  selain::keyboard::bind(sequence, args);

  return true;
}

static bool
cmd_command(selain::ui::MainWindow& window, Glib::ustring args)
{
  if (!args.empty())
  {
    window.get_command_entry().set_text(args);
  }
  window.set_mode(selain::Mode::COMMAND);

  return true;
}

static bool
cmd_open(selain::ui::MainWindow& window, Glib::ustring args)
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
cmd_open_tab(selain::ui::MainWindow& window, Glib::ustring args)
{
  window.open_tab(args);

  return true;
}

static bool
cmd_paste_tab(selain::ui::MainWindow& window, Glib::ustring)
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
cmd_quit(selain::ui::MainWindow& window, Glib::ustring)
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
cmd_quit_all(selain::ui::MainWindow& window, Glib::ustring)
{
  window.schedule_close();

  return true;
}

static bool
cmd_set(selain::ui::MainWindow& window, Glib::ustring args)
{
  const auto index = args.find('=');

  if (index != Glib::ustring::npos)
  {
    const auto name = selain::utils::trim(args.substr(0, index));
    const auto value = selain::utils::trim(args.substr(index + 1));

    if (!selain::setting::set(window, name, value))
    {
      window.show_error("Unknown setting: " + name);

      return false;
    }

    return true;
  }
  else if (!selain::utils::is_blank(args))
  {
    const auto name = selain::utils::trim(args);

    if (const auto value = selain::setting::get(window, name))
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
cmd_tab(selain::ui::MainWindow& window, Glib::ustring args)
{
  int number;

  if (!require_int_arg(window, "tab number", args, number))
  {
    return false;
  }
  else if (number < 1)
  {
    window.show_error("Invalid tab number.");

    return false;
  }
  window.set_current_tab(number - 1);

  return true;
}

static bool
cmd_tab_prev(selain::ui::MainWindow& window, Glib::ustring)
{
  window.prev_tab();

  return true;
}

static bool
cmd_tab_next(selain::ui::MainWindow& window, Glib::ustring)
{
  window.next_tab();

  return true;
}

static bool
cmd_unbind(selain::ui::MainWindow& window, Glib::ustring args)
{
  if (selain::utils::is_blank(args))
  {
    window.show_error("Missing sequence.");
  }
  else if (selain::keyboard::unbind(selain::utils::trim(args)))
  {
    return true;
  }
  window.show_error("No such binding.");

  return false;
}

// Commands that require tab.

static bool
cmd_back(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring)
{
  tab.go_back();

  return true;
}

static bool
cmd_forward(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring)
{
  tab.go_forward();

  return true;
}

static bool
cmd_inspector(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring)
{
  // TODO: Add support for argument such as on/off.
  tab.toggle_inspector();

  return true;
}

static void
js_command_callback(
  GObject* web_view_object,
  GAsyncResult* result,
  gpointer tab_data
)
{
  const auto tab = static_cast<selain::ui::Tab*>(tab_data);
  const auto window = tab ? tab->get_main_window() : nullptr;
  GError* error = nullptr;
  const auto value = webkit_web_view_evaluate_javascript_finish(
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
      g_error_free(error);
    }
    return;
  }

  if (window)
  {
    const auto exception = jsc_context_get_exception(
      jsc_value_get_context(value)
    );

    if (exception)
    {
      window->show_error(jsc_exception_get_message(exception));
    } else {
      window->show_info(selain::utils::js_value_to_text(value));
    }
  }
  g_object_unref(value);
}

static bool
cmd_js(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring args
)
{
  if (selain::utils::is_blank(args))
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
cmd_only(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring)
{
  tab.close_other_views();

  return true;
}

static bool
cmd_paste(selain::ui::MainWindow& window, selain::ui::Tab& tab, Glib::ustring)
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
cmd_reload(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring)
{
  return do_reload(tab, false);
}

static bool
cmd_force_reload(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring)
{
  return do_reload(tab, true);
}

static bool
cmd_scroll(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring args
)
{
  // TODO: Allow finer control by accepting direction _and_ amount as
  // arguments.
  const auto arg = selain::utils::trim(args);
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
cmd_search_next(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring)
{
  tab.search_next();

  return true;
}

static bool
cmd_search_prev(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring)
{
  tab.search_prev();

  return true;
}

static bool
cmd_split(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring args)
{
  return do_split(tab, false, args);
}

static bool
cmd_split_focus(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring args
)
{
  static const std::unordered_map<Glib::ustring, Gtk::DirectionType> mapping =
  {
    { "left", Gtk::DirectionType::LEFT },
    { "right", Gtk::DirectionType::RIGHT },
    { "up", Gtk::DirectionType::UP },
    { "down", Gtk::DirectionType::DOWN },
    { "next", Gtk::DirectionType::TAB_FORWARD },
    { "prev", Gtk::DirectionType::TAB_BACKWARD },
  };

  Gtk::DirectionType direction;

  if (require_enum_arg(window, "direction", args, direction, mapping))
  {
    tab.focus_view(direction);

    return true;
  }

  return false;
}

static bool
cmd_stop(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring)
{
  tab.stop_loading();

  return true;
}

static bool
cmd_vsplit(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring args)
{
  return do_split(tab, true, args);
}

static bool
cmd_yank(selain::ui::MainWindow& window, selain::ui::Tab& tab, Glib::ustring)
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
