// Aurore's website (site/README.md). The aurora behind the home page's header
// and hero (canvas#sky), Boréale's own (boreale/site/assets/site.js): three
// soft curtains under a few stars, moving only while the page is visible and
// the visitor has not asked for less motion. Without script, site.css's
// gradient stands in.
(function () {
  const canvas = document.getElementById("sky");
  if (!canvas || !canvas.getContext) return;
  const ctx = canvas.getContext("2d");
  // The curtains are drawn here at a quarter of the size, then laid on the
  // sky blurred and scaled up in one go: blurring each strip would be slow.
  const glow = document.createElement("canvas");
  const gtx = glow.getContext("2d");
  const still = window.matchMedia("(prefers-reduced-motion: reduce)");
  const curtains = [
    { rgb: [0, 255, 204], y: 0.34, amp: 0.08, freq: 1.2, speed: 0.00012, height: 0.42, alpha: 0.34 },
    { rgb: [51, 187, 255], y: 0.26, amp: 0.06, freq: 1.7, speed: -0.00009, height: 0.34, alpha: 0.26 },
    { rgb: [138, 92, 255], y: 0.18, amp: 0.05, freq: 0.9, speed: 0.00007, height: 0.3, alpha: 0.26 },
  ];
  let w = 0, h = 0, stars = [];

  function size() {
    const ratio = Math.min(window.devicePixelRatio || 1, 2);
    w = canvas.clientWidth; h = canvas.clientHeight;
    canvas.width = Math.max(1, Math.round(w * ratio));
    canvas.height = Math.max(1, Math.round(h * ratio));
    ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
    glow.width = Math.max(1, Math.round(w / 4));
    glow.height = Math.max(1, Math.round(h / 4));
    let seed = 7; // the same stars at each load
    const random = () => (seed = (seed * 16807) % 2147483647) / 2147483647;
    stars = Array.from({ length: Math.round((w * h) / 2600) }, () => ({
      x: random() * w, y: random() * h * 0.75, r: random() * 1.1 + 0.2, a: random() * 0.6 + 0.2,
    }));
  }

  function draw(t) {
    const gw = glow.width, gh = glow.height;
    gtx.globalCompositeOperation = "source-over";
    gtx.clearRect(0, 0, gw, gh);
    gtx.globalCompositeOperation = "lighter";
    const step = Math.max(2, gw / 120);
    for (const c of curtains) {
      const [r, g, b] = c.rgb;
      const phase = t * c.speed;
      for (let x = -step; x < gw + step; x += step) {
        const u = x / gw;
        const base = gh * (c.y + c.amp * Math.sin((u * c.freq + phase) * 2 * Math.PI) + 0.03 * Math.sin(u * 17 + phase * 20));
        const length = gh * c.height * (0.6 + 0.4 * Math.sin(u * 9 + phase * 11));
        const flicker = 0.65 + 0.35 * Math.sin(u * 31 + phase * 40);
        const fill = gtx.createLinearGradient(0, base - length, 0, base);
        fill.addColorStop(0, `rgba(${r}, ${g}, ${b}, 0)`);
        fill.addColorStop(0.75, `rgba(${r}, ${g}, ${b}, ${c.alpha * flicker * 0.6})`);
        fill.addColorStop(1, `rgba(${r}, ${g}, ${b}, ${c.alpha * flicker})`);
        gtx.fillStyle = fill;
        gtx.fillRect(x, base - length, step + 1, length);
      }
    }

    ctx.fillStyle = "#0b1424";
    ctx.fillRect(0, 0, w, h);
    ctx.filter = "blur(8px)"; // where the browser can; scaling up softens it anyway
    ctx.drawImage(glow, 0, 0, w, h);
    ctx.filter = "none";
    for (const s of stars) {
      ctx.fillStyle = `rgba(232, 238, 247, ${s.a})`;
      ctx.beginPath(); ctx.arc(s.x, s.y, s.r, 0, 2 * Math.PI); ctx.fill();
    }
  }

  let frame = 0;
  function loop(t) { draw(t); frame = requestAnimationFrame(loop); }
  function start() {
    cancelAnimationFrame(frame);
    if (still.matches || document.hidden) draw(0);
    else frame = requestAnimationFrame(loop);
  }

  size();
  start();
  window.addEventListener("resize", () => { size(); draw(performance.now()); });
  document.addEventListener("visibilitychange", start);
  still.addEventListener("change", start);
})();

// The screenshots open larger over the page: the arrows (or a swipe) go
// from one to the next, Esc or the close button (or a swipe down) closes.
(function () {
  const box = document.querySelector("dialog.lightbox");
  const links = Array.from(document.querySelectorAll("a[data-caption]"));
  if (!box || !box.showModal || !links.length) return;
  const img = box.querySelector("img");
  const caption = box.querySelector("figcaption");
  let at = 0;
  function show(i) {
    at = (i + links.length) % links.length;
    img.src = links[at].href;
    img.alt = links[at].querySelector("img") ? links[at].querySelector("img").alt : "";
    caption.textContent = links[at].dataset.caption;
  }
  links.forEach((link, i) => link.addEventListener("click", (event) => {
    if (event.metaKey || event.ctrlKey || event.shiftKey) return;
    event.preventDefault();
    show(i);
    box.showModal();
  }));
  box.querySelector(".lightbox-close").addEventListener("click", () => box.close());
  box.querySelector(".lightbox-prev").addEventListener("click", () => show(at - 1));
  box.querySelector(".lightbox-next").addEventListener("click", () => show(at + 1));
  box.addEventListener("click", (event) => { if (event.target === box || event.target.tagName === "FIGURE") box.close(); });
  box.addEventListener("keydown", (event) => {
    if (event.key === "ArrowLeft") show(at - 1);
    if (event.key === "ArrowRight") show(at + 1);
  });
  let start = null;
  box.addEventListener("touchstart", (e) => { start = e.touches[0]; }, { passive: true });
  box.addEventListener("touchend", (e) => {
    if (!start) return;
    const dx = e.changedTouches[0].clientX - start.clientX, dy = e.changedTouches[0].clientY - start.clientY;
    if (Math.abs(dx) > 50 && Math.abs(dx) > Math.abs(dy)) show(at + (dx < 0 ? 1 : -1));
    else if (dy > 80) box.close();
    start = null;
  });
})();

// A copy button on each block of code.
(function () {
  const prose = document.querySelector("[data-copy]");
  const label = prose ? prose.dataset.copy : document.documentElement.lang === "fr" ? "Copier" : "Copy";
  const done = prose ? prose.dataset.copied : document.documentElement.lang === "fr" ? "Copié" : "Copied";
  if (!navigator.clipboard) return;
  for (const pre of document.querySelectorAll("pre")) {
    const wrap = document.createElement("div");
    wrap.className = "code-wrap";
    pre.parentNode.insertBefore(wrap, pre);
    wrap.appendChild(pre);
    const button = document.createElement("button");
    button.type = "button";
    button.className = "copy";
    button.textContent = label;
    button.addEventListener("click", () => {
      navigator.clipboard.writeText(pre.innerText.replace(/\n$/, "")).then(() => {
        button.textContent = done;
        button.classList.add("done");
        setTimeout(() => { button.textContent = label; button.classList.remove("done"); }, 1600);
      });
    });
    wrap.appendChild(button);
  }
})();

// The section in view, marked in the page's table of contents.
(function () {
  const links = Array.from(document.querySelectorAll(".toc a"));
  if (!links.length || !("IntersectionObserver" in window)) return;
  const byId = new Map(links.map((a) => [decodeURIComponent(a.hash.slice(1)), a]));
  const visible = new Set();
  const observer = new IntersectionObserver((entries) => {
    for (const e of entries) e.isIntersecting ? visible.add(e.target.id) : visible.delete(e.target.id);
    const first = Array.from(byId.keys()).find((id) => visible.has(id));
    if (!first) return;
    links.forEach((a) => a.classList.toggle("active", byId.get(first) === a));
  }, { rootMargin: "0px 0px -70% 0px" });
  for (const id of byId.keys()) {
    const heading = document.getElementById(id);
    if (heading) observer.observe(heading);
  }
})();

// The documentation's search, over docs/search.json: each word typed must be
// in the section; its name counts more than its text.
(function () {
  const box = document.querySelector(".doc-search");
  if (!box || !window.fetch) return;
  const input = box.querySelector("input");
  const list = box.querySelector("ol");
  const base = input.dataset.index.replace(/docs\/search\.json$/, "");
  let index = null;
  box.hidden = false;
  function load() {
    if (index) return Promise.resolve(index);
    return fetch(input.dataset.index).then((r) => r.json()).then((data) => (index = data));
  }
  function fold(s) { return s.toLowerCase().normalize("NFD").replace(/[̀-ͯ]/g, ""); }
  function search() {
    const words = fold(input.value).split(/\s+/).filter((w) => w.length > 1);
    if (!words.length) { list.hidden = true; list.textContent = ""; return; }
    load().then((entries) => {
      const found = [];
      for (const e of entries) {
        const name = fold(`${e.p} ${e.h}`), text = fold(e.t);
        let score = 0;
        for (const w of words) {
          if (name.includes(w)) score += 10;
          else if (text.includes(w)) score += 1;
          else { score = 0; break; }
        }
        if (score) found.push([score, e]);
      }
      found.sort((a, b) => b[0] - a[0]);
      list.textContent = "";
      if (!found.length) {
        const p = document.createElement("p");
        p.textContent = input.dataset.none;
        list.appendChild(p);
      }
      for (const [, e] of found.slice(0, 12)) {
        const li = document.createElement("li");
        const a = document.createElement("a");
        a.href = base + e.u;
        a.textContent = e.h || e.p;
        if (e.h) {
          const small = document.createElement("small");
          small.textContent = e.p;
          a.appendChild(small);
        }
        li.appendChild(a);
        list.appendChild(li);
      }
      list.hidden = false;
    });
  }
  input.addEventListener("input", search);
  input.addEventListener("focus", load, { once: true });
  input.addEventListener("keydown", (event) => {
    if (event.key === "Escape") { input.value = ""; search(); }
    if (event.key === "Enter") { const a = list.querySelector("a"); if (a) location.href = a.href; }
  });
  // "/" goes to the search, as on GitHub.
  document.addEventListener("keydown", (event) => {
    if (event.key === "/" && !/^(INPUT|TEXTAREA|SELECT)$/.test(document.activeElement.tagName)) {
      event.preventDefault();
      input.focus();
    }
  });
})();

// The page not found, served at any address: in the address' language
// (/fr/…), else the visitor's.
(function () {
  const blocks = document.querySelectorAll(".not-found[data-lang]");
  if (blocks.length < 2) return;
  let lang = /\/fr(\/|$)/.test(location.pathname) ? "fr" : null;
  if (!lang) {
    try { lang = localStorage.getItem("aurore-lang"); } catch (e) { /* storage refused */ }
  }
  if (!lang) lang = (navigator.language || "en").toLowerCase().split("-")[0];
  if (!Array.from(blocks).some((b) => b.dataset.lang === lang)) return;
  for (const b of blocks) b.hidden = b.dataset.lang !== lang;
  document.documentElement.lang = lang;
})();

// On a phone, the documentation's list of pages starts folded, under the
// page's title (open in the HTML, for a page read without script).
(function () {
  const menu = document.querySelector(".doc-menu");
  if (menu && window.matchMedia("(max-width: 899px)").matches) menu.open = false;
})();

// Boréale's site switcher: the menu of the four sites opens under the name
// beside its button, wherever the header is. The same block in boreale.dev's,
// PlayGuard's, PlayShelf's and Aurore's site.js; boreale's is the reference.
// Without popovers (an old browser), the name still leads home.
(function () {
  const menu = document.getElementById("sites");
  const anchor = menu && menu.closest(".switcher");
  if (!anchor || !menu.showPopover) return;
  menu.addEventListener("beforetoggle", (event) => {
    if (event.newState !== "open") return;
    const box = anchor.getBoundingClientRect();
    const screen = document.documentElement.clientWidth;
    const width = Math.min(400, screen - 24); // site.css's width
    menu.style.top = `${box.bottom + window.scrollY + 6}px`;
    menu.style.left = `${Math.max(12, Math.min(box.left, screen - width - 12)) + window.scrollX}px`;
  });
})();

// The documentation: once the header is scrolled away, a slim bar at the top
// (the way home, the page"s name, the search, the light reading); and the
// light reading itself, kept as aurore-reading (reading.js sets it before the
// first paint).
(function () {
  const root = document.documentElement;
  const toggles = [...document.querySelectorAll(".reading-toggle")];
  const shown = () => toggles.forEach((b) => b.setAttribute("aria-pressed", String(root.dataset.reading === "light")));
  for (const b of toggles) {
    b.addEventListener("click", () => {
      const light = root.dataset.reading !== "light";
      if (light) root.dataset.reading = "light"; else delete root.dataset.reading;
      try { localStorage.setItem("aurore-reading", light ? "light" : "night"); } catch { /* storage refused */ }
      shown();
    });
  }
  shown();

  const bar = document.querySelector(".doc-bar");
  const sky = document.querySelector(".sky");
  if (!bar || !sky || !("IntersectionObserver" in window)) return;
  bar.hidden = false;
  new IntersectionObserver((entries) => {
    bar.classList.toggle("on", !entries[entries.length - 1].isIntersecting);
  }).observe(sky);
  bar.querySelector(".doc-bar-search").addEventListener("click", () => {
    const input = document.querySelector(".doc-search input");
    if (!input) return;
    window.scrollTo({ top: 0 });
    input.focus();
  });
})();
