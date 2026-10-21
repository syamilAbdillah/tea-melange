import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'toast');
  const toast = key => page.locator('#notifications [data-key="' + key + '"]');
  const count = () => page.locator('#notifications [data-key]').count();
  const settled = async () => {
    await frames(page);
    await page.waitForFunction(() => !document.querySelector('#notifications [data-transition]'));
    await frames(page);
  };
  const send = async id => { await page.locator('#' + id).click(); await settled(); };
  const clear = async () => {
    await page.locator('#clear').click();
    await page.waitForFunction(() => !document.querySelector('#notifications [data-key]'));
    await frames(page);
  };
  await ariaSnapshot(page.locator('#notifications'), '- region "Notifications":\n  - status\n  - alert\n  - paragraph: 0 waiting');
  assert.equal(await page.locator('#notifications-polite').textContent(), '');
  assert.equal(await page.locator('#notifications-assertive').textContent(), '');
  await axe(page);
  await page.evaluate(() => {
    window.toastKeys = [];
    document.querySelector('main').addEventListener('keydown', e => { if (!['Control', 'Alt', 'Meta', 'Shift'].includes(e.key)) window.toastKeys.push(e.key); });
    window.politeNode = document.querySelector('#notifications-polite');
    window.alertNode = document.querySelector('#notifications-assertive');
  });
  await send('info');
  assert.equal(await page.evaluate(() => window.politeNode === document.querySelector('#notifications-polite') && window.alertNode === document.querySelector('#notifications-assertive')), true, 'live regions predate first insertion');
  assert.match(await page.locator('#notifications-polite').textContent(), /Background sync complete/);
  assert.equal(await page.locator('#notifications-assertive').textContent(), '');
  await ariaSnapshot(toast('info'), '- group "Background sync complete":\n  - heading "Background sync complete" [level=3]\n  - paragraph: All files are up to date.\n  - button "Dismiss notification": Dismiss');
  await axe(page);
  await page.evaluate(() => { window.firstToast = document.querySelector('#notifications [data-key="info"]'); });
  await send('update');
  assert.equal(await page.evaluate(() => window.firstToast === document.querySelector('#notifications [data-key="info"]')), true, 'duplicate updates in place');
  assert.match(await page.locator('#notifications-polite').textContent(), /Information updated/);
  assert.equal(await toast('info').getAttribute('data-urgency'), 'normal', 'visible duplicate does not relocate a focusable subtree');
  assert.equal(await count(), 1);
  await page.locator('#dismiss-info').click(); await settled();
  assert.equal(await count(), 0);
  assert.match(await page.locator('#events').textContent(), /Dismissed info Programmatic/);

  await page.locator('#success').focus(); await pressKeys(page, 'Enter'); await settled();
  assert.match(await page.locator('#notifications-polite').textContent(), /Changes saved/);
  await toast('success').getByRole('button', { name: 'Dismiss notification' }).focus();
  await pressKeys(page, 'Space'); await settled();
  assert.equal(await count(), 0);
  assert.match(await page.locator('#events').textContent(), /Dismissed success Close_button/);

  await send('error');
  assert.match(await page.locator('#notifications-assertive').textContent(), /Connection lost/);
  assert.equal(await page.locator('#notifications-polite').textContent(), '');
  await ariaSnapshot(toast('error'), '- group "Connection lost":\n  - heading "Connection lost" [level=3]\n  - paragraph: Your edits are safe. Reconnect before closing this page.\n  - button "Dismiss notification": Dismiss');
  await axe(page);
  await clear();

  await send('action');
  const undo = toast('action').getByRole('button', { name: 'Undo', exact: true });
  assert.match(await undo.getAttribute('aria-description'), /Undo archive/);
  await axe(page);
  await undo.click(); await settled();
  assert.match(await page.locator('#undos').textContent(), /Restored: 1/);
  assert.match(await page.locator('#events').textContent(), /Action action\nDismissed action Action/);
  await page.locator('#undo').click(); await frames(page);
  assert.match(await page.locator('#undos').textContent(), /Restored: 2/, 'equivalent action outside toast');

  await send('persistent'); await send('action');
  await page.locator('#info').focus();
  let before = await page.evaluate(() => window.toastKeys.length);
  await pressKeys(page, 'F8');
  await assertFocus(page, '#notifications-item-action');
  assert.equal(await page.evaluate(() => window.toastKeys.length), before, 'claimed hotkey never reaches ancestor');
  await pressKeys(page, 'Control+Escape', 'Alt+Escape', 'Meta+Escape', 'ArrowDown');
  assert.equal(await count(), 2);
  assert.equal(await page.evaluate(() => window.toastKeys.length), before + 4, 'unclaimed shortcuts reach ancestor');
  await page.locator('#notifications-item-action').dispatchEvent('keydown', { key: 'Escape', isComposing: true, bubbles: true, cancelable: true });
  assert.equal(await count(), 2, 'IME Escape is not claimed');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#notifications-item-action-action-undo');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#notifications-item-action-close');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#info');
  await pressKeys(page, 'F8');
  before = await page.evaluate(() => window.toastKeys.length);
  await pressKeys(page, 'Escape'); await settled();
  await assertFocus(page, '#info');
  assert.equal(await toast('action').count(), 0);
  assert.equal(await page.evaluate(() => window.toastKeys.length), before, 'claimed Escape does not dismiss enclosing overlay');
  assert.match(await page.locator('#events').textContent(), /Dismissed action Escape/);
  await pressKeys(page, 'F8', 'Shift+Tab'); await assertFocus(page, '#info');
  await pressKeys(page, 'F8');
  await page.locator('#after').focus(); await frames(page);
  await assertFocus(page, '#after'); // Intentional non-Tab departures are not stolen.
  await clear();
  await page.locator('#info').focus(); await pressKeys(page, 'F8'); await assertFocus(page, '#notifications');
  await pressKeys(page, 'Escape'); await assertFocus(page, '#info');
  await axe(page);

  await send('burst');
  assert.equal(await count(), 3);
  assert.equal(await page.locator('#notifications [data-queue]').getAttribute('data-queue'), '7');
  assert.match(await page.locator('#notifications-polite').textContent(), /Queued notification 1/);
  assert.doesNotMatch(await page.locator('#notifications-polite').textContent(), /Queued notification 4/);
  assert.deepEqual(await page.locator('#notifications [data-key]').evaluateAll(es => es.map(e => [e.dataset.index, e.style.getPropertyValue('--tea-toast-index')])), [['0', '0'], ['1', '1'], ['2', '2']]);
  await page.locator('#notifications [data-key]').first().getByRole('button', { name: 'Dismiss notification' }).click();
  await settled();
  assert.equal(await count(), 3);
  assert.equal(await page.locator('#notifications [data-queue]').getAttribute('data-queue'), '6');
  assert.match(await page.locator('#notifications-polite').textContent(), /Queued notification 4/);
  await axe(page);
  await clear();

  await page.locator('#alternate').click(); await frames(page);
  await page.locator('#alternate').click(); await frames(page);
  await page.locator('#alternate').click(); await frames(page);
  await page.waitForFunction(() => document.querySelectorAll('#alternative [data-key]').length === 2 && !document.querySelector('#alternative [data-transition]'));
  const alternateKeys = await page.locator('#alternative [data-key]').evaluateAll(es => es.map(e => e.dataset.key));
  assert.ok(Number(alternateKeys[0].split('-')[1]) > Number(alternateKeys[1].split('-')[1]), 'newest first');
  assert.equal(await page.locator('#alternative [data-queue]').getAttribute('data-queue'), '1');
  await page.locator('#alternate').focus(); await pressKeys(page, 'Control+Shift+F9');
  await assertFocus(page, '#alternative-item-' + alternateKeys[0]);
  await pressKeys(page, 'Escape'); await assertFocus(page, '#alternate');
  await axe(page);

  // Swipe is explicitly out of scope: gestures are not half-claimed. Pointer
  // dragging neither dismisses nor prevents ordinary page interaction.
  await send('persistent');
  const box = await toast('persistent').boundingBox();
  await page.mouse.move(box.x + 20, box.y + 20);
  await page.mouse.down(); await page.mouse.move(box.x + 180, box.y + 20, { steps: 5 }); await page.mouse.up();
  assert.equal(await toast('persistent').count(), 1);
  assert.equal(await toast('persistent').getAttribute('data-swipe'), null);
  await clear();

  // Deterministic browser timers (Playwright clock), without replacing Toast.
  // Native keyboard activation avoids actionability waits on a paused rAF.
  const timed = await openExample(context, 'toast');
  const epoch = new Date('2025-01-01T00:00:00Z');
  await timed.clock.install({ time: epoch });
  await timed.clock.pauseAt(new Date(epoch.getTime() + 1000));
  const run = ms => timed.clock.runFor(ms);
  const info = timed.locator('#notifications [data-key="info"]');
  const add = async () => {
    await timed.mouse.move(0, 0);
    await timed.locator('#info').focus(); await timed.keyboard.press('Enter'); await run(200);
    assert.equal(await info.count(), 1);
  };
  const over = async () => {
    const r = await info.boundingBox();
    await timed.mouse.move(r.x + 20, r.y + 20);
  };
  await add(); await run(1000); await over(); await run(10000);
  assert.equal(await info.count(), 1, 'hover pauses timeout');
  await timed.mouse.move(0, 0); await run(3500);
  assert.equal(await info.count(), 1, 'resume preserves remaining time');
  await run(800); assert.equal(await info.count(), 0, 'resume does not reset full duration');
  assert.match(await timed.locator('#events').textContent(), /Dismissed info Timeout/);

  await add(); await run(1000); await timed.locator('#notifications-item-info-close').focus(); await run(10000);
  assert.equal(await info.count(), 1, 'focus pauses timeout');
  await timed.locator('#after').focus(); await run(3500); assert.equal(await info.count(), 1);
  await run(800); assert.equal(await info.count(), 0);

  const visible = hidden => timed.evaluate(hidden => {
    Object.defineProperty(document, 'hidden', { configurable: true, value: hidden });
    Object.defineProperty(document, 'visibilityState', { configurable: true, value: hidden ? 'hidden' : 'visible' });
    document.dispatchEvent(new Event('visibilitychange'));
  }, hidden);
  await add(); await run(1000); await visible(true); await run(10000);
  assert.equal(await info.count(), 1, 'visibility source pauses timeout');
  await visible(false); await run(3500); assert.equal(await info.count(), 1);
  await run(800); assert.equal(await info.count(), 0);

  await timed.locator('#action').focus(); await timed.keyboard.press('Enter'); await run(20000);
  assert.equal(await timed.locator('#notifications [data-key="action"]').count(), 1, 'action toast remains persistent');
  await timed.evaluate(() => { delete document.hidden; delete document.visibilityState; });
  await timed.clock.resume();
  await axe(timed);

  // Real CSS transition remains mounted until Left, with reduced motion off.
  await page.emulateMedia({ reducedMotion: 'no-preference' });
  await send('persistent');
  await page.evaluate(() => {
    window.toastPhases = [];
    const root = document.querySelector('#notifications [data-key="persistent"]');
    const observer = new MutationObserver(() => {
      if (root.hasAttribute('data-leave')) {
        window.toastPhases.push({ connected: root.isConnected, hidden: root.hidden, disabled: root.querySelector('button').disabled });
      }
    });
    observer.observe(root, { attributes: true });
    root.addEventListener('transitionend', () => { window.toastTransitionEnded = true; observer.disconnect(); }, { once: true });
  });
  await toast('persistent').getByRole('button', { name: 'Dismiss notification' }).click();
  await page.waitForFunction(() => {
    const root = document.querySelector('#notifications [data-key="persistent"][data-leave]');
    if (!root) return false;
    const animations = root.getAnimations();
    if (!animations.length) return false;
    window.toastLeaving = animations;
    animations.forEach(animation => animation.pause());
    return true;
  });
  await ariaSnapshot(toast('persistent'), '- group "Export ready":\n  - heading "Export ready" [level=3]\n  - paragraph: Download it from the library whenever you are ready.\n  - button "Dismiss notification" [disabled]: Dismiss');
  await axe(page);
  await page.evaluate(() => window.toastLeaving.forEach(animation => animation.play()));
  await page.waitForFunction(() => !document.querySelector('#notifications [data-key="persistent"]'));
  assert.ok(await page.evaluate(() => window.toastTransitionEnded), 'waits for real CSS leave transition');
  const phases = await page.evaluate(() => window.toastPhases);
  assert.ok(phases.length && phases.every(p => p.connected && !p.hidden && p.disabled), 'leaving remains mounted and non-activatable');
  await axe(page);
}
