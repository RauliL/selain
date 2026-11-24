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
#pragma once

#include <functional>
#include <vector>

#include <glibmm.h>

namespace selain::integration::buku
{
  struct Bookmark
  {
    int index = 0;
    Glib::ustring uri;
    Glib::ustring title;
    Glib::ustring tags;
    Glib::ustring description;
  };

  using list_callback = std::function<void(
    std::vector<Bookmark> bookmarks,
    Glib::ustring error
  )>;

  using add_callback = std::function<void(
    bool ok,
    Glib::ustring message
  )>;

  /**
   * Runs `buku -p --json` asynchronously and invokes @p callback with the
   * parsed bookmarks (or an empty list when the store has no records).
   */
  void list_async(list_callback callback);

  /**
   * Runs `buku -s <keywords…> --json` asynchronously. An empty @p keywords
   * vector lists all bookmarks (same as list_async).
   */
  void search_async(
    const std::vector<Glib::ustring>& keywords,
    list_callback callback
  );

  /**
   * Runs `buku -a <url> --title <title> [--tag tags]` asynchronously.
   */
  void add_async(
    const Glib::ustring& url,
    const Glib::ustring& title,
    const std::vector<Glib::ustring>& tags,
    add_callback callback
  );
}
