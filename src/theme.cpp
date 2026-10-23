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
#include <selain/theme.hpp>

#include <filesystem>
#include <format>

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
  load_default_css()
  {
    auto provider = Gtk::CssProvider::create();

    provider->load_from_string(
      std::format(
        "* {{\n"
        "  --selain-window-background: {};\n"
        "  --selain-window-foreground: {};\n"
        "  --selain-status-bar-background: {};\n"
        "  --selain-status-bar-foreground: {};\n"
        "  --selain-status-bar-error-background: {};\n"
        "  --selain-status-bar-error-foreground: {};\n"
        "  --selain-mode-bar-normal-background: {};\n"
        "  --selain-mode-bar-normal-foreground: {};\n"
        "  --selain-mode-bar-insert-background: {};\n"
        "  --selain-mode-bar-insert-foreground: {};\n"
        "}}\n"
        ".selain-window {{\n"
        "  background-color: var(--selain-window-background);\n"
        "}}\n"
        ".selain-status-bar {{\n"
        "  background-color: var(--selain-status-bar-background);\n"
        "}}\n"
        ".selain-status-bar label {{\n"
        "  font-family: monospace;\n"
        "  padding: 0.1em 0.4em;\n"
        "}}\n"
        ".selain-status-bar .mode-label {{\n"
        "  background-color: var(--selain-mode-bar-normal-background);\n"
        "  color: var(--selain-mode-bar-normal-foreground);\n"
        "}}\n"
        ".selain-status-bar .mode-label.mode-insert {{\n"
        "  background-color: var(--selain-mode-bar-insert-background);\n"
        "  color: var(--selain-mode-bar-insert-foreground);\n"
        "}}\n"
        ".selain-status-bar .status-label {{\n"
        "  background-color: var(--selain-status-bar-background);\n"
        "  color: var(--selain-status-bar-foreground);\n"
        "}}\n"
        ".selain-command-entry {{\n"
        "  background-color: var(--selain-window-background);\n"
        "}}\n"
        ".selain-command-entry entry {{\n"
        "  background: var(--selain-window-background);\n"
        "  border: none;\n"
        "  color: var(--selain-status-bar-error-foreground);\n"
        "  font-family: monospace;\n"
        "  margin: 0;\n"
        "  outline: none;\n"
        "  padding: 0.2em;\n"
        "}}\n"
        ".selain-command-entry .notification-info {{\n"
        "  background-color: var(--selain-window-background);\n"
        "  color: var(--selain-window-foreground);\n"
        "  font-family: monospace;\n"
        "}}\n"
        ".selain-command-entry .notification-error {{\n"
        "  background-color: var(--selain-status-bar-error-background);\n"
        "  color: var(--selain-status-bar-error-foreground);\n"
        "  font-family: monospace;\n"
        "}}\n"
        ".selain-view {{\n"
        "  border: 1px solid transparent;\n"
        "}}\n"
        ".selain-view-active {{\n"
        "  border-color: var(--selain-window-foreground);\n"
        "}}\n",
        window_background.to_string().raw(),
        window_foreground.to_string().raw(),
        status_bar_background.to_string().raw(),
        status_bar_foreground.to_string().raw(),
        status_bar_error_background.to_string().raw(),
        status_bar_error_foreground.to_string().raw(),
        mode_bar_normal_background.to_string().raw(),
        mode_bar_normal_foreground.to_string().raw(),
        mode_bar_insert_background.to_string().raw(),
        mode_bar_insert_foreground.to_string().raw()
      )
    );
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
}
