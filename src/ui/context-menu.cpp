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
#include <format>

#include <selain/core/setting.hpp>
#include <selain/ui/main-window.hpp>
#include <selain/ui/view.hpp>
#include <selain/utils/string.hpp>
#include <selain/utils/url.hpp>

namespace selain::ui
{
  static void create_editable_menu(WebKitContextMenu*, WebKitWebView*);
  static void create_image_menu(WebKitContextMenu*, WebKitHitTestResult*, View*);
  static void create_link_menu(WebKitContextMenu*, WebKitHitTestResult*, View*);
  static void create_media_menu(
    WebKitContextMenu*,
    WebKitHitTestResult*,
    View*,
    bool is_video,
    WebKitContextMenuItem* play_item,
    WebKitContextMenuItem* pause_item,
    WebKitContextMenuItem* mute_item,
    WebKitContextMenuItem* controls_item,
    WebKitContextMenuItem* loop_item,
    WebKitContextMenuItem* fullscreen_item
  );
  static void create_selection_menu(WebKitContextMenu*, View*);
  static void create_default_menu(WebKitContextMenu*, WebKitWebView*);

  static void
  append_separator(WebKitContextMenu* menu)
  {
    webkit_context_menu_append(
      menu,
      webkit_context_menu_item_new_separator()
    );
  }

  static void
  append_stock(
    WebKitContextMenu* menu,
    WebKitContextMenuAction action,
    const char* label
  )
  {
    webkit_context_menu_append(
      menu,
      webkit_context_menu_item_new_from_stock_action_with_label(action, label)
    );
  }

  static void
  append_stock_enabled(
    WebKitContextMenu* menu,
    WebKitContextMenuAction action,
    const char* label,
    bool enabled
  )
  {
    const auto item = webkit_context_menu_item_new_from_stock_action_with_label(
      action,
      label
    );
    const auto gaction = webkit_context_menu_item_get_gaction(item);

    if (G_IS_SIMPLE_ACTION(gaction))
    {
      g_simple_action_set_enabled(G_SIMPLE_ACTION(gaction), enabled);
    }

    webkit_context_menu_append(menu, item);
  }

  static void
  append_inspect(WebKitContextMenu* menu)
  {
    append_stock(
      menu,
      WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT,
      "Inspect element"
    );
  }

  static WebKitContextMenuItem*
  find_stock_item(WebKitContextMenu* menu, WebKitContextMenuAction action)
  {
    for (
      GList* items = webkit_context_menu_get_items(menu);
      items;
      items = items->next
    )
    {
      const auto item = WEBKIT_CONTEXT_MENU_ITEM(items->data);

      if (webkit_context_menu_item_get_stock_action(item) == action)
      {
        return WEBKIT_CONTEXT_MENU_ITEM(g_object_ref(item));
      }
    }

    return nullptr;
  }

  static bool
  menu_has_stock(WebKitContextMenu* menu, WebKitContextMenuAction action)
  {
    if (const auto item = find_stock_item(menu, action))
    {
      g_object_unref(item);

      return true;
    }

    return false;
  }

  static void
  append_existing_item(WebKitContextMenu* menu, WebKitContextMenuItem* item)
  {
    if (item)
    {
      webkit_context_menu_append(menu, item);
      g_object_unref(item);
    }
  }

  static void
  on_open_in_new_tab(
    GSimpleAction*,
    GVariant* parameter,
    View* view
  )
  {
    const auto uri = parameter
      ? g_variant_get_string(parameter, nullptr)
      : nullptr;

    if (!uri || !*uri || !view)
    {
      return;
    }
    if (const auto window = view->get_main_window())
    {
      window->open_tab(uri, false);
    }
  }

  static void
  on_open_in_split(
    GSimpleAction* action,
    GVariant* parameter,
    View* view
  )
  {
    const auto uri = parameter
      ? g_variant_get_string(parameter, nullptr)
      : nullptr;

    if (!uri || !*uri || !view)
    {
      return;
    }
    if (const auto tab = view->get_tab())
    {
      tab->split(
        std::strstr(g_action_get_name(G_ACTION(action)), "horizontal")
          ? Gtk::Orientation::HORIZONTAL
          : Gtk::Orientation::VERTICAL,
        uri
      );
    }
  }

  static void
  append_open_uri_items(
    WebKitContextMenu* menu,
    View* view,
    const char* uri,
    const char* new_tab_label,
    const char* horizontal_label,
    const char* vertical_label
  )
  {
    const auto action_open_in_new_tab = g_simple_action_new(
      "selain-open-in-new-tab",
      G_VARIANT_TYPE_STRING
    );
    const auto action_open_in_horizontal_split = g_simple_action_new(
      "selain-open-in-horizontal-split",
      G_VARIANT_TYPE_STRING
    );
    const auto action_open_in_vertical_split = g_simple_action_new(
      "selain-open-in-vertical-split",
      G_VARIANT_TYPE_STRING
    );

    g_signal_connect(
      action_open_in_new_tab,
      "activate",
      G_CALLBACK(on_open_in_new_tab),
      static_cast<gpointer>(view)
    );
    g_signal_connect(
      action_open_in_horizontal_split,
      "activate",
      G_CALLBACK(on_open_in_split),
      static_cast<gpointer>(view)
    );
    g_signal_connect(
      action_open_in_vertical_split,
      "activate",
      G_CALLBACK(on_open_in_split),
      static_cast<gpointer>(view)
    );

    webkit_context_menu_append(
      menu,
      webkit_context_menu_item_new_from_gaction(
        G_ACTION(action_open_in_new_tab),
        new_tab_label,
        g_variant_new_string(uri)
      )
    );
    webkit_context_menu_append(
      menu,
      webkit_context_menu_item_new_from_gaction(
        G_ACTION(action_open_in_horizontal_split),
        horizontal_label,
        g_variant_new_string(uri)
      )
    );
    webkit_context_menu_append(
      menu,
      webkit_context_menu_item_new_from_gaction(
        G_ACTION(action_open_in_vertical_split),
        vertical_label,
        g_variant_new_string(uri)
      )
    );

    g_object_unref(action_open_in_new_tab);
    g_object_unref(action_open_in_horizontal_split);
    g_object_unref(action_open_in_vertical_split);
  }

  gboolean
  on_context_menu(
    WebKitWebView* web_view,
    WebKitContextMenu* menu,
    WebKitHitTestResult* result,
    View* view
  )
  {
    const auto is_video = (
      menu_has_stock(
        menu,
        WEBKIT_CONTEXT_MENU_ACTION_COPY_VIDEO_LINK_TO_CLIPBOARD
      ) ||
      menu_has_stock(
        menu,
        WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_VIDEO_TO_DISK
      ) ||
      menu_has_stock(
        menu,
        WEBKIT_CONTEXT_MENU_ACTION_OPEN_VIDEO_IN_NEW_WINDOW
      )
    );
    auto* play_item = find_stock_item(
      menu,
      WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PLAY
    );
    auto* pause_item = find_stock_item(
      menu,
      WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PAUSE
    );
    auto* mute_item = find_stock_item(
      menu,
      WEBKIT_CONTEXT_MENU_ACTION_MEDIA_MUTE
    );
    auto* controls_item = find_stock_item(
      menu,
      WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_CONTROLS
    );
    auto* loop_item = find_stock_item(
      menu,
      WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_LOOP
    );
    auto* fullscreen_item = find_stock_item(
      menu,
      WEBKIT_CONTEXT_MENU_ACTION_ENTER_VIDEO_FULLSCREEN
    );

    webkit_context_menu_remove_all(menu);

    if (webkit_hit_test_result_context_is_editable(result))
    {
      create_editable_menu(menu, web_view);
    }
    else if (webkit_hit_test_result_context_is_image(result))
    {
      create_image_menu(menu, result, view);
    }
    else if (webkit_hit_test_result_context_is_link(result))
    {
      create_link_menu(menu, result, view);
    }
    else if (webkit_hit_test_result_context_is_media(result))
    {
      create_media_menu(
        menu,
        result,
        view,
        is_video,
        play_item,
        pause_item,
        mute_item,
        controls_item,
        loop_item,
        fullscreen_item
      );
      play_item = pause_item = mute_item = nullptr;
      controls_item = loop_item = fullscreen_item = nullptr;
    }
    else if (webkit_hit_test_result_context_is_scrollbar(result))
    {
      create_default_menu(menu, web_view);
    }
    else if (webkit_hit_test_result_context_is_selection(result))
    {
      create_selection_menu(menu, view);
    } else {
      create_default_menu(menu, web_view);
    }

    if (play_item)
    {
      g_object_unref(play_item);
    }
    if (pause_item)
    {
      g_object_unref(pause_item);
    }
    if (mute_item)
    {
      g_object_unref(mute_item);
    }
    if (controls_item)
    {
      g_object_unref(controls_item);
    }
    if (loop_item)
    {
      g_object_unref(loop_item);
    }
    if (fullscreen_item)
    {
      g_object_unref(fullscreen_item);
    }

    return FALSE;
  }

  static Glib::ustring
  build_search_label(
    View* view,
    const std::optional<Glib::ustring>& query = std::nullopt
  )
  {
    Glib::ustring result = "Search the web";

    if (const auto window = view->get_main_window())
    {
      if (const auto setting = setting::get(*window, "search-engine"))
      {
        if (const auto host = utils::url::get_host(*setting))
        {
          result = std::format("Search with {}", host->raw());
        }
      }
    }
    if (query && !query->empty())
    {
      return std::format(
        "{} for \"{}\"",
        result.raw(),
        utils::string::ellipsize(*query, 20).raw()
      );
    }

    return result;
  }

  static void
  on_editing_command(
    GSimpleAction* action,
    GVariant*,
    WebKitWebView* web_view
  )
  {
    if (web_view)
    {
      webkit_web_view_execute_editing_command(
        web_view,
        g_action_get_name(G_ACTION(action))
      );
    }
  }

  static void
  append_editing_command(
    WebKitContextMenu* menu,
    WebKitWebView* web_view,
    const char* command,
    const char* label
  )
  {
    const auto action = g_simple_action_new(command, nullptr);

    g_signal_connect(
      action,
      "activate",
      G_CALLBACK(on_editing_command),
      web_view
    );
    webkit_context_menu_append(
      menu,
      webkit_context_menu_item_new_from_gaction(
        G_ACTION(action),
        label,
        nullptr
      )
    );
    g_object_unref(action);
  }

  static void
  create_editable_menu(WebKitContextMenu* menu, WebKitWebView* web_view)
  {
    append_editing_command(
      menu,
      web_view,
      WEBKIT_EDITING_COMMAND_UNDO,
      "Undo"
    );
    append_editing_command(
      menu,
      web_view,
      WEBKIT_EDITING_COMMAND_REDO,
      "Redo"
    );
    append_separator(menu);
    append_stock(menu, WEBKIT_CONTEXT_MENU_ACTION_CUT, "Cut");
    append_stock(menu, WEBKIT_CONTEXT_MENU_ACTION_COPY, "Copy");
    append_stock(menu, WEBKIT_CONTEXT_MENU_ACTION_PASTE, "Paste");
    append_stock(
      menu,
      WEBKIT_CONTEXT_MENU_ACTION_PASTE_AS_PLAIN_TEXT,
      "Paste as plain text"
    );
    append_stock(menu, WEBKIT_CONTEXT_MENU_ACTION_DELETE, "Delete");
    append_separator(menu);
    append_stock(menu, WEBKIT_CONTEXT_MENU_ACTION_SELECT_ALL, "Select all");
    append_stock(menu, WEBKIT_CONTEXT_MENU_ACTION_INSERT_EMOJI, "Insert emoji");
    append_separator(menu);
    append_inspect(menu);
  }

  static void
  create_image_menu(
    WebKitContextMenu* menu,
    WebKitHitTestResult* result,
    View* view
  )
  {
    const auto uri = webkit_hit_test_result_get_image_uri(result);

    if (uri && *uri)
    {
      append_open_uri_items(
        menu,
        view,
        uri,
        "Open image in new tab",
        "Open image in horizontal split",
        "Open image in vertical split"
      );
      append_separator(menu);
      // TODO: Re-enable this once we have implemented download management.
      #if 0
      append_stock(
        menu,
        WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK,
        "Save image as"
      );
      #endif
      append_stock(
        menu,
        WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD,
        "Copy image"
      );
      append_stock(
        menu,
        WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD,
        "Copy image address"
      );
      append_separator(menu);
    }

    // TODO: Add support for reverse image search at some point.

    append_inspect(menu);
  }

  static void
  on_bookmark_link(
    GSimpleAction*,
    GVariant* parameter,
    View* view
  )
  {
    const gchar* uri = nullptr;
    const gchar* title = nullptr;

    if (!parameter || !view)
    {
      return;
    }

    g_variant_get(parameter, "(&s&s)", &uri, &title);

    if (!uri || !*uri)
    {
      return;
    }
    if (const auto window = view->get_main_window())
    {
      if (const auto manager = window->bookmark_manager())
      {
        const Glib::ustring bookmark_title = (
          title && *title ? title : uri
        );

        manager->add(
          uri,
          bookmark_title,
          {},
          [window](
            bool ok,
            const std::optional<Glib::ustring>& message
          )
          {
            if (!ok)
            {
              window->show_error(
                !message || message->empty()
                  ? "Failed to add bookmark."
                  : *message
              );
              return;
            }
            window->show_info(
              !message || message->empty() ? "Bookmark added." : *message
            );
          }
        );
      }
    }
  }

  static void
  on_search_for_link_title(
    GSimpleAction*,
    GVariant* parameter,
    View* view
  )
  {
    const auto title = parameter
      ? g_variant_get_string(parameter, nullptr)
      : nullptr;

    if (!title || !*title || !view)
    {
      return;
    }
    if (const auto window = view->get_main_window())
    {
      window->open_search_tab(title);
    }
  }

  static void
  create_link_menu(
    WebKitContextMenu* menu,
    WebKitHitTestResult* result,
    View* view
  )
  {
    const auto uri = webkit_hit_test_result_get_link_uri(result);
    auto title = webkit_hit_test_result_get_link_title(result);

    if (!title || !*title)
    {
      title = webkit_hit_test_result_get_link_label(result);
    }

    if (uri && *uri)
    {
      append_open_uri_items(
        menu,
        view,
        uri,
        "Open link in new tab",
        "Open link in horizontal split",
        "Open link in vertical split"
      );
      append_separator(menu);

      const auto action_bookmark = g_simple_action_new(
        "selain-bookmark-link",
        G_VARIANT_TYPE("(ss)")
      );

      g_signal_connect(
        action_bookmark,
        "activate",
        G_CALLBACK(on_bookmark_link),
        static_cast<gpointer>(view)
      );
      webkit_context_menu_append(
        menu,
        webkit_context_menu_item_new_from_gaction(
          G_ACTION(action_bookmark),
          "Bookmark link",
          g_variant_new("(ss)", uri, title && *title ? title : "")
        )
      );
      g_object_unref(action_bookmark);

      // TODO: Re-enable this once we have implemented download management.
      #if 0
      append_stock(
        menu,
        WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK,
        "Save link as"
      );
      #endif
      append_stock(
        menu,
        WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD,
        "Copy link"
      );
      append_separator(menu);
    }

    if (title && *title)
    {
      const auto action_search = g_simple_action_new(
        "selain-search-for-link-title",
        G_VARIANT_TYPE_STRING
      );

      g_signal_connect(
        action_search,
        "activate",
        G_CALLBACK(on_search_for_link_title),
        static_cast<gpointer>(view)
      );
      webkit_context_menu_append(
        menu,
        webkit_context_menu_item_new_from_gaction(
          G_ACTION(action_search),
          build_search_label(view, title).c_str(),
          g_variant_new_string(title)
        )
      );
      append_separator(menu);
      g_object_unref(action_search);
    }

    append_inspect(menu);
  }

  static void
  create_media_menu(
    WebKitContextMenu* menu,
    WebKitHitTestResult* result,
    View* view,
    bool is_video,
    WebKitContextMenuItem* play_item,
    WebKitContextMenuItem* pause_item,
    WebKitContextMenuItem* mute_item,
    WebKitContextMenuItem* controls_item,
    WebKitContextMenuItem* loop_item,
    WebKitContextMenuItem* fullscreen_item
  )
  {
    const auto uri = webkit_hit_test_result_get_media_uri(result);
    const auto open_tab_label = is_video
      ? "Open video in new tab"
      : "Open audio in new tab";
    const auto open_horizontal_label = is_video
      ? "Open video in horizontal split"
      : "Open audio in horizontal split";
    const auto open_vertical_label = is_video
      ? "Open video in vertical split"
      : "Open audio in vertical split";
    bool added_controls = false;

    if (uri && *uri)
    {
      append_open_uri_items(
        menu,
        view,
        uri,
        open_tab_label,
        open_horizontal_label,
        open_vertical_label
      );
      append_separator(menu);
      // TODO: Re-enable this once we have implemented download management.
      #if 0
      append_stock(
        menu,
        is_video
          ? WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_VIDEO_TO_DISK
          : WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_AUDIO_TO_DISK,
        is_video ? "Save video as" : "Save audio as"
      );
      #endif
      append_stock(
        menu,
        is_video
          ? WEBKIT_CONTEXT_MENU_ACTION_COPY_VIDEO_LINK_TO_CLIPBOARD
          : WEBKIT_CONTEXT_MENU_ACTION_COPY_AUDIO_LINK_TO_CLIPBOARD,
        is_video ? "Copy video address" : "Copy audio address"
      );
      append_separator(menu);
    }

    append_existing_item(menu, play_item);
    append_existing_item(menu, pause_item);
    append_existing_item(menu, mute_item);
    append_existing_item(menu, controls_item);
    append_existing_item(menu, loop_item);
    append_existing_item(menu, fullscreen_item);

    added_controls = (
      play_item ||
      pause_item ||
      mute_item ||
      controls_item ||
      loop_item ||
      fullscreen_item
    );
    if (added_controls)
    {
      append_separator(menu);
    }

    append_inspect(menu);
  }

  static void
  on_selection_search(
    GSimpleAction*,
    GVariant*,
    View* view
  )
  {
    view->get_selected_text(
      [view](const std::optional<Glib::ustring>& text)
      {
        const auto window = view->get_main_window();
        Glib::ustring query;

        if (
          text &&
          window &&
          !(query = utils::string::trim(*text)).empty()
        )
        {
          window->open_search_tab(query);
        }
      }
    );
  }

  static void
  create_selection_menu(WebKitContextMenu* menu, View* view)
  {
    const auto action_search = g_simple_action_new("selain-search", nullptr);

    g_signal_connect(
      action_search,
      "activate",
      G_CALLBACK(on_selection_search),
      static_cast<gpointer>(view)
    );

    append_stock(menu, WEBKIT_CONTEXT_MENU_ACTION_COPY, "Copy");
    append_inspect(menu);
    append_separator(menu);
    webkit_context_menu_append(
      menu,
      webkit_context_menu_item_new_from_gaction(
        G_ACTION(action_search),
        build_search_label(view).c_str(),
        nullptr
      )
    );

    g_object_unref(action_search);
  }

  static void
  create_default_menu(WebKitContextMenu* menu, WebKitWebView* web_view)
  {
    append_stock_enabled(
      menu,
      WEBKIT_CONTEXT_MENU_ACTION_GO_BACK,
      "Back",
      webkit_web_view_can_go_back(web_view)
    );
    append_stock_enabled(
      menu,
      WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD,
      "Forward",
      webkit_web_view_can_go_forward(web_view)
    );
    append_stock_enabled(
      menu,
      WEBKIT_CONTEXT_MENU_ACTION_STOP,
      "Stop",
      webkit_web_view_is_loading(web_view)
    );
    append_stock(menu, WEBKIT_CONTEXT_MENU_ACTION_RELOAD, "Refresh");
    append_separator(menu);
    append_inspect(menu);
  }
}
