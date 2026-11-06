// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0
//
// Renders the demo's platform icons from Aurore's icon (.github/branding/icon.svg,
// copied from Boréale's brand kit), in a headless Chromium: the window icon,
// the Switch's, PS4's, PS Vita's (with its LiveArea), Android's, iOS's and
// macOS's, Windows', webOS's, and the template's. Android 8+ gets its adaptive
// icon as three layers (background, foreground, and the themed monochrome of
// Android 13), drawn from the kit's mark.
//
//   node tools/render_icons.mjs
//
// Needs Playwright with its Chromium (npm i -g playwright, npx playwright
// install chromium), Python 3 with Pillow (the Vita's PNGs are 8-bit indexed)
// and the boreale-site submodule (its fonts).
import { mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { createRequire } from "node:module";
import { execFileSync } from "node:child_process";

const require = createRequire(import.meta.url);
const { chromium } = require("playwright");

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const at = (p) => join(root, p);
const icon = readFileSync(at(".github/branding/icon.svg"), "utf8").replace(/<!--[\s\S]*?-->/, "");
const mark = readFileSync(at(".github/branding/mark.svg"), "utf8").replace(/<!--[\s\S]*?-->/, "");
const mono = readFileSync(at(".github/branding/mark-mono-white.svg"), "utf8").replace(/<!--[\s\S]*?-->/, "");
const fonts = at("library/boreale-site/assets/fonts");

const SNOW = "#E8EEF7", AURORA = "#00FFCC", ICE = "#33BBFF", VIOLET = "#8A5CFF";

// The icon as an <svg>: full-bleed (where the system rounds it), a tile
// (rounded on a transparent ground, where nothing rounds it), or padded
// (macOS's grid: the tile on 824 of 1024, the rest left empty).
function drawn(kind, size) {
  const body = icon.replace(/<svg[^>]*>/, "").replace(/<\/svg>\s*$/, "");
  const inner = (s) => `<svg viewBox="0 0 256 256" width="${s}" height="${s}"`;
  if (kind === "full") return `${inner(size)} xmlns="http://www.w3.org/2000/svg">${body}</svg>`;
  const pad = kind === "padded" ? 100 * size / 1024 : 0;
  const s = size - 2 * pad;
  return `<svg xmlns="http://www.w3.org/2000/svg" width="${size}" height="${size}" viewBox="0 0 ${size} ${size}">` +
    `<clipPath id="r"><rect x="${pad}" y="${pad}" width="${s}" height="${s}" rx="${s * 56 / 256}"/></clipPath>` +
    `<g clip-path="url(#r)"><svg x="${pad}" y="${pad}" width="${s}" height="${s}" viewBox="0 0 256 256">${body}</svg></g></svg>`;
}

// What each platform reads: [file, size, kind].
const ios = readdirSizes("demo/ios/Images.xcassets/AppIcon.appiconset");
const ICONS = [
  ["resources/icon/icon.png", 256, "tile"],
  ["resources/img/aurore_256.png", 288, "tile"],
  ["resources/img/aurore_96.png", 96, "tile"],
  ["resources/img/demo_icon.jpg", 256, "full"],
  ["template/icon.jpg", 256, "full"],
  ["ps4/sce_sys/icon0.png", 512, "full"],
  ["psv/sce_sys/icon0.png", 128, "full"],
  ["webos/icon.png", 80, "tile"],
  ["webos/largeIcon.png", 130, "tile"],
  ...[["mdpi", 48], ["hdpi", 72], ["xhdpi", 96], ["xxhdpi", 144], ["xxxhdpi", 192]]
    .map(([d, n]) => [`android-project/app/src/main/res/mipmap-${d}/ic_launcher.png`, n, "tile"]),
  ...ios.map((n) => [`demo/ios/Images.xcassets/AppIcon.appiconset/${n}.png`, n, "full"]),
];

// Android's adaptive icon: a 108 dp canvas of which the system shows a
// rounded or circular 72 dp and moves the layers apart a little, so the mark
// sits in the central 66 dp (its frame spans 56 dp) over a night of stars.
const DENSITIES = [["mdpi", 1], ["hdpi", 1.5], ["xhdpi", 2], ["xxhdpi", 3], ["xxxhdpi", 4]];
const layer = (svg) => {
  const body = svg.replace(/<svg[^>]*>/, "").replace(/<\/svg>\s*$/, "");
  return `<svg xmlns="http://www.w3.org/2000/svg" width="108" height="108" viewBox="0 0 108 108"><svg x="12" y="12" width="84" height="84" viewBox="0 0 256 256">${body}</svg></svg>`;
};
const sized = (svg, px) => svg.replace('width="108" height="108"', `width="${px}" height="${px}"`);
const NIGHT_BACKGROUND = `<svg xmlns="http://www.w3.org/2000/svg" width="108" height="108" viewBox="0 0 108 108">
  <defs><linearGradient id="n" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#0A1020"/><stop offset="1" stop-color="#16203A"/></linearGradient></defs>
  <rect width="108" height="108" fill="url(#n)"/>
  <g fill="${SNOW}"><circle cx="22" cy="26" r="0.8" opacity="0.7"/><circle cx="84" cy="20" r="0.7" opacity="0.8"/><circle cx="90" cy="78" r="0.6" opacity="0.5"/><circle cx="16" cy="84" r="0.6" opacity="0.5"/><circle cx="54" cy="12" r="0.6" opacity="0.6"/></g>
</svg>`;

function readdirSizes(dir) {
  const json = JSON.parse(readFileSync(at(join(dir, "Contents.json")), "utf8"));
  return [...new Set(json.images.map((i) => i.filename).filter(Boolean).map((f) => parseInt(f, 10)))];
}

// The Vita's LiveArea: its background (840 × 500, the night and the aurora's
// curtain) and its gate (280 × 158, the icon and the name).
const wave = "M0 300 C 160 210, 320 330, 500 250 S 760 170, 840 210";
const LIVEAREA_BG = `<svg xmlns="http://www.w3.org/2000/svg" width="840" height="500" viewBox="0 0 840 500">
  <defs>
    <linearGradient id="sky" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#0A1020"/><stop offset="1" stop-color="#141B2E"/></linearGradient>
    <linearGradient id="colour" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="${AURORA}" stop-opacity="0"/><stop offset="0.25" stop-color="${AURORA}"/><stop offset="0.65" stop-color="${ICE}"/><stop offset="0.9" stop-color="${VIOLET}"/><stop offset="1" stop-color="${VIOLET}" stop-opacity="0"/></linearGradient>
    <linearGradient id="rise" x1="0" y1="1" x2="0" y2="0"><stop offset="0" stop-color="#fff" stop-opacity="0.9"/><stop offset="0.3" stop-color="#fff" stop-opacity="0.5"/><stop offset="1" stop-color="#fff" stop-opacity="0"/></linearGradient>
    <pattern id="rays" width="14" height="500" patternUnits="userSpaceOnUse"><rect width="14" height="500" fill="#fff" opacity="0.5"/><rect x="3.5" width="6.3" height="500" fill="#fff"/></pattern>
    <filter id="soft" x="-10%" y="-10%" width="120%" height="120%"><feGaussianBlur stdDeviation="4.2 2"/></filter>
    <filter id="line" x="-10%" y="-100%" width="120%" height="300%"><feGaussianBlur stdDeviation="6"/></filter>
    <mask id="rays-mask" maskUnits="userSpaceOnUse" x="0" y="0" width="840" height="500"><rect width="840" height="500" fill="url(#rays)" filter="url(#soft)"/></mask>
    <mask id="fade" maskUnits="userSpaceOnUse" x="0" y="0" width="840" height="500"><path d="${wave} L 840 0 L 0 0 Z" fill="url(#rise)"/></mask>
  </defs>
  <rect width="840" height="500" fill="url(#sky)"/>
  <g opacity="0.55">
    <g mask="url(#fade)"><rect width="840" height="500" fill="url(#colour)" mask="url(#rays-mask)"/></g>
    <path d="${wave}" fill="none" stroke="url(#colour)" stroke-width="10" filter="url(#line)"/>
    <path d="${wave}" fill="none" stroke="url(#colour)" stroke-width="2.5" opacity="0.8"/>
  </g>
  <g fill="${SNOW}"><circle cx="90" cy="50" r="1.6" opacity="0.7"/><circle cx="740" cy="60" r="1.4" opacity="0.8"/><circle cx="420" cy="34" r="1.3" opacity="0.6"/><circle cx="790" cy="430" r="1.2" opacity="0.6"/><circle cx="60" cy="450" r="1.2" opacity="0.5"/></g>
</svg>`;
const LIVEAREA_GATE = `<svg xmlns="http://www.w3.org/2000/svg" width="280" height="158" viewBox="0 0 280 158">
  <defs><linearGradient id="word" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="${AURORA}"/><stop offset="0.55" stop-color="${ICE}"/><stop offset="1" stop-color="${VIOLET}"/></linearGradient></defs>
  <rect width="280" height="158" fill="#0E1A33"/>
  <svg x="22" y="35" width="88" height="88">${drawn("tile", 88).replace(/^<svg[^>]*>/, "").replace(/<\/svg>$/, "")}</svg>
  <text x="126" y="90" font-family="Sora" font-weight="700" font-size="38" letter-spacing="-1" fill="${SNOW}">Aurore</text>
  <rect x="128" y="104" width="40" height="4" rx="2" fill="url(#word)"/>
</svg>`;

const face = (family, file) =>
  `@font-face{font-family:"${family}";src:url("data:font/woff2;base64,${readFileSync(join(fonts, file)).toString("base64")}") format("woff2");font-weight:100 800;}`;
const css = face("Sora", "sora.woff2") + "html,body{margin:0;background:transparent}svg{display:block}";

const browser = await chromium.launch();
const page = await browser.newPage();
async function render(svg, width, height, type = "png", opaque = false) {
  await page.setViewportSize({ width, height });
  await page.setContent(`<!doctype html><style>${css}</style>${svg}`);
  await page.evaluate(() => document.fonts.load("700 20px Sora"));
  const el = page.locator("body > svg").first();
  return type === "jpeg" ? el.screenshot({ type, quality: 95 }) : el.screenshot({ omitBackground: !opaque });
}

for (const [file, size, kind] of ICONS) {
  const type = file.endsWith(".jpg") ? "jpeg" : "png";
  writeFileSync(at(file), await render(drawn(kind, size), size, size, type, kind === "full"));
  console.log(file);
}
writeFileSync(at("psv/sce_sys/livearea/contents/bg.png"), await render(LIVEAREA_BG, 840, 500, "png", true));
writeFileSync(at("psv/sce_sys/livearea/contents/startup.png"), await render(LIVEAREA_GATE, 280, 158, "png", true));
console.log("psv/sce_sys/livearea/contents/{bg,startup}.png");

for (const [d, k] of DENSITIES) {
  const res = `android-project/app/src/main/res/mipmap-${d}`;
  const px = Math.round(108 * k);
  writeFileSync(at(`${res}/ic_launcher_background.png`), await render(sized(NIGHT_BACKGROUND, px), px, px, "png", true));
  writeFileSync(at(`${res}/ic_launcher_foreground.png`), await render(sized(layer(mark), px), px, px, "png"));
  writeFileSync(at(`${res}/ic_launcher_monochrome.png`), await render(sized(layer(mono), px), px, px, "png"));
}
mkdirSync(at("android-project/app/src/main/res/mipmap-anydpi-v26"), { recursive: true });
writeFileSync(at("android-project/app/src/main/res/mipmap-anydpi-v26/ic_launcher.xml"), `<?xml version="1.0" encoding="utf-8"?>
<!-- SPDX-FileCopyrightText: 2026 JigSawFr -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Written by tools/render_icons.mjs. -->
<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">
    <background android:drawable="@mipmap/ic_launcher_background"/>
    <foreground android:drawable="@mipmap/ic_launcher_foreground"/>
    <monochrome android:drawable="@mipmap/ic_launcher_monochrome"/>
</adaptive-icon>
`);
console.log("android adaptive: mipmap-*/ic_launcher_{background,foreground,monochrome}.png, mipmap-anydpi-v26/ic_launcher.xml");

// Windows: an ICO holding PNGs (16 to 256). macOS: an ICNS of PNGs on its grid.
const tiles = {};
for (const n of [16, 32, 48, 64, 128, 256, 512, 1024]) tiles[n] = await render(drawn(n >= 128 ? "padded" : "tile", n), n, n);
const winSizes = [16, 32, 48, 256];
const ico = [];
for (const n of winSizes) ico.push(await render(drawn("tile", n), n, n));
writeFileSync(at("demo/aurore.ico"), icoFile(ico, winSizes));
writeFileSync(at("demo/aurore.icns"), icnsFile([["icp4", 16], ["icp5", 32], ["icp6", 64], ["ic07", 128], ["ic08", 256], ["ic09", 512], ["ic10", 1024]]
  .map(([type, n]) => [type, tiles[n]])));
console.log("demo/aurore.ico, demo/aurore.icns");
await browser.close();

function icoFile(pngs, sizes) {
  const head = Buffer.alloc(6 + 16 * pngs.length);
  head.writeUInt16LE(1, 2); head.writeUInt16LE(pngs.length, 4);
  let offset = head.length;
  pngs.forEach((png, i) => {
    const o = 6 + 16 * i;
    head.writeUInt8(sizes[i] % 256, o); head.writeUInt8(sizes[i] % 256, o + 1);
    head.writeUInt16LE(1, o + 4); head.writeUInt16LE(32, o + 6);
    head.writeUInt32LE(png.length, o + 8); head.writeUInt32LE(offset, o + 12);
    offset += png.length;
  });
  return Buffer.concat([head, ...pngs]);
}

function icnsFile(entries) {
  const parts = entries.map(([type, png]) => {
    const h = Buffer.alloc(8); h.write(type, 0, "ascii"); h.writeUInt32BE(png.length + 8, 4);
    return Buffer.concat([h, png]);
  });
  const body = Buffer.concat(parts);
  const h = Buffer.alloc(8); h.write("icns", 0, "ascii"); h.writeUInt32BE(body.length + 8, 4);
  return Buffer.concat([h, body]);
}

// The Vita reads 8-bit indexed PNGs only.
execFileSync("python3", ["-c", `
import sys
from PIL import Image
for f in sys.argv[1:]:
    Image.open(f).convert("RGB").quantize(256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.FLOYDSTEINBERG).save(f, optimize=True)
`, ...["psv/sce_sys/icon0.png", "psv/sce_sys/livearea/contents/bg.png", "psv/sce_sys/livearea/contents/startup.png"].map(at)]);
console.log("psv: 8-bit indexed");
