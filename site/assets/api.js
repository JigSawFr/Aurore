// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// The API reference (/api/, tools/build_api.py): a signature wider than its
// box fades at its right edge while there is more to scroll, as the code
// blocks do (site.js); and the sidebar's theme of the page in view.
(function () {
  for (const code of document.querySelectorAll("code.sig")) {
    const box = code.parentElement;
    const update = () => box.classList.toggle("more", code.scrollLeft + code.clientWidth < code.scrollWidth - 2);
    code.addEventListener("scroll", update, { passive: true });
    if ("ResizeObserver" in window) new ResizeObserver(update).observe(code);
    update();
  }
  // The page's link in the sidebar, in view in its own scroll.
  const current = document.querySelector(".doc-nav a[aria-current='page']");
  const nav = document.querySelector(".doc-nav");
  if (current && nav && nav.scrollHeight > nav.clientHeight) {
    const top = current.offsetTop - nav.offsetTop;
    if (top > nav.clientHeight * 0.6) nav.scrollTop = top - nav.clientHeight / 3;
  }
})();
