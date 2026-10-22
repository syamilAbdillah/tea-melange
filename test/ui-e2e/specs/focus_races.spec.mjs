import assert from 'node:assert/strict';
import { openExample, frames, assertFocus } from '../lib.mjs';

// Hold the render/effect boundary so native Tab can run in the opening gap.
// This changes scheduling only: focus and keyboard traversal remain Chromium's.
async function holdFrames(page) {
  await frames(page);
  await page.evaluate(() => {
    const request = window.requestAnimationFrame;
    const cancel = window.cancelAnimationFrame;
    const pending = new Map();
    let serial = 1000000;
    window.requestAnimationFrame = callback => { pending.set(++serial, callback); return serial; };
    window.cancelAnimationFrame = id => { if (!pending.delete(id)) cancel.call(window, id); };
    window.releaseFocusRaceFrames = () => {
      window.requestAnimationFrame = request;
      window.cancelAnimationFrame = cancel;
      return new Promise(resolve => request.call(window, stamp => {
        for (const callback of pending.values()) callback(stamp);
        pending.clear();
        resolve();
      }));
    };
  });
}
async function releaseFrames(page) {
  await page.evaluate(() => window.releaseFocusRaceFrames());
  for (let i = 0; i < 8; i++) await frames(page);
}

export async function run(context) {
  for (const [name, trigger, outside, activation] of [
    ['menu', '#file-button', '#after', 'ArrowDown'],
    ['listbox', '#single-button', '#outside', 'ArrowDown'],
    ['date_picker', '#basic-trigger', '#outside', 'click'],
    ['combobox', '#country-control', '#tags-control', 'toggle'],
    ['toast', '#info', '#clear', 'F8'],
  ]) {
    for (const movement of (name === 'date_picker' ? ['Tab', 'Shift+Tab', 'focus'] : ['Tab', 'focus'])) {
      const page = await openExample(context, name);
      await holdFrames(page);
      await page.locator(trigger).focus();
      // Dispatch synchronously without giving a frame to either render or focus.
      await page.evaluate(({trigger, activation}) => {
        const element = document.querySelector(activation === 'toggle' ? '#country-toggle' : trigger);
        element.dispatchEvent(activation === 'click' || activation === 'toggle'
          ? new MouseEvent('click', {bubbles: true, cancelable: true})
          : new KeyboardEvent('keydown', {key: activation, bubbles: true, cancelable: true}));
      }, {trigger, activation});
      if (movement !== 'focus') await page.keyboard.press(movement);
      else await page.locator(outside).focus();
      const destination = await page.evaluate(() => document.activeElement.id);
      assert.notEqual(destination, trigger.slice(1), `${name}: native traversal left the opener`);
      await releaseFrames(page);
      await assertFocus(page, '#' + destination);
      await page.locator(trigger).scrollIntoViewIfNeeded();
      for (let i = 0; i < 4; i++) await frames(page);
      await assertFocus(page, '#' + destination);
      if (name === 'date_picker') assert.equal(await page.locator(trigger).getAttribute('aria-expanded'), 'false');
      await page.close();
    }
  }
}
