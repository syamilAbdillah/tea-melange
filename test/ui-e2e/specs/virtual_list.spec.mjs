import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

const settle = async page => { for (let i = 0; i < 5; i++) await frames(page); };
const rows = (page, id) => page.locator(`#${id}-content > [data-tea-virtual-key]`);
async function activeVisible(page) {
  const state = await page.locator('#options-viewport').evaluate(container => {
    const id = container.getAttribute('aria-activedescendant');
    const row = document.getElementById(id);
    const box = row?.getBoundingClientRect(), viewport = container.getBoundingClientRect();
    return { exists: !!row, pinned: row?.hasAttribute('data-pinned'), pos: row?.getAttribute('aria-posinset'), visible: box && box.top >= viewport.top && box.bottom <= viewport.bottom, focus: document.activeElement === container };
  });
  assert.ok(state.exists && state.pinned && state.visible && state.focus, JSON.stringify(state));
  assert.ok(await rows(page, 'options').count() <= 13, 'bounded option DOM');
  return Number(state.pos);
}
async function jump(page, kind, index) {
  await page.locator('#jump').fill(String(index));
  await page.locator(`#jump-${kind}`).click();
  await settle(page);
}
async function topDelta(page, kind, index) {
  return page.locator(`#${kind}-viewport`).evaluate((container, index) => {
    const row = [...container.querySelectorAll('[aria-posinset]')].find(row => Number(row.getAttribute('aria-posinset')) === index + 1);
    return row ? row.getBoundingClientRect().top - container.getBoundingClientRect().top - container.clientTop : null;
  }, index);
}
async function alignmentCases(page, kind, index) {
  for (const align of ['center', 'end']) {
    await page.locator('#align').selectOption(align);
    await jump(page, kind, index);
    const metrics = await page.locator(`#${kind}-viewport`).evaluate((container, index) => {
      const row = [...container.querySelectorAll('[aria-posinset]')].find(row => Number(row.getAttribute('aria-posinset')) === index + 1);
      return { height: row.getBoundingClientRect().height, viewport: container.clientHeight };
    }, index);
    const expected = (metrics.viewport - metrics.height) / (align === 'center' ? 2 : 1);
    assert.ok(Math.abs((await topDelta(page, kind, index)) - expected) <= 1, `${kind} ${align}`);
    await axe(page);
  }
  if (kind === 'fixed') {
    const before = await page.locator('#fixed-viewport').evaluate(el => el.scrollTop);
    await page.locator('#align').selectOption('auto'); await jump(page, kind, index);
    assert.equal(await page.locator('#fixed-viewport').evaluate(el => el.scrollTop), before, 'Auto retains fully visible row');
  }
  await page.locator('#align').selectOption('start'); await jump(page, kind, index);
}
export async function run(context) {
  const page = await openExample(context, 'virtual_list');
  await settle(page);
  await ariaSnapshot(page.locator('#options-item-option-0'), '- option "Row 0" [selected]');
  await ariaSnapshot(page.locator('#fixed-item-fixed-0'), '- listitem: Row 0');
  assert.equal(await page.locator('#fixed-item-fixed-0').getAttribute('aria-setsize'), '100000');
  assert.equal(await page.locator('#variable-item-variable-0').getAttribute('aria-setsize'), '10000');
  for (const kind of ['fixed', 'variable', 'options']) assert.ok(await rows(page, kind).count() < 40);
  await axe(page);
  await page.locator('#table').check(); await frames(page);
  await ariaSnapshot(page.locator('#fixed-item-fixed-0'), '- row "Row 0":\n  - cell "Row 0"');
  assert.equal(await page.locator('#fixed-viewport').getAttribute('aria-rowcount'), '100000');
  assert.equal(await page.locator('#fixed-item-fixed-0').getAttribute('aria-rowindex'), '1');
  assert.equal(await page.locator('#fixed-item-fixed-0').getAttribute('aria-setsize'), null);
  await axe(page);
  await page.locator('#table').uncheck(); await frames(page);
  await page.locator('#fixed-viewport').focus();
  await pressKeys(page, 'End'); await settle(page);
  assert.ok(await page.locator('#fixed-viewport').evaluate(el => el.scrollTop > 1000), 'unclaimed native End scrolls');
  await pressKeys(page, 'Home'); await settle(page);
  assert.equal(await page.locator('#fixed-viewport').evaluate(el => el.scrollTop), 0);

  // Element scrolling with the wheel, bounded row DOM and keyed identity.
  await page.locator('#fixed-viewport').hover();
  await page.mouse.wheel(0, 1600); await settle(page);
  assert.ok((await page.locator('#fixed-viewport').evaluate(el => el.scrollTop)) > 0);
  assert.ok(await rows(page, 'fixed').count() <= 14);
  await jump(page, 'fixed', 50000);
  assert.ok(Math.abs(await topDelta(page, 'fixed', 50000)) <= 1, 'fixed unseen target aligns at start');
  await ariaSnapshot(page.locator('#fixed-item-fixed-50000'), '- listitem: Row 50000');
  assert.equal(await page.locator('#fixed-item-fixed-50000').getAttribute('aria-posinset'), '50001');
  await page.locator('#fixed-item-fixed-50000').evaluate(el => { window.virtualIdentity = el; });
  await page.locator('#fixed-viewport').evaluate(el => { el.scrollTop += 32; }); await settle(page);
  assert.ok(await page.locator('#fixed-item-fixed-50000').evaluate(el => el === window.virtualIdentity));
  await axe(page);
  await alignmentCases(page, 'fixed', 50000);

  await jump(page, 'variable', 5000);
  assert.ok(Math.abs(await topDelta(page, 'variable', 5000)) <= 1, 'measured unseen target refines estimate');
  assert.ok(await rows(page, 'variable').count() < 40);
  await alignmentCases(page, 'variable', 5000);
  await page.locator('#grow').click(); await settle(page);
  assert.ok(Math.abs(await topDelta(page, 'variable', 5000)) <= 1, 'pending target remains aligned after remeasure');
  await axe(page);
  await page.locator('#grow').click(); await settle(page);
  // Cancel target by a user-like scroll, anchor ten pixels inside row 5000.
  await page.locator('#variable-viewport').evaluate(el => { el.scrollTop += 10; }); await settle(page);
  const before = await topDelta(page, 'variable', 5000);
  const beforeScroll = await page.locator('#variable-viewport').evaluate(el => el.scrollTop);
  const changed = await page.locator('#variable-content').evaluate(content => {
    const above = [...content.children].filter(row => Number(row.getAttribute('aria-posinset')) < 5001);
    for (const row of above) row.style.paddingBottom = `${parseFloat(getComputedStyle(row).paddingBottom) + 73}px`;
    return above.length;
  });
  assert.ok(changed > 0, 'test changes measured rows above the anchor');
  await settle(page);
  assert.ok(Math.abs((await topDelta(page, 'variable', 5000)) - before) <= 1, 'anchor does not jump when preceding rows grow');
  const afterScroll = await page.locator('#variable-viewport').evaluate(el => el.scrollTop);
  assert.ok(Math.abs(afterScroll - beforeScroll - 73 * changed) <= 1, `border-box growth really was measured and compensated: before=${beforeScroll} after=${afterScroll} changed=${changed}`);
  await axe(page);

  const listbox = page.locator('#options-viewport');
  await listbox.focus();
  assert.equal(await activeVisible(page), 1);
  await pressKeys(page, 'ArrowDown'); assert.equal(await activeVisible(page), 2);
  await pressKeys(page, 'ArrowUp'); assert.equal(await activeVisible(page), 1);
  await pressKeys(page, 'PageDown'); assert.equal(await activeVisible(page), 8);
  await pressKeys(page, 'PageUp'); assert.equal(await activeVisible(page), 1);
  await pressKeys(page, 'End'); assert.equal(await activeVisible(page), 1000);
  await ariaSnapshot(page.locator('#options-item-option-999'), '- option "Row 999" [selected]');
  await axe(page);
  await pressKeys(page, 'Home'); assert.equal(await activeVisible(page), 1);
  await pressKeys(page, 'Control+End', 'Alt+ArrowDown', 'Meta+End');
  // Browser shortcut defaults may scroll, but must not change the active key.
  assert.equal(await listbox.getAttribute('aria-activedescendant'), 'options-item-option-0');
  await pressKeys(page, 'Home');
  assert.equal(await activeVisible(page), 1);
  await listbox.evaluate(el => el.dispatchEvent(new KeyboardEvent('keydown', {key:'ArrowDown', isComposing:true, bubbles:true, cancelable:true})));
  await frames(page); assert.equal(await activeVisible(page), 1);
  await pressKeys(page, 'Tab'); await assertFocus(page, '#after');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#options-viewport');
  await page.locator('#options-item-option-3').click(); await frames(page);
  assert.equal(await activeVisible(page), 4);
  await ariaSnapshot(page.locator('#options-item-option-3'), '- option "Row 3" [selected]');
  await axe(page);

  // A pinned active descendant remains resolvable even after pointer scrolling.
  await listbox.evaluate(el => { el.scrollTop = 20000; }); await settle(page);
  assert.equal(await page.locator('#options-item-option-3').count(), 1);
  assert.equal(await page.locator('#options-item-option-3').getAttribute('data-pinned'), '');
  assert.ok(await rows(page, 'options').count() <= 14);
  await pressKeys(page, 'Home'); assert.equal(await activeVisible(page), 1);

  // Visit every index with real DOM keyboard intents and a rendered frame per
  // movement. Assert active idref, visibility and row bound on every step.
  const traversed = await listbox.evaluate(async container => {
    for (let i = 1; i < 1000; i++) {
      container.dispatchEvent(new KeyboardEvent('keydown', {key:'ArrowDown', bubbles:true, cancelable:true}));
      await new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve)));
      const row = document.getElementById(container.getAttribute('aria-activedescendant'));
      const r = row?.getBoundingClientRect(), c = container.getBoundingClientRect();
      if (!row || row.getAttribute('aria-posinset') !== String(i+1) || r.top < c.top || r.bottom > c.bottom || container.querySelectorAll('[role=option]').length > 13) return i;
    }
    return 1000;
  });
  assert.equal(traversed, 1000, 'all 1000 active rows rendered and visible');
  await activeVisible(page);
  await ariaSnapshot(page.locator('#options-item-option-999'), '- option "Row 999" [selected]');
  await axe(page);
}
