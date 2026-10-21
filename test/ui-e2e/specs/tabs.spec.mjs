import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

const keys = ['profile', 'billing', 'privacy', 'alerts'];
const labels = ['Profile', 'Billing (unavailable)', 'Privacy', 'Alerts'];
const tab = (page, group, key) => page.locator(`#${group}-item-${key}`);
const panel = (page, group, key) => page.locator(`#${group}-panel-item-${key}`);
async function state(page, group, selected) {
  for (let i = 0; i < keys.length; i++) {
    const target = tab(page, group, keys[i]);
    if (!await target.count()) continue;
    const flags = (selected === keys[i] ? ' [selected]' : '') + (i === 1 ? ' [disabled]' : '');
    await ariaSnapshot(target, `- tab "${labels[i]}"${flags}`);
    assert.equal(await panel(page, group, keys[i]).isVisible(), selected === keys[i]);
    const controls = await target.getAttribute('aria-controls');
    assert.equal(await page.locator('#' + controls).count(), 1);
  }
  if (selected) {
  const content = group === 'ha' && selected === 'privacy'
    ? '- link "Read the privacy guide":\n  - /url: "#privacy-help"'
    : `- paragraph: ${labels[keys.indexOf(selected)]} settings are ready to edit.`;
  await ariaSnapshot(panel(page, group, selected), `- tabpanel "${labels[keys.indexOf(selected)]}":\n${content.split('\n').map(line => '  ' + line).join('\n')}`);
  }
  await axe(page);
}

export async function run(context) {
  const page = await openExample(context, 'tabs');
  const selected = group => page.locator(`#${group} [aria-selected=true]`).getAttribute('id');
  for (const group of ['ha', 'hm', 'va', 'vm', 'rtl']) await state(page, group, 'profile');
  assert.equal(await page.locator('#ha').getAttribute('aria-orientation'), 'horizontal');
  assert.equal(await page.locator('#va').getAttribute('aria-orientation'), 'vertical');
  assert.equal(await page.locator('#rtl').getAttribute('aria-label'), 'Right-to-left automatic');

  await tab(page, 'ha', 'profile').focus();
  for (const [key, target] of [
    ['ArrowRight', 'privacy'], ['ArrowRight', 'alerts'], ['ArrowRight', 'profile'],
    ['ArrowLeft', 'alerts'], ['Home', 'profile'], ['End', 'alerts'],
  ]) {
    await pressKeys(page, key); await assertFocus(page, '#ha-item-' + target);
    assert.equal(await selected('ha'), 'ha-item-' + target);
  }
  await state(page, 'ha', 'alerts');
  await pressKeys(page, 'ArrowUp', 'ArrowDown', 'Control+ArrowLeft', 'Alt+Home', 'Meta+End', 'Delete');
  await assertFocus(page, '#ha-item-alerts');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#ha-panel-item-alerts');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#ha-item-alerts');
  await pressKeys(page, 'Home', 'ArrowRight');
  await state(page, 'ha', 'privacy');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#privacy-link');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#ha-item-privacy');
  await tab(page, 'ha', 'profile').click(); await frames(page);
  await tab(page, 'ha', 'profile').evaluate(el => {
    for (let i = 0; i < 2; i++) el.dispatchEvent(new KeyboardEvent('keydown', {key: 'ArrowRight', bubbles: true, cancelable: true}));
  });
  await frames(page); await assertFocus(page, '#ha-item-alerts');
  assert.equal(await selected('ha'), 'ha-item-alerts');

  for (const [group, next, previous, orthogonal] of [
    ['hm', 'ArrowRight', 'ArrowLeft', 'ArrowDown'],
    ['vm', 'ArrowDown', 'ArrowUp', 'ArrowRight'],
  ]) {
    await tab(page, group, 'profile').focus();
    await pressKeys(page, next); await assertFocus(page, `#${group}-item-privacy`);
    assert.equal(await selected(group), group + '-item-profile');
    await state(page, group, 'profile');
    await pressKeys(page, 'Enter'); await state(page, group, 'privacy');
    await pressKeys(page, 'End'); await assertFocus(page, `#${group}-item-alerts`);
    assert.equal(await selected(group), group + '-item-privacy');
    await pressKeys(page, 'Space'); await state(page, group, 'alerts');
    await pressKeys(page, next); await assertFocus(page, `#${group}-item-profile`);
    await pressKeys(page, previous); await assertFocus(page, `#${group}-item-alerts`);
    await pressKeys(page, 'Home'); await assertFocus(page, `#${group}-item-profile`);
    await pressKeys(page, previous); await assertFocus(page, `#${group}-item-alerts`);
    await pressKeys(page, orthogonal); await assertFocus(page, `#${group}-item-alerts`);
    await pressKeys(page, 'Home', 'Tab'); await assertFocus(page, `#${group}-panel-item-alerts`);
    await pressKeys(page, 'Shift+Tab'); await assertFocus(page, `#${group}-item-alerts`);
    await tab(page, group, 'privacy').click(); await frames(page);
    await assertFocus(page, `#${group}-item-privacy`);
    assert.equal(await selected(group), group + '-item-privacy');
    const disabledBox = await tab(page, group, 'billing').boundingBox();
    await page.mouse.click(disabledBox.x + disabledBox.width / 2, disabledBox.y + disabledBox.height / 2); await frames(page);
    await assertFocus(page, `#${group}-item-privacy`);
    assert.equal(await selected(group), group + '-item-privacy');
  }
  assert.equal(await panel(page, 'hm', 'profile').textContent(), '');
  assert.notEqual(await panel(page, 'vm', 'profile').textContent(), '');

  await tab(page, 'va', 'profile').focus();
  for (const [key, target] of [
    ['ArrowDown', 'privacy'], ['ArrowDown', 'alerts'], ['ArrowDown', 'profile'],
    ['ArrowUp', 'alerts'], ['Home', 'profile'], ['End', 'alerts'],
  ]) {
    await pressKeys(page, key); await assertFocus(page, '#va-item-' + target);
    assert.equal(await selected('va'), 'va-item-' + target);
  }
  await pressKeys(page, 'ArrowLeft', 'ArrowRight'); await assertFocus(page, '#va-item-alerts');
  await state(page, 'va', 'alerts');
  await tab(page, 'rtl', 'profile').focus();
  await pressKeys(page, 'ArrowRight'); await assertFocus(page, '#rtl-item-alerts');
  await state(page, 'rtl', 'alerts');
  await pressKeys(page, 'ArrowLeft'); await assertFocus(page, '#rtl-item-profile');
  await pressKeys(page, 'ArrowLeft'); await assertFocus(page, '#rtl-item-privacy');
  await state(page, 'rtl', 'privacy');

  const before = await page.locator('#activity').textContent();
  await page.locator('#select').click(); await frames(page);
  assert.equal(await selected('ha'), 'ha-item-alerts');
  assert.equal(await page.locator('#activity').textContent(), before);
  await assertFocus(page, '#select');
  await page.locator('#remove').click(); await frames(page);
  assert.equal(await tab(page, 'ha', 'alerts').count(), 0);
  await state(page, 'ha', 'profile');
  await page.locator('#restore').click(); await frames(page);
  await state(page, 'ha', 'profile');
  for (let i = 0; i < 3; i++) {
    await page.locator('#remove').click(); await frames(page);
  }
  await state(page, 'ha', null);
  assert.equal(await page.locator('#ha [tabindex="0"]').count(), 0);
  await page.locator('#empty').click(); await frames(page);
  await state(page, 'ha', null);
  assert.equal(await page.locator('#ha [role=tab]').count(), 0);
  await page.locator('#restore').click(); await frames(page);
  await state(page, 'ha', 'profile');
  await tab(page, 'ha', 'profile').focus();
  await tab(page, 'ha', 'profile').evaluate(el => {
    el.dispatchEvent(new KeyboardEvent('keydown', {key: 'ArrowRight', bubbles: true, cancelable: true}));
    document.querySelector('#after').focus();
  });
  await frames(page); await assertFocus(page, '#after');
  await state(page, 'ha', 'privacy');
}
