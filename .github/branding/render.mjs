// Renders the fork's branding (.github/branding/*.svg) to PNG:
//   icon.png (256x256), social-preview.png (1280x640, the repository's social
//   preview, uploaded by hand in Settings › General) and the README's banner,
//   .github/readme/banner.png (1280x320).
// Needs Node and Playwright (Chromium); loads Sora from Google Fonts.
//   node .github/branding/render.mjs
import { chromium } from 'playwright';
import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const jobs = [
  { svg: 'icon.svg', out: 'icon.png', w: 256, h: 256, transparent: true },
  { svg: 'banner.svg', out: '../readme/banner.png', w: 1280, h: 320 },
  { svg: 'social-preview.svg', out: 'social-preview.png', w: 1280, h: 640 },
];

// <image href="icon.svg"> becomes the icon itself, inline.
const icon = readFileSync(join(here, 'icon.svg'), 'utf8');
const inline = (svg) => svg.replace(/<image href="icon\.svg" x="([\d.]+)" y="([\d.]+)" width="([\d.]+)" height="[\d.]+"\/>/g,
  (_, x, y, w) => `<g transform="translate(${x} ${y}) scale(${w / 256})">${icon.replace(/<\/?svg[^>]*>/g, '')}</g>`);

const browser = await chromium.launch();
for (const job of jobs) {
  const page = await browser.newPage({ viewport: { width: job.w, height: job.h } });
  const svg = inline(readFileSync(join(here, job.svg), 'utf8'));
  await page.setContent(`<!doctype html><html><head>
<link href="https://fonts.googleapis.com/css2?family=Sora:wght@400;700&display=block" rel="stylesheet">
<style>html,body{margin:0;background:transparent}svg{display:block}</style></head><body>${svg}</body></html>`);
  await page.evaluate(() => document.fonts.ready);
  await page.screenshot({ path: join(here, job.out), omitBackground: !!job.transparent });
  await page.close();
}
await browser.close();
