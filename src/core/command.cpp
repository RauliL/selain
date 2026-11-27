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
#include <fstream>
#include <type_traits>
#include <unordered_set>

#include <cmrc/cmrc.hpp>
#include <glibmm/ustring_hash.h>

#include <selain/config.hpp>

#include <selain/core/command.hpp>
#include <selain/core/keyboard.hpp>
#include <selain/core/setting.hpp>
#include <selain/ui/list-dialog.hpp>
#include <selain/ui/main-window.hpp>
#include <selain/ui/tab.hpp>
#include <selain/utils/string.hpp>
#include <selain/utils/url.hpp>
#include <selain/webkit/userscript.hpp>

CMRC_DECLARE(selain::rc);

using tab_callback_type = std::function<void(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
)>;
using tab_method_type = std::function<void(selain::ui::Tab*)>;
using window_method_type = std::function<void(selain::ui::MainWindow*)>;
using paste_callback_type = std::function<void(const Glib::ustring&)>;

static void finish(
  const selain::command::result_callback_type& callback,
  const std::optional<Glib::ustring>& error = std::nullopt
);
static selain::command::callback_type switch_mode(selain::Mode);
static selain::command::callback_type require_tab(const tab_callback_type&);
static selain::command::callback_type tab_method(const tab_method_type&);
static selain::command::callback_type tab_reload(bool);
static selain::command::callback_type tab_split(Gtk::Orientation);
static selain::command::callback_type window_method(const window_method_type&);

// Commands that do not require tab.
static void cmd_bind(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_command(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_echo(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_history(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_history_clear(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_open(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_open_tab(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_open_closed_tab(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_paste_tab(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_quit(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_session_clear(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_session_save(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_set(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_source(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_tab(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_unbind(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_userscripts_reload(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_version(
  selain::ui::MainWindow&,
  Glib::ustring,
  selain::command::result_callback_type
);

// Commands that require tab.
static void cmd_bookmark(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_execute(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_go_root(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_go_up(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_insert_input(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_insert_search(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_js(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_login(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_paste(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_scroll(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_split_focus(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_yank(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);
static void cmd_zoom(
  selain::ui::MainWindow&,
  selain::ui::Tab&,
  Glib::ustring,
  selain::command::result_callback_type
);

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
      "bookmark", { "bm" },
      "Opens or adds bookmarks via bookmark manager.",
      require_tab(cmd_bookmark)
    },
    {
      "command", { "c" },
      "Switches to command mode.",
      cmd_command
    },
    {
      "echo", { "ec" },
      "Displays given arguments as a notification.",
      cmd_echo
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
      "history", { "hist" },
      "Opens searchable visit history.",
      cmd_history
    },
    {
      "history-clear", { "histc" },
      "Clears persistent visit history.",
      cmd_history_clear
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
      "insert-search", { "is" },
      "Focuses the first search input and enters insert mode.",
      require_tab(cmd_insert_search)
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
      "login", { "l" },
      "Fills login form with password manager.",
      require_tab(cmd_login)
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
      "open-closed-tab", { "oct" },
      "Reopens the last closed tab layout.",
      cmd_open_closed_tab
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
      "session-clear", { "sessc" },
      "Deletes the saved session file.",
      cmd_session_clear
    },
    {
      "session-save", { "sess" },
      "Writes the current session to disk now.",
      cmd_session_save
    },
    {
      "set", { "se" },
      "Views/changes an setting.",
      cmd_set
    },
    {
      "source", { "so" },
      "Execute script.",
      cmd_source
    },
    {
      "split", { "sp" },
      "Splits the current view horizontally.",
      tab_split(Gtk::Orientation::HORIZONTAL)
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
      "userscripts-reload", { "usr" },
      "Reloads GreaseMonkey-style userscripts from disk.",
      cmd_userscripts_reload
    },
    {
      "version", { "ve" },
      "Displays version number of the browser.",
      cmd_version
    },
    {
      "view-list", { "vl" },
      "Opens a searchable list of all open views.",
      window_method(&ui::MainWindow::show_view_list_dialog)
    },
    {
      "vsplit", { "vs", "vsp" },
      "Splits the current view vertically.",
      tab_split(Gtk::Orientation::VERTICAL)
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
  using selain::utils::string::trim;
  using selain::utils::string::trim_start;

  const auto pos = input.find(' ');

  if (pos == Glib::ustring::npos)
  {
    name = trim(input);
  } else {
    name = trim(input.substr(0, pos));
    // Keep trailing whitespace so prefills like "command :open " stay intact.
    args = trim_start(input.substr(pos + 1));
  }
}

namespace selain::ui
{
  void
  MainWindow::run_command(
    const Glib::ustring& command,
    bool interactive,
    const command::result_callback_type& callback
  )
  {
    Glib::ustring name;
    Glib::ustring args;

    // Skip empty commands and comments.
    if (
      utils::string::is_blank(command) ||
      utils::string::starts_with(utils::string::trim_start(command), '"')
    )
    {
      finish(callback);
      return;
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
        finish(callback);
        return;
      }
      // Activate backwards search mode.
      else if (command[0] == '?')
      {
        if (const auto tab = get_current_tab())
        {
          tab->search(command.substr(1), false);
        }
        finish(callback);
        return;
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
      finish(callback, "Missing command.");
      return;
    }

    const auto entry = command::mapping.find(name);

    if (entry == std::end(command::mapping))
    {
      finish(callback, std::format("Unknown command: {}", command.raw()));
      return;
    }

    entry->second(*this, args, std::move(callback));
  }

  static void
  run_script_line(
    MainWindow* window,
    const std::vector<Glib::ustring>& lines,
    std::vector<Glib::ustring>::size_type index,
    const command::result_callback_type& callback
  )
  {
    if (index >= lines.size())
    {
      finish(callback);
      return;
    }
    window->run_command(
      lines[index],
      false,
      [
        window,
        lines,
        index,
        callback
      ](const std::optional<Glib::ustring>& error)
      {
        if (error)
        {
          finish(
            callback,
            std::format("Line {}: {}", index + 1, error->raw())
          );
        } else {
          run_script_line(window, lines, index + 1, callback);
        }
      }
    );
  }

  // TODO: Add possibility to run JavaScript. Detect this either by filename
  // extension or some other hackery.
  void
  MainWindow::run_script(
    const std::filesystem::path& path,
    const command::result_callback_type& callback
  )
  {
    std::ifstream is(path);
    std::string line;
    std::vector<Glib::ustring> lines;

    if (!is)
    {
      finish(
        callback,
        std::format("Unable to open {} for reading.", path.string())
      );
      return;
    }
    while (std::getline(is, line))
    {
      lines.emplace_back(line);
    }
    is.close();
    run_script_line(this, lines, 0, callback);
  }
}

static void
finish(
  const selain::command::result_callback_type& callback,
  const std::optional<Glib::ustring>& error
)
{
  if (callback)
  {
    callback(error);
  }
}

static selain::command::callback_type
switch_mode(selain::Mode mode)
{
  return [mode](
    selain::ui::MainWindow& window,
    Glib::ustring,
    selain::command::result_callback_type callback
  )
  {
    window.set_mode(mode);
    finish(callback);
  };
}

static selain::command::callback_type
require_tab(const tab_callback_type& callback)
{
  return [callback](
    selain::ui::MainWindow& window,
    Glib::ustring args,
    selain::command::result_callback_type result_callback
  )
  {
    if (const auto tab = window.get_current_tab())
    {
      callback(window, *tab, args, std::move(result_callback));
      return;
    }
    finish(result_callback, "No tab open.");
  };
}

static selain::command::callback_type
tab_method(const tab_method_type& callback)
{
  return [callback](
    selain::ui::MainWindow& window,
    Glib::ustring,
    selain::command::result_callback_type result_callback
  )
  {
    if (const auto tab = window.get_current_tab())
    {
      callback(tab);
      finish(result_callback);
      return;
    }
    finish(result_callback, "No tab open.");
  };
}

static selain::command::callback_type
tab_reload(bool force)
{
  return require_tab(
    [force](
      selain::ui::MainWindow&,
      selain::ui::Tab& tab,
      Glib::ustring,
      selain::command::result_callback_type callback
    )
    {
      tab.reload(force);
      finish(callback);
    }
  );
}

static selain::command::callback_type
tab_split(Gtk::Orientation orientation)
{
  return require_tab(
    [orientation](
      selain::ui::MainWindow&,
      selain::ui::Tab& tab,
      Glib::ustring uri,
      selain::command::result_callback_type callback
    )
    {
      tab.split(orientation, uri);
      finish(callback);
    }
  );
}

static selain::command::callback_type
window_method(const window_method_type& callback)
{
  return [callback](
    selain::ui::MainWindow& window,
    Glib::ustring,
    selain::command::result_callback_type result_callback
  )
  {
    callback(&window);
    finish(result_callback);
  };
}

static void
do_paste(
  selain::ui::MainWindow& window,
  const paste_callback_type& on_uri,
  selain::command::result_callback_type callback
)
{
  const auto clipboard = window.get_clipboard();

  clipboard->read_text_async(
    [on_uri, clipboard, callback = std::move(callback)](
      const Glib::RefPtr<Gio::AsyncResult>& result
    )
    {
      try
      {
        const auto uri = clipboard->read_text_finish(result);

        if (!selain::utils::string::is_blank(uri))
        {
          on_uri(uri);
        }
        finish(callback);
      }
      catch (const Glib::Error& error)
      {
        finish(callback, error.what());
      }
    }
  );
}

static std::optional<Glib::ustring>
require_arg(
  const char* what,
  Glib::ustring& args,
  Glib::ustring& slot
)
{
  args = selain::utils::string::trim_start(args);

  if (args.empty())
  {
    return std::format("Missing {}.", what);
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
  args = selain::utils::string::trim_start(args.substr(i));

  return std::nullopt;
}

static std::optional<Glib::ustring>
require_int_arg(
  const char* what,
  Glib::ustring& args,
  int& slot
)
{
  Glib::ustring arg;

  if (const auto error = require_arg(what, args, arg))
  {
    return error;
  }
  try
  {
    slot = std::atoi(arg.raw().c_str());

    return std::nullopt;
  }
  catch (const std::exception&)
  {
    return std::format("Invalid {}: {}", what, arg.raw());
  }
}

template<
  typename T,
  typename = typename std::enable_if<std::is_enum<T>::value>::type
>
static std::optional<Glib::ustring>
require_enum_arg(
  const char* what,
  Glib::ustring& args,
  T& slot,
  const std::unordered_map<Glib::ustring, T>& mapping
)
{
  Glib::ustring arg;

  if (const auto error = require_arg(what, args, arg))
  {
    return error;
  }

  const auto it = mapping.find(arg);

  if (it != std::end(mapping))
  {
    slot = it->second;

    return std::nullopt;
  }

  return std::format("Unrecognized {}: {}", what, arg.raw());
}

// Commands that do not require tab.

static void
cmd_bind(
  selain::ui::MainWindow&,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  Glib::ustring sequence;

  if (const auto error = require_arg("sequence", args, sequence))
  {
    finish(callback, error);
    return;
  }
  if (selain::utils::string::is_blank(args))
  {
    finish(callback, "Missing command.");
    return;
  }
  selain::keyboard::bind(sequence, args);
  finish(callback);
}

static void
cmd_command(
  selain::ui::MainWindow& window,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  if (!args.empty())
  {
    window.get_command_entry().set_text(args);
  }
  window.set_mode(selain::Mode::COMMAND);
  finish(callback);
}

static void
cmd_echo(
  selain::ui::MainWindow& window,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  window.show_info(args);
  finish(callback);
}

static void
cmd_open(
  selain::ui::MainWindow& window,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  if (const auto tab = window.get_current_tab())
  {
    tab->load_uri(args);
  } else {
    window.open_tab(args);
  }
  finish(callback);
}

static void
cmd_open_tab(
  selain::ui::MainWindow& window,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  window.open_tab(args);
  finish(callback);
}

static void
cmd_open_closed_tab(
  selain::ui::MainWindow& window,
  Glib::ustring,
  selain::command::result_callback_type callback
)
{
  if (!window.restore_last_closed_tab())
  {
    window.show_info("No closed tab to reopen.");
  }
  finish(callback);
}

static void
cmd_session_clear(
  selain::ui::MainWindow& window,
  Glib::ustring,
  selain::command::result_callback_type callback
)
{
  if (!window.clear_session())
  {
    window.show_error("Failed to clear session file.");
  } else {
    window.show_info("Session cleared.");
  }
  finish(callback);
}

static void
cmd_session_save(
  selain::ui::MainWindow& window,
  Glib::ustring,
  selain::command::result_callback_type callback
)
{
  window.flush_session_save(true);
  window.show_info("Session saved.");
  finish(callback);
}

static void
cmd_paste_tab(
  selain::ui::MainWindow& window,
  Glib::ustring,
  selain::command::result_callback_type callback
)
{
  do_paste(
    window,
    [&window](const Glib::ustring& uri)
    {
      window.open_tab(uri);
    },
    std::move(callback)
  );
}

static void
cmd_quit(
  selain::ui::MainWindow& window,
  Glib::ustring,
  selain::command::result_callback_type callback
)
{
  if (const auto tab = window.get_current_tab())
  {
    tab->close_active_view();
  } else {
    window.schedule_close();
  }
  finish(callback);
}

static void
cmd_set(
  selain::ui::MainWindow& window,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  using selain::utils::string::trim;

  const auto index = args.find('=');

  if (index != Glib::ustring::npos)
  {
    const auto name = trim(args.substr(0, index));
    const auto value = trim(args.substr(index + 1));

    finish(callback, selain::setting::set(window, name, value));
    return;
  }
  if (!selain::utils::string::is_blank(args))
  {
    const auto name = trim(args);

    if (const auto value = selain::setting::get(window, name))
    {
      window.show_info(std::format("  {}={}", name.raw(), value->raw()));
      finish(callback);
      return;
    }
    finish(callback, "Unknown setting: " + name.raw());
    return;
  }
  finish(callback, "Missing setting name.");
}

static void
cmd_source(
  selain::ui::MainWindow& window,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  args = selain::utils::string::trim(args);
  if (!args.empty())
  {
    // TODO: Expand tilde in the path.
    window.run_script(args.raw(), callback);
  }
}

static void
cmd_tab(
  selain::ui::MainWindow& window,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  int number = -1;

  if (const auto error = require_int_arg("tab number", args, number))
  {
    finish(callback, error);
    return;
  }
  if (number < 1)
  {
    finish(callback, "Invalid tab number.");
    return;
  }
  window.set_current_tab(number - 1);
  finish(callback);
}

static void
cmd_unbind(
  selain::ui::MainWindow&,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  args = selain::utils::string::trim(args);
  if (args.empty())
  {
    finish(callback, "Missing sequence.");
    return;
  }
  if (selain::keyboard::unbind(args))
  {
    finish(callback);
    return;
  }
  finish(callback, "No such binding.");
}

static void
cmd_userscripts_reload(
  selain::ui::MainWindow& window,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  if (!selain::utils::string::trim(args).empty())
  {
    finish(callback, "Usage: :userscripts-reload");
    return;
  }

  selain::webkit::userscript::reload(nullptr);
  window.show_info("Userscripts reloaded.");
  finish(callback);
}

static void
cmd_version(
  selain::ui::MainWindow& window,
  Glib::ustring,
  selain::command::result_callback_type callback
)
{
  window.show_info(std::format("v{}", SELAIN_VERSION));
  finish(callback);
}

// Commands that require tab.

namespace
{
  enum class BookmarkMode
  {
    OPEN,
    ADD,
  };

  struct BookmarkOptions
  {
    BookmarkMode mode = BookmarkMode::OPEN;
    bool new_tab = false;
    std::vector<Glib::ustring> tokens;
  };
}

static bool
tab_still_open(selain::ui::MainWindow& window, selain::ui::Tab* tab)
{
  if (!tab)
  {
    return false;
  }
  for (int i = 0; i < window.get_tab_count(); ++i)
  {
    if (window.get_nth_tab(i) == tab)
    {
      return true;
    }
  }

  return false;
}

static std::vector<Glib::ustring>
split_args(const Glib::ustring& args)
{
  std::vector<Glib::ustring> tokens;
  Glib::ustring current;
  bool in_token = false;

  for (const auto ch : args)
  {
    if (g_unichar_isspace(ch))
    {
      if (in_token)
      {
        tokens.push_back(current);
        current.clear();
        in_token = false;
      }
      continue;
    }
    current.push_back(ch);
    in_token = true;
  }
  if (in_token)
  {
    tokens.push_back(current);
  }

  return tokens;
}

static std::optional<BookmarkOptions>
parse_bookmark_options(Glib::ustring& args)
{
  BookmarkOptions options = {};

  options.tokens = split_args(selain::utils::string::trim(args));

  if (options.tokens.empty())
  {
    return options;
  }

  const auto& first = options.tokens.front();

  if (first == "-a" || first == "--add")
  {
    options.mode = BookmarkMode::ADD;
    options.tokens.erase(options.tokens.begin());

    return options;
  }
  else if (first == "-t" || first == "--tab")
  {
    options.new_tab = true;
    options.tokens.erase(options.tokens.begin());

    return options;
  }
  if (first.size() > 1 && first[0] == '-')
  {
    return std::nullopt;
  }

  return options;
}

static void
show_history(
  selain::ui::MainWindow& window,
  selain::ui::Tab* tab,
  bool new_tab,
  const std::vector<selain::HistoryManager::Entry>& entries,
  const Glib::ustring& initial_filter,
  selain::command::result_callback_type callback
)
{
  if (entries.empty())
  {
    finish(callback, "No history entries found.");
    return;
  }

  std::vector<selain::ui::ListDialogEntry> dialog_entries;

  dialog_entries.reserve(entries.size());
  for (const auto& visit : entries)
  {
    selain::ui::ListDialogEntry entry;

    entry.title = visit.title.empty() ? visit.uri : visit.title;
    entry.secondary = visit.uri;
    entry.filter_text = visit.title + ' ' + visit.uri;
    entry.icon_name = "document-open-recent";
    dialog_entries.push_back(std::move(entry));
  }

  selain::ui::ListDialog::show(
    window,
    "History",
    "Filter history…",
    std::move(dialog_entries),
    [&window, tab, new_tab, entries](std::size_t index)
    {
      if (index >= entries.size())
      {
        return;
      }

      const auto& uri = entries[index].uri;

      if (new_tab)
      {
        window.open_tab(uri);

        return;
      }
      if (tab_still_open(window, tab))
      {
        tab->load_uri(uri);
      } else {
        window.open_tab(uri);
      }
    },
    0,
    initial_filter
  );
  finish(callback);
}

static void
cmd_history(
  selain::ui::MainWindow& window,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  const auto history = window.history_manager();

  if (!history)
  {
    finish(callback, "History manager is not available.");
    return;
  }

  bool new_tab = false;
  auto tokens = split_args(args);

  if (!tokens.empty() && (tokens.front() == "-t" || tokens.front() == "--tab"))
  {
    new_tab = true;
    tokens.erase(tokens.begin());
  }
  else if (!tokens.empty() &&
           tokens.front().size() > 1 &&
           tokens.front()[0] == '-')
  {
    finish(callback, "Usage: :history [-t|--tab] [query…]");
    return;
  }

  Glib::ustring filter;

  for (std::size_t i = 0; i < tokens.size(); ++i)
  {
    if (i > 0)
    {
      filter += ' ';
    }
    filter += tokens[i];
  }

  auto* tab = window.get_current_tab();
  auto on_result = [
    &window,
    tab,
    new_tab,
    filter,
    callback = std::move(callback)
  ](
    std::vector<selain::HistoryManager::Entry> entries,
    Glib::ustring error
  )
  {
    if (!error.empty())
    {
      finish(callback, error);
      return;
    }
    show_history(
      window,
      tab,
      new_tab,
      std::move(entries),
      filter,
      callback
    );
  };

  if (filter.empty())
  {
    history->list_recent(
      selain::HistoryManager::default_limit,
      std::move(on_result)
    );
  } else {
    history->search(
      filter,
      selain::HistoryManager::default_limit,
      std::move(on_result)
    );
  }
}

static void
cmd_history_clear(
  selain::ui::MainWindow& window,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  if (!selain::utils::string::trim(args).empty())
  {
    finish(callback, "Usage: :history-clear");
    return;
  }

  const auto history = window.history_manager();

  if (!history)
  {
    finish(callback, "History manager is not available.");
    return;
  }

  history->clear(
    [&window, callback = std::move(callback)](
      bool ok,
      const std::optional<Glib::ustring>& error
    )
    {
      if (!ok)
      {
        finish(
          callback,
          !error || error->empty() ? "Failed to clear history." : *error
        );
        return;
      }
      window.show_info("History cleared.");
      finish(callback);
    }
  );
}

static void
show_bookmarks(
  selain::ui::MainWindow& window,
  selain::ui::Tab* tab,
  bool new_tab,
  const std::vector<selain::BookmarkManager::Entry>& bookmarks,
  const Glib::ustring& initial_filter,
  selain::command::result_callback_type callback
)
{
  if (bookmarks.empty())
  {
    finish(callback, "No bookmarks found.");
    return;
  }

  std::vector<selain::ui::ListDialogEntry> dialog_entries;

  dialog_entries.reserve(bookmarks.size());
  for (const auto& bookmark : bookmarks)
  {
    selain::ui::ListDialogEntry entry;

    entry.title = bookmark.title;
    entry.secondary = bookmark.uri;
    entry.filter_text = bookmark.title + ' ' + bookmark.uri;
    if (!bookmark.tags.empty())
    {
      entry.filter_text += ' ' + bookmark.tags;
    }
    entry.icon_name = "user-bookmarks";
    dialog_entries.push_back(std::move(entry));
  }

  selain::ui::ListDialog::show(
    window,
    "Buku",
    "Filter bookmarks…",
    std::move(dialog_entries),
    [&window, tab, new_tab, bookmarks = std::move(bookmarks)](std::size_t index)
    {
      if (index >= bookmarks.size())
      {
        return;
      }

      const auto& uri = bookmarks[index].uri;

      if (new_tab)
      {
        window.open_tab(uri);

        return;
      }
      if (tab_still_open(window, tab))
      {
        tab->load_uri(uri);
      } else {
        window.open_tab(uri);
      }
    },
    0,
    initial_filter
  );
  finish(callback);
}

static void
open_bookmarks(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  bool new_tab,
  const std::vector<Glib::ustring>& keywords,
  selain::command::result_callback_type callback
)
{
  auto* tab_ptr = &tab;
  Glib::ustring filter;

  for (std::size_t i = 0; i < keywords.size(); ++i)
  {
    if (i > 0)
    {
      filter += ' ';
    }
    filter += keywords[i];
  }

  auto on_result = [
    &window,
    tab_ptr,
    new_tab,
    filter,
    callback = std::move(callback)
  ](
    const std::vector<selain::BookmarkManager::Entry>& bookmarks,
    Glib::ustring error
  )
  {
    if (!error.empty())
    {
      finish(callback, error);
      return;
    }
    show_bookmarks(
      window,
      tab_ptr,
      new_tab,
      std::move(bookmarks),
      filter,
      callback
    );
  };

  if (keywords.empty())
  {
    window.bookmark_manager()->list(std::move(on_result));
  } else {
    window.bookmark_manager()->search(keywords, std::move(on_result));
  }
}

static void
add_current_page(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  const std::vector<Glib::ustring>& tags,
  selain::command::result_callback_type callback
)
{
  const auto uri = tab.get_uri();

  if (!uri || uri->empty())
  {
    finish(callback, "Current view has no URI.");
    return;
  }

  Glib::ustring title = *uri;

  if (const auto* view = tab.get_active_view())
  {
    if (const auto page_title = view->get_title())
    {
      if (!page_title->empty())
      {
        title = *page_title;
      }
    }
  }

  window.bookmark_manager()->add(
    *uri,
    title,
    tags,
    [&window, callback = std::move(callback)](
      bool ok,
      const std::optional<Glib::ustring>& message
    )
    {
      if (!ok)
      {
        finish(
          callback,
          !message || message->empty() ? "Failed to add bookmark." : *message
        );
        return;
      }
      window.show_info(
        !message || message->empty() ? "Bookmark added." : *message
      );
      finish(callback);
    }
  );
}

static void
cmd_bookmark(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  const auto manager = window.bookmark_manager();
  const auto options = parse_bookmark_options(args);

  if (!options)
  {
    finish(callback, "Usage: :bookmark [-t|-a] [query|tags…]");
    return;
  }

  if (!manager)
  {
    finish(callback, "No bookmark manager found.");
    return;
  }

  if (options->mode == BookmarkMode::ADD)
  {
    add_current_page(window, tab, options->tokens, std::move(callback));
    return;
  }

  open_bookmarks(
    window,
    tab,
    options->new_tab,
    options->tokens,
    std::move(callback)
  );
}

static void
cmd_execute(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  if (selain::utils::string::is_blank(args))
  {
    finish(callback, "Usage: :execute <expression>");
    return;
  }
  tab.execute_script(
    args,
    [&window, callback = std::move(callback)](JSCValue* value, GError* error)
    {
      if (value)
      {
        const auto exception = jsc_context_get_exception(
          jsc_value_get_context(value)
        );

        if (exception)
        {
          finish(callback, jsc_exception_get_message(exception));
          return;
        }
        if (
          jsc_value_is_undefined(value) ||
          jsc_value_is_null(value)
        )
        {
          finish(callback, "Expression did not produce a command.");
          return;
        }

        char* text = jsc_value_to_string(value);

        if (!text)
        {
          finish(callback, "Expression did not produce a command.");
          return;
        }

        const Glib::ustring command(text);

        g_free(text);
        window.run_command(command, false, callback);
      }
      else if (error)
      {
        finish(callback, error->message);
      } else {
        finish(callback, "Expression did not produce a command.");
      }
    }
  );
}

static void
cmd_insert_input(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring,
  selain::command::result_callback_type callback
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
    [&window, callback = std::move(callback)](JSCValue* value, GError* error)
    {
      if (error)
      {
        finish(callback, error->message);
        return;
      }
      if (value && jsc_value_to_boolean(value))
      {
        window.set_mode(selain::Mode::INSERT);
        finish(callback);
        return;
      }
      finish(callback, "No input found.");
    }
  );
}

static void
cmd_insert_search(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring,
  selain::command::result_callback_type callback
)
{
  auto fs = cmrc::selain::rc::get_filesystem();
  auto script = fs.open("src/webkit/find-search-input.js");

  tab.execute_script(
    Glib::ustring(std::begin(script), std::end(script)),
    [&window, callback = std::move(callback)](JSCValue* value, GError* error)
    {
      if (error)
      {
        finish(callback, error->message);
        return;
      }
      if (value && jsc_value_to_boolean(value))
      {
        window.set_mode(selain::Mode::INSERT);
        finish(callback);
        return;
      }
      finish(callback, "No search found.");
    }
  );
}

static void
cmd_js(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  if (selain::utils::string::is_blank(args))
  {
    finish(callback, "Usage: :js <script>");
    return;
  }
  tab.execute_script(
    args,
    [&window, callback = std::move(callback)](JSCValue* value, GError* error)
    {
      if (value)
      {
        const auto exception = jsc_context_get_exception(
          jsc_value_get_context(value)
        );

        if (exception)
        {
          finish(callback, jsc_exception_get_message(exception));
          return;
        }
        window.show_info(selain::utils::string::js_value_to_text(value));
        finish(callback);
        return;
      }
      if (error)
      {
        finish(callback, error->message);
        return;
      }
      finish(callback);
    }
  );
}

static void
fill_login_entry(
  selain::ui::MainWindow& window,
  selain::ui::Tab* tab,
  const Glib::ustring& entry,
  selain::PasswordManager::FillMode mode,
  selain::command::result_callback_type callback
)
{
  window.password_manager()->fetch(
    entry,
    [&window, tab, mode, callback = std::move(callback)](
      const std::optional<selain::PasswordManager::Entry>& secret,
      const std::optional<Glib::ustring>& error
    )
    {
      if (!tab_still_open(window, tab))
      {
        finish(callback);
        return;
      }
      if (!secret)
      {
        finish(
          callback,
          !error || error->empty() ? "Failed to read pass entry." : *error
        );
        return;
      }

      const char* username_js = "null";
      const char* password_js = "null";
      std::string username_literal;
      std::string password_literal;

      if (mode != selain::PasswordManager::FillMode::PASSWORD)
      {
        username_literal = std::format(
          "\"{}\"",
          selain::utils::string::js_string_escape(secret->username.raw())
        );
        username_js = username_literal.c_str();
      }
      if (mode != selain::PasswordManager::FillMode::USERNAME)
      {
        password_literal = std::format(
          "\"{}\"",
          selain::utils::string::js_string_escape(secret->password.raw())
        );
        password_js = password_literal.c_str();
      }

      const auto script = std::format(
        R"JS(
          ((username, password) => {{
            const setValue = (el, value) => {{
              el.focus();
              el.value = value;
              el.dispatchEvent(new Event("input", {{ bubbles: true }}));
              el.dispatchEvent(new Event("change", {{ bubbles: true }}));
            }};

            const isVisible = (el) => {{
              if (typeof el.checkVisibility === "function") {{
                return el.checkVisibility({{
                  checkOpacity: false,
                  checkVisibilityCSS: true
                }});
              }}
              const s = getComputedStyle(el);
              return s.display !== "none" && s.visibility !== "hidden";
            }};

            const usable = (el) =>
              el && !el.disabled && !el.readOnly && isVisible(el);

            const form =
              document.activeElement?.closest?.("form") ||
              [...document.querySelectorAll("form")].find((f) =>
                [...f.querySelectorAll('input[type="password"]')].some(usable)
              );

            if (!form) {{
              return false;
            }}

            const user = form.querySelector(
              'input[type="email"],input[type="text"],input[type="username"],input:not([type])'
            );
            const pass = form.querySelector('input[type="password"]');

            if (user && username !== null && usable(user)) {{
              setValue(user, username);
            }}
            if (pass && password !== null && usable(pass)) {{
              setValue(pass, password);
            }}

            return true;
          }})({}, {})
        )JS",
        username_js,
        password_js
      );

      tab->execute_script(
        script,
        [&window, callback](JSCValue* value, GError* err)
        {
          if (err)
          {
            finish(callback, err->message);
            return;
          }
          if (value && jsc_value_to_boolean(value))
          {
            window.set_mode(selain::Mode::INSERT);
            finish(callback);
            return;
          }
          finish(callback, "No login form found.");
        }
      );
    }
  );
}

static std::optional<selain::PasswordManager::FillMode>
parse_login_fill_mode(Glib::ustring& args)
{
  args = selain::utils::string::trim(args);
  if (args.empty())
  {
    return selain::PasswordManager::FillMode::BOTH;
  }
  else if (args == "-u" || args == "--username")
  {
    return selain::PasswordManager::FillMode::USERNAME;
  }
  else if (args == "-p" || args == "--password")
  {
    return selain::PasswordManager::FillMode::PASSWORD;
  }

  return std::nullopt;
}

static void
cmd_login(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  const auto mode = parse_login_fill_mode(args);
  const auto manager = window.password_manager();
  const auto host = tab.get_host();

  if (!mode)
  {
    finish(callback, "Usage: :login [-u|-p]");
    return;
  }

  if (!manager)
  {
    finish(callback, "No password manager found.");
    return;
  }

  if (!host || host->empty())
  {
    finish(callback, "Cannot determine current hostname.");
    return;
  }

  const auto entries = manager->find_entries(*host);

  if (entries.empty())
  {
    finish(callback, std::format("No pass entries for {}.", host->raw()));
    return;
  }

  auto* tab_ptr = &tab;

  if (entries.size() == 1)
  {
    fill_login_entry(
      window,
      tab_ptr,
      entries.front(),
      *mode,
      std::move(callback)
    );
    return;
  }

  std::vector<selain::ui::ListDialogEntry> dialog_entries;

  dialog_entries.reserve(entries.size());
  for (const auto& path : entries)
  {
    selain::ui::ListDialogEntry entry;

    entry.title = path;
    entry.icon_name = "dialog-password";
    dialog_entries.push_back(std::move(entry));
  }

  selain::ui::ListDialog::show(
    window,
    "Password manager",
    "Filter password entries…",
    std::move(dialog_entries),
    [&window, tab_ptr, mode, entries, callback](std::size_t index)
    {
      if (index >= entries.size())
      {
        return;
      }
      // Report only failures from the follow-up fill; the command itself
      // already completed successfully by presenting the entry picker.
      fill_login_entry(
        window,
        tab_ptr,
        entries[index],
        *mode,
        [callback](const std::optional<Glib::ustring>& error)
        {
          if (error)
          {
            finish(callback, error);
          }
        }
      );
    },
    0,
    *host
  );
  finish(callback);
}

static void
cmd_go_root(
  selain::ui::MainWindow&,
  selain::ui::Tab& tab,
  Glib::ustring,
  selain::command::result_callback_type callback
)
{
  if (const auto uri = tab.get_uri())
  {
    if (const auto next = selain::utils::url::get_root(*uri))
    {
      tab.load_uri(*next);
    }
  }
  finish(callback);
}

static void
cmd_go_up(
  selain::ui::MainWindow&,
  selain::ui::Tab& tab,
  Glib::ustring,
  selain::command::result_callback_type callback
)
{
  if (const auto uri = tab.get_uri())
  {
    if (const auto next = selain::utils::url::get_parent(*uri))
    {
      tab.load_uri(*next);
    }
  }
  finish(callback);
}

static void
cmd_paste(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring,
  selain::command::result_callback_type callback
)
{
  do_paste(
    window,
    [&tab](const Glib::ustring& uri)
    {
      tab.load_uri(uri);
    },
    std::move(callback)
  );
}

static void
cmd_scroll(
  selain::ui::MainWindow&,
  selain::ui::Tab& tab,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  // TODO: Allow finer control by accepting direction _and_ amount as
  // arguments.
  static const std::unordered_set<Glib::ustring> actions = {
    "top",
    "bottom",
    "left",
    "right",
    "up",
    "down",
    "page-up",
    "page-down"
  };
  const auto arg = selain::utils::string::trim(args);

  if (arg.empty())
  {
    finish(callback, "Missing direction/amount.");
    return;
  }
  if (!actions.contains(arg))
  {
    finish(callback, "Unrecognized direction.");
    return;
  }

  // Scroll via the injected helper (all frames). window.scrollBy only affects
  // the main frame; WebKit's PDF.js viewer lives in a cross-origin iframe.
  tab.execute_script(std::format(
    R"((() => {{
  const payload = {{ type: "selain-scroll", action: "{}" }};
  window.postMessage(payload, "*");
  for (const iframe of document.querySelectorAll("iframe")) {{
    try {{
      iframe.contentWindow.postMessage(payload, "*");
    }} catch (e) {{}}
  }}
}})())",
    arg.raw()
  ));
  finish(callback);
}

static void
cmd_split_focus(
  selain::ui::MainWindow&,
  selain::ui::Tab& tab,
  Glib::ustring args,
  selain::command::result_callback_type callback
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

  Gtk::DirectionType direction = Gtk::DirectionType::LEFT;

  if (const auto error = require_enum_arg(
    "direction",
    args,
    direction,
    mapping
  ))
  {
    finish(callback, error);
    return;
  }
  tab.focus_view(direction);
  finish(callback);
}

static void
cmd_yank(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring,
  selain::command::result_callback_type callback
)
{
  const auto clipboard = window.get_clipboard();
  const auto uri = tab.get_uri();

  if (!uri || uri->empty())
  {
    finish(callback, "Current view has no URI.");
    return;
  }
  clipboard->set_text(*uri);
  window.show_info(std::format("Yanked {}", uri->raw()));
  finish(callback);
}

static void
cmd_zoom(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  Glib::ustring args,
  selain::command::result_callback_type callback
)
{
  static constexpr double min_zoom = 0.1;
  static constexpr double max_zoom = 5.0;

  const auto arg = selain::utils::string::trim(args);
  const auto current = tab.get_zoom_level();

  if (arg.empty())
  {
    window.show_info(std::format(
      "Zoom: {}%",
      static_cast<int>(std::lround(current * 100.0))
    ));
    finish(callback);
    return;
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
    finish(
      callback,
      "Invalid zoom. Use percentage (e.g. 120%) or relative (+20, -5)."
    );
    return;
  }

  level = std::clamp(level, min_zoom, max_zoom);
  tab.set_zoom_level(level);
  finish(callback);
}
