# Commands

Selain currently supports following commands:

|Command       |Shortcut|                                       |
|--------------|--------|---------------------------------------|
|`:back`       |`:b`    |Goes back in browsing history.         |
|`:bind`       |        |Add keyboard binding.                  |
|`:command`    |`:c`    |Switches to command mode.              |
|`:forward`    |`:f`    |Goes forward in browsing history.      |
|`:hint`       |`:h`    |Switches to hint mode.                 |
|`:hint-tab`   |`:ht`   |Switches to open new tab hint mode.    |
|`:insert`     |`:i`    |Switches to insert mode.               |
|`:inspector`  |`:insp` |Toggles developer tools.               |
|`:js`         |        |Runs JavaScript in the current tab.    |
|`:only`       |`:on`   |Closes all splits except the current.  |
|`:open`       |`:o`    |Opens URI given as argument.           |
|`:open-tab`   |`:ot`   |Opens URI given as argument in new tab.|
|`:paste`      |`:p`    |Opens URI from clipboard.              |
|`:paste-tab`  |`:pt`   |Opens URI from clipboard in new tab.   |
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
|`:tabnext`    |`:tn`   |Switches to next tab.                  |
|`:tabprev`    |`:tp`   |Switches to previous tab.              |
|`:unbind`     |        |Remove keyboard binding.               |
|`:vsplit`     |`:vs`   |Splits the current view vertically.    |
|`:yank`       |`:y`    |Copies current URI to clipboard.       |

`:js` takes a JavaScript expression or statement as argument and shows the
result (or an error) as a notification. Example: `:js document.title`.

`:split` and `:vsplit` accept an optional URI argument. Without one, the new
split loads the same URI as the current view.
