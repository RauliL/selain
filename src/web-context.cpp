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
  static ::WebKitWebContext* create_web_context();

  Glib::RefPtr<WebContext>
  WebContext::create()
  {
    return Glib::RefPtr<WebContext>(new WebContext());
  }

  WebContext::WebContext()
    : m_context(create_web_context())
  {
    initialize(G_OBJECT(m_context));
  }

  ::WebKitWebView*
  WebContext::create_web_view()
  {
    return WEBKIT_WEB_VIEW(::webkit_web_view_new_with_context(m_context));
  }

  static inline void
  free_string(::gchar* str)
  {
    if (str)
    {
      ::g_free(str);
    }
  }

  static ::WebKitWebContext*
  create_web_context()
  {
    ::gchar* base_cache_dir = nullptr;
    ::gchar* base_data_dir = nullptr;
    ::gchar* disk_cache_dir = nullptr;
    ::gchar* indexeddb_dir = nullptr;
    ::gchar* local_storage_dir = nullptr;
    ::gchar* offline_app_dir = nullptr;
    ::gchar* websql_dir = nullptr;
    ::gchar* cookie_file = nullptr;
    ::gchar* favicon_cache_dir = nullptr;
    ::WebKitWebContext* context;

    if (const auto user_cache_dir = ::g_get_user_cache_dir())
    {
      base_cache_dir = ::g_build_filename(
        user_cache_dir,
        "selain",
        nullptr
      );
      base_data_dir = ::g_build_filename(
        user_cache_dir,
        "selain",
        "data",
        nullptr
      );
      disk_cache_dir = ::g_build_filename(
        user_cache_dir,
        "selain",
        "disk",
        nullptr
      );
      indexeddb_dir = ::g_build_filename(
        user_cache_dir,
        "selain",
        "indexeddb",
        nullptr
      );
      local_storage_dir = ::g_build_filename(
        user_cache_dir,
        "selain",
        "local-storage",
        nullptr
      );
      offline_app_dir = ::g_build_filename(
        user_cache_dir,
        "selain",
        "offline-app",
        nullptr
      );
      websql_dir = ::g_build_filename(
        user_cache_dir,
        "selain",
        "websql",
        nullptr
      );
      cookie_file = ::g_build_filename(
        user_cache_dir,
        "selain",
        "cookies.sqlite",
        nullptr
      );
      favicon_cache_dir = ::g_build_filename(
        user_cache_dir,
        "selain",
        "favicons",
        nullptr
      );
    }

    context = ::webkit_web_context_new_with_website_data_manager(
      ::webkit_website_data_manager_new(
        "base-cache-directory",
        base_cache_dir,
        "base-data-directory",
        base_data_dir,
        "disk-cache-directory",
        disk_cache_dir,
        "indexeddb-directory",
        indexeddb_dir,
        "local-storage-directory",
        local_storage_dir,
        "offline-application-cache-directory",
        offline_app_dir,
        "websql-directory",
        websql_dir,
        nullptr
      )
    );

    ::webkit_web_context_set_favicon_database_directory(
      context,
      favicon_cache_dir
    );

    if (cookie_file)
    {
      ::webkit_cookie_manager_set_persistent_storage(
        ::webkit_web_context_get_cookie_manager(context),
        cookie_file,
        WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE
      );
    }

    free_string(base_cache_dir);
    free_string(base_data_dir);
    free_string(disk_cache_dir);
    free_string(indexeddb_dir);
    free_string(offline_app_dir);
    free_string(websql_dir);
    free_string(cookie_file);
    free_string(favicon_cache_dir);

    return context;
  }
}
