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
#include <selain/main-window.hpp>

#define SELAIN_JS_STRINGIFY(source) #source

namespace selain
{
  static Glib::ustring hint_mode_source_code =
  #include "./hint-mode.js"
  ;

  Glib::RefPtr<HintContext>
  HintContext::create()
  {
    return Glib::RefPtr<HintContext>(new HintContext());
  }

  HintContext::HintContext() {}

  void
  HintContext::install(Tab& tab)
  {
    tab.execute_script(hint_mode_source_code);
  }

  void
  HintContext::uninstall(Tab& tab)
  {
    tab.execute_script("window.SelainHintMode.uninstall();");
  }

  static void
  hint_mode_callback(::GObject* web_view_object,
                     ::GAsyncResult* result,
                     ::gpointer tab_data)
  {
    const auto web_view = WEBKIT_WEB_VIEW(web_view_object);
    const auto tab = static_cast<Tab*>(tab_data);
    ::GError* error = nullptr;
    const auto js_result = ::webkit_web_view_run_javascript_finish(
      web_view,
      result,
      &error
    );
    ::JSCValue* value;

    if (!js_result)
    {
      ::g_warning("Error running JavaScript: %s", error->message);
      ::g_error_free(error);
      return;
    }
    value = ::webkit_javascript_result_get_js_value(js_result);
    if (::jsc_value_is_string(value))
    {
      auto str_value = ::jsc_value_to_string(value);
      auto exception = ::jsc_context_get_exception(
        ::jsc_value_get_context(value)
      );

      if (exception)
      {
        g_warning(
          "Error running JavaScript: %s",
          ::jsc_exception_get_message(exception)
        );
      }
      else if (!::g_strcmp0(str_value, "mode::normal"))
      {
        if (const auto window = tab->get_main_window())
        {
          window->set_mode(Mode::NORMAL);
        }
      }
      else if (!::g_strcmp0(str_value, "mode::insert"))
      {
        if (const auto window = tab->get_main_window())
        {
          window->set_mode(Mode::INSERT);
        }
      }
      ::g_free(str_value);
    } else {
      ::g_warning("Unexpected return value from hint mode.");
    }
    ::webkit_javascript_result_unref(js_result);
  }

  void
  HintContext::add_digit(Tab& tab, int digit)
  {
    if (digit < 0 || digit > 9)
    {
      return;
    }
    tab.execute_script(
      Glib::ustring::compose("window.SelainHintMode.addDigit(%1);", digit),
      nullptr,
      hint_mode_callback,
      static_cast<void*>(&tab)
    );
  }

  void
  HintContext::remove_digit(Tab& tab)
  {
    tab.execute_script("window.SelainHintMode.removeDigit();");
  }

  void
  HintContext::activate_current_match(Tab& tab)
  {
    tab.execute_script(
      "window.SelainHintMode.activateCurrentMatch();",
      nullptr,
      hint_mode_callback,
      static_cast<void*>(&tab)
    );
  }
}
