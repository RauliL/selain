/**
 * Injected into every frame (including WebKit's cross-origin PDF.js iframe).
 * Listens for { type: 'selain-scroll', action } messages so the UI can scroll
 * both normal pages and PDF.js viewerContainer from the main-frame script.
 */
(function () {
  if (window.__selainScrollInstalled) {
    return;
  }
  window.__selainScrollInstalled = true;

  function isVerticallyScrollable(el) {
    return !!el && el.scrollHeight > el.clientHeight + 1;
  }

  function isOverflowScrollable(el) {
    if (!isVerticallyScrollable(el)) {
      return false;
    }

    const style = getComputedStyle(el);
    const overflowY = style.overflowY;

    return overflowY === "auto" || overflowY === "scroll" || overflowY === "overlay";
  }

  function scrollTarget() {
    // PDF.js (WebKit viewer) scrolls #viewerContainer, not the document.
    const viewer = document.getElementById("viewerContainer");

    if (isVerticallyScrollable(viewer)) {
      return viewer;
    }

    // Prefer the document whenever it can scroll. Nested overflow panels
    // (TOCs, sidebars, menus) are common on long articles and must not steal
    // vim-style page scrolling the way window.scrollBy used to behave.
    const scrolling = document.scrollingElement || document.documentElement;

    if (isVerticallyScrollable(scrolling)) {
      return scrolling;
    }

    // Document is locked (typical SPA / overflow:hidden body): pick the
    // largest nested scrollport instead.
    let best = null;
    let bestScore = 0;

    for (const el of document.querySelectorAll("body *")) {
      if (!isOverflowScrollable(el)) {
        continue;
      }

      const score = el.clientHeight * el.clientWidth;

      if (score > bestScore) {
        best = el;
        bestScore = score;
      }
    }

    return best || scrolling;
  }

  function scroll(action) {
    const el = scrollTarget();

    if (!el) {
      return;
    }

    const page = (el.clientHeight || window.innerHeight) / 2;

    switch (action) {
      case "top":
        el.scrollTop = 0;
        break;
      case "bottom":
        el.scrollTop = el.scrollHeight;
        break;
      case "up":
        el.scrollTop -= 100;
        break;
      case "down":
        el.scrollTop += 100;
        break;
      case "left":
        el.scrollLeft -= 100;
        break;
      case "right":
        el.scrollLeft += 100;
        break;
      case "page-up":
        el.scrollTop -= page;
        break;
      case "page-down":
        el.scrollTop += page;
        break;
    }
  }

  window.addEventListener("message", function (event) {
    const data = event.data;

    if (
      !data ||
      data.type !== "selain-scroll" ||
      typeof data.action !== "string"
    ) {
      return;
    }
    scroll(data.action);
  });
})();
