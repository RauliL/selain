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
#include <selain/split-view.hpp>
#include <selain/web-view.hpp>

namespace selain
{
  SplitView::SplitView(const Glib::RefPtr<View>& view1,
                       const Glib::RefPtr<View>& view2,
                       Gtk::Orientation orientation)
    : View()
    , m_current_view_index(1)
    , m_view1(view1)
    , m_view2(view2)
    , m_paned(orientation)
  {
    const auto mode_changed_callback = sigc::mem_fun(
      *this,
      &SplitView::on_mode_changed
    );
    const auto title_changed_callback = sigc::mem_fun(
      *this,
      &SplitView::on_title_changed
    );
    const auto favicon_changed_callback = sigc::mem_fun(
      *this,
      &SplitView::on_favicon_changed
    );

    m_paned.add1(*m_view1.get());
    m_paned.add2(*m_view2.get());
    m_paned.show_all();
    add(m_paned);
    show_all();

    m_view1->signal_mode_changed().connect(mode_changed_callback);
    m_view2->signal_mode_changed().connect(mode_changed_callback);

    m_view1->signal_title_changed().connect(title_changed_callback);
    m_view2->signal_title_changed().connect(title_changed_callback);

    m_view1->signal_favicon_changed().connect(favicon_changed_callback);
    m_view2->signal_favicon_changed().connect(favicon_changed_callback);
  }

  void
  SplitView::close()
  {
    // TODO: Somehow close the active view instead.
    signal_close().emit(*this);
  }

  void
  SplitView::grab_focus()
  {
    get_active_view()->grab_focus();
  }

  Glib::RefPtr<View>
  SplitView::split(const Glib::RefPtr<WebContext>& context,
                   const Glib::RefPtr<WebSettings>& settings,
                   Gtk::Orientation orientation)
  {
    auto split_view = get_active_view()->split(context, settings, orientation);

    if (split_view)
    {
      return split_view;
    }
    split_view = Glib::RefPtr<View>(new WebView(context, settings));
    if (m_current_view_index == 1)
    {
      m_paned.remove(*m_view1.get());
      m_view1 = Glib::RefPtr<View>(new SplitView(m_view1, split_view));
      m_paned.add1(*m_view1.get());
    } else {
      m_paned.remove(*m_view2.get());
      m_view2 = Glib::RefPtr<View>(new SplitView(m_view2, split_view));
      m_paned.add2(*m_view2.get());
    }
    m_paned.show_all();
    show_all();

    return split_view;
  }

  void
  SplitView::on_mode_changed(View& view, Mode mode)
  {
    if (get_active_view().get() == &view)
    {
      signal_mode_changed().emit(view, mode);
    }
  }

  void
  SplitView::on_title_changed(View& view, const Glib::ustring& title)
  {
    if (get_active_view().get() == &view)
    {
      signal_title_changed().emit(view, title);
    }
  }

  void
  SplitView::on_favicon_changed(View& view, ::cairo_surface_t* icon)
  {
    if (get_active_view().get() == &view)
    {
      signal_favicon_changed().emit(view, icon);
    }
  }
}
