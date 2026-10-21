import assert from 'node:assert/strict';
import { createRequire } from 'node:module';
import { serveExample } from '../../scripts/serve.js';

const require = createRequire(import.meta.url);
const axePath = require.resolve('axe-core/axe.min.js');
const servers = new Map();
const errors = new WeakMap();

/** Open the actual emitted example, not a test reimplementation. */
export async function openExample(context, name) {
  if (!servers.has(name)) servers.set(name, await serveExample('ui/' + name));
  const page = await context.newPage();
  const failures = [];
  errors.set(page, failures);
  page.on('pageerror', error => failures.push(error.message));
  page.on('console', message => { if (message.type() === 'error') failures.push(message.text()); });
  page.on('response', response => { if (response.status() >= 400) failures.push(response.status() + ' ' + response.url()); });
  await page.goto(servers.get(name).url);
  await page.locator('#app > *').first().waitFor();
  return page;
}

/** Flush both the draw and post-render DOM effects, without a timer sleep. */
export async function frames(page) {
  await page.evaluate(() => new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve))));
}

/** Press a Playwright key/chord, allowing render/effect frames between keys. */
export async function pressKeys(page, ...keys) {
  for (const key of keys.flat()) { await page.keyboard.press(key); await frames(page); }
}

export async function assertFocus(page, selector) {
  assert.equal(await page.locator(selector).evaluate(element => element === document.activeElement), true, 'Expected focus: ' + selector);
}

/** Compare exact normalized Playwright YAML, scoped to the supplied locator. */
export async function ariaSnapshot(locator, expected) {
  const normalize = value => value.trim().replace(/\r\n/g, '\n');
  assert.equal(normalize(await locator.ariaSnapshot()), normalize(expected));
}

/** No rule exclusions, impact threshold, or allowed violations. */
export async function axe(page) {
  if (!await page.evaluate(() => !!window.axe)) await page.addScriptTag({ path: axePath });
  const result = await page.evaluate(() => window.axe.run(document));
  assert.deepEqual(result.violations.map(v => ({ id: v.id, impact: v.impact, nodes: v.nodes.map(n => n.target) })), [], 'axe violations');
}

export function assertHealthy(pages) {
  for (const page of pages) assert.deepEqual(errors.get(page) ?? [], [], 'Browser errors');
}

export async function closeExamples() {
  await Promise.all([...servers.values()].map(({ server }) => new Promise((resolve, reject) => server.close(error => error ? reject(error) : resolve()))));
  servers.clear();
}
