# TODO: Session restore (persist open tabs on startup)

Reuse the existing closed-tab layout snapshot (`LayoutSnapshot` / `LayoutNode`,
`Tab::capture_layout` / `Tab::restore_layout`) to remember all open tabs across
browser restarts.

**Status:** implemented (phases 1–3)  
**Depends on:** last-closed-tab restore (`X` / `:open-closed-tab`) already in tree

Related code today:

- Snapshot model + capture/restore: `include/selain/ui/tab.hpp`, `src/ui/tab.cpp`
- Session + closed-tab persistence: `include/selain/core/session.hpp`,
  `src/core/session.cpp` (`session.json`, `closed-tabs.json`)
- Last-closed stack: `MainWindow::m_closed_tabs`, `restore_last_closed_tab()`
- Startup / first tab: `src/main.cpp` (`on_command_line`, homepage fallback)
- Persistence precedent: `HistoryManager` → `~/.local/share/selain/history.sqlite`
- Settings: `src/core/setting.cpp`

---

## Goals

1. On exit (and periodically while running), persist every open tab’s split
   layout (URI tree + focused leaf) and which tab was active.
2. On startup with no CLI URLs, restore that session instead of only opening
   the homepage.
3. Keep keyboard-first UX: optional commands to save/clear session; a setting to
   disable restore.
4. Share one serialization of `LayoutSnapshot` with last-closed-tab if useful
   (e.g. durable undo stack later), but v1 can keep last-closed in-memory.

## Non-goals

- Restoring WebKit back/forward lists, scroll position, cookies, or form state
  (cookies already live in the WebKit data dir). Split ratios and per-view zoom
  **are** restored (Phase 3).
- Multi-window sessions (Selain is single main window today).
- Cloud sync / profile manager.

---

## Data model

Extend the in-memory snapshot into a **session**:

```text
Session
  active_tab_index: int   # 0-based notebook page
  tabs: LayoutSnapshot[]  # one per notebook tab, left-to-right
```

Each `LayoutSnapshot` is already:

- binary tree of leaves (URI) and splits (orientation + start/end)
- `active_leaf_index` for focus inside that tab

Serialize to a single JSON file (readable, easy to debug), e.g.:

`~/.local/share/selain/session.json`

Suggested JSON shape:

```json
{
  "version": 1,
  "active_tab": 0,
  "tabs": [
    {
      "active_leaf": 0,
      "root": {
        "type": "split",
        "orientation": "horizontal",
        "start": { "type": "leaf", "uri": "https://example.com/" },
        "end": { "type": "leaf", "uri": "https://example.org/" }
      }
    }
  ]
}
```

Use nlohmann JSON via the existing Inja dependency (`inja/json.hpp`) unless a
lighter hand-rolled writer is preferred for zero coupling.

Skip tabs that have no non-empty URI (same rule as last-closed capture), or
persist them as empty leaves only if we want blank tabs restored—default:
**omit tabs with no URIs**; if the session would be empty, do not write / treat
as no session.

---

## When to save

Write the session file:

1. **Debounced** after structural or navigational changes (e.g. 500ms–1s idle):
   tab open/close, split open/close, URI commit (`load-finished` /
   `record_history_visit`), tab switch (for `active_tab` only).
2. **Synchronously** (or flush pending debounce) in
   `MainWindow::schedule_close` / destructor path so quit does not lose the
   last state.
3. Avoid writing on every scroll/keypress.

Implementation sketch:

- `MainWindow::capture_session() const` → walk `get_tabs()`, call
  `capture_layout()` per tab, record current notebook page.
- `SessionStore::save(session)` / `load()` around the JSON file.
- Hook save triggers from `close_tab`, `open_tab`, tab switch, and view load
  finished (or a single `request_session_save()` that resets a timeout).

---

## When to restore

In `on_command_line` / startup (`src/main.cpp`), after init script:

| Condition | Behavior |
|-----------|----------|
| CLI args open one or more URLs | Open those tabs only; **do not** restore session (CLI wins). Optionally still keep the previous `session.json` on disk until a later save overwrites it. |
| No CLI URLs, setting `session-restore=true`, file present | Restore all tabs from session; focus saved active tab. Skip homepage. |
| No CLI URLs, no usable session | Current behavior: open homepage. |
| `session-restore=false` | Ignore file; homepage / CLI only. |

Restore API:

- `MainWindow::restore_session(const Session&)` — for each snapshot,
  `open_tab()` then `restore_layout()` (same as last-closed); then
  `set_current_tab(active_tab)`.
- Open restored tabs without stealing focus until the end (use `open_tab(uri,
  false)` / empty tab + restore) so the notebook does not thrash.

Empty or corrupt file: log / ignore and fall back to homepage; never crash
startup.

---

## Settings & commands

| Setting | Default | Meaning |
|---------|---------|---------|
| `session-restore` | `true` | Load session on startup when no CLI URLs. |
| `session-save` | `true` | Persist session while running / on quit. |

Optional commands (v1 or follow-up):

- `:session-save` — flush now  
- `:session-clear` — delete `session.json`  
- `:session-restore` — reload from disk into new tabs (or replace)—decide UX
  carefully so it does not duplicate tabs by accident  

Document in `doc/settings.md` / `doc/commands.md`.

---

## Interaction with last-closed (`X`)

| Feature | Scope | Storage |
|---------|-------|---------|
| `X` / `:open-closed-tab` | Last closed tab layout | In-memory (`m_last_closed_tab`) |
| Session restore | All tabs at quit/startup | `session.json` |

Shared pieces to extract when implementing:

- JSON (or other) encode/decode of `LayoutNode` / `LayoutSnapshot`
- Possibly move snapshot types to something like
  `include/selain/core/session.hpp` if main-window/tab headers get too heavy

Do **not** require persisting the last-closed stack for v1.

---

## Privacy / safety

- Session file contains full URIs (including query strings). Same class of data
  as visit history; keep under `~/.local/share/selain/` with normal user file
  modes.
- No passwords in the file (form state not restored).
- Clearing history does **not** have to clear session unless we document that
  coupling; prefer independent `:session-clear`.

---

## Implementation phases

### Phase 1 — Serialize & restore one window worth of tabs

1. Add encode/decode for `LayoutSnapshot` (+ session wrapper).
2. `SessionStore` load/save path under user data dir.
3. `MainWindow::capture_session` / `restore_session`.
4. Save on quit; restore on startup when no CLI URLs.
5. Setting `session-restore` (and save-on-quit always on, or paired
   `session-save`).
6. Docs + manual test checklist.

### Phase 2 — Live autosave

1. Debounced `request_session_save()` on tab/split/navigation/focus changes.
2. Ensure rapid open/close does not corrupt the file (write temp + rename).

### Phase 3 — Polish (done)

1. `:session-save` / `:session-clear` / `:session-restore` (replace all tabs).
2. Persist last-closed stack to `closed-tabs.json` (multi-level `X`, cap 25).
3. Split ratios + per-leaf zoom in `LayoutSnapshot` / session JSON v2.

---

## Test plan (manual)

- [ ] Open two tabs with splits; quit; restart → same URIs, split orientations,
      focused leaf and tab.
- [ ] Quit with only homepage → restart restores or opens homepage cleanly.
- [ ] `selain https://example.com` → opens CLI tab only, does not merge old
      session into the new window on that launch.
- [ ] `:set session-restore=false` → startup ignores `session.json`.
- [ ] Corrupt `session.json` → startup falls back without crash.
- [ ] `X` after close still works and does not fight session file.

---

## Open questions

1. After a CLI-only launch, should the next no-arg launch restore the **pre-CLI**
   session or the CLI session that was autosaved on quit?
   **Proposal:** whatever was last saved on quit wins (CLI session overwrites).
2. Should blank “Untitled” tabs be restored? **Proposal:** no.
3. Multiple `MainWindow` instances later? File format can grow a
   `windows: []` array; v1 writes a single implicit window.
