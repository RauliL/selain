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

#include <limits>

namespace selain
{
  namespace
  {
    struct Rect
    {
      double x;
      double y;
      double width;
      double height;
    };

    Rect
    widget_rect(const Gtk::Widget& widget, const Gtk::Widget& relative_to)
    {
      ::graphene_rect_t bounds;

      if (::gtk_widget_compute_bounds(
            const_cast<::GtkWidget*>(widget.gobj()),
            const_cast<::GtkWidget*>(relative_to.gobj()),
            &bounds
          ))
      {
        return {
          bounds.origin.x,
          bounds.origin.y,
          bounds.size.width,
          bounds.size.height
        };
      }

      return { 0.0, 0.0, 0.0, 0.0 };
    }

    bool
    is_in_direction(FocusDirection direction,
                    const Rect& from,
                    const Rect& to)
    {
      const auto from_cx = from.x + from.width / 2.0;
      const auto from_cy = from.y + from.height / 2.0;
      const auto to_cx = to.x + to.width / 2.0;
      const auto to_cy = to.y + to.height / 2.0;

      switch (direction)
      {
        case FocusDirection::LEFT:
          return to_cx < from_cx;
        case FocusDirection::RIGHT:
          return to_cx > from_cx;
        case FocusDirection::UP:
          return to_cy < from_cy;
        case FocusDirection::DOWN:
          return to_cy > from_cy;
      }

      return false;
    }

    double
    direction_score(FocusDirection direction,
                    const Rect& from,
                    const Rect& to)
    {
      const auto from_cx = from.x + from.width / 2.0;
      const auto from_cy = from.y + from.height / 2.0;
      const auto to_cx = to.x + to.width / 2.0;
      const auto to_cy = to.y + to.height / 2.0;
      const auto dx = to_cx - from_cx;
      const auto dy = to_cy - from_cy;

      switch (direction)
      {
        case FocusDirection::LEFT:
          return (-dx) + (std::abs(dy) * 2.0);
        case FocusDirection::RIGHT:
          return dx + (std::abs(dy) * 2.0);
        case FocusDirection::UP:
          return (-dy) + (std::abs(dx) * 2.0);
        case FocusDirection::DOWN:
          return dy + (std::abs(dx) * 2.0);
      }

      return std::numeric_limits<double>::max();
    }
  }

  Tab::Tab(const Glib::RefPtr<WebContext>& context,
           const Glib::RefPtr<WebSettings>& settings)
    : Gtk::Box(Gtk::Orientation::VERTICAL)
    , m_web_context(context)
    , m_web_settings(settings)
    , m_active_view(nullptr)
  {
    m_tab_label.signal_close_requested().connect(sigc::mem_fun(
      *this,
      &Tab::on_close_button_clicked
    ));

    const auto view = create_view();

    append(*view.get());
    m_active_view = view.get();
    m_active_view->add_css_class("selain-view-active");

    set_hexpand(true);
    set_vexpand(true);
    add_css_class("selain-tab");
  }

  Tab::~Tab() = default;

  void
  Tab::set_hint_context(const Glib::RefPtr<HintContext>& hint_context)
  {
    if (m_hint_context)
    {
      m_hint_context->uninstall(*this);
      m_hint_context.reset();
    }
    if ((m_hint_context = hint_context))
    {
      m_hint_context->install(*this);
    }
  }

  MainWindow*
  Tab::get_main_window()
  {
    return dynamic_cast<MainWindow*>(get_root());
  }

  const MainWindow*
  Tab::get_main_window() const
  {
    return dynamic_cast<const MainWindow*>(get_root());
  }

  View*
  Tab::get_active_view()
  {
    return m_active_view;
  }

  const View*
  Tab::get_active_view() const
  {
    return m_active_view;
  }

  void
  Tab::set_active_view(View& view)
  {
    if (m_active_view == &view)
    {
      return;
    }
    if (m_active_view)
    {
      m_active_view->remove_css_class("selain-view-active");
    }
    m_active_view = &view;
    m_active_view->add_css_class("selain-view-active");
    sync_active_chrome();
    m_active_view->grab_focus();
  }

  void
  Tab::split(bool vertical, const Glib::ustring& uri)
  {
    if (!m_active_view)
    {
      return;
    }

    auto& current = *m_active_view;
    const auto next = create_view();
    const auto load_uri = uri.empty() ? current.get_uri() : uri;

    if (!load_uri.empty())
    {
      next->load_uri(load_uri);
    }

    replace_view_with_paned(
      current,
      *next.get(),
      vertical ? Gtk::Orientation::HORIZONTAL : Gtk::Orientation::VERTICAL
    );
    set_active_view(*next.get());
  }

  void
  Tab::close_active_view()
  {
    if (!m_active_view)
    {
      return;
    }
    if (m_views.size() <= 1)
    {
      if (const auto window = get_main_window())
      {
        window->close_tab(*this);
      }
      return;
    }

    auto* closing = m_active_view;
    auto* parent = dynamic_cast<Gtk::Paned*>(closing->get_parent());

    if (!parent)
    {
      return;
    }

    auto* start = parent->get_start_child();
    auto* end = parent->get_end_child();
    auto* sibling = start == closing ? end : start;

    if (!sibling)
    {
      return;
    }

    parent->unset_start_child();
    parent->unset_end_child();
    promote_sibling(*parent, *sibling);

    for (auto it = std::begin(m_views); it != std::end(m_views); ++it)
    {
      if (it->get() == closing)
      {
        m_views.erase(it);
        break;
      }
    }

    if (auto* next_view = dynamic_cast<View*>(sibling))
    {
      set_active_view(*next_view);
    } else {
      std::vector<View*> remaining;

      collect_views(remaining);
      if (!remaining.empty())
      {
        set_active_view(*remaining.front());
      }
    }
  }

  void
  Tab::close_other_views()
  {
    if (!m_active_view || m_views.size() <= 1)
    {
      return;
    }

    Glib::RefPtr<View> active;

    for (const auto& view : m_views)
    {
      if (view.get() == m_active_view)
      {
        active = view;
        break;
      }
    }
    if (!active)
    {
      return;
    }

    while (auto* child = get_first_child())
    {
      remove(*child);
    }

    m_views.clear();
    m_views.push_back(active);
    if (active->get_parent())
    {
      active->unparent();
    }
    append(*active.get());
    m_active_view = active.get();
    m_active_view->add_css_class("selain-view-active");
    sync_active_chrome();
    m_active_view->grab_focus();
  }

  void
  Tab::focus_view(FocusDirection direction)
  {
    if (!m_active_view)
    {
      return;
    }

    std::vector<View*> views;

    collect_views(views);
    if (views.size() < 2)
    {
      return;
    }

    const auto from = widget_rect(*m_active_view, *this);
    View* best = nullptr;
    auto best_score = std::numeric_limits<double>::max();

    for (auto* view : views)
    {
      if (view == m_active_view)
      {
        continue;
      }

      const auto to = widget_rect(*view, *this);

      if (!is_in_direction(direction, from, to))
      {
        continue;
      }

      const auto score = direction_score(direction, from, to);

      if (score < best_score)
      {
        best_score = score;
        best = view;
      }
    }

    if (best)
    {
      set_active_view(*best);
    }
  }

  void
  Tab::cycle_view(bool backwards)
  {
    std::vector<View*> views;

    collect_views(views);
    if (views.size() < 2 || !m_active_view)
    {
      return;
    }

    std::size_t index = 0;

    for (; index < views.size(); ++index)
    {
      if (views[index] == m_active_view)
      {
        break;
      }
    }
    if (index >= views.size())
    {
      index = 0;
    }

    if (backwards)
    {
      index = index == 0 ? views.size() - 1 : index - 1;
    } else {
      index = (index + 1) % views.size();
    }
    set_active_view(*views[index]);
  }

  Glib::ustring
  Tab::get_uri() const
  {
    return m_active_view ? m_active_view->get_uri() : Glib::ustring();
  }

  void
  Tab::collect_history_uris(std::vector<Glib::ustring>& uris) const
  {
    for (const auto& view : m_views)
    {
      if (view)
      {
        view->collect_history_uris(uris);
      }
    }
  }

  void
  Tab::load_uri(const Glib::ustring& uri)
  {
    if (m_active_view)
    {
      m_active_view->load_uri(uri);
    }
  }

  void
  Tab::reload(bool bypass_cache)
  {
    if (m_active_view)
    {
      m_active_view->reload(bypass_cache);
    }
  }

  void
  Tab::stop_loading()
  {
    if (m_active_view)
    {
      m_active_view->stop_loading();
    }
  }

  void
  Tab::execute_script(const Glib::ustring& script,
                      ::GCancellable* cancellable,
                      ::GAsyncReadyCallback callback,
                      void* user_data)
  {
    if (m_active_view)
    {
      m_active_view->execute_script(script, cancellable, callback, user_data);
    }
  }

  void
  Tab::go_back()
  {
    if (m_active_view)
    {
      m_active_view->go_back();
    }
  }

  void
  Tab::go_forward()
  {
    if (m_active_view)
    {
      m_active_view->go_forward();
    }
  }

  void
  Tab::search(const Glib::ustring& text, bool forwards)
  {
    if (m_active_view)
    {
      m_active_view->search(text, forwards);
    }
  }

  void
  Tab::search_next()
  {
    if (m_active_view)
    {
      m_active_view->search_next();
    }
  }

  void
  Tab::search_prev()
  {
    if (m_active_view)
    {
      m_active_view->search_prev();
    }
  }

  void
  Tab::grab_focus()
  {
    if (m_active_view)
    {
      m_active_view->grab_focus();
    }
  }

  const Glib::ustring&
  Tab::get_status() const
  {
    static const Glib::ustring empty;

    return m_active_view ? m_active_view->get_status() : empty;
  }

  void
  Tab::set_status(const Glib::ustring& status, bool permanent)
  {
    if (m_active_view)
    {
      m_active_view->set_status(status, permanent);
    }
  }

  void
  Tab::on_view_status_changed(View& view, const Glib::ustring& status)
  {
    if (m_active_view == &view)
    {
      m_signal_status_changed.emit(this, status);
    }
  }

  void
  Tab::on_view_title_changed(View& view)
  {
    if (m_active_view != &view)
    {
      return;
    }

    const auto title = view.get_title();

    m_tab_label.set_text(title.empty() ? "Untitled" : title);
  }

  void
  Tab::on_view_favicon_changed(View& view)
  {
    if (m_active_view == &view)
    {
      m_tab_label.set_icon(view.get_favicon());
    }
  }

  Glib::RefPtr<View>
  Tab::create_view()
  {
    auto view = Glib::make_refptr_for_instance<View>(
      new View(m_web_context, m_web_settings)
    );

    m_views.push_back(view);

    return view;
  }

  void
  Tab::replace_view_with_paned(View& current,
                               View& next,
                               Gtk::Orientation orientation)
  {
    auto* paned = Gtk::make_managed<Gtk::Paned>(orientation);
    auto* parent = current.get_parent();

    paned->set_hexpand(true);
    paned->set_vexpand(true);
    paned->set_wide_handle(true);
    paned->set_resize_start_child(true);
    paned->set_resize_end_child(true);
    paned->set_shrink_start_child(true);
    paned->set_shrink_end_child(true);

    if (parent == this)
    {
      remove(current);
      // Vim default: new split appears above / to the left of the current one.
      paned->set_start_child(next);
      paned->set_end_child(current);
      append(*paned);
    }
    else if (auto* parent_paned = dynamic_cast<Gtk::Paned*>(parent))
    {
      const bool is_start = parent_paned->get_start_child() == &current;

      if (is_start)
      {
        parent_paned->unset_start_child();
      } else {
        parent_paned->unset_end_child();
      }
      paned->set_start_child(next);
      paned->set_end_child(current);
      if (is_start)
      {
        parent_paned->set_start_child(*paned);
      } else {
        parent_paned->set_end_child(*paned);
      }
    }
  }

  void
  Tab::promote_sibling(Gtk::Paned& paned, Gtk::Widget& sibling)
  {
    auto* grandparent = paned.get_parent();

    if (grandparent == this)
    {
      remove(paned);
      append(sibling);
      return;
    }

    auto* parent_paned = dynamic_cast<Gtk::Paned*>(grandparent);

    if (!parent_paned)
    {
      return;
    }

    const bool paned_is_start = parent_paned->get_start_child() == &paned;

    if (paned_is_start)
    {
      parent_paned->unset_start_child();
      parent_paned->set_start_child(sibling);
    } else {
      parent_paned->unset_end_child();
      parent_paned->set_end_child(sibling);
    }
  }

  void
  Tab::collect_views(std::vector<View*>& output) const
  {
    output.clear();
    for (const auto& view : m_views)
    {
      if (view)
      {
        output.push_back(view.get());
      }
    }
  }

  void
  Tab::sync_active_chrome()
  {
    if (!m_active_view)
    {
      m_tab_label.set_text("Untitled");
      m_tab_label.set_icon(Glib::RefPtr<Gdk::Paintable>());
      m_signal_status_changed.emit(this, Glib::ustring());
      return;
    }

    const auto title = m_active_view->get_title();

    m_tab_label.set_text(title.empty() ? "Untitled" : title);
    m_tab_label.set_icon(m_active_view->get_favicon());
    m_signal_status_changed.emit(this, m_active_view->get_status());
  }

  void
  Tab::on_close_button_clicked()
  {
    if (const auto window = get_main_window())
    {
      window->close_tab(*this);
    }
  }
}
