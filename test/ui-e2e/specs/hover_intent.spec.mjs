import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'hover_intent');
  const panel = id => page.locator('#' + id + '-content');
  const opened = async id => (await page.locator('#' + id).getAttribute('data-open')) !== null;
  const state = async (id, value) => {
    await page.waitForFunction(({ id, value }) => document.getElementById(id).hasAttribute('data-open') === value, { id, value });
    await frames(page);
  };
  const move = async id => {
    const target = page.locator('#' + id);
    await target.scrollIntoViewIfNeeded();
    const box = await target.boundingBox();
    assert.ok(box, id + ' has a pointer target');
    await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2);
  };
  const away = () => page.mouse.move(5, 5);
  // Measure real browser time from enter to the first rendered open, rather
  // than imposing a narrow CI-dependent upper deadline on normal delays.
  await page.evaluate(() => {
    window.hoverTrace = {};
    for (const id of ['card', 'instant', 'group-a', 'group-b', 'disabled']) {
      const element = document.getElementById(id);
      element.addEventListener('pointerenter', event => {
        if (event.pointerType !== 'touch') window.hoverTrace[id] = { enter: performance.now() };
      });
      new MutationObserver(() => {
        const trace = window.hoverTrace[id];
        if (trace && element.hasAttribute('data-open') && trace.open === undefined) trace.open = performance.now();
      }).observe(element, { attributes: true, attributeFilter: ['data-open'] });
    }
    window.hoverKeys = [];
    document.addEventListener('keydown', event => window.hoverKeys.push({ key: event.key, prevented: event.defaultPrevented }));
  });
  const elapsed = id => page.evaluate(id => window.hoverTrace[id].open - window.hoverTrace[id].enter, id);
  const activity = () => page.locator('#activity').textContent();

  await ariaSnapshot(page.locator('#card'), '- button "Mountain tea"');
  await ariaSnapshot(panel('card'), '');
  await ariaSnapshot(page.locator('#disabled'), '- button "Seasonal reserve" [disabled]');
  await axe(page);

  await move('card');
  assert.equal(await opened('card'), false, 'cold enter does not open synchronously');
  await ariaSnapshot(page.locator('#card'), '- button "Mountain tea"');
  await state('card', true);
  assert.ok(await elapsed('card') >= 420, 'configured 450 ms open delay is respected');
  await ariaSnapshot(page.locator('#card'), '- button "Mountain tea" [expanded]');
  await ariaSnapshot(panel('card'), '- region "Mountain tea":\n  - paragraph: A cool, mineral cup from the high slopes.');
  assert.equal(await page.evaluate(() => document.activeElement.tagName), 'BODY', 'pointer hover never moves focus');
  await axe(page);

  // Cross an actual 12 px gap to floating content, then back. Remaining there
  // longer than the close delay proves the old leave timer was cancelled.
  await move('card-content');
  await page.waitForTimeout(420);
  assert.equal(await opened('card'), true, 'trigger-to-content grace retains intent');
  await axe(page);
  await move('card');
  await page.waitForTimeout(420);
  assert.equal(await opened('card'), true, 'content-to-trigger grace retains intent');
  assert.match(await activity(), /Pointer opens: 1\. Pointer closes: 0\./);
  await away();
  assert.equal(await opened('card'), true, 'leave is not an immediate close');
  await ariaSnapshot(page.locator('#card'), '- button "Mountain tea" [expanded]');
  await state('card', false);
  await ariaSnapshot(panel('card'), '');
  assert.match(await activity(), /Pointer opens: 1\. Pointer closes: 1\./);
  await axe(page);

  // Enter/leave within one frame cannot let a delayed callback revive content.
  await move('card'); await away();
  await page.waitForTimeout(600);
  assert.equal(await opened('card'), false, 'abandoned opening is cancelled');
  assert.match(await activity(), /Pointer opens: 1\. Pointer closes: 1\./);
  await move('card'); await away(); await move('card');
  await state('card', true);
  assert.ok(await elapsed('card') >= 420, 'rapid reenter starts a fresh opening generation');
  await away(); await move('card');
  await page.waitForTimeout(420);
  assert.equal(await opened('card'), true, 'rapid leave/reenter cancels close');
  await away(); await state('card', false);

  await move('instant'); await state('instant', true);
  assert.ok(await elapsed('instant') < 400, 'zero opening delay is immediate');
  await ariaSnapshot(page.locator('#instant'), '- button "Evening blend" [expanded]');
  await axe(page);
  await away(); await state('instant', false);
  await ariaSnapshot(page.locator('#instant'), '- button "Evening blend"');
  await axe(page);

  const disabledBefore = await activity();
  await move('disabled'); await page.waitForTimeout(600);
  assert.equal(await opened('disabled'), false, 'disabled native trigger emits no hover intent');
  const disabledBox = await page.locator('#disabled').boundingBox();
  await page.mouse.click(disabledBox.x + 5, disabledBox.y + 5);
  assert.equal(await activity(), disabledBefore);
  await ariaSnapshot(page.locator('#disabled'), '- button "Seasonal reserve" [disabled]');
  await axe(page);
  await away();

  await move('group-a'); await state('group-a', true);
  assert.ok(await elapsed('group-a') >= 1170, 'cold group uses its full delay');
  await ariaSnapshot(page.locator('#group-a'), '- button "Steep" [expanded]');
  await axe(page);
  await away(); await state('group-a', false);
  // Immediately after the parent's recorded close, a different member is warm.
  await move('group-b'); await state('group-b', true);
  assert.ok(await elapsed('group-b') < 1000, 'immutable parent-owned group skips 1200 ms delay');
  await ariaSnapshot(page.locator('#group-b'), '- button "Pour" [expanded]');
  await axe(page);
  await away(); await state('group-b', false);
  await page.waitForTimeout(1950);
  await move('group-a');
  assert.equal(await opened('group-a'), false, 'group becomes cold again');
  await state('group-a', true);
  assert.ok(await elapsed('group-a') >= 1170, 'expired warm-up restores full delay');
  await away(); await state('group-a', false);
  await axe(page);

  // Every keyboard-table row is consumer-owned: this example composes native
  // focus, activation and Escape. None increments the pointer-intent counters.
  const beforeKeyboard = await activity();
  await page.locator('#card').focus(); await frames(page);
  await assertFocus(page, '#card');
  assert.equal(await opened('card'), true, 'consumer focus calls open_now immediately');
  await pressKeys(page, 'Escape');
  await assertFocus(page, '#card');
  await ariaSnapshot(page.locator('#card'), '- button "Mountain tea"');
  await page.waitForTimeout(600);
  assert.equal(await opened('card'), false, 'close_now cannot be revived by an old timer');
  await axe(page);
  await pressKeys(page, 'Enter');
  await ariaSnapshot(page.locator('#card'), '- button "Mountain tea" [expanded]');
  await assertFocus(page, '#card');
  await axe(page);
  await pressKeys(page, 'Escape', 'Space');
  assert.equal(await opened('card'), true, 'consumer native Space activation');
  await page.locator('#card').dispatchEvent('keydown', { key: 'Escape', isComposing: true, bubbles: true, cancelable: true });
  await frames(page);
  assert.equal(await opened('card'), true, 'IME Escape is untouched even by the consumer keyboard hook');
  await assertFocus(page, '#card');
  await pressKeys(page, 'Tab');
  await assertFocus(page, '#instant');
  assert.equal(await opened('card'), false, 'consumer blur closes previous card');
  await pressKeys(page, 'Shift+Tab');
  await assertFocus(page, '#card');
  await pressKeys(page, 'Escape', 'ArrowDown', 'Control+ArrowDown', 'Alt+ArrowDown', 'Meta+ArrowDown');
  assert.equal(await opened('card'), false);
  assert.equal(await activity(), beforeKeyboard, 'programmatic focus/keyboard setters never emit pointer intents');
  assert.ok(await page.evaluate(() => window.hoverKeys.every(event => !event.prevented)), 'Tab, modifiers and other native keyboard defaults are never prevented');
  await axe(page);

  // Hold a real emulated touch longer than the open delay, then cancel before
  // the browser's tap default focuses the button (focus belongs to consumer).
  await page.locator('#card').evaluate(element => element.blur());
  await away();
  const session = await context.newCDPSession(page);
  await session.send('Emulation.setTouchEmulationEnabled', { enabled: true, maxTouchPoints: 1 });
  const box = await page.locator('#card').boundingBox();
  const beforeTouch = await activity();
  await session.send('Input.dispatchTouchEvent', { type: 'touchStart', touchPoints: [{ x: box.x + box.width / 2, y: box.y + box.height / 2 }] });
  await page.waitForTimeout(650);
  assert.equal(await opened('card'), false, 'trusted touch pointerenter never opens hover');
  await session.send('Input.dispatchTouchEvent', { type: 'touchCancel', touchPoints: [] });
  await frames(page);
  assert.equal(await activity(), beforeTouch, 'touch causes no hover intents');
  await ariaSnapshot(page.locator('#card'), '- button "Mountain tea"');
  await axe(page);
  await session.send('Emulation.setTouchEmulationEnabled', { enabled: false });
  await session.detach();

  // Crossing the viewport boundary generates a real null-relatedTarget exit.
  await move('card'); await state('card', true);
  await page.mouse.move(-10, -10);
  await state('card', false);
  await ariaSnapshot(page.locator('#card'), '- button "Mountain tea"');
  await axe(page);
}
