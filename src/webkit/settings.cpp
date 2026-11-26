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
#include <selain/config.hpp>
#include <selain/webkit/settings.hpp>

namespace selain::webkit
{
  Glib::RefPtr<Settings>
  Settings::create()
  {
    return Glib::make_refptr_for_instance<Settings>(new Settings());
  }

  Settings::Settings()
    : Glib::ObjectBase(typeid(Settings))
    , m_settings(webkit_settings_new())
  {
    webkit_settings_set_enable_developer_extras(m_settings, true);
    webkit_settings_set_user_agent_with_application_details(
      m_settings,
      "Selain",
      SELAIN_VERSION
    );
  }

  Settings::~Settings()
  {
    g_clear_object(&m_settings);
  }

  void
  Settings::install(WebKitWebView* view)
  {
    webkit_web_view_set_settings(view, m_settings);
  }

  bool
  Settings::get_bool(const Glib::ustring& setting) const
  {
    gboolean value = FALSE;

    g_object_get(m_settings, setting.c_str(), &value, nullptr);

    return value;
  }

  guint
  Settings::get_int(const Glib::ustring& setting) const
  {
    guint value = 0;

    g_object_get(m_settings, setting.c_str(), &value, nullptr);

    return value;
  }

  Glib::ustring
  Settings::get_string(const Glib::ustring& setting) const
  {
    gchar* value = nullptr;
    Glib::ustring result;

    g_object_get(m_settings, setting.c_str(), &value, nullptr);
    if (value)
    {
      result = value;
    }
    g_free(value);

    return result;
  }

  WebKitHardwareAccelerationPolicy
  Settings::get_hardware_acceleration_policy() const
  {
    return webkit_settings_get_hardware_acceleration_policy(m_settings);
  }

  void
  Settings::set(const Glib::ustring& setting, bool value)
  {
    g_object_set(
      m_settings,
      setting.c_str(),
      static_cast<gboolean>(value),
      nullptr
    );
  }

  void
  Settings::set(const Glib::ustring& setting, guint value)
  {
    g_object_set(m_settings, setting.c_str(), value, nullptr);
  }

  void
  Settings::set(const Glib::ustring& setting, const Glib::ustring& value)
  {
    g_object_set(m_settings, setting.c_str(), value.c_str(), nullptr);
  }

  void
  Settings::set_hardware_acceleration_policy(
    WebKitHardwareAccelerationPolicy policy
  )
  {
    webkit_settings_set_hardware_acceleration_policy(m_settings, policy);
  }
}
