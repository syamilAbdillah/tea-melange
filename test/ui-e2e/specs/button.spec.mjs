import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'button');
  const activity = () => page.locator('#activity').textContent();
  await ariaSnapshot(page.locator('#native'), '- button "Native action"');
  await ariaSnapshot(page.locator('#custom'), '- button "Neutral action"');
  await ariaSnapshot(page.locator('#parts'), '- button "Parts action"');
  await ariaSnapshot(page.locator('#native-toggle'), '- button "Mute"');
  await ariaSnapshot(page.locator('#custom-toggle'), '- button "Pin"');
  for (const [id, label] of [['native-disabled', 'Disabled native'], ['custom-disabled', 'Disabled neutral'], ['native-focusable', 'Focusable disabled native'], ['custom-focusable', 'Focusable disabled neutral']]) {
    await ariaSnapshot(page.locator('#' + id), `- button "${label}" [disabled]`);
  }
  await ariaSnapshot(page.locator('#submit'), '- button "Submit form"');
  await ariaSnapshot(page.locator('#reset'), '- button "Reset form"');
  await axe(page);

  // Browser Tab order (including disabled-but-focusable buttons), both ways.
  await page.locator('#disable').focus();
  const order = ['native', 'custom', 'parts', 'native-toggle', 'custom-toggle', 'native-focusable', 'custom-focusable', 'submit', 'reset'];
  for (const id of order) { await pressKeys(page, 'Tab'); await assertFocus(page, '#' + id); }
  for (const id of order.slice(0, -1).reverse()) { await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#' + id); }

  for (const id of ['native', 'custom', 'parts']) {
    await page.locator('#' + id).focus();
    const before = await activity();
    await pressKeys(page, 'Enter');
    assert.notEqual(await activity(), before, id + ' Enter activates');
    const afterEnter = await activity();
    const scroll = await page.evaluate(() => scrollY);
    await page.keyboard.down('Space');
    await frames(page);
    assert.equal(await activity(), afterEnter, id + ' Space waits for keyup');
    assert.equal(await page.evaluate(() => scrollY), scroll, 'Space keydown must not scroll');
    await page.keyboard.up('Space'); await frames(page);
    assert.notEqual(await activity(), afterEnter, id + ' Space activates on keyup');
    assert.equal(await page.evaluate(() => scrollY), scroll, 'Space must not scroll');
    const afterSpace = await activity();
    await page.locator('#' + id).click(); await frames(page);
    assert.notEqual(await activity(), afterSpace, id + ' pointer click activates');
  }
  await page.locator('#custom').focus();
  const beforeShortcuts = await activity();
  await pressKeys(page, 'Control+Enter', 'Alt+Enter', 'Meta+Enter', 'ArrowDown');
  assert.equal(await activity(), beforeShortcuts, 'Shortcuts and unhandled keys do not activate');
  await pressKeys(page, 'Home', 'End');
  assert.match(await activity(), /Hooks: 2\./);

  for (const [id, label] of [['native-toggle', 'Mute'], ['custom-toggle', 'Pin']]) {
    await page.locator('#' + id).focus();
    await pressKeys(page, 'Enter');
    await ariaSnapshot(page.locator('#' + id), `- button "${label}" [pressed]`);
    assert.equal(await page.locator('#' + id).getAttribute('data-pressed'), '');
    await axe(page);
    await pressKeys(page, 'Space');
    await ariaSnapshot(page.locator('#' + id), `- button "${label}"`);
    assert.equal(await page.locator('#' + id).getAttribute('data-pressed'), null);
    await axe(page);
    await page.locator('#' + id).click(); await frames(page);
    await ariaSnapshot(page.locator('#' + id), `- button "${label}" [pressed]`);
    await axe(page);
  }

  await page.locator('#disable').check(); await frames(page);
  await ariaSnapshot(page.locator('#native'), '- button "Native action" [disabled]');
  await ariaSnapshot(page.locator('#custom'), '- button "Neutral action" [disabled]');
  assert.equal(await page.locator('#native').getAttribute('data-disabled'), '');
  assert.equal(await page.locator('#custom').getAttribute('tabindex'), '-1');
  await axe(page);
  const disabledBefore = await activity();
  for (const id of ['native', 'custom', 'native-disabled', 'custom-disabled', 'native-focusable', 'custom-focusable']) {
    const target = page.locator('#' + id);
    await target.scrollIntoViewIfNeeded();
    const box = await target.boundingBox();
    await page.mouse.click(box.x + box.width / 2, box.y + box.height / 2);
    await frames(page);
  }
  for (const id of ['native-focusable', 'custom-focusable']) {
    await page.locator('#' + id).focus(); await assertFocus(page, '#' + id);
    await pressKeys(page, 'Enter', 'Space');
  }
  assert.equal(await activity(), disabledBefore, 'Disabled pointer/keyboard never activate');
  await page.locator('#disable').focus(); await pressKeys(page, 'Tab'); await assertFocus(page, '#parts');
  await page.locator('#disable').uncheck(); await frames(page);
  await ariaSnapshot(page.locator('#native'), '- button "Native action"');
  await ariaSnapshot(page.locator('#custom'), '- button "Neutral action"');
  await axe(page);

  await page.locator('#submit').click(); await frames(page);
  assert.match(await activity(), /Submissions: 1\./);
  await page.locator('#reset').focus(); await pressKeys(page, 'Enter');
  assert.match(await activity(), /Resets: 1\./);
  assert.equal(new URL(page.url()).pathname, '/');
  // No button action steals focus or removes the focused element.
  await assertFocus(page, '#reset');
  await axe(page);
}
