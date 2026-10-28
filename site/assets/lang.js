// The language at a first visit: the English home page (the root, marked
// data-first-visit) sends a visitor whose browser prefers French to /fr/,
// unless a language was chosen before. Choosing one in the header keeps it.
// Loaded in <head>, before the page is drawn, so the move shows nothing.
(function () {
  const KEY = "aurore-lang";
  const SITE = ["en", "fr"]; // the languages the site has; the first at the root
  let chosen = null;
  try { chosen = localStorage.getItem(KEY); } catch (e) { /* storage refused: no memory */ }

  const root = document.documentElement;
  if (root.hasAttribute("data-first-visit") && !chosen) {
    const wanted = (navigator.languages || [navigator.language || ""])
      .map((l) => String(l).toLowerCase().split("-")[0])
      .find((l) => SITE.includes(l));
    if (wanted && wanted !== SITE[0]) {
      location.replace(`${wanted}/${location.hash}`);
      return;
    }
  }

  document.addEventListener("click", (event) => {
    const link = event.target.closest && event.target.closest("a[hreflang]");
    if (!link) return;
    try { localStorage.setItem(KEY, link.getAttribute("hreflang")); } catch (e) { /* idem */ }
  });
})();
