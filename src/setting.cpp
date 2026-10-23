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
#include <unordered_map>

#include <glibmm/ustring_hash.h>

#include "selain/setting.hpp"

namespace selain::setting
{
  struct Setting
  {
    virtual Glib::ustring get(MainWindow& window) const = 0;

    virtual bool set(MainWindow& window, const Glib::ustring& new_value) = 0;
  };

  struct StringSetting : Setting
  {
    Glib::ustring value;

    explicit StringSetting(const Glib::ustring& initial)
      : value(initial) {}

    Glib::ustring get(MainWindow&) const override
    {
      return value;
    }

    bool set(MainWindow&, const Glib::ustring& new_value) override
    {
      value = new_value;

      return true;
    }
  };

  struct WebKitSetting : Setting
  {
    const Glib::ustring name;

    explicit WebKitSetting(const Glib::ustring& name_)
      : name(name_) {}
  };

  struct WebKitBoolSetting : WebKitSetting
  {
    explicit WebKitBoolSetting(const Glib::ustring& name)
      : WebKitSetting(name) {}

    Glib::ustring get(MainWindow& window) const override
    {
      return window.web_settings()->get_bool(name) ? "true" : "false";
    }

    bool set(MainWindow& window, const Glib::ustring& new_value) override
    {
      bool value;

      if (new_value == "true" || new_value == "1")
      {
        value = true;
      }
      else if (new_value == "false" || new_value == "0")
      {
        value = false;
      } else {
        return false; // TODO: Return error message.
      }
      window.web_settings()->set(name, value);

      return true;
    }
  };

  struct WebKitIntSetting : WebKitSetting
  {
    explicit WebKitIntSetting(const Glib::ustring& name)
      : WebKitSetting(name) {}

    Glib::ustring get(MainWindow& window) const override
    {
      return std::to_string(window.web_settings()->get_int(name));
    }

    bool set(MainWindow& window, const Glib::ustring& new_value) override
    {
      ::guint numeric_value;

      try
      {
        numeric_value = static_cast<::guint>(std::stoi(new_value.raw()));
      }
      catch (const std::invalid_argument&)
      {
        return false; // TODO: Return error message instead.
      }
      catch (const std::out_of_range&)
      {
        return false; // TODO: Return error message instead.
      }
      window.web_settings()->set(name, numeric_value);

      return true;
    }
  };

  struct WebKitStringSetting : WebKitSetting
  {
    explicit WebKitStringSetting(const Glib::ustring& name)
      : WebKitSetting(name) {}

    Glib::ustring get(MainWindow& window) const override
    {
      return window.web_settings()->get_string(name);
    }

    bool set(MainWindow& window, const Glib::ustring& new_value) override
    {
      window.web_settings()->set(name, new_value);

      return true;
    }
  };

  struct WebKitHardwareAccelerationPolicySetting : Setting
  {
    Glib::ustring get(MainWindow& window) const override
    {
      switch (window.web_settings()->get_hardware_acceleration_policy())
      {
        case ::WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS:
          return "always";

        case ::WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER:
          return "never";
      }

      return "always";
    }

    bool set(MainWindow& window, const Glib::ustring& new_value) override
    {
      ::WebKitHardwareAccelerationPolicy policy;

      if (new_value == "always")
      {
        policy = ::WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS;
      }
      else if (new_value == "never")
      {
        policy = ::WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER;
      } else {
        return false; // TODO: Return error message.
      }
      window.web_settings()->set_hardware_acceleration_policy(policy);

      return true;
    }
  };

  #define WK_BOOL_SETTING(name) \
    { name, std::make_shared<WebKitBoolSetting>(name) }
  #define WK_INT_SETTING(name) \
    { name, std::make_shared<WebKitIntSetting>(name) }
  #define WK_STRING_SETTING(name) \
    { name, std::make_shared<WebKitStringSetting>(name) }

  static std::unordered_map<Glib::ustring, std::shared_ptr<Setting>> mapping =
  {
    { "homepage", std::make_shared<StringSetting>("https://duckduckgo.com") },

    // WebKit boolean settings.
    WK_BOOL_SETTING("allow-file-access-from-file-urls"),
    WK_BOOL_SETTING("allow-modal-dialogs"),
    WK_BOOL_SETTING("allow-top-navigation-to-data-urls"),
    WK_BOOL_SETTING("allow-universal-access-from-file-urls"),
    WK_BOOL_SETTING("auto-load-images"),
    WK_BOOL_SETTING("disable-web-security"),
    WK_BOOL_SETTING("draw-compositing-indicators"),
    WK_BOOL_SETTING("enable-2d-canvas-acceleration"),
    WK_BOOL_SETTING("enable-back-forward-navigation-gestures"),
    WK_BOOL_SETTING("enable-caret-browsing"),
    WK_BOOL_SETTING("enable-developer-extras"),
    WK_BOOL_SETTING("enable-encrypted-media"),
    WK_BOOL_SETTING("enable-fullscreen"),
    WK_BOOL_SETTING("enable-html5-database"),
    WK_BOOL_SETTING("enable-html5-local-storage"),
    WK_BOOL_SETTING("enable-javascript"),
    WK_BOOL_SETTING("enable-javascript-markup"),
    WK_BOOL_SETTING("enable-media"),
    WK_BOOL_SETTING("enable-media-capabilities"),
    WK_BOOL_SETTING("enable-media-stream"),
    WK_BOOL_SETTING("enable-mediasource"),
    WK_BOOL_SETTING("enable-mock-capture-devices"),
    WK_BOOL_SETTING("enable-page-cache"),
    WK_BOOL_SETTING("enable-resizable-text-areas"),
    WK_BOOL_SETTING("enable-site-specific-quirks"),
    WK_BOOL_SETTING("enable-smooth-scrolling"),
    WK_BOOL_SETTING("enable-spatial-navigation"),
    WK_BOOL_SETTING("enable-tabs-to-links"),
    WK_BOOL_SETTING("enable-webaudio"),
    WK_BOOL_SETTING("enable-webgl"),
    WK_BOOL_SETTING("enable-webrtc"),
    WK_BOOL_SETTING("enable-write-console-messages-to-stdout"),
    WK_BOOL_SETTING("javascript-can-access-clipboard"),
    WK_BOOL_SETTING("javascript-can-open-windows-automatically"),
    WK_BOOL_SETTING("media-playback-allows-inline"),
    WK_BOOL_SETTING("media-playback-requires-user-gesture"),
    WK_BOOL_SETTING("print-backgrounds"),
    WK_BOOL_SETTING("zoom-text-only"),

    // WebKit integer settings.
    WK_INT_SETTING("default-font-size"),
    WK_INT_SETTING("default-monospace-font-size"),
    WK_INT_SETTING("minimum-font-size"),

    // WebKit string settings.
    WK_STRING_SETTING("cursive-font-family"),
    WK_STRING_SETTING("default-charset"),
    WK_STRING_SETTING("default-font-family"),
    WK_STRING_SETTING("fantasy-font-family"),
    WK_STRING_SETTING("math-font-family"),
    WK_STRING_SETTING("media-content-types-requiring-hardware-support"),
    WK_STRING_SETTING("monospace-font-family"),
    WK_STRING_SETTING("pictograph-font-family"),
    WK_STRING_SETTING("sans-serif-font-family"),
    WK_STRING_SETTING("serif-font-family"),
    WK_STRING_SETTING("user-agent"),
    WK_STRING_SETTING("webrtc-udp-ports-range"),

    // WebKit enumeration settings.
    {
      "hardware-acceleration-policy",
      std::make_shared<WebKitHardwareAccelerationPolicySetting>()
    },
  };

  #undef WK_BOOL_SETTING
  #undef WK_INT_SETTING
  #undef WK_STRING_SETTING

  std::optional<Glib::ustring>
  get(MainWindow& window, const Glib::ustring& key)
  {
    const auto entry = mapping.find(key);

    if (entry != std::end(mapping))
    {
      return entry->second->get(window);
    }

    return std::nullopt;
  }

  bool
  set(MainWindow& window, const Glib::ustring& key, const Glib::ustring& value)
  {
    auto entry = mapping.find(key);

    if (entry != std::end(mapping))
    {
      return entry->second->set(window, value);
    }

    return false;
  }
}
