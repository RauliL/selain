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
#include <selain/web-settings.hpp>
#include <selain/version.hpp>

namespace selain
{
  Glib::RefPtr<WebSettings>
  WebSettings::create()
  {
    return Glib::make_refptr_for_instance<WebSettings>(new WebSettings());
  }

  WebSettings::WebSettings()
    : Glib::ObjectBase(typeid(WebSettings))
    , m_settings(::webkit_settings_new())
  {
    ::webkit_settings_set_enable_developer_extras(m_settings, true);
    ::webkit_settings_set_user_agent_with_application_details(
      m_settings,
      "Selain",
      SELAIN_VERSION
    );
  }

  WebSettings::~WebSettings()
  {
    g_clear_object(&m_settings);
  }

  void
  WebSettings::install(::WebKitWebView* view)
  {
    ::webkit_web_view_set_settings(view, m_settings);
  }

  bool
  WebSettings::get_bool(const Glib::ustring& setting) const
  {
    ::gboolean value = FALSE;

    ::g_object_get(m_settings, setting.c_str(), &value, nullptr);

    return value;
  }

  ::guint
  WebSettings::get_int(const Glib::ustring& setting) const
  {
    ::guint value = 0;

    ::g_object_get(m_settings, setting.c_str(), &value, nullptr);

    return value;
  }

  Glib::ustring
  WebSettings::get_string(const Glib::ustring& setting) const
  {
    ::gchar* value = nullptr;

    ::g_object_get(m_settings, setting.c_str(), &value, nullptr);

    const Glib::ustring result = value ? value : "";

    g_free(value);

    return result;
  }

  ::WebKitHardwareAccelerationPolicy
  WebSettings::get_hardware_acceleration_policy() const
  {
    return ::webkit_settings_get_hardware_acceleration_policy(m_settings);
  }

  void
  WebSettings::set(const Glib::ustring& setting, bool value)
  {
    ::g_object_set(
      m_settings,
      setting.c_str(),
      static_cast<::gboolean>(value),
      nullptr
    );
  }

  void
  WebSettings::set(const Glib::ustring& setting, ::guint value)
  {
    ::g_object_set(m_settings, setting.c_str(), value, nullptr);
  }

  void
  WebSettings::set(const Glib::ustring& setting, const Glib::ustring& value)
  {
    ::g_object_set(m_settings, setting.c_str(), value.c_str(), nullptr);
  }

  void
  WebSettings::set_hardware_acceleration_policy(
    ::WebKitHardwareAccelerationPolicy policy
  )
  {
    ::webkit_settings_set_hardware_acceleration_policy(m_settings, policy);
  }
}
