# Commands

Selain currently supports following commands:

|Command       |Shortcut|                                       |
|--------------|--------|---------------------------------------|
|`:back`       |`:b`    |Goes back in browsing history.         |
|`:bind`       |        |Add keyboard binding.                  |
|`:command`    |`:c`    |Switches to command mode.              |
|`:forward`    |`:f`    |Goes forward in browsing history.      |
|`:go-root`    |`:gr`   |Goes up to the root of the URL hierarchy.|
|`:go-up`      |`:gu`   |Goes up one level in the URL hierarchy.|
|`:hint`       |`:h`    |Switches to hint mode.                 |
|`:hint-tab`   |`:ht`   |Switches to open new tab hint mode.    |
|`:insert`     |`:i`    |Switches to insert mode.               |
|`:insert-input`|`:ii`  |Focuses first input/textarea; insert mode.|
|`:inspector`  |`:insp` |Toggles developer tools.               |
|`:js`         |`:j`    |Runs JavaScript in the current tab.    |
|`:only`       |`:on`   |Closes all splits except the current.  |
|`:open`       |`:o`    |Opens URI, domain, or search query.    |
|`:open-tab`   |`:ot`   |Same as `:open`, in a new tab.         |
|`:paste`      |`:p`    |Opens clipboard text as URI/search.    |
|`:paste-tab`  |`:pt`   |Same as `:paste`, in a new tab.        |
|`:quit`       |`:q`    |Closes current split, or tab if alone. |
|`:qall`       |`:qa`   |Closes all tabs.                       |
|`:reload`     |`:r`    |Reloads page.                          |
|`:reload!`    |`:r!`   |Reloads page bypassing the cache.      |
|`:scroll`     |`:sc`   |Scrolls the current tab.               |
|`:search-next`|`:sen`  |Goes to next search result.            |
|`:search-prev`|`:sep`  |Goes to previous search result.        |
|`:set`        |`:se`   |Views/changes an setting.              |
|`:split`      |`:sp`   |Splits the current view horizontally.  |
|`:split-focus`|`:spf`  |Changes focus of splitted view.        |
|`:stop`       |`:s`    |Stops page from loading content.       |
|`:tab`        |`:t`    |Switches to tab given as 1-based index.|
|`:tab-next`   |`:tn`   |Switches to next tab.                  |
|`:tab-prev`   |`:tp`   |Switches to previous tab.              |
|`:unbind`     |        |Remove keyboard binding.               |
|`:vsplit`     |`:vs`   |Splits the current view vertically.    |
|`:yank`       |`:y`    |Copies current URI to clipboard.       |
|`:zoom`       |`:z`    |Changes zoom level of the current view.|

`:js` takes a JavaScript expression or statement as argument and shows the
result (or an error) as a notification. Example: `:js document.title`.

`:split` and `:vsplit` accept an optional URI argument. Without one, the new
split loads the same URI as the current view.

`:zoom` sets the page zoom of the current view. Without an argument it shows
the current level. Absolute values use a percentage (`:zoom 120%`); relative
adjustments use a signed number of percentage points (`:zoom +20`,
`:zoom -5`).
