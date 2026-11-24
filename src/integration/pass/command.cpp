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

#include <selain/integration/pass/storage.hpp>
#include <selain/ui/list-dialog.hpp>
#include <selain/ui/main-window.hpp>
#include <selain/ui/tab.hpp>
#include <selain/utils/string.hpp>
#include <selain/utils/url.hpp>

namespace
{
  enum class PassFillMode
  {
    BOTH,
    USERNAME,
    PASSWORD,
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

static void
fill_pass_entry(
  selain::ui::MainWindow& window,
  selain::ui::Tab* tab,
  const Glib::ustring& entry,
  PassFillMode mode
)
{
  selain::integration::pass::show_async(
    entry,
    [&window, tab, mode](
      std::optional<selain::integration::pass::Secret> secret,
      Glib::ustring error
    )
    {
      if (!tab_still_open(window, tab))
      {
        return;
      }
      if (!secret)
      {
        window.show_error(
          error.empty() ? "Failed to read pass entry." : error
        );

        return;
      }

      const char* username_js = "null";
      const char* password_js = "null";
      std::string username_literal;
      std::string password_literal;

      if (mode != PassFillMode::PASSWORD)
      {
        username_literal = std::format(
          "\"{}\"",
          selain::utils::string::js_string_escape(secret->username.raw())
        );
        username_js = username_literal.c_str();
      }
      if (mode != PassFillMode::USERNAME)
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
        [&window](JSCValue* value, GError* err)
        {
          if (err)
          {
            window.show_error(err->message);

            return;
          }
          if (value && ::jsc_value_to_boolean(value))
          {
            window.set_mode(selain::Mode::INSERT);
          } else {
            window.show_error("No login form found.");
          }
        }
      );
    }
  );
}

static PassFillMode
parse_pass_mode(selain::ui::MainWindow& window, Glib::ustring& args, bool& ok)
{
  ok = true;
  args = selain::utils::string::trim(args);

  if (args.empty())
  {
    return PassFillMode::BOTH;
  }
  if (args == "-u" || args == "--username")
  {
    return PassFillMode::USERNAME;
  }
  if (args == "-p" || args == "--password")
  {
    return PassFillMode::PASSWORD;
  }

  window.show_error("Usage: :pass [-u|-p]");
  ok = false;

  return PassFillMode::BOTH;
}

namespace selain::integration::pass
{
  bool
  command(
    selain::ui::MainWindow& window,
    selain::ui::Tab& tab,
    Glib::ustring args
  )
  {
    bool args_ok = true;
    const auto mode = parse_pass_mode(window, args, args_ok);

    if (!args_ok)
    {
      return false;
    }

    const auto uri = tab.get_uri();

    if (!uri || uri->empty())
    {
      window.show_error("Current view has no URI.");

      return false;
    }

    const auto host = selain::utils::url::get_host(*uri);

    if (!host || host->empty())
    {
      window.show_error("Current URI has no host.");

      return false;
    }

    const auto entries = find_entries(*host);

    if (entries.empty())
    {
      window.show_error(std::format("No pass entries for {}.", host->raw()));

      return false;
    }

    auto* tab_ptr = &tab;
    auto fill = [&window, tab_ptr, mode](const Glib::ustring& entry)
    {
      fill_pass_entry(window, tab_ptr, entry, mode);
    };

    if (entries.size() == 1)
    {
      fill(entries.front());
    } else {
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
        "Pass",
        "Filter pass entries…",
        std::move(dialog_entries),
        [fill, entries](std::size_t index)
        {
          if (index < entries.size())
          {
            fill(entries[index]);
          }
        },
        0,
        *host
      );
    }

    return true;
  }
}
