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
#pragma once

#include <glibmm.h>

namespace selain
{
  /**
   * Abstract base class for bookmark manager implementation.
   */
  class BookmarkManager : public Glib::Object
  {
  public:
    /**
     * Represents a single bookmark in the bookmark manager.
     */
    struct Entry
    {
      int index = 0;
      Glib::ustring uri;
      Glib::ustring title;
      Glib::ustring tags;
      Glib::ustring description;
    };

    using list_callback_type = std::function<void(
      std::vector<Entry> bookmarks,
      Glib::ustring error
    )>;
    using add_callback_type = std::function<void(
      bool,
      const std::optional<Glib::ustring>&
    )>;

    /**
     * Lists all entries found in the bookmark manager.
     */
    virtual void list(const list_callback_type& callback) const = 0;

    /**
     * Searches for available bookmarks found in the bookmark manager.
     */
    virtual void search(
      const std::vector<Glib::ustring>& keywords,
      const list_callback_type& callback
    ) const = 0;

    /**
     * Inserts new entry to the bookmark manager.
     */
    virtual void add(
      const Glib::ustring& url,
      const Glib::ustring& title,
      const std::vector<Glib::ustring>& tags,
      const add_callback_type& callback
    ) = 0;
  };
}
