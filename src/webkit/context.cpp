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
#include <cmrc/cmrc.hpp>

#include <selain/config.hpp>

#if defined(SELAIN_ENABLE_FINGER)
# include <selain/protocol/finger/scheme.hpp>
#endif
#if defined(SELAIN_ENABLE_GEMINI)
# include <selain/protocol/gemini/scheme.hpp>
#endif
#if defined(SELAIN_ENABLE_GOPHER)
# include <selain/protocol/gopher/scheme.hpp>
#endif
#include <selain/core/theme.hpp>
#include <selain/webkit/adblock.hpp>
#include <selain/webkit/context.hpp>
#include <selain/webkit/userscript.hpp>

CMRC_DECLARE(selain::rc);

namespace selain::webkit
{
  static WebKitNetworkSession* create_network_session();
  static void install_scroll_helper(WebKitUserContentManager* manager);

  Glib::RefPtr<Context>
  Context::create()
  {
    return Glib::make_refptr_for_instance<Context>(new Context());
  }

  Context::Context()
    : Glib::ObjectBase(typeid(Context))
    , m_context(webkit_web_context_new())
    , m_session(create_network_session())
    , m_user_content_manager(webkit_user_content_manager_new())
  {
#if defined(SELAIN_ENABLE_FINGER)
    protocol::finger::register_schemes(m_context);
#endif
#if defined(SELAIN_ENABLE_GEMINI)
    protocol::gemini::register_schemes(m_context);
#endif
#if defined(SELAIN_ENABLE_GOPHER)
    protocol::gopher::register_schemes(m_context);
#endif
    // Theme CSS must be loaded before colors are resolved for the sheet.
    theme::initialize();
    install_scroll_helper(m_user_content_manager);
    adblock::initialize(m_user_content_manager);
    userscript::initialize(m_user_content_manager);
  }

  Context::~Context()
  {
    userscript::shutdown();
    adblock::shutdown();
    g_clear_object(&m_session);
    g_clear_object(&m_context);
    g_clear_object(&m_user_content_manager);
  }

  WebKitWebView*
  Context::create_web_view()
  {
    return WEBKIT_WEB_VIEW(g_object_new(
      WEBKIT_TYPE_WEB_VIEW,
      "web-context",
      m_context,
      "network-session",
      m_session,
      "user-content-manager",
      m_user_content_manager,
      nullptr
    ));
  }

  static void
  install_scroll_helper(WebKitUserContentManager* manager)
  {
    // Document-start + all frames: PDF.js lives in a cross-origin iframe
    // (webkit-pdfjs-viewer://) that page scripts cannot touch, and document-end
    // scripts do not run on WebKit's PDF wrapper document.
    auto fs = cmrc::selain::rc::get_filesystem();
    auto file = fs.open("src/webkit/scroll-helper.js");
    const std::string source(std::begin(file), std::end(file));
    auto* script = webkit_user_script_new(
      source.c_str(),
      WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
      WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
      nullptr,
      nullptr
    );

    webkit_user_content_manager_add_script(manager, script);
    webkit_user_script_unref(script);
  }

  static WebKitNetworkSession*
  create_network_session()
  {
    const auto user_cache_dir = Glib::get_user_cache_dir();
    const auto base_cache_dir = Glib::build_filename(
      user_cache_dir,
      "selain",
      "cache"
    );
    const auto base_data_dir = Glib::build_filename(
      user_cache_dir,
      "selain",
      "data"
    );
    const auto cookie_file = Glib::build_filename(
      user_cache_dir,
      "selain",
      "cookies.sqlite"
    );
    WebKitNetworkSession* session;

    session = webkit_network_session_new(
      base_data_dir.c_str(),
      base_cache_dir.c_str()
    );

    webkit_website_data_manager_set_favicons_enabled(
      webkit_network_session_get_website_data_manager(session),
      true
    );
    webkit_cookie_manager_set_persistent_storage(
      webkit_network_session_get_cookie_manager(session),
      cookie_file.c_str(),
      WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE
    );

    return session;
  }
}
