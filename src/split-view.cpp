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

namespace selain
{
  SplitView::SplitView(const Glib::RefPtr<View>& child1,
                       const Glib::RefPtr<View>& child2,
                       Gtk::Orientation orientation)
    : View()
    , m_active_child_index(1)
    , m_child1(child1)
    , m_child2(child2)
    , m_paned(orientation)
  {
    const auto title_changed_callback = sigc::mem_fun(
      this,
      &SplitView::on_title_changed
    );
    const auto favicon_changed_callback = sigc::mem_fun(
      this,
      &SplitView::on_favicon_changed
    );

    m_paned.add1(*child1.get());
    m_paned.add2(*child2.get());
    m_paned.show_all();

    add(m_paned);
    show_all();

    signal_focus_in_event().connect(sigc::hide(sigc::mem_fun(
      this,
      &SplitView::on_focus_in
    )));

    m_child1->signal_title_changed().connect(title_changed_callback);
    m_child2->signal_title_changed().connect(title_changed_callback);

    m_child1->signal_favicon_changed().connect(favicon_changed_callback);
    m_child2->signal_favicon_changed().connect(favicon_changed_callback);

    // TODO: Connect rest of the fucking signals and store them, so they can be
    // disconnected.
  }

  void SplitView::close()
  {
    View::close(); // TODO: Do we even need this? We should use signals.
  }

  bool
  SplitView::on_focus_in()
  {
    get_active_child_view()->grab_focus();

    return true;
  }

  void
  SplitView::on_title_changed(View& view, const Glib::ustring& title)
  {
    if (get_active_child_view().get() == &view)
    {
      signal_title_changed().emit(*this, title);
    }
  }

  void
  SplitView::on_favicon_changed(View& view, ::cairo_surface_t* icon)
  {
    if (get_active_child_view().get() == &view)
    {
      signal_favicon_changed().emit(*this, icon);
    }
  }
}
