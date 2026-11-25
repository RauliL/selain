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

  function scrollTarget() {
    const viewer = document.getElementById("viewerContainer");

    if (viewer && viewer.scrollHeight > viewer.clientHeight) {
      return viewer;
    }

    let best = null;
    let bestScore = 0;

    for (const el of document.querySelectorAll("body *")) {
      const style = getComputedStyle(el);

      if (
        (style.overflowY === "auto" || style.overflowY === "scroll") &&
        el.scrollHeight > el.clientHeight + 1
      ) {
        const score = el.scrollHeight * el.clientWidth;

        if (score > bestScore) {
          best = el;
          bestScore = score;
        }
      }
    }

    return best || document.scrollingElement || document.documentElement;
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
