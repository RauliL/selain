# Settings

View or change a setting with `:set` (shortcut `:se`):

```
:set homepage=https://example.com
:set enable-javascript=false
:set default-font-size=16
:set user-agent=Mozilla/5.0
:set hardware-acceleration-policy=never
```

Omit the value to print the current one (`:set enable-javascript`).

Boolean settings accept `true`/`false` or `1`/`0`. Integer settings accept a
decimal number. String settings accept any text after `=`. Enumeration settings
accept one of the values listed for that setting.

## Application

| Setting    | Description                                       |
|------------|---------------------------------------------------|
| `homepage` | URL that is opened when the browser starts.       |

## WebKit (boolean)

These map to [`WebKitSettings`](https://webkitgtk.org/reference/webkit2gtk/stable/class.Settings.html)
properties and apply to all views in the window.

| Setting                                      | Description |
|----------------------------------------------|-------------|
| `allow-file-access-from-file-urls`           | Allow cross-origin requests to other `file:` resources from a page loaded via a file URI (e.g. XHR to a local file). |
| `allow-modal-dialogs`                        | Allow JavaScript to create and run modal dialogs (`window.showModalDialog`). |
| `allow-top-navigation-to-data-urls`          | Allow the top frame to navigate to `data:` URLs. Disabled by default because of phishing risk. |
| `allow-universal-access-from-file-urls`      | Allow JavaScript in a `file:` page to access content from any origin (e.g. local storage). |
| `auto-load-images`                           | Automatically load images. Useful to disable on constrained networks. |
| `disable-web-security`                       | Disable the same-origin policy. Unsafe; only for special environments. |
| `draw-compositing-indicators`                | Draw compositing borders and repaint counters for GPU debugging. |
| `enable-2d-canvas-acceleration`              | Prefer hardware acceleration for 2D canvas when possible. |
| `enable-back-forward-navigation-gestures`    | Enable horizontal swipe gestures for back/forward navigation. |
| `enable-caret-browsing`                      | Enable accessibility-enhanced keyboard navigation (caret browsing). |
| `enable-developer-extras`                    | Enable developer tools such as the Web Inspector. |
| `enable-encrypted-media`                     | Enable the Encrypted Media API for playing encrypted media. |
| `enable-fullscreen`                          | Enable the JavaScript Fullscreen API. |
| `enable-html5-database`                      | Enable HTML5 client-side database support (IndexedDB). |
| `enable-html5-local-storage`                 | Enable HTML5 local storage. |
| `enable-javascript`                          | Allow JavaScript to execute within a page. |
| `enable-javascript-markup`                   | Allow JavaScript-related elements and attributes in the document; when disabled they are stripped during parsing. |
| `enable-media`                               | Enable media playback (`<audio>`, `<video>`, `<track>`). |
| `enable-media-capabilities`                  | Enable the Media Capabilities API for format/device capability queries. |
| `enable-media-stream`                        | Enable MediaStream / `getUserMedia` access to audio and video capture devices. |
| `enable-mediasource`                         | Enable Media Source Extensions for script-generated media streams. |
| `enable-mock-capture-devices`                | Use fake microphone and camera devices as MediaStream sources. |
| `enable-page-cache`                          | Enable the back-forward page cache (faster history navigation). |
| `enable-resizable-text-areas`                | Allow text areas to be resized. |
| `enable-site-specific-quirks`                | Enable site-specific compatibility workarounds. |
| `enable-smooth-scrolling`                    | Enable smooth scrolling. |
| `enable-spatial-navigation`                  | Navigate focusable elements with the arrow keys. |
| `enable-tabs-to-links`                       | Cycle focus through page elements with Tab (inserts a tab character in editable fields). |
| `enable-webaudio`                            | Enable the Web Audio API. |
| `enable-webgl`                               | Enable WebGL. |
| `enable-webrtc`                              | Enable WebRTC (also implies `enable-media-stream`). |
| `enable-write-console-messages-to-stdout`    | Write `console.log` and related messages to stdout. |
| `javascript-can-access-clipboard`            | Allow JavaScript cut/copy/paste via `document.execCommand()`. |
| `javascript-can-open-windows-automatically`  | Allow JavaScript to open popup windows without user intervention. |
| `media-playback-allows-inline`               | Allow inline media playback; when false, playback is always fullscreen. |
| `media-playback-requires-user-gesture`       | Require a user gesture to start media playback or load media. |
| `print-backgrounds`                         | Draw background images when printing. |
| `zoom-text-only`                             | Apply zoom only to text (and text-containing form controls), not all content. |

## WebKit (integer)

| Setting                       | Description |
|-------------------------------|-------------|
| `default-font-size`           | Default font size in pixels when none is specified. |
| `default-monospace-font-size` | Default monospace font size in pixels when none is specified. |
| `minimum-font-size`           | Absolute minimum font size in pixels; non-zero values may break layouts. |

## WebKit (string)

| Setting                                           | Description |
|---------------------------------------------------|-------------|
| `cursive-font-family`                             | Default font family for cursive content. |
| `default-charset`                                 | Default text charset when content does not specify one. |
| `default-font-family`                             | Default font family when content does not specify a font. |
| `fantasy-font-family`                             | Default font family for fantasy content. |
| `math-font-family`                                | Default font family for math content. |
| `media-content-types-requiring-hardware-support`  | Semicolon-separated media content types that require hardware support. |
| `monospace-font-family`                           | Default font family for monospace content. |
| `pictograph-font-family`                          | Default font family for pictograph content. |
| `sans-serif-font-family`                          | Default font family for sans-serif content. |
| `serif-font-family`                               | Default font family for serif content. |
| `user-agent`                                      | User-agent string. Empty value reverts to the standard user agent. Unusual strings may break sites. |
| `webrtc-udp-ports-range`                          | WebRTC UDP port range as `min:max` (e.g. `20000:30000`). Empty means no hint to the OS. |

## WebKit (enumeration)

| Setting                          | Values           | Description |
|----------------------------------|------------------|-------------|
| `hardware-acceleration-policy`   | `always`, `never` | Whether hardware acceleration is always enabled or always disabled, regardless of what the page requests. |
