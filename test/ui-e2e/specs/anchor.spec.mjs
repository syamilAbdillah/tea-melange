import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

const panelSnapshot = '- region "Floating panel":\n  - paragraph: Attached, not contained.\n  - paragraph: Scroll to see it follow.';
const sides = ['top', 'bottom', 'left', 'right'];
const alignments = ['start', 'center', 'end'];
const near = (actual, expected, label) => assert.ok(Math.abs(actual - expected) <= 1, `${label}: ${actual} vs ${expected}`);

async function settle(page) {
  await frames(page); await frames(page);
}
async function setCheck(page, id, value) {
  await page.locator('#' + id).setChecked(value); await settle(page);
}
async function placement(page, value) {
  await page.locator('#placement').selectOption(value); await settle(page);
}
async function geometry(page, { side, alignment, rtl = false, offset = 8, shift = true, match = false }) {
  const g = await page.evaluate(() => {
    const a = document.querySelector('#reference').getBoundingClientRect();
    const f = document.querySelector('#floating').getBoundingClientRect();
    const css = getComputedStyle(document.querySelector('#floating'));
    return { a: a.toJSON(), f: f.toJSON(), width: innerWidth, height: innerHeight,
      placement: document.querySelector('#floating').dataset.placement,
      anchorWidth: parseFloat(css.getPropertyValue('--tea-anchor-width')),
      anchorHeight: parseFloat(css.getPropertyValue('--tea-anchor-height')),
      availableWidth: parseFloat(css.getPropertyValue('--tea-available-width')),
      availableHeight: parseFloat(css.getPropertyValue('--tea-available-height')) };
  });
  const { a, f } = g;
  assert.equal(g.placement, `${side}-${alignment}`);
  const vertical = side === 'top' || side === 'bottom';
  const logical = vertical && rtl && alignment !== 'center' ? (alignment === 'start' ? 'end' : 'start') : alignment;
  let cross = vertical ? a.left : a.top;
  const anchorSize = vertical ? a.width : a.height;
  const floatingSize = vertical ? f.width : f.height;
  if (logical === 'center') cross += (anchorSize - floatingSize) / 2;
  if (logical === 'end') cross += anchorSize - floatingSize;
  if (shift) cross = Math.max(8, Math.min(Math.max(8, (vertical ? g.width : g.height) - 8 - floatingSize), cross));
  const main = { top: a.top - offset - f.height, bottom: a.bottom + offset, left: a.left - offset - f.width, right: a.right + offset }[side];
  near(vertical ? f.left : f.top, cross, 'cross-axis');
  near(vertical ? f.top : f.left, main, 'main-axis');
  near(g.anchorWidth, a.width, 'anchor width variable');
  near(g.anchorHeight, a.height, 'anchor height variable');
  const pad = shift ? 8 : 0;
  const available = { top: a.top - offset - pad, bottom: g.height - pad - a.bottom - offset, left: a.left - offset - pad, right: g.width - pad - a.right - offset }[side];
  near(vertical ? g.availableHeight : g.availableWidth, Math.max(0, Math.min((vertical ? g.height : g.width) - 2 * pad, available)), 'available main extent');
  near(vertical ? g.availableWidth : g.availableHeight, (vertical ? g.width : g.height) - 2 * pad, 'available cross extent');
  if (match) near(f.width, a.width, 'matched border-box width');
  return g;
}
async function moveReference(page, x, y) {
  await page.evaluate(({ x, y }) => {
    const resizer = document.querySelector('#resizer');
    const r = document.querySelector('#reference').getBoundingClientRect();
    resizer.style.left = `${parseFloat(getComputedStyle(resizer).left) + x - r.left}px`;
    resizer.style.top = `${parseFloat(getComputedStyle(resizer).top) + y - r.top}px`;
    window.dispatchEvent(new Event('resize'));
  }, { x, y });
  await settle(page);
}
async function visibleState(page) {
  await ariaSnapshot(page.locator('#floating'), panelSnapshot);
  await axe(page);
}
export async function run(context) {
  const page = await openExample(context, 'anchor');
  await page.setViewportSize({ width: 1100, height: 900 }); await settle(page);
  assert.equal(await page.locator('#floating').evaluate(el => el.matches(':popover-open')), true);
  await visibleState(page);

  // Anchor owns no keyboard. Native control semantics, Tab, Escape, browser
  // chords and IME are left intact; merely positioning cannot move focus.
  await page.locator('#reference').focus();
  await assertFocus(page, '#reference');
  await pressKeys(page, 'Escape', 'Control+ArrowDown', 'Alt+ArrowUp', 'Meta+ArrowLeft');
  await assertFocus(page, '#reference');
  assert.equal(await page.locator('#reference').getAttribute('aria-expanded'), 'true');
  const accepted = await page.locator('#reference').evaluate(el => {
    const e = new KeyboardEvent('keydown', { key: 'Escape', isComposing: true, cancelable: true, bubbles: true });
    return el.dispatchEvent(e);
  });
  assert.equal(accepted, true);
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#scroller');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#reference');
  await pressKeys(page, 'Enter'); await settle(page);
  await ariaSnapshot(page.locator('#reference'), '- button "Toggle panel"');
  assert.equal(await page.locator('#floating').evaluate(el => el.hidden && !el.matches(':popover-open')), true);
  await axe(page);
  await pressKeys(page, 'Space'); await settle(page);
  await assertFocus(page, '#reference');
  await ariaSnapshot(page.locator('#reference'), '- button "Toggle panel" [expanded]');
  await visibleState(page);
  await page.locator('#reference').click(); await settle(page); await axe(page);
  await page.locator('#reference').click(); await settle(page); await visibleState(page);

  await setCheck(page, 'flip', false);
  await moveReference(page, 400, 420);
  // Every placement and logical alignment, both directions; geometry and
  // accessible tree are checked independently of the implementation.
  for (const rtl of [false, true]) {
    await setCheck(page, 'rtl', rtl);
    for (const side of sides) for (const alignment of alignments) {
      await placement(page, `${side}-${alignment}`);
      await geometry(page, { side, alignment, rtl });
      await visibleState(page);
    }
  }
  await setCheck(page, 'rtl', false);
  await page.locator('#offset').fill('17'); await settle(page);
  await placement(page, 'bottom-center');
  await geometry(page, { side: 'bottom', alignment: 'center', offset: 17 }); await visibleState(page);
  await page.locator('#offset').fill('8'); await settle(page);

  // Flip independently at every viewport edge. Neither-side-fits is covered
  // exhaustively by pure tests; Chrome verifies the actual top-layer box.
  await setCheck(page, 'flip', true);
  for (const side of sides) {
    const { width, height } = page.viewportSize();
    const box = await page.locator('#reference').boundingBox();
    const x = side === 'left' ? 2 : side === 'right' ? width - box.width - 2 : 400;
    const y = side === 'top' ? 2 : side === 'bottom' ? height - box.height - 2 : 420;
    await moveReference(page, x, y);
    await placement(page, `${side}-start`);
    const opposite = { top: 'bottom', bottom: 'top', left: 'right', right: 'left' }[side];
    await geometry(page, { side: opposite, alignment: 'start' }); await visibleState(page);
  }
  await setCheck(page, 'flip', false);
  await moveReference(page, 2, 420); await placement(page, 'bottom-end');
  await geometry(page, { side: 'bottom', alignment: 'end' }); await visibleState(page);
  await setCheck(page, 'shift', false);
  const unshifted = await geometry(page, { side: 'bottom', alignment: 'end', shift: false });
  assert.ok(unshifted.f.left < 0, 'disabling shift preserves overflow'); await visibleState(page);
  await setCheck(page, 'shift', true);

  await moveReference(page, 400, 420); await placement(page, 'bottom-start');
  await setCheck(page, 'match', true);
  await geometry(page, { side: 'bottom', alignment: 'start', match: true }); await visibleState(page);
  await page.locator('#resizer').evaluate(el => { el.style.width = '246px'; el.style.height = '90px'; });
  await settle(page);
  await geometry(page, { side: 'bottom', alignment: 'start', match: true }); await visibleState(page);
  await setCheck(page, 'match', false);
  near((await page.locator('#floating').boundingBox()).width, 220, 'intrinsic width restored');

  // Captured nested scroll and window scroll follow the viewport reference.
  await page.locator('#scroller').evaluate(el => { el.scrollLeft = 90; el.scrollTop = 60; });
  await settle(page); await geometry(page, { side: 'bottom', alignment: 'start' }); await visibleState(page);
  await page.evaluate(() => { document.body.style.minHeight = '1800px'; window.scrollTo(0, 50); });
  await settle(page); await geometry(page, { side: 'bottom', alignment: 'start' }); await visibleState(page);
  await page.setViewportSize({ width: 900, height: 800 }); await settle(page);
  await geometry(page, { side: 'bottom', alignment: 'start' }); await visibleState(page);

  // The panel is still a DOM descendant of a clipping, transformed ancestor,
  // but is painted/hit-tested outside its border in the native top layer.
  const clip = await page.locator('#clip').boundingBox();
  await moveReference(page, clip.x + 100, clip.y + clip.height - 42);
  await placement(page, 'bottom-start');
  const escaped = await geometry(page, { side: 'bottom', alignment: 'start' });
  assert.ok(escaped.f.bottom > clip.y + clip.height + 40, 'floating box extends past clipping ancestor');
  const hit = await page.evaluate(({ x, y }) => document.elementFromPoint(x, y)?.closest('#floating')?.id,
    { x: escaped.f.left + 10, y: Math.max(escaped.f.top + 10, clip.y + clip.height + 25) });
  assert.equal(hit, 'floating', 'top layer is actually painted outside the transformed clip: ' + JSON.stringify({ clip, escaped, viewport: page.viewportSize() }));
  assert.equal(await page.locator('#floating').evaluate(el => !!el.closest('#clip')), true, 'no portal/reparenting');
  await visibleState(page);

  // Reference fully outside viewport: consumers can close on the state, or
  // retain the active hidden panel. Partially visible references remain shown.
  await moveReference(page, -400, 300);
  assert.equal(await page.locator('#floating').getAttribute('data-reference-hidden'), '');
  assert.equal(await page.locator('#floating').evaluate(el => getComputedStyle(el).visibility), 'hidden');
  await ariaSnapshot(page.locator('#floating'), ''); await axe(page);
  await moveReference(page, 400, 350); await visibleState(page);

  // Plain fixed fallback is usable in an untransformed root, but deliberately
  // cannot promise escape when an ancestor establishes a containing block.
  await page.locator('#clip').evaluate(el => { el.style.transform = 'none'; el.style.overflow = 'visible'; });
  await setCheck(page, 'top-layer', false);
  assert.equal(await page.locator('#floating').getAttribute('popover'), null);
  await geometry(page, { side: 'bottom', alignment: 'start' }); await visibleState(page);
  await setCheck(page, 'top-layer', true);
  await geometry(page, { side: 'bottom', alignment: 'start' }); await visibleState(page);

  const before = await page.locator('#measurements').textContent();
  await page.evaluate(() => { for (let i = 0; i < 100; i++) document.dispatchEvent(new Event('scroll')); });
  await settle(page);
  assert.equal(await page.locator('#measurements').textContent(), before, 'unchanged scroll burst never dispatches');
}
