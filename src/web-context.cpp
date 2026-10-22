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
#include <selain/web-context.hpp>

namespace selain
{
  static ::WebKitNetworkSession* create_network_session();

  Glib::RefPtr<WebContext>
  WebContext::create()
  {
    return Glib::make_refptr_for_instance<WebContext>(new WebContext());
  }

  WebContext::WebContext()
    : Glib::ObjectBase(typeid(WebContext))
    , m_session(create_network_session())
  {
  }

  WebContext::~WebContext()
  {
    g_clear_object(&m_session);
  }

  ::WebKitWebView*
  WebContext::create_web_view()
  {
    return WEBKIT_WEB_VIEW(::g_object_new(
      WEBKIT_TYPE_WEB_VIEW,
      "network-session",
      m_session,
      nullptr
    ));
  }

  static inline void
  free_string(::gchar* str)
  {
    if (str)
    {
      g_free(str);
    }
  }

  static ::WebKitNetworkSession*
  create_network_session()
  {
    ::gchar* base_cache_dir = nullptr;
    ::gchar* base_data_dir = nullptr;
    ::gchar* cookie_file = nullptr;
    ::WebKitNetworkSession* session;

    if (const auto user_cache_dir = ::g_get_user_cache_dir())
    {
      base_cache_dir = ::g_build_filename(
        user_cache_dir,
        "selain",
        "cache",
        nullptr
      );
      base_data_dir = ::g_build_filename(
        user_cache_dir,
        "selain",
        "data",
        nullptr
      );
      cookie_file = ::g_build_filename(
        user_cache_dir,
        "selain",
        "cookies.sqlite",
        nullptr
      );
    }

    session = ::webkit_network_session_new(base_data_dir, base_cache_dir);

    ::webkit_website_data_manager_set_favicons_enabled(
      ::webkit_network_session_get_website_data_manager(session),
      true
    );

    if (cookie_file)
    {
      ::webkit_cookie_manager_set_persistent_storage(
        ::webkit_network_session_get_cookie_manager(session),
        cookie_file,
        WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE
      );
    }

    free_string(base_cache_dir);
    free_string(base_data_dir);
    free_string(cookie_file);

    return session;
  }
}
