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
#include <selain/adblock.hpp>

#include <selain/adblock/easylist.hpp>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <glibmm.h>

namespace selain::adblock
{
  namespace
  {
    struct LoadJob
    {
      ::WebKitUserContentManager* manager{nullptr};
      ::WebKitUserContentFilterStore* store{nullptr};
      ::GBytes* bytes{nullptr};
      std::string identifier;
      std::filesystem::path source_path;
      std::filesystem::path meta_path;
      std::string fingerprint;
    };

    [[nodiscard]] std::filesystem::path
    meta_directory()
    {
      return std::filesystem::path(Glib::build_filename(
        Glib::get_user_cache_dir(),
        "selain",
        "blocklists"
      ));
    }

    [[nodiscard]] std::filesystem::path
    meta_path_for(std::string_view identifier)
    {
      return meta_directory() / (std::string(identifier) + ".meta");
    }

    [[nodiscard]] std::string
    source_fingerprint(const std::filesystem::path& path)
    {
      const auto size = std::filesystem::file_size(path);
      const auto mtime = std::filesystem::last_write_time(path);
      const auto mtime_count = std::chrono::duration_cast<std::chrono::seconds>(
        mtime.time_since_epoch()
      ).count();
      return std::to_string(size) + ':' + std::to_string(mtime_count);
    }

    [[nodiscard]] bool
    meta_matches(const std::filesystem::path& meta_path, std::string_view fingerprint)
    {
      std::ifstream input(meta_path);
      if (!input)
      {
        return false;
      }
      std::string stored;
      std::getline(input, stored);
      return stored == fingerprint;
    }

    void
    write_meta(const std::filesystem::path& meta_path, std::string_view fingerprint)
    {
      std::error_code ec;
      std::filesystem::create_directories(meta_path.parent_path(), ec);
      std::ofstream output(meta_path, std::ios::trunc);
      if (output)
      {
        output << fingerprint << '\n';
      }
    }

    void
    add_filter(
      ::WebKitUserContentManager* manager,
      ::WebKitUserContentFilter* filter
    )
    {
      ::webkit_user_content_manager_add_filter(manager, filter);
      ::webkit_user_content_filter_unref(filter);
    }

    void destroy_job(LoadJob* job);

    void
    compile_and_save(LoadJob* job)
    {
      std::string source;
      try
      {
        std::ifstream input(job->source_path, std::ios::binary);
        if (!input)
        {
          throw std::runtime_error("unable to open file");
        }
        std::ostringstream buffer;
        buffer << input.rdbuf();
        source = buffer.str();
      }
      catch (const std::exception& ex)
      {
        std::cerr
          << "selain: failed to read blocklist '"
          << job->source_path.string()
          << "': "
          << ex.what()
          << std::endl;
        destroy_job(job);
        return;
      }

      std::string json;
      try
      {
        if (looks_like_content_blocker_json(source))
        {
          json = std::move(source);
        }
        else
        {
          json = easylist_to_content_blocker_json(source);
        }
      }
      catch (const std::exception& ex)
      {
        std::cerr
          << "selain: failed to convert blocklist '"
          << job->source_path.string()
          << "': "
          << ex.what()
          << std::endl;
        destroy_job(job);
        return;
      }

      if (json == "[]")
      {
        destroy_job(job);
        return;
      }

      auto* owned = new std::string(std::move(json));
      job->bytes = g_bytes_new_with_free_func(
        owned->data(),
        static_cast<gsize>(owned->size()),
        [](gpointer data) {
          delete static_cast<std::string*>(data);
        },
        owned
      );

      ::webkit_user_content_filter_store_save(
        job->store,
        job->identifier.c_str(),
        job->bytes,
        nullptr,
        [](GObject* source, GAsyncResult* result, gpointer user_data) {
          auto* job = static_cast<LoadJob*>(user_data);
          GError* error = nullptr;
          auto* filter = webkit_user_content_filter_store_save_finish(
            WEBKIT_USER_CONTENT_FILTER_STORE(source),
            result,
            &error
          );

          if (filter)
          {
            add_filter(job->manager, filter);
            write_meta(job->meta_path, job->fingerprint);
          }
          else
          {
            std::cerr
              << "selain: failed to compile content filter '"
              << job->identifier
              << "': "
              << (error ? error->message : "unknown error")
              << std::endl;
            if (error)
            {
              g_error_free(error);
            }
          }
          destroy_job(job);
        },
        job
      );
    }

    void
    destroy_job(LoadJob* job)
    {
      g_clear_pointer(&job->bytes, g_bytes_unref);
      g_object_unref(job->store);
      delete job;
    }

    void
    on_filter_loaded(
      GObject* source,
      GAsyncResult* result,
      gpointer user_data
    )
    {
      auto* job = static_cast<LoadJob*>(user_data);
      GError* error = nullptr;
      auto* filter = webkit_user_content_filter_store_load_finish(
        WEBKIT_USER_CONTENT_FILTER_STORE(source),
        result,
        &error
      );

      if (filter)
      {
        add_filter(job->manager, filter);
        destroy_job(job);
        return;
      }

      if (error)
      {
        g_error_free(error);
      }

      // Source fingerprint matched but compiled filter is missing: rebuild.
      compile_and_save(job);
    }

    [[nodiscard]] std::string
    identifier_for_path(const std::filesystem::path& path)
    {
      return path.filename().string();
    }

    void
    load_file_with_store(
      ::WebKitUserContentManager* manager,
      ::WebKitUserContentFilterStore* store,
      const std::filesystem::path& path,
      std::string_view identifier
    )
    {
      std::string fingerprint;
      try
      {
        fingerprint = source_fingerprint(path);
      }
      catch (const std::exception& ex)
      {
        std::cerr
          << "selain: failed to stat blocklist '"
          << path.string()
          << "': "
          << ex.what()
          << std::endl;
        return;
      }

      auto* job = new LoadJob{};
      job->manager = manager;
      job->store = WEBKIT_USER_CONTENT_FILTER_STORE(g_object_ref(store));
      job->identifier = std::string(identifier);
      job->source_path = path;
      job->meta_path = meta_path_for(identifier);
      job->fingerprint = std::move(fingerprint);

      if (meta_matches(job->meta_path, job->fingerprint))
      {
        webkit_user_content_filter_store_load(
          store,
          job->identifier.c_str(),
          nullptr,
          on_filter_loaded,
          job
        );
        return;
      }

      compile_and_save(job);
    }
  }

  std::filesystem::path
  blocklists_directory()
  {
    return std::filesystem::path(Glib::build_filename(
      Glib::get_user_config_dir(),
      "selain",
      "blocklists"
    ));
  }

  std::filesystem::path
  filter_store_directory()
  {
    return std::filesystem::path(Glib::build_filename(
      Glib::get_user_cache_dir(),
      "selain",
      "content-filters"
    ));
  }

  void
  load_file(
    ::WebKitUserContentManager* manager,
    const std::filesystem::path& path,
    std::string_view identifier
  )
  {
    if (!manager)
    {
      return;
    }

    std::error_code ec;
    std::filesystem::create_directories(filter_store_directory(), ec);

    auto* store = webkit_user_content_filter_store_new(
      filter_store_directory().c_str()
    );
    load_file_with_store(manager, store, path, identifier);
    g_object_unref(store);
  }

  void
  initialize(::WebKitUserContentManager* manager)
  {
    if (!manager)
    {
      return;
    }

    const auto directory = blocklists_directory();
    std::error_code ec;

    if (!std::filesystem::exists(directory, ec))
    {
      std::filesystem::create_directories(directory, ec);
      return;
    }

    if (!std::filesystem::is_directory(directory, ec))
    {
      std::cerr
        << "selain: blocklists path is not a directory: "
        << directory.string()
        << std::endl;
      return;
    }

    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
    {
      if (ec)
      {
        break;
      }
      if (!entry.is_regular_file(ec) || ec)
      {
        continue;
      }
      const auto name = entry.path().filename().string();
      if (name.empty() || name.front() == '.')
      {
        continue;
      }
      files.push_back(entry.path());
    }

    if (files.empty())
    {
      return;
    }

    std::sort(files.begin(), files.end());
    std::filesystem::create_directories(filter_store_directory(), ec);

    auto* store = webkit_user_content_filter_store_new(
      filter_store_directory().c_str()
    );
    for (const auto& path : files)
    {
      load_file_with_store(
        manager,
        store,
        path,
        identifier_for_path(path)
      );
    }
    g_object_unref(store);
  }
}
