# TODO: GreaseMonkey JavaScript APIs

Plan for adding GreaseMonkey-compatible privileged APIs on top of the existing
userscript loader (`~/.config/selain/scripts/`, metadata + WebKit injection).

**Status:** planning
**Compatibility target:** GreaseMonkey 4 `GM.*` (Promise-based), with optional
legacy `GM_*` aliases where cheap. Prefer GM4 semantics over Tampermonkey /
Violentmonkey extensions unless noted.

Related code today:

- Metadata: `src/userscript/metadata.cpp`, `include/selain/userscript/metadata.hpp`
- Injection: `src/webkit/userscript.cpp`
- Docs: [userscripts.md](userscripts.md)
- Precedent for JS ↔ C++ with reply: `src/webkit/hint-context.cpp`
  (`register_script_message_handler_with_reply` + named script world)

Current limitation (from [userscripts.md](userscripts.md)): metadata + injection
only — no `GM_*` / `GM.*`, `@grant`, `@require`, `@resource`, or `unsafeWindow`.

---

## Goals

1. Scripts that declare `@grant` get a sandboxed environment with the granted
   APIs (and always `GM.info` / `unsafeWindow` when sandboxed).
2. Scripts with `@grant none` or no `@grant` keep today’s behavior: inject into
   the page world with no privileged bridge (compatible with most simple scripts).
3. Storage and network privileges are per-script and durable across reloads.
4. Keyboard-first UX stays intact (menu commands, notifications, open-in-tab map
   to existing Selain UI / commands where possible).

## Non-goals (for the first several phases)

- Full Tampermonkey / Violentmonkey surface (`GM_cookie`, `GM_download`,
  `GM_addElement`, `@antifeature`, cloud sync, remote script install UI).
- Perfect sandbox hardening against a hostile page (best-effort isolation via
  WebKit content worlds is enough for v1).
- Auto-update of scripts from `@updateURL` / `@downloadURL`.

---

## Architecture sketch

```
┌─ page (main world) ──────────────────────────────────────┐
│  unsafeWindow === window                                 │
└──────────────────────────────────────────────────────────┘
        ▲ DOM / same document
┌─ userscript world (webkit_user_script_new_for_world) ────┐
│  wrapper: GM / GM_* / unsafeWindow                       │
│  window.webkit.messageHandlers.selainGM.postMessage(...) │
└───────────────────────────┬──────────────────────────────┘
                            │ script-message-with-reply
┌─ UI process (C++) ───────────────────────────────────────┐
│  selain::webkit::userscript::GmBridge                    │
│    ├─ grant check by script id                           │
│    ├─ value store (SQLite / JSON under config dir)        │
│    ├─ Soup / WebKit fetch for GM.xmlHttpRequest          │
│    └─ UI hooks: tab open, clipboard, notification, menu  │
└──────────────────────────────────────────────────────────┘
```

### Injection model

| `@grant` | World | Privileged APIs |
|----------|--------|-----------------|
| absent or `none` | main (`webkit_user_script_new`) | none (current behavior) |
| one or more APIs | isolated world + bootstrap wrapper | only granted names (+ always `GM.info`, `unsafeWindow`) |

If both `none` and other grants appear, GreaseMonkey rule: **`none` wins**.

### Bootstrap wrapper (per script)

At install time, wrap the script body roughly as:

1. Assign a stable `scriptId` (hash of path + `@namespace` + `@name`, or UUID
   persisted beside the script).
2. Inject a small prelude that builds `GM` / `GM_*` stubs which call
   `webkit.messageHandlers.selainGM` with `{ scriptId, method, args }` and
   await the reply (Promise for `GM.*`; sync-style wrappers for `GM_*` only if
   the reply path can be made synchronous — otherwise ship async `GM.*` first
   and document that legacy sync `GM_*` is incomplete).
3. Expose `GM.info` from metadata baked into the prelude (no round-trip needed
   for static fields).
4. For sandboxed scripts, set `unsafeWindow` to the page `window` if WebKit
   allows cross-world access; otherwise document limitation and provide
   `wrappedJSObject`-style helpers later.

Reuse the hint-mode pattern: named world +
`webkit_user_content_manager_register_script_message_handler_with_reply`.

### Storage layout

Suggested path:

```
~/.config/selain/scripts-data/<scriptId>/values.json
```

or one SQLite DB (`userscripts.sqlite`) keyed by `(script_id, key)` — prefer
SQLite if history already uses it and we want atomic list/delete.

Values must be JSON-serializable (GM contract).

### Security notes

- Enforce `@grant` on the C++ side; never trust the page to request arbitrary
  methods for another `scriptId`.
- For `GM.xmlHttpRequest`, honor `@connect` (phase 3+); until then either deny
  cross-origin or require explicit `@connect *` / host list once parsed.
- Do not expose raw filesystem or shell APIs through the bridge.

---

## Phase 0 — Metadata & plumbing

Foundation before any JS API works.

- [ ] Extend `ParsedScript` with: `namespace`, `description`, `version`,
      `author`, `grants`, `requires`, `resources`, `connect`, `homepage`,
      `support_url`, raw `script_meta_str`.
- [ ] Parse `@grant`, `@namespace`, `@description`, `@version`, `@author`,
      `@require`, `@resource`, `@connect` (store even if unused).
- [ ] Implement GreaseMonkey grant resolution: default `none`; `none` overrides
      other grants; normalize `GM_getValue` ↔ `GM.getValue` for matching.
- [ ] Assign stable `script_id` per installable script; keep mapping
      path → id across reload.
- [ ] Register `selainGM` script-message-with-reply handler once on the shared
      `WebKitUserContentManager` (same lifetime as hint handler).
- [ ] Split install path: `grant none` → current `webkit_user_script_new`;
      granted → `webkit_user_script_new_for_world` + prelude.
- [ ] Unit tests for new metadata fields and grant resolution
      (`tests/userscript_metadata_test.cpp`).

**Exit criteria:** scripts still inject as today; granted scripts can call a
no-op bridge method and get a JSON reply.

---

## Phase 1 — `GM.info` and value store

Highest value for real-world scripts; mostly local, no network.

### APIs

| API | Notes |
|-----|--------|
| `GM.info` / `GM_info` | Always available when sandboxed; bake static fields into prelude |
| `GM.getValue` | Promise; default value if missing |
| `GM.setValue` | Promise; JSON serialize |
| `GM.deleteValue` | Promise |
| `GM.listValues` | Promise → `string[]` |

### Tasks

- [ ] Design `GmValueStore` (get/set/delete/list) with per-`script_id` isolation.
- [ ] Persist under config dir; flush safely on set/delete.
- [ ] Bridge methods + grant checks for value APIs.
- [ ] Populate `GM.info` (`scriptHandler: "Selain"`, `version` from
      `SELAIN_VERSION`, `script` meta object, `scriptMetaStr`, `uuid` /
      `script_id`).
- [ ] Docs: update [userscripts.md](userscripts.md) with grant + storage
      behavior and example.
- [ ] Tests: store unit tests; optional integration fixture script.

**Exit criteria:** a `@grant GM.getValue` / `GM.setValue` counter script works
across page reloads and `:userscripts-reload`.

---

## Phase 2 — Browser / UI APIs

Map to existing Selain facilities.

| API | Backend hook |
|-----|----------------|
| `GM.openInTab` | `MainWindow::open_tab` (options: active / insert — document subset) |
| `GM.setClipboard` | `Gtk::Clipboard` / window clipboard (same as yank) |
| `GM.notification` | `MainWindow::show_info` / notification UI; optional click callback later |
| `GM.registerMenuCommand` | Command palette or `:userscript-menu` / status-bar listing; must be keyboard-reachable |

### Tasks

- [ ] Bridge → UI process callbacks with owning `WebKitWebView` / `Tab` lookup
      (message must identify which view sent it).
- [ ] `GM.openInTab(url, open_in_background?)` — define supported option bag.
- [ ] `GM.setClipboard(text)`.
- [ ] `GM.notification(text)` or `{ text, title, image, onclick, ondone }` —
      start with text (+ title); defer rich image/onclick if awkward on GTK.
- [ ] `GM.registerMenuCommand(name, callback, accessKey?)` — store commands
      per script; invoke callback via JS evaluation in the script world or a
      long-lived reply channel; expose in UI with a keyboard command.
- [ ] Unregister / clear menu commands on `:userscripts-reload` and script
      disable.

**Exit criteria:** script can open a tab, copy text, show a notification, and
register a command runnable without the mouse.

---

## Phase 3 — `GM.xmlHttpRequest` and `@connect`

| API | Notes |
|-----|--------|
| `GM.xmlHttpRequest` | Cross-origin XHR-like; async; callbacks and/or Promise |

### Tasks

- [ ] Parse and enforce `@connect` host patterns (GM semantics: hostname,
      `.domain`, `*`, `self`, `localhost`).
- [ ] Implement request via libsoup / WebKit network session (share cookie jar
      policy — decide whether to send page cookies; document choice).
- [ ] Support subset first: `method`, `url`, `headers`, `data`, `timeout`,
      `responseType` (`text` / `json` / `arraybuffer`), `onload` / `onerror` /
      `ontimeout` / `onabort`, `responseHeaders`, `status`, `finalUrl`.
- [ ] Defer or explicitly reject: sync mode, upload progress, binary Blob
      edge cases, streaming.
- [ ] Grant name: accept `GM.xmlHttpRequest` and `GM_xmlhttpRequest`.
- [ ] Tests with a local HTTP fixture or mocked soup session.

**Exit criteria:** mashup script can `GET`/`POST` a non-origin URL allowed by
`@connect` and read `responseText`.

---

## Phase 4 — Resources, `@require`, sandbox polish

| API / header | Notes |
|--------------|--------|
| `@resource` + `GM.getResourceUrl` | Fetch/cache at install; serve via custom scheme or data URL |
| `@require` | Concatenate/preload dependencies before script body |
| `unsafeWindow` | Document real WebKit world capabilities; improve if possible |
| Legacy `GM_*` aliases | Sync-looking wrappers where feasible; else thin Promise wrappers + docs |

### Tasks

- [ ] Download/cache `@require` / `@resource` URLs into
      `~/.config/selain/scripts-data/<id>/` on load/reload; fail soft with
      warnings.
- [ ] `GM.getResourceUrl(name)` → blob/data/custom-scheme URL.
- [ ] Optional later: `GM.getResourceText` (common in TM/VM; not in core GM4
      list — mark as extension if added).
- [ ] Order: requires → prelude → user script.
- [ ] Decide on `document-idle` (today mapped to document-end); optionally
      emulate idle with `requestIdleCallback` / `DOMContentLoaded`+timeout
      inside the wrapper.

**Exit criteria:** script with `@require` + `@resource` image and
`GM.getResourceUrl` works offline after first successful fetch.

---

## Phase 5 — Quality, UX, compatibility

- [ ] `:userscripts` or status listing: name, grants, id, load errors.
- [ ] Clearer reload diagnostics (failed grants, failed requires, bridge
      errors).
- [ ] Compatibility matrix in docs: GM4 yes / TM extras no (or partial).
- [ ] Sample scripts under `examples/userscripts/` (counter, XHR, openInTab).
- [ ] Stress: many scripts, large value stores, rapid reload.
- [ ] Privacy: document where data lives; optional `:userscripts-clear-data`.

---

## API checklist (implementation tracking)

Use this as a living checklist when coding. Mark when bridge + grants + docs
are done.

### GreaseMonkey 4 core

- [ ] `GM.info`
- [ ] `GM.getValue` / `GM.setValue` / `GM.deleteValue` / `GM.listValues`
- [ ] `GM.xmlHttpRequest`
- [ ] `GM.getResourceUrl`
- [ ] `GM.notification`
- [ ] `GM.openInTab`
- [ ] `GM.registerMenuCommand`
- [ ] `GM.setClipboard`
- [ ] `unsafeWindow`

### Legacy aliases (optional / best-effort)

- [ ] `GM_info`
- [ ] `GM_getValue` / `GM_setValue` / `GM_deleteValue` / `GM_listValues`
- [ ] `GM_xmlhttpRequest`
- [ ] `GM_getResourceURL`
- [ ] `GM_notification` / `GM_openInTab` / `GM_registerMenuCommand` /
      `GM_setClipboard`

### Explicitly deferred (track but do not block GM core)

- [ ] `GM_download` / `GM.download`
- [ ] `GM_addStyle` / `GM_addElement` (often polyfillable in-page)
- [ ] `GM_cookie`
- [ ] `GM_webRequest`
- [ ] Script install / update UI from URL
- [ ] `@unwrap`, `@inject-into`, full regex `@include`

---

## Suggested file layout

```
include/selain/userscript/metadata.hpp   # extend ParsedScript
include/selain/userscript/gm-store.hpp   # value persistence
include/selain/webkit/gm-bridge.hpp      # message handler + dispatch
src/userscript/gm-store.cpp
src/webkit/gm-bridge.cpp
src/webkit/userscript.cpp                # wrap + world selection
resources/js/gm-prelude.js               # or cmrc / inja template
tests/userscript_metadata_test.cpp
tests/gm_store_test.cpp
doc/userscripts.md                       # user-facing updates
todo/gm-api.md                           # this plan
```

---

## Decision log

Record choices here as implementation proceeds.

| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-03-25 | Target GM4 `GM.*` first | Matches modern GreaseMonkey; Promises fit message-with-reply |
| 2026-03-25 | `@grant none` / missing `@grant` → page world, no bridge | Preserves current scripts; matches GM default |
| 2026-03-25 | Reuse hint-style script-message-with-reply | Already proven in-tree for isolated worlds |

---

## References

- [Greasemonkey Manual: API](https://wiki.greasespot.net/Greasemonkey_Manual:API)
- [Greasespot `@grant`](https://wiki.greasespot.net/@grant)
- [GM.xmlHttpRequest](https://wiki.greasespot.net/GM.xmlHttpRequest)
- [Greasemonkey 4 for script authors](https://www.greasespot.net/2017/09/greasemonkey-4-for-script-authors.html)
- Violentmonkey GM API (compatibility reference only):
  https://violentmonkey.github.io/api/gm/
- WebKitGTK: `webkit_user_script_new_for_world`,
  `webkit_user_content_manager_register_script_message_handler_with_reply`
