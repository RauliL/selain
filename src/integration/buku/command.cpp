/*
 * Copyright (c) 2026, Rauli Laine
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
#include <selain/integration/buku/storage.hpp>
#include <selain/ui/list-dialog.hpp>
#include <selain/ui/main-window.hpp>
#include <selain/ui/tab.hpp>
#include <selain/utils/string.hpp>

namespace
{
  enum class BukuMode
  {
    OPEN,
    ADD,
  };

  struct BukuOptions
  {
    BukuMode mode = BukuMode::OPEN;
    bool new_tab = false;
    std::vector<Glib::ustring> tokens;
  };
}

static bool
tab_still_open(selain::ui::MainWindow& window, selain::ui::Tab* tab)
{
  if (!tab)
  {
    return false;
  }
  for (int i = 0; i < window.get_tab_count(); ++i)
  {
    if (window.get_nth_tab(i) == tab)
    {
      return true;
    }
  }

  return false;
}

static std::vector<Glib::ustring>
split_args(const Glib::ustring& args)
{
  std::vector<Glib::ustring> tokens;
  Glib::ustring current;
  bool in_token = false;

  for (const auto ch : args)
  {
    if (g_unichar_isspace(ch))
    {
      if (in_token)
      {
        tokens.push_back(current);
        current.clear();
        in_token = false;
      }
      continue;
    }
    current.push_back(ch);
    in_token = true;
  }
  if (in_token)
  {
    tokens.push_back(current);
  }

  return tokens;
}

static bool
parse_buku_options(
  selain::ui::MainWindow& window,
  const Glib::ustring& args,
  BukuOptions& options
)
{
  options = {};
  options.tokens = split_args(selain::utils::string::trim(args));

  if (options.tokens.empty())
  {
    return true;
  }

  const auto& first = options.tokens.front();

  if (first == "-a" || first == "--add")
  {
    options.mode = BukuMode::ADD;
    options.tokens.erase(options.tokens.begin());

    return true;
  }
  if (first == "-t" || first == "--tab")
  {
    options.new_tab = true;
    options.tokens.erase(options.tokens.begin());

    return true;
  }
  if (first.size() > 1 && first[0] == '-')
  {
    window.show_error("Usage: :buku [-t|-a] [query|tags…]");

    return false;
  }

  return true;
}

static void
show_bookmarks(
  selain::ui::MainWindow& window,
  selain::ui::Tab* tab,
  bool new_tab,
  std::vector<selain::integration::buku::Bookmark> bookmarks,
  const Glib::ustring& initial_filter
)
{
  if (bookmarks.empty())
  {
    window.show_error("No bookmarks found.");

    return;
  }

  std::vector<selain::ui::ListDialogEntry> dialog_entries;

  dialog_entries.reserve(bookmarks.size());
  for (const auto& bookmark : bookmarks)
  {
    selain::ui::ListDialogEntry entry;

    entry.title = bookmark.title;
    entry.secondary = bookmark.uri;
    entry.filter_text = bookmark.title + ' ' + bookmark.uri;
    if (!bookmark.tags.empty())
    {
      entry.filter_text += ' ' + bookmark.tags;
    }
    entry.icon_name = "user-bookmarks";
    dialog_entries.push_back(std::move(entry));
  }

  selain::ui::ListDialog::show(
    window,
    "Buku",
    "Filter bookmarks…",
    std::move(dialog_entries),
    [&window, tab, new_tab, bookmarks = std::move(bookmarks)](std::size_t index)
    {
      if (index >= bookmarks.size())
      {
        return;
      }

      const auto& uri = bookmarks[index].uri;

      if (new_tab)
      {
        window.open_tab(uri);

        return;
      }
      if (tab_still_open(window, tab))
      {
        tab->load_uri(uri);
      } else {
        window.open_tab(uri);
      }
    },
    0,
    initial_filter
  );
}

static void
open_bookmarks(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  bool new_tab,
  const std::vector<Glib::ustring>& keywords
)
{
  auto* tab_ptr = &tab;
  Glib::ustring filter;

  for (std::size_t i = 0; i < keywords.size(); ++i)
  {
    if (i > 0)
    {
      filter += ' ';
    }
    filter += keywords[i];
  }

  auto on_result = [&window, tab_ptr, new_tab, filter](
    std::vector<selain::integration::buku::Bookmark> bookmarks,
    Glib::ustring error
  )
  {
    if (!error.empty())
    {
      window.show_error(error);

      return;
    }
    show_bookmarks(
      window,
      tab_ptr,
      new_tab,
      std::move(bookmarks),
      filter
    );
  };

  if (keywords.empty())
  {
    selain::integration::buku::list_async(std::move(on_result));
  } else {
    selain::integration::buku::search_async(keywords, std::move(on_result));
  }
}

static void
add_current_page(
  selain::ui::MainWindow& window,
  selain::ui::Tab& tab,
  const std::vector<Glib::ustring>& tags
)
{
  const auto uri = tab.get_uri();

  if (!uri || uri->empty())
  {
    window.show_error("Current view has no URI.");

    return;
  }

  Glib::ustring title = *uri;

  if (const auto* view = tab.get_active_view())
  {
    if (const auto page_title = view->get_title())
    {
      if (!page_title->empty())
      {
        title = *page_title;
      }
    }
  }

  selain::integration::buku::add_async(
    *uri,
    title,
    tags,
    [&window](bool ok, Glib::ustring message)
    {
      if (!ok)
      {
        window.show_error(
          message.empty() ? "Failed to add bookmark." : message
        );

        return;
      }
      window.show_info(
        message.empty() ? "Bookmark added." : message
      );
    }
  );
}

namespace selain::integration::buku
{
  bool
  command(
    selain::ui::MainWindow& window,
    selain::ui::Tab& tab,
    Glib::ustring args
  )
  {
    BukuOptions options;

    if (!parse_buku_options(window, args, options))
    {
      return false;
    }

    if (options.mode == BukuMode::ADD)
    {
      add_current_page(window, tab, options.tokens);

      return true;
    }

    open_bookmarks(window, tab, options.new_tab, options.tokens);

    return true;
  }
}
