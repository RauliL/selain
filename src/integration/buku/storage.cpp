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
#include <inja/inja.hpp>

#include <giomm.h>

#include <selain/integration/buku/storage.hpp>
#include <selain/utils/string.hpp>

namespace selain::integration::buku
{
  static Glib::ustring
  json_string(const inja::json& value, const char* key)
  {
    if (!value.contains(key) || !value[key].is_string())
    {
      return {};
    }

    return value[key].get<std::string>();
  }

  static bool
  parse_bookmarks(
    const Glib::ustring& stdout_buf,
    std::vector<Bookmark>& out,
    Glib::ustring& error
  )
  {
    out.clear();

    const auto trimmed = utils::string::trim(stdout_buf);

    if (trimmed.empty())
    {
      return true;
    }

    const auto parsed = inja::json::parse(trimmed.raw(), nullptr, false);

    if (parsed.is_discarded() || !parsed.is_array())
    {
      error = "Failed to parse buku JSON output.";

      return false;
    }

    out.reserve(parsed.size());
    for (const auto& item : parsed)
    {
      if (!item.is_object())
      {
        continue;
      }

      Bookmark bookmark;

      if (item.contains("index") && item["index"].is_number_integer())
      {
        bookmark.index = item["index"].get<int>();
      }
      bookmark.uri = json_string(item, "uri");
      bookmark.title = json_string(item, "title");
      bookmark.tags = json_string(item, "tags");
      bookmark.description = json_string(item, "description");

      if (bookmark.uri.empty())
      {
        continue;
      }
      if (bookmark.title.empty())
      {
        bookmark.title = bookmark.uri;
      }
      out.push_back(std::move(bookmark));
    }

    return true;
  }

  static bool
  looks_like_empty_store(const Glib::ustring& stderr_buf)
  {
    const auto text = utils::string::trim(stderr_buf);

    return text.find("0 records") != Glib::ustring::npos;
  }

  static void
  run_buku_async(
    std::vector<std::string> argv,
    std::function<void(
      bool successful,
      Glib::ustring stdout_buf,
      Glib::ustring stderr_buf,
      Glib::ustring error
    )> callback
  )
  {
    if (!callback)
    {
      return;
    }

    gchar* buku_path = g_find_program_in_path("buku");

    if (!buku_path)
    {
      callback(false, {}, {}, "buku executable not found in PATH.");

      return;
    }

    argv.insert(argv.begin(), buku_path);
    g_free(buku_path);

    try
    {
      const auto flags =
        Gio::Subprocess::Flags::STDOUT_PIPE |
        Gio::Subprocess::Flags::STDERR_PIPE;
      auto subprocess = Gio::Subprocess::create(argv, flags);

      subprocess->communicate_utf8_async(
        Glib::ustring(),
        [subprocess, callback](const Glib::RefPtr<Gio::AsyncResult>& result)
        {
          try
          {
            const auto [stdout_buf, stderr_buf] =
              subprocess->communicate_utf8_finish(result);

            callback(
              subprocess->get_successful(),
              stdout_buf,
              stderr_buf,
              {}
            );
          }
          catch (const Glib::Error& error)
          {
            callback(false, {}, {}, error.what());
          }
        }
      );
    }
    catch (const Glib::Error& error)
    {
      callback(false, {}, {}, error.what());
    }
  }

  static void
  list_or_search_async(
    const std::vector<Glib::ustring>& keywords,
    list_callback callback
  )
  {
    if (!callback)
    {
      return;
    }

    std::vector<std::string> argv = {
      "--nostdin",
      "--np",
    };

    if (keywords.empty())
    {
      argv.push_back("-p");
      argv.push_back("--json");
    } else {
      argv.push_back("-s");
      for (const auto& keyword : keywords)
      {
        argv.push_back(keyword.raw());
      }
      argv.push_back("--json");
    }

    run_buku_async(
      std::move(argv),
      [callback](
        bool successful,
        Glib::ustring stdout_buf,
        Glib::ustring stderr_buf,
        Glib::ustring spawn_error
      )
      {
        if (!spawn_error.empty())
        {
          callback({}, spawn_error);

          return;
        }

        std::vector<Bookmark> bookmarks;
        Glib::ustring parse_error;

        if (parse_bookmarks(stdout_buf, bookmarks, parse_error))
        {
          callback(std::move(bookmarks), {});

          return;
        }
        if (looks_like_empty_store(stderr_buf))
        {
          callback({}, {});

          return;
        }
        if (!successful)
        {
          auto message = utils::string::trim(stderr_buf);

          if (message.empty())
          {
            message = "buku search failed.";
          }
          callback({}, message);

          return;
        }
        callback({}, parse_error);
      }
    );
  }

  void
  list_async(list_callback callback)
  {
    list_or_search_async({}, std::move(callback));
  }

  void
  search_async(
    const std::vector<Glib::ustring>& keywords,
    list_callback callback
  )
  {
    list_or_search_async(keywords, std::move(callback));
  }

  void
  add_async(
    const Glib::ustring& url,
    const Glib::ustring& title,
    const std::vector<Glib::ustring>& tags,
    add_callback callback
  )
  {
    if (!callback)
    {
      return;
    }
    if (url.empty())
    {
      callback(false, "Missing URL.");

      return;
    }

    std::vector<std::string> argv = {
      "--nostdin",
      "--np",
      "-a",
      url.raw(),
      "--title",
      title.empty() ? url.raw() : title.raw(),
    };

    if (!tags.empty())
    {
      Glib::ustring joined;

      for (std::size_t i = 0; i < tags.size(); ++i)
      {
        if (i > 0)
        {
          joined += ',';
        }
        joined += tags[i];
      }
      argv.push_back("--tag");
      argv.push_back(joined.raw());
    }

    run_buku_async(
      std::move(argv),
      [callback](
        bool successful,
        Glib::ustring stdout_buf,
        Glib::ustring stderr_buf,
        Glib::ustring spawn_error
      )
      {
        if (!spawn_error.empty())
        {
          callback(false, spawn_error);

          return;
        }
        if (!successful)
        {
          auto message = utils::string::trim(stderr_buf);

          if (message.empty())
          {
            message = "buku add failed.";
          }
          callback(false, message);

          return;
        }

        auto message = utils::string::trim(stdout_buf);

        if (message.empty())
        {
          message = "Bookmark added.";
        }
        callback(true, message);
      }
    );
  }
}
