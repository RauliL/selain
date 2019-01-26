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
#include <selain/split-view.hpp>
#include <selain/web-view.hpp>

namespace selain
{
  Tab::Tab(const Glib::RefPtr<WebContext>& context,
           const Glib::RefPtr<WebSettings>& settings)
    : View()
    , m_top_view(Glib::RefPtr<View>(new WebView(context, settings)))
  {
    add(*m_top_view.get());
    show_all();

    m_tab_label.signal_close_button_clicked().connect(sigc::mem_fun(
      this,
      &Tab::close
    ));
    m_top_view->signal_close().connect(sigc::mem_fun(
      this,
      &Tab::on_top_view_closed
    ));
    m_top_view->signal_mode_changed().connect(sigc::mem_fun(
      this,
      &Tab::on_mode_changed
    ));
    m_top_view->signal_title_changed().connect(sigc::mem_fun(
      this,
      &Tab::on_title_changed
    ));
    m_top_view->signal_favicon_changed().connect(sigc::mem_fun(
      this,
      &Tab::on_favicon_changed
    ));
  }

  void
  Tab::grab_focus()
  {
    if (m_top_view->get_mode() == Mode::COMMAND)
    {
      if (const auto window = get_main_window())
      {
        window->get_command_entry().grab_focus();
      }
    } else {
      m_top_view->grab_focus();
    }
  }

  Glib::RefPtr<View>
  Tab::split(const Glib::RefPtr<WebContext>& context,
             const Glib::RefPtr<WebSettings>& settings,
             Gtk::Orientation orientation)
  {
    Glib::RefPtr<View> split_view = m_top_view->split(
      context,
      settings,
      orientation
    );

    if (!split_view)
    {
      split_view = Glib::RefPtr<View>(new WebView(context, settings));
      remove();
      m_top_view = Glib::RefPtr<View>(new SplitView(
        m_top_view,
        split_view,
        orientation
      ));
      add(*m_top_view.get());
      show_all();
    }

    return split_view;
  }

  void
  Tab::on_top_view_closed(View&)
  {
    close();
  }

  void
  Tab::on_mode_changed(View&, Mode mode)
  {
    signal_mode_changed().emit(*this, mode);
  }

  void
  Tab::on_title_changed(View&, const Glib::ustring& title)
  {
    m_tab_label.set_text(title.empty() ? "Untitled" : title);
  }

  void
  Tab::on_favicon_changed(View&, ::cairo_surface_t* icon)
  {
    int width;
    int height;

    if (icon && Gtk::IconSize::lookup(Gtk::ICON_SIZE_BUTTON, width, height))
    {
      const auto pixbuf = Gdk::Pixbuf::create(
        Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(icon)),
        0,
        0,
        ::cairo_image_surface_get_width(icon),
        ::cairo_image_surface_get_height(icon)
      );

      m_tab_label.set_icon(pixbuf->scale_simple(
        width,
        height,
        Gdk::INTERP_BILINEAR
      ));
    } else {
      m_tab_label.set_icon(Glib::RefPtr<Gdk::Pixbuf>());
    }
  }
}
