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
#include <filesystem>
#include <format>

#include <cmrc/cmrc.hpp>
#include <inja/inja.hpp>

#include <selain/theme.hpp>
#include <selain/utils.hpp>

CMRC_DECLARE(selain::rc);

namespace selain::theme
{
  const Gdk::RGBA window_background("#262626");
  const Gdk::RGBA window_foreground("#b3deef");
  const Gdk::RGBA status_bar_background("#444444");
  const Gdk::RGBA status_bar_foreground("#a7d6f8");
  const Gdk::RGBA status_bar_error_background("#f43753");
  const Gdk::RGBA status_bar_error_foreground("#eeeeee");
  const Gdk::RGBA mode_bar_normal_background("#b3deef");
  const Gdk::RGBA mode_bar_normal_foreground("#282828");
  const Gdk::RGBA mode_bar_insert_background("#d7d75f");
  const Gdk::RGBA mode_bar_insert_foreground("#262626");

  static void
  add_theme_colors(inja::json& data)
  {
    data["window-background"] = window_background.to_string().raw();
    data["window-foreground"] = window_foreground.to_string().raw();
    data["status-bar-background"] = status_bar_background.to_string().raw();
    data["status-bar-foreground"] = status_bar_foreground.to_string().raw();
    data["status-bar-error-background"] = status_bar_error_background.to_string().raw();
    data["status-bar-error-foreground"] = status_bar_error_foreground.to_string().raw();
    data["mode-bar-normal-background"] = mode_bar_normal_background.to_string().raw();
    data["mode-bar-normal-foreground"] = mode_bar_normal_foreground.to_string().raw();
    data["mode-bar-insert-background"] = mode_bar_insert_background.to_string().raw();
    data["mode-bar-insert-foreground"] = mode_bar_insert_foreground.to_string().raw();
  }

  static void
  load_default_css()
  {
    auto provider = Gtk::CssProvider::create();
    auto fs = cmrc::selain::rc::get_filesystem();
    auto stylesheet = fs.open("src/theme.css");
    inja::json data;

    add_theme_colors(data);
    provider->load_from_string(inja::render(
      std::string(std::begin(stylesheet), std::end(stylesheet)),
      data
    ));
    Gtk::StyleProvider::add_provider_for_display(
      Gdk::Display::get_default(),
      provider,
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
  }

  static void
  load_custom_css()
  {
    const auto path = Glib::build_filename(
      Glib::get_user_config_dir(),
      "selain",
      "style.css"
    );

    if (std::filesystem::exists(path))
    {
      auto provider = Gtk::CssProvider::create();

      provider->load_from_path(path);
      Gtk::StyleProvider::add_provider_for_display(
        Gdk::Display::get_default(),
        provider,
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
      );
    }
  }

  void
  initialize()
  {
    load_default_css();
    load_custom_css();
  }

  std::string
  render_document(const std::string& title, const std::string& body)
  {
    auto fs = cmrc::selain::rc::get_filesystem();
    auto tmpl = fs.open("src/document.html");
    inja::json data;

    add_theme_colors(data);
    data["title"] = utils::html_escape(title);
    data["body"] = body;

    return inja::render(std::string(std::begin(tmpl), std::end(tmpl)), data);
  }
}
