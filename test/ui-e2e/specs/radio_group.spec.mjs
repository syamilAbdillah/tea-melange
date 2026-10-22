import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

const keys = ['daily', 'instant', 'weekly', 'never'];
const labels = ['Daily', 'Instant (unavailable)', 'Weekly', 'Never'];
const radio = (page, group, key) => page.locator(`#${group}-item-${key}`);
async function state(page, group, checked, disabled = false) {
  for (let i = 0; i < keys.length; i++) {
    const target = radio(page, group, keys[i]);
    if (!await target.count()) continue;
    const flags = (checked === keys[i] ? ' [checked]' : '') + (disabled || i === 1 ? ' [disabled]' : '');
    await ariaSnapshot(target, `- radio "${labels[i]}"${flags}`);
    assert.equal(await target.getAttribute('aria-checked'), String(checked === keys[i]));
  }
  await axe(page);
}

export async function run(context) {
  const page = await openExample(context, 'radio_group');
  const selected = group => page.locator(`#${group} [aria-checked=true]`).getAttribute('id');
  await state(page, 'schedule', null);
  await state(page, 'managed', 'weekly');
  await state(page, 'paused', 'daily', true);
  assert.equal(await page.locator('#schedule').getAttribute('aria-orientation'), null);
  assert.equal(await page.locator('#schedule').getAttribute('aria-required'), 'true');
  assert.equal(await page.locator('#schedule').getAttribute('aria-invalid'), 'true');

  await page.locator('#clear').focus();
  // Shift+Tab passes the validation checkbox and enters at first enabled.
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#invalid');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#schedule-item-daily');
  await pressKeys(page, 'Space'); await state(page, 'schedule', 'daily');
  assert.equal(await page.locator('input[name=schedule-0]').inputValue(), 'daily');
  for (const [key, target] of [
    ['ArrowRight', 'weekly'], ['ArrowDown', 'never'], ['ArrowRight', 'daily'],
    ['ArrowLeft', 'never'], ['ArrowUp', 'weekly'], ['ArrowLeft', 'daily'],
  ]) {
    await pressKeys(page, key); await assertFocus(page, '#schedule-item-' + target);
    assert.equal(await selected('schedule'), 'schedule-item-' + target);
  }
  await pressKeys(page, 'End', 'Home', 'Enter', 'Control+ArrowRight', 'Alt+ArrowDown', 'Meta+ArrowLeft');
  await assertFocus(page, '#schedule-item-daily');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#invalid');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#schedule-item-daily');
  await pressKeys(page, 'ArrowRight'); await state(page, 'schedule', 'weekly');
  await pressKeys(page, 'ArrowDown'); await state(page, 'schedule', 'never');

  await radio(page, 'schedule', 'daily').click(); await frames(page);
  await assertFocus(page, '#schedule-item-daily');
  const disabledBox = await radio(page, 'schedule', 'instant').boundingBox();
  await page.mouse.click(disabledBox.x + disabledBox.width / 2, disabledBox.y + disabledBox.height / 2); await frames(page);
  await assertFocus(page, '#schedule-item-daily');
  assert.equal(await selected('schedule'), 'schedule-item-daily');
  // Two real DOM key events before a draw must advance twice, not once.
  await radio(page, 'schedule', 'daily').evaluate(el => {
    for (let i = 0; i < 2; i++) el.dispatchEvent(new KeyboardEvent('keydown', {key: 'ArrowRight', bubbles: true, cancelable: true}));
  });
  await frames(page); await assertFocus(page, '#schedule-item-never');
  assert.equal(await selected('schedule'), 'schedule-item-never');

  await radio(page, 'managed', 'weekly').focus();
  await pressKeys(page, 'ArrowDown'); await assertFocus(page, '#managed-item-never');
  await pressKeys(page, 'Space');
  assert.equal(await selected('managed'), 'managed-item-weekly');
  await radio(page, 'managed', 'daily').click(); await frames(page);
  await assertFocus(page, '#managed-item-daily');
  await state(page, 'managed', 'weekly');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#rtl-item-daily');
  assert.equal(await radio(page, 'managed', 'weekly').getAttribute('tabindex'), '0');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#managed-item-weekly');

  await radio(page, 'rtl', 'daily').focus();
  await pressKeys(page, 'ArrowRight'); await assertFocus(page, '#rtl-item-never');
  await state(page, 'rtl', 'never');
  await pressKeys(page, 'ArrowLeft'); await assertFocus(page, '#rtl-item-daily');
  await pressKeys(page, 'ArrowDown'); await assertFocus(page, '#rtl-item-weekly');
  await pressKeys(page, 'ArrowUp'); await assertFocus(page, '#rtl-item-daily');

  await pressKeys(page, 'Tab'); await assertFocus(page, '#paused-item-daily');
  const before = await page.locator('#activity').textContent();
  await pressKeys(page, 'ArrowDown', 'ArrowRight', 'Space');
  await radio(page, 'paused', 'never').dispatchEvent('click'); await frames(page);
  assert.equal(await page.locator('#activity').textContent(), before);
  assert.equal(await page.locator('input[name=schedule-3]').isDisabled(), true);
  await pressKeys(page, 'Tab'); await assertFocus(page, '#after');

  await page.locator('#remove').click(); await frames(page);
  await state(page, 'schedule', null);
  assert.equal(await radio(page, 'schedule', 'never').count(), 0);
  assert.equal(await page.locator('input[name=schedule-0]').count(), 0);
  await page.locator('#restore').click(); await frames(page);
  await page.locator('#invalid').uncheck(); await frames(page);
  assert.equal(await page.locator('#schedule').getAttribute('aria-invalid'), null);
  await state(page, 'schedule', null);
  await radio(page, 'schedule', 'daily').click(); await frames(page);
  await page.locator('#clear').click(); await frames(page);
  await state(page, 'schedule', null);
  for (const key of ['daily', 'weekly', 'never']) {
    await radio(page, 'schedule', key).click(); await frames(page);
    await page.locator('#remove').click(); await frames(page);
  }
  await state(page, 'schedule', null);
  assert.equal(await page.locator('#schedule [tabindex="0"]').count(), 0);
  await page.locator('#empty').click(); await frames(page);
  await state(page, 'schedule', null);
  assert.equal(await page.locator('#schedule [role=radio]').count(), 0);
  await page.locator('#restore').click(); await frames(page);
  await state(page, 'schedule', null);
  await radio(page, 'schedule', 'daily').focus();
  await radio(page, 'schedule', 'daily').evaluate(el => {
    el.dispatchEvent(new KeyboardEvent('keydown', {key: 'ArrowRight', bubbles: true, cancelable: true}));
    document.querySelector('#after').focus();
  });
  await frames(page); await assertFocus(page, '#after');
  await state(page, 'schedule', 'weekly');
}
