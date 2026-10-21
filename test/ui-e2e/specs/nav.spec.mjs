import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';
export async function run(context) {
  const page = await openExample(context, 'nav');
  const nav = page.locator('#sections');
  const snapshot = names => '- navigation "Sections":\n  - heading "Sections" [level=2]' + (names.length ? '\n  - list:\n' + names.map(n => `    - listitem:\n      - link "${n}":\n        - /url: "#destination"`).join('\n') : '\n  - list');
  const current = async key => {
    assert.equal(await nav.locator('[aria-current]').count(), 1);
    assert.equal(await page.locator('#sections-item-' + key).getAttribute('data-current'), '');
  };
  await ariaSnapshot(nav, snapshot(['Overview','Resources','Contact'])); await axe(page);
  await current('Overview');
  await page.locator('#labelled').focus();
  for (const id of ['Overview','Resources','Contact']) {
    await pressKeys(page, 'Tab'); await assertFocus(page, '#sections-item-' + id);
  }
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#sections-item-Resources');
  await pressKeys(page, 'Enter'); await current('Resources'); await assertFocus(page, '#destination');
  assert.equal(await page.locator('#sections-item-Overview').getAttribute('aria-current'), null);
  assert.equal(await page.locator('#sections-item-Overview').getAttribute('data-current'), null);
  await ariaSnapshot(nav, snapshot(['Overview','Resources','Contact'])); await axe(page);
  await page.locator('#sections-item-Contact').click(); await frames(page); await current('Contact');
  await assertFocus(page, '#destination');
  await page.locator('#sections-item-Contact').focus();
  await pressKeys(page, 'Space', 'ArrowDown', 'ArrowUp', 'Home', 'End');
  await current('Contact'); await assertFocus(page, '#sections-item-Contact');
  for (const token of ['page','step','location','date','time','true']) {
    await page.locator('#token').selectOption(token); await frames(page);
    assert.equal(await page.locator('#sections-item-Contact').getAttribute('aria-current'), token);
    await current('Contact'); await ariaSnapshot(nav, snapshot(['Overview','Resources','Contact'])); await axe(page);
  }
  await page.locator('#labelled').check(); await frames(page);
  assert.equal(await nav.getAttribute('aria-label'), null);
  assert.equal(await nav.getAttribute('aria-labelledby'), 'sections-label');
  await ariaSnapshot(nav, snapshot(['Overview','Resources','Contact'])); await axe(page);
  await page.locator('#count').selectOption('Single'); await frames(page);
  assert.equal(await nav.locator('[aria-current]').count(), 0);
  await ariaSnapshot(nav, snapshot(['Overview'])); await axe(page);
  await page.locator('#sections-item-Overview').click(); await frames(page); await current('Overview'); await axe(page);
  await page.locator('#count').selectOption('Empty'); await frames(page);
  await ariaSnapshot(nav, snapshot([])); assert.equal(await nav.locator('[aria-current]').count(), 0); await axe(page);
  await page.locator('#count').selectOption('Multiple'); await frames(page);
  await current('Overview'); await ariaSnapshot(nav, snapshot(['Overview','Resources','Contact'])); await axe(page);
}
