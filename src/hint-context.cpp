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
#include <atomic>
#include <cctype>
#include <mutex>
#include <unordered_map>

#include <cmrc/cmrc.hpp>
#include <inja/inja.hpp>

#include <selain/core/setting.hpp>
#include <selain/hint-context.hpp>
#include <selain/ui/main-window.hpp>
#include <selain/ui/view.hpp>
#include <selain/utils.hpp>

CMRC_DECLARE(selain::rc);

namespace selain
{
  using selain::ui::Tab;

  using context_map_type = std::unordered_map<std::string, HintContext*>;

  static constexpr const char* hint_handler_name = "hintMode";
  static constexpr const char* hint_script_world = "selain-hint";

  static context_map_type&
  active_contexts()
  {
    static context_map_type contexts;

    return contexts;
  }

  static std::string
  next_session_id()
  {
    static std::atomic<std::uint64_t> next_id{1};

    return std::to_string(next_id.fetch_add(1));
  }

  Glib::RefPtr<HintContext>
  HintContext::create(bool open_in_new_tab)
  {
    return Glib::make_refptr_for_instance<HintContext>(
      new HintContext(open_in_new_tab)
    );
  }

  HintContext::HintContext(bool open_in_new_tab)
    : Glib::ObjectBase(typeid(HintContext))
    , m_open_in_new_tab(open_in_new_tab)
    , m_id(next_session_id())
  {
  }

  HintContext::~HintContext()
  {
    active_contexts().erase(m_id);
    if (m_reply)
    {
      reply_with_json(m_reply, R"({"action":"uninstall"})");
      ::webkit_script_message_reply_unref(m_reply);
      m_reply = nullptr;
    }
    clear_js_context();
  }

  void
  HintContext::ensure_message_handler(::WebKitUserContentManager* manager)
  {
    static std::once_flag once_flag;

    std::call_once(once_flag, [manager]()
    {
      ::g_signal_connect(
        manager,
        "script-message-with-reply-received::hintMode",
        G_CALLBACK(on_script_message_with_reply),
        nullptr
      );
      if (!::webkit_user_content_manager_register_script_message_handler_with_reply(
            manager,
            hint_handler_name,
            hint_script_world
          ))
      {
        ::g_warning("Failed to register hint mode script message handler.");
      }
    });
  }

  ::gboolean
  HintContext::on_script_message_with_reply(
    ::WebKitUserContentManager*,
    ::JSCValue* value,
    ::WebKitScriptMessageReply* reply,
    ::gpointer
  )
  {
    g_autofree gchar* json = ::jsc_value_to_json(value, 0);

    if (!json)
    {
      ::webkit_script_message_reply_return_error_message(
        reply,
        "Invalid hint mode message."
      );
      return TRUE;
    }

    const auto message = inja::json::parse(json, nullptr, false);

    if (message.is_discarded() || !message.contains("id") || !message["id"].is_string())
    {
      ::webkit_script_message_reply_return_error_message(
        reply,
        "Invalid hint mode message."
      );
      return TRUE;
    }

    const auto id = message["id"].get<std::string>();
    const auto it = active_contexts().find(id);

    if (it == std::end(active_contexts()))
    {
      ::webkit_script_message_reply_return_error_message(
        reply,
        "Unknown hint mode session."
      );
      return TRUE;
    }

    it->second->handle_script_message(value, reply);

    return TRUE;
  }

  void
  HintContext::reply_with_json(
    ::WebKitScriptMessageReply* reply,
    const std::string& json
  )
  {
    if (!m_js_context)
    {
      ::webkit_script_message_reply_return_error_message(
        reply,
        "Missing JavaScript context for hint mode reply."
      );
      return;
    }

    auto* value = ::jsc_value_new_from_json(m_js_context, json.c_str());

    if (!value)
    {
      ::webkit_script_message_reply_return_error_message(
        reply,
        "Failed to encode hint mode command."
      );
      return;
    }
    ::webkit_script_message_reply_return_value(reply, value);
    ::g_object_unref(value);
  }

  void
  HintContext::handle_script_message(
    ::JSCValue* value,
    ::WebKitScriptMessageReply* reply
  )
  {
    g_autofree gchar* json = ::jsc_value_to_json(value, 0);
    const auto message = json
      ? inja::json::parse(json, nullptr, false)
      : inja::json();
    std::string mode;
    std::string uri;

    if (!message.is_discarded())
    {
      if (message.contains("mode") && message["mode"].is_string())
      {
        mode = message["mode"].get<std::string>();
      }
      if (message.contains("uri") && message["uri"].is_string())
      {
        uri = message["uri"].get<std::string>();
      }
    }

    // Replies must be created in the same JSCContext as the incoming message;
    // values from a fresh jsc_context_new() serialize as empty objects.
    if (auto* context = ::jsc_value_get_context(value))
    {
      if (m_js_context != context)
      {
        clear_js_context();
        m_js_context = static_cast<::JSCContext*>(g_object_ref(context));
      }
    }

    clear_reply();
    m_reply = ::webkit_script_message_reply_ref(reply);

    if (mode == "normal" || mode == "insert")
    {
      // Tell the script to leave its command loop, then switch mode. set_mode()
      // may destroy this HintContext, so do not touch members afterward.
      m_commands = {};
      reply_with_json(m_reply, R"({"action":"uninstall"})");
      ::webkit_script_message_reply_unref(m_reply);
      m_reply = nullptr;
      m_active = false;
      active_contexts().erase(m_id);
      clear_js_context();

      const auto tab = m_tab;
      const Glib::ustring open_uri = uri;

      m_tab = nullptr;
      if (tab)
      {
        if (const auto window = tab->get_main_window())
        {
          if (!open_uri.empty())
          {
            // Same deferred open as decide-policy: avoid loading from inside
            // the script-message handler.
            Glib::signal_idle().connect_once([window, open_uri]() {
              window->open_tab(open_uri, false);
            });
          }
          window->set_mode(mode == "insert" ? Mode::INSERT : Mode::NORMAL);
        }
      }
      return;
    }

    flush_commands();
    if (!m_active)
    {
      active_contexts().erase(m_id);
      clear_reply();
      clear_js_context();
    }
  }

  void
  HintContext::clear_reply()
  {
    if (m_reply)
    {
      ::webkit_script_message_reply_unref(m_reply);
      m_reply = nullptr;
    }
  }

  void
  HintContext::clear_js_context()
  {
    if (m_js_context)
    {
      ::g_object_unref(m_js_context);
      m_js_context = nullptr;
    }
  }

  void
  HintContext::enqueue_command(std::string command_json)
  {
    if (!m_active)
    {
      return;
    }
    m_commands.push(std::move(command_json));
    flush_commands();
  }

  void
  HintContext::flush_commands()
  {
    if (!m_reply || m_commands.empty())
    {
      return;
    }

    const auto command = std::move(m_commands.front());

    m_commands.pop();
    reply_with_json(m_reply, command);
    ::webkit_script_message_reply_unref(m_reply);
    m_reply = nullptr;
  }

  void
  HintContext::install(Tab& tab)
  {
    auto* view = tab.get_active_view();

    if (!view)
    {
      return;
    }

    auto* manager = view->get_user_content_manager();

    if (!manager)
    {
      return;
    }

    ensure_message_handler(manager);

    auto fs = cmrc::selain::rc::get_filesystem();
    auto script = fs.open("src/hint-mode.js");
    inja::json data;

    data["id"] = m_id;
    data["open-to-new-tab"] = m_open_in_new_tab;
    if (const auto window = tab.get_main_window())
    {
      data["hint-chars"] = utils::js_string_escape(
        setting::get(*window, "hint-chars").value_or("").raw()
      );
    } else {
      data["hint-chars"] = "";
    }

    m_tab = &tab;
    m_active = true;
    active_contexts()[m_id] = this;

    tab.execute_script(
      inja::render(
        std::string(std::begin(script), std::end(script)),
        data
      ),
      nullptr,
      nullptr,
      hint_script_world
    );
  }

  void
  HintContext::uninstall(Tab&)
  {
    if (!m_active)
    {
      return;
    }

    m_active = false;
    m_tab = nullptr;
    // Drop any pending key commands; only uninstall matters now.
    m_commands = {};
    if (m_reply)
    {
      reply_with_json(m_reply, R"({"action":"uninstall"})");
      ::webkit_script_message_reply_unref(m_reply);
      m_reply = nullptr;
      active_contexts().erase(m_id);
      clear_js_context();
    } else {
      // Script has not posted ready yet; deliver uninstall when it does.
      m_commands.push(R"({"action":"uninstall"})");
    }
  }

  void
  HintContext::add_char(Tab&, Glib::ustring::value_type ch)
  {
    if (!std::isalnum(static_cast<unsigned char>(ch)))
    {
      return;
    }

    inja::json command = {
      {"action", "addChar"},
      {"char", Glib::ustring(1, ch).uppercase().raw()},
    };

    enqueue_command(command.dump());
  }

  void
  HintContext::remove_char(Tab&)
  {
    enqueue_command(R"({"action":"removeChar"})");
  }

  void
  HintContext::activate_current_match(Tab&)
  {
    enqueue_command(R"({"action":"activate"})");
  }
}
