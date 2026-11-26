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
#include <array>
#include <filesystem>
#include <format>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include <cmrc/cmrc.hpp>
#include <gtkmm.h>
#include <inja/inja.hpp>

#include <selain/core/theme.hpp>
#include <selain/utils/string.hpp>

CMRC_DECLARE(selain::rc);

namespace selain::theme
{
  static const std::array color_names =
  {
    "window-background",
    "window-foreground",
    "status-bar-background",
    "status-bar-foreground",
    "status-bar-error-background",
    "status-bar-error-foreground",
    "mode-bar-normal-background",
    "mode-bar-normal-foreground",
    "mode-bar-insert-background",
    "mode-bar-insert-foreground",
  };

  static std::vector<std::pair<std::string_view, std::string>>
  resolve_theme_colors()
  {
    static Gtk::Label probe;
    const auto context = probe.get_style_context();
    std::vector<std::pair<std::string_view, std::string>> colors;

    colors.reserve(color_names.size());
    for (const auto& name : color_names)
    {
      Gdk::RGBA color;

      if (context->lookup_color(std::format("selain-{}", name), color))
      {
        colors.emplace_back(name, color.to_string());
      }
      else
      {
        colors.emplace_back(name, "");
      }
    }

    return colors;
  }

  static void
  add_theme_colors(inja::json& data)
  {
    for (const auto& [name, value] : resolve_theme_colors())
    {
      data[std::string(name)] = value;
    }
  }

  static void
  load_default_css()
  {
    auto provider = Gtk::CssProvider::create();
    auto fs = cmrc::selain::rc::get_filesystem();
    auto stylesheet = fs.open("src/theme.css");
    const auto css = std::string(
      std::begin(stylesheet),
      std::end(stylesheet)
    );

    // load_from_string() requires gtkmm 4.12+; Ubuntu 24.04 ships 4.10.
#if GTKMM_CHECK_VERSION(4, 12, 0)
    provider->load_from_string(css);
#else
    provider->load_from_data(css);
#endif
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
        GTK_STYLE_PROVIDER_PRIORITY_USER
      );
    }
  }

  void
  initialize()
  {
    static bool initialized = false;

    if (initialized)
    {
      return;
    }

    load_default_css();
    load_custom_css();
    initialized = true;
  }

  std::string
  render_document(const std::string& title, const std::string& body)
  {
    auto fs = cmrc::selain::rc::get_filesystem();
    auto tmpl = fs.open("src/document.html");
    inja::json data;

    add_theme_colors(data);
    data["title"] = utils::string::html_escape(title);
    data["body"] = body;

    return inja::render(std::string(std::begin(tmpl), std::end(tmpl)), data);
  }
}
