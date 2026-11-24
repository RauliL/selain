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
#include <selain/ui/list-dialog.hpp>
#include <selain/ui/main-window.hpp>

namespace selain::ui
{
  static Glib::ustring
  view_label(const View& view)
  {
    const auto title = view.get_title();

    if (title && !title->empty())
    {
      return *title;
    }

    return view.get_uri().value_or("Untitled");
  }

  static Glib::ustring
  view_haystack(const View& view)
  {
    return view_label(view) + ' ' + view.get_uri().value_or("");
  }

  void
  MainWindow::show_view_list_dialog()
  {
    std::vector<View*> views;
    std::vector<ListDialogEntry> entries;
    int preferred_index = 0;
    View* preferred = nullptr;

    if (auto* tab = get_current_tab())
    {
      preferred = tab->get_active_view();
    }

    for (auto* tab : get_tabs())
    {
      std::vector<View*> tab_views;

      tab->collect_views(tab_views);
      for (auto* view : tab_views)
      {
        ListDialogEntry entry;

        entry.title = view_label(*view);
        entry.secondary = view->get_uri().value_or("");
        entry.filter_text = view_haystack(*view);
        entry.icon = view->get_favicon();
        if (view == preferred)
        {
          preferred_index = static_cast<int>(entries.size());
        }
        entries.push_back(std::move(entry));
        views.push_back(view);
      }
    }

    ListDialog::show(
      *this,
      "Views",
      "Filter views…",
      std::move(entries),
      [&, views = std::move(views)](std::size_t index)
      {
        if (index < views.size() && views[index])
        {
          focus_view(*views[index]);
        }
      },
      preferred_index
    );
  }
}
