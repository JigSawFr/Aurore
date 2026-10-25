// Renders banner.svg to banner.png (1280x320), the README's banner, with the
// logo of resources/img and the Sora typeface from Google Fonts.
// Needs Node and Playwright (Chromium):  node .github/readme/render.mjs
import { chromium } from 'playwright';
import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const logo = readFileSync(join(here, '../../resources/img/borealis_256.png')).toString('base64');
const svg = readFileSync(join(here, 'banner.svg'), 'utf8')
  .replace('href="logo.png"', `href="data:image/png;base64,${logo}"`);

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 320 } });
await page.setContent(`<!doctype html><html><head>
<link href="https://fonts.googleapis.com/css2?family=Sora:wght@400;700&display=block" rel="stylesheet">
<style>html,body{margin:0}svg{display:block}</style></head><body>${svg}</body></html>`);
await page.evaluate(() => document.fonts.ready);
await page.screenshot({ path: join(here, 'banner.png') });
await browser.close();
