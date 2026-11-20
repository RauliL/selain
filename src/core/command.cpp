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
#include <algorithm>
#include <cmath>
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
using tab_method_type = std::function<void(selain::ui::Tab*)>;
using window_method_type = std::function<void(selain::ui::MainWindow*)>;
using paste_callback_type = std::function<void(const Glib::ustring&)>;

static selain::command::callback_type switch_mode(selain::Mode);
static selain::command::callback_type require_tab(const tab_callback_type&);
static selain::command::callback_type tab_method(const tab_method_type&);
static selain::command::callback_type tab_reload(bool);
static selain::command::callback_type tab_split(bool);
static selain::command::callback_type window_method(const window_method_type&);

// Commands that do not require tab.
static bool cmd_bind(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_command(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_open(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_open_tab(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_paste_tab(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_quit(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_set(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_tab(selain::ui::MainWindow&, Glib::ustring);
static bool cmd_unbind(selain::ui::MainWindow&, Glib::ustring);

// Commands that require tab.
static bool cmd_execute(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_go_root(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_go_up(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_insert_input(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_js(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_paste(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_scroll(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_split_focus(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_yank(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);
static bool cmd_zoom(selain::ui::MainWindow&, selain::ui::Tab&, Glib::ustring);

namespace selain::command
{
  static const std::vector<Info> list =
  {
    {
      "back", { "b" },
      "Goes back in browsing history.",
      tab_method(&ui::Tab::go_back)
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
      "execute", { "ex" },
      "Evaluates a JavaScript expression and runs the result as a command.",
      require_tab(cmd_execute)
    },
    {
      "forward", { "f" },
      "Goes forward in browsing history.",
      tab_method(&ui::Tab::go_forward)
    },
    {
      "go-root", { "gr" },
      "Goes up to the root of the URL hierarchy.",
      require_tab(cmd_go_root)
    },
    {
      "go-up", { "gu" },
      "Goes up one level in the URL hierarchy.",
      require_tab(cmd_go_up)
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
      "insert-input", { "ii" },
      "Focuses the first input or textarea and enters insert mode.",
      require_tab(cmd_insert_input)
    },
    {
      "inspector", { "insp" },
      "Toggles developer tools.",
      // TODO: Add support for argument such as on/off.
      tab_method(&ui::Tab::toggle_inspector)
    },
    {
      "js", { "j" },
      "Runs JavaScript in the current tab.",
      require_tab(cmd_js)
    },
    {
      "only", { "on" },
      "Closes all splits except the current.",
      tab_method(&ui::Tab::close_other_views)
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
      window_method(&ui::MainWindow::schedule_close)
    },
    {
      "quit", { "q" },
      "Closes current split, or tab if alone.",
      cmd_quit
    },
    {
      "reload", { "r" },
      "Reloads page.",
      tab_reload(false)
    },
    {
      "reload!", { "r!" },
      "Reloads page bypassing the cache.",
      tab_reload(true)
    },
    {
      "scroll", { "sc" },
      "Scrolls the current tab.",
      require_tab(cmd_scroll)
    },
    {
      "search-next", { "sen" },
      "Goes to next search result.",
      tab_method(&ui::Tab::search_next)
    },
    {
      "search-prev", { "sep" },
      "Goes to previous search result.",
      tab_method(&ui::Tab::search_prev)
    },
    {
      "set", { "se" },
      "Views/changes an setting.",
      cmd_set
    },
    {
      "split", { "sp" },
      "Splits the current view horizontally.",
      tab_split(false)
    },
    {
      "split-focus", { "spf" },
      "Changes focus of splitted view.",
      require_tab(cmd_split_focus)
    },
    {
      "stop", { "s" },
      "Stops page from loading content.",
      tab_method(&ui::Tab::stop_loading)
    },
    {
      "tab", { "t" },
      "Switches to tab given as 1-based index.",
      cmd_tab
    },
    {
      "tab-next", { "tn" },
      "Switches to next tab.",
      window_method(&ui::MainWindow::next_tab)
    },
    {
      "tab-prev", { "tp" },
      "Switches to previous tab.",
      window_method(&ui::MainWindow::prev_tab)
    },
    {
      "unbind", {},
      "Remove keyboard binding.",
      cmd_unbind
    },
    {
      "view-list", { "vl" },
      "Opens a searchable list of all open views.",
      window_method(&ui::MainWindow::show_view_list_dialog)
    },
    {
      "vsplit", { "vs", "vsp" },
      "Splits the current view vertically.",
      tab_split(true)
    },
    {
      "yank", { "y" },
      "Copies current URI to clipboard.",
      require_tab(cmd_yank)
    },
    {
      "zoom", { "z" },
      "Changes zoom level of the current view.",
      require_tab(cmd_zoom)
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

static selain::command::callback_type
tab_method(const tab_method_type& callback)
{
  return [callback](selain::ui::MainWindow& window, Glib::ustring)
  {
    if (const auto tab = window.get_current_tab())
    {
      callback(tab);

      return true;
    }
    window.show_error("No tab open.");

    return false;
  };
}

static selain::command::callback_type
tab_reload(bool force)
{
  return require_tab(
    [force](selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring)
    {
      tab.reload(force);

      return true;
    }
  );
}

static selain::command::callback_type
tab_split(bool vertical)
{
  return require_tab(
    [vertical](selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring uri)
    {
      tab.split(vertical, uri);

      return true;
    }
  );
}

static selain::command::callback_type
window_method(const window_method_type& callback)
{
  return [callback](selain::ui::MainWindow& window, Glib::ustring)
  {
    callback(&window);

    return true;
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
cmd_execute(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring args
)
{
  if (selain::utils::is_blank(args))
  {
    window.show_error("Usage: :execute <expression>");

    return false;
  }
  tab.execute_script(
    args,
    [&window](JSCValue* value, GError* error)
    {
      if (value)
      {
        const auto exception = jsc_context_get_exception(
          jsc_value_get_context(value)
        );

        if (exception)
        {
          window.show_error(jsc_exception_get_message(exception));

          return;
        }
        if (
          ::jsc_value_is_undefined(value) ||
          ::jsc_value_is_null(value)
        )
        {
          window.show_error("Expression did not produce a command.");

          return;
        }

        char* text = ::jsc_value_to_string(value);

        if (!text)
        {
          window.show_error("Expression did not produce a command.");

          return;
        }

        const Glib::ustring command(text);

        g_free(text);
        window.run_command(command, false);
      }
      else if (error)
      {
        window.show_error(error->message);
      }
    }
  );

  return true;
}

static bool
cmd_insert_input(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring
)
{
  tab.execute_script(
    R"JS(
      (() => {
        const nodes = document.querySelectorAll(
          'input:not([type="hidden"]),textarea'
        );

        for (const el of nodes) {
          const type = (el.type || "").toLowerCase();

          if (
            type === "button" ||
            type === "submit" ||
            type === "reset" ||
            type === "image" ||
            el.disabled
          ) {
            continue;
          }
          el.focus();

          return true;
        }

        return false;
      })()
    )JS",
    [&window](JSCValue* value, GError*)
    {
      if (value && ::jsc_value_to_boolean(value))
      {
        window.set_mode(selain::Mode::INSERT);
      } else {
        window.show_error("No input found.");
      }
    }
  );

  return true;
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
    [&window](JSCValue* value, GError* error)
    {
      if (value)
      {
        const auto exception = jsc_context_get_exception(jsc_value_get_context(value));

        if (exception)
        {
          window.show_error(jsc_exception_get_message(exception));
        } else {
          window.show_info(selain::utils::js_value_to_text(value));
        }
      }
      else if (error)
      {
        window.show_error(error->message);
      }
    }
  );

  return true; // TODO: Try to get JavaScript execution status.
}

static bool
cmd_go_root(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring)
{
  const auto next = selain::utils::url_hierarchy_root(tab.get_uri());

  if (next.empty())
  {
    return true;
  }
  tab.load_uri(next);

  return true;
}

static bool
cmd_go_up(selain::ui::MainWindow&, selain::ui::Tab& tab, Glib::ustring)
{
  const auto next = selain::utils::url_hierarchy_up(tab.get_uri());

  if (next.empty())
  {
    return true;
  }
  tab.load_uri(next);

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

static bool
cmd_zoom(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring args
)
{
  static constexpr double min_zoom = 0.1;
  static constexpr double max_zoom = 5.0;

  const auto arg = selain::utils::trim(args);
  const auto current = tab.get_zoom_level();

  if (arg.empty())
  {
    window.show_info(std::format(
      "Zoom: {}%",
      static_cast<int>(std::lround(current * 100.0))
    ));

    return true;
  }

  const auto& raw = arg.raw();
  double level = current;
  bool ok = false;

  try
  {
    if (!raw.empty() && raw.back() == '%')
    {
      level = std::stod(raw.substr(0, raw.size() - 1)) / 100.0;
      ok = true;
    }
    else if (!raw.empty() && (raw.front() == '+' || raw.front() == '-'))
    {
      level = current + std::stod(raw) / 100.0;
      ok = true;
    }
  }
  catch (const std::exception&)
  {
    ok = false;
  }

  if (!ok)
  {
    window.show_error(
      "Invalid zoom. Use percentage (e.g. 120%) or relative (+20, -5)."
    );

    return false;
  }

  level = std::clamp(level, min_zoom, max_zoom);
  tab.set_zoom_level(level);

  return true;
}
