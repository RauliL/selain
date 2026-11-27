(() => {
  const elements = document.querySelectorAll(
    'input:not([type="hidden"]),textarea',
  );

  for (const el of elements) {
    const type = (el.type || "").toLowerCase();

    if (
      el.disabled ||
      el.readOnly ||
      type === "button" ||
      type === "submit" ||
      type === "reset" ||
      type === "image"
    )
      continue;

    const style = getComputedStyle(el);

    if (
      style.display === "none" ||
      style.visibility === "hidden" ||
      el.getBoundingClientRect().width === 0 ||
      el.getBoundingClientRect().height === 0
    )
      continue;

    el.focus();
    el.scrollIntoView({
      behavior: "smooth",
      block: "nearest",
      inline: "nearest",
    });

    return true;
  }

  return false;
})();
