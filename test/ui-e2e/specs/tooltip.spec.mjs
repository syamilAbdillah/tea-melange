import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'tooltip');
  const cdp = await context.newCDPSession(page);
  async function ax(selector) {
    const { result } = await cdp.send('Runtime.evaluate', {
      expression: `document.querySelector(${JSON.stringify(selector)})`,
    });
    try {
      const { nodes } = await cdp.send('Accessibility.getPartialAXTree', { objectId: result.objectId, fetchRelatives: false });
      return nodes[0];
    } finally { await cdp.send('Runtime.releaseObject', { objectId: result.objectId }); }
  }
  const tip = id => page.locator(`#${id}-tooltip`);
  const trigger = id => page.locator(`#${id}-trigger`);
  async function hidden(id) {
    await page.waitForFunction(id => document.getElementById(`${id}-tooltip`).hidden, id);
    assert.equal(await tip(id).evaluate(el => el.matches(':popover-open')), false, 'closed popover released');
  }
  async function visible(id) {
    await tip(id).waitFor({ state: 'visible' });
    assert.equal(await tip(id).evaluate(el => el.hidden), false);
    assert.equal(await tip(id).evaluate(el => el.matches(':popover-open')), true, 'native top layer');
  }
  await ariaSnapshot(trigger('cut'), '- button "Cut selection"');
  await ariaSnapshot(trigger('copy'), '- button "Copy selection"');
  assert.equal((await ax('#cut-trigger')).name.value, 'Cut selection', 'hidden Label text supplies accessible name');
  assert.equal((await ax('#help-trigger')).name.value, 'Project name');
  assert.equal((await ax('#help-trigger')).description.value, 'Use a name your team will recognize.', 'hidden description computes');
  await axe(page);

  // Cold hover is delayed; brief accidental hover is physically cancelled.
  await trigger('cut').hover();
  await page.waitForTimeout(100);
  assert.equal(await tip('cut').evaluate(el => el.hidden), true);
  await page.mouse.move(2, 2);
  await page.waitForTimeout(650);
  await hidden('cut');
  const cold = Date.now();
  await trigger('cut').hover();
  await visible('cut');
  assert.ok(Date.now() - cold >= 430, 'cold opening respects 600ms delay with tolerance');
  await ariaSnapshot(tip('cut'), '- tooltip "Cut selection"');
  assert.equal((await ax('#cut-trigger')).name.value, 'Cut selection');
  await axe(page);
  await tip('cut').hover();
  await page.waitForTimeout(250);
  await visible('cut');
  await trigger('cut').hover();
  await visible('cut');
  // Defaults: 600ms open, 150ms close and an 8px placement gap. The first
  // member must still be open when its sibling receives pointerenter: warm-up
  // comes from the recorded open member, not an already recorded close.
  const triggerBox = await trigger('cut').boundingBox();
  const tipBox = await tip('cut').boundingBox();
  assert.ok(Math.abs(triggerBox.y - (tipBox.y + tipBox.height) - 8) < 1, 'default placement gap');
  await trigger('copy').evaluate(el => el.addEventListener('pointerenter', () => {
    el.dataset.previousOpen = String(document.querySelector('#cut-trigger').hasAttribute('data-open'));
  }, { once: true }));
  const warm = Date.now();
  await trigger('copy').hover();
  await visible('copy');
  assert.ok(Date.now() - warm < 450, 'warm sibling skips the 600ms delay');
  assert.equal(await trigger('copy').getAttribute('data-previous-open'), 'true', 'switch happens inside the previous close grace');
  assert.equal(await tip('cut').evaluate(el => el.hidden), true, 'takeover hides the previous member in the same render');
  await hidden('cut');
  await ariaSnapshot(tip('copy'), '- tooltip "Copy selection"');
  await axe(page);
  await page.mouse.move(2, 2);
  await hidden('copy');
  await page.waitForTimeout(200);
  const reentry = Date.now();
  await trigger('cut').hover();
  await visible('cut');
  assert.ok(Date.now() - reentry < 450, 're-entry within the skip window is immediate');
  assert.equal(await tip('copy').evaluate(el => el.hidden), true);
  await page.mouse.move(2, 2);
  await hidden('cut');
  await page.waitForTimeout(1850);
  const expired = Date.now();
  await trigger('cut').hover();
  await visible('cut');
  assert.ok(Date.now() - expired >= 430, 'group warm-up expires');
  // WCAG dismissal works even if the hovered trigger has never had focus.
  await pressKeys(page, 'Escape');
  await hidden('cut');
  await page.mouse.move(2, 2);

  // Mouse click focus alone is not keyboard focus, even after waiting.
  await trigger('help').click();
  await page.mouse.move(2, 2);
  await page.waitForTimeout(750);
  await assertFocus(page, '#help-trigger');
  await hidden('help');
  await axe(page);
  await page.locator('#start').focus();
  const keyboardFocus = Date.now();
  await pressKeys(page, 'Tab'); await assertFocus(page, '#cut-trigger'); await visible('cut');
  assert.ok(Date.now() - keyboardFocus < 450, 'keyboard focus skips the hover delay');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#copy-trigger'); await visible('copy'); await hidden('cut');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#help-trigger'); await visible('help'); await hidden('copy');
  await ariaSnapshot(trigger('help'), '- textbox "Project name"');
  await ariaSnapshot(tip('help'), '- tooltip "Use a name your team will recognize."');
  assert.equal((await ax('#help-trigger')).description.value, 'Use a name your team will recognize.');
  await axe(page);
  await pressKeys(page, 'Control+Escape', 'Alt+Escape', 'Meta+Escape', 'ArrowDown');
  await visible('help');
  const composition = await trigger('help').evaluate(el => el.dispatchEvent(new KeyboardEvent('keydown', { key: 'Escape', isComposing: true, bubbles: true, cancelable: true })));
  assert.equal(composition, true); await visible('help');
  await pressKeys(page, 'Escape'); await hidden('help'); await assertFocus(page, '#help-trigger');
  await pressKeys(page, 'Shift+Tab'); await visible('copy'); await assertFocus(page, '#copy-trigger');
  await pressKeys(page, 'Escape'); await hidden('copy');
  // Native activation remains intact; Tooltip neither handles Enter nor Space.
  await pressKeys(page, 'Enter', 'Space');
  assert.match(await page.locator('#activity').textContent(), /Actions: 2/);
  await page.locator('#start').focus(); await frames(page);

  // Real motion, arrow geometry, hoverability and persistence.
  await page.emulateMedia({ reducedMotion: 'no-preference' });
  await trigger('animated').hover(); await visible('animated');
  await page.waitForFunction(() => !document.querySelector('#animated-tooltip').hasAttribute('data-transition'));
  assert.match(await tip('animated').getAttribute('data-placement'), /^(top|bottom)-center$/);
  const arrow = tip('animated').locator('.arrow');
  assert.ok(parseFloat(await arrow.evaluate(el => el.style.getPropertyValue('--tea-arrow-offset'))) >= 0);
  await tip('animated').hover();
  await page.waitForTimeout(850);
  await visible('animated');
  await ariaSnapshot(tip('animated'), '- tooltip "Only invited teammates can see this project."');
  await axe(page);
  await page.mouse.move(2, 2);
  await page.waitForTimeout(65);
  assert.equal(await tip('animated').evaluate(el => el.hidden), false, '150ms closing grace');
  await trigger('animated').hover();
  await visible('animated');
  await page.mouse.move(2, 2);
  await hidden('animated');
  await axe(page);
  // Disabling the tooltip is distinct from disabling its native trigger.
  await page.locator('#disable').check(); await frames(page);
  await trigger('animated').hover(); await page.waitForTimeout(700); await hidden('animated');
  assert.equal(await trigger('animated').isEnabled(), true);
  await axe(page);
  await page.locator('#disable').uncheck();
  await page.mouse.move(2, 2);
  await trigger('disabled').hover(); await visible('disabled');
  await ariaSnapshot(tip('disabled'), '- tooltip "Add a project name before publishing."');
  assert.equal(await trigger('disabled').locator('button').isDisabled(), true);
  await axe(page);
  await pressKeys(page, 'Escape'); await hidden('disabled');
  await page.mouse.move(2, 2);
  await page.locator('#disable').focus(); await pressKeys(page, 'Tab');
  await assertFocus(page, '#disabled-trigger'); await visible('disabled');
  await pressKeys(page, 'Tab'); await hidden('disabled');

  // A native modal Dialog must survive the tooltip's claimed Escape.
  await page.locator('#open-dialog').click();
  const dialog = page.locator('#settings-dialog');
  await dialog.waitFor({ state: 'visible' });
  await page.waitForFunction(() => document.activeElement?.id === 'settings-close');
  await assertFocus(page, '#settings-close');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#modal-help-trigger'); await visible('modal-help');
  await ariaSnapshot(trigger('modal-help'), '- button "Settings help"');
  await ariaSnapshot(tip('modal-help'), '- tooltip "Settings help"');
  await axe(page);
  await pressKeys(page, 'Escape'); await hidden('modal-help');
  assert.equal(await dialog.evaluate(el => el.open), true, 'first Escape closes only tooltip');
  await assertFocus(page, '#modal-help-trigger'); await axe(page);
  await pressKeys(page, 'Escape');
  await page.waitForFunction(() => !document.querySelector('#settings-dialog').open);
  await assertFocus(page, '#open-dialog'); await axe(page);

  // Trusted touch never opens a tooltip through compatibility hover or focus.
  await page.mouse.move(2, 2);
  const touch = await cdp.send('Emulation.setTouchEmulationEnabled', { enabled: true });
  void touch;
  const box = await trigger('help').boundingBox();
  await cdp.send('Input.dispatchTouchEvent', { type: 'touchStart', touchPoints: [{ x: box.x + box.width / 2, y: box.y + box.height / 2 }] });
  await cdp.send('Input.dispatchTouchEvent', { type: 'touchEnd', touchPoints: [] });
  await page.waitForTimeout(750); await hidden('help');
  await axe(page);
  await cdp.send('Emulation.setTouchEmulationEnabled', { enabled: false });
  await cdp.detach();
}
