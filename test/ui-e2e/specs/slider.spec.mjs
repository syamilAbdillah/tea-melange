import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'slider');
  const thumb = (id, key) => page.locator(`#${id}-item-${key}`);
  const volume = thumb('volume', 'volume');
  const lower = thumb('price', 'minimum');
  const upper = thumb('price', 'maximum');
  const vertical = thumb('vertical', 'temperature');
  const rtl = thumb('rtl', 'balance');
  const disabled = thumb('disabled', 'locked');
  const value = async locator => Number(await locator.getAttribute('aria-valuenow'));
  const activity = () => page.locator('#activity').textContent();
  const snapshot = async (locator, label, unit, disabled = false) => {
    const current = await value(locator);
    await ariaSnapshot(locator, `- slider "${label}"${disabled ? ' [disabled]' : ''}: "${current}"`);
    assert.equal(await locator.getAttribute('aria-valuetext'), `${current} ${unit}`);
  };
  const audit = async () => {
    await snapshot(volume, 'Volume', 'percent');
    await snapshot(lower, 'Minimum price', 'dollars');
    await snapshot(upper, 'Maximum price', 'dollars');
    await snapshot(vertical, 'Temperature', 'degrees');
    await snapshot(rtl, 'Balance', 'percent');
    await snapshot(disabled, 'Locked volume', 'percent', true);
    await axe(page);
  };
  await audit();
  // Native tab order includes each range thumb, never the disabled thumb.
  await page.locator('#reset').focus();
  for (const target of [volume, lower, upper, vertical, rtl]) {
    await pressKeys(page, 'Tab');
    assert.equal(await target.evaluate(e => e === document.activeElement), true);
  }
  for (const target of [vertical, upper, lower, volume]) {
    await pressKeys(page, 'Shift+Tab');
    assert.equal(await target.evaluate(e => e === document.activeElement), true);
  }
  const key = async (target, key, expected) => {
    await target.focus(); await pressKeys(page, key);
    assert.equal(await value(target), expected, key);
    assert.equal(await target.evaluate(e => e === document.activeElement), true, 'keyboard retains focus');
    await audit();
  };
  await key(volume, 'ArrowRight', 40.1);
  await key(volume, 'ArrowUp', 40.2);
  await key(volume, 'ArrowLeft', 40.1);
  await key(volume, 'ArrowDown', 40);
  await key(volume, 'PageUp', 41);
  await key(volume, 'PageDown', 40);
  await key(volume, 'Shift+ArrowRight', 41);
  await key(volume, 'Shift+ArrowLeft', 40);
  await key(volume, 'Home', 0);
  await key(volume, 'End', 100);
  const beforeBound = await activity();
  await pressKeys(page, 'End', 'ArrowRight');
  assert.equal(await activity(), beforeBound, 'bounds do not emit');
  await key(volume, 'Home', 0);
  const beforeShortcut = await activity();
  await pressKeys(page, 'Control+ArrowRight', 'Alt+ArrowUp', 'Meta+End');
  assert.equal(await activity(), beforeShortcut);
  // Same-frame events read the latest submodel, not the rendered value.
  await volume.evaluate(e => {
    e.dispatchEvent(new KeyboardEvent('keydown', { key: 'ArrowUp', bubbles: true, cancelable: true }));
    e.dispatchEvent(new KeyboardEvent('keydown', { key: 'ArrowUp', bubbles: true, cancelable: true }));
  }); await frames(page);
  assert.equal(await value(volume), 0.2);
  await audit();
  await key(lower, 'End', 70);
  assert.equal(await upper.getAttribute('aria-valuemin'), '80');
  await key(upper, 'Home', 80);
  await key(upper, 'End', 100);
  assert.equal(await lower.getAttribute('aria-valuemax'), '90');
  await key(lower, 'End', 90);
  await key(lower, 'Home', 0);
  await lower.focus(); await pressKeys(page, 'Tab');
  assert.equal(await upper.evaluate(e => e === document.activeElement), true, 'range tab order remains constant');
  await key(vertical, 'ArrowUp', 51);
  await key(vertical, 'ArrowDown', 50);
  await key(vertical, 'ArrowRight', 51);
  await key(vertical, 'ArrowLeft', 50);
  await key(vertical, 'PageUp', 60);
  await key(vertical, 'PageDown', 50);
  await key(vertical, 'Home', 0);
  await key(vertical, 'End', 100);
  await key(rtl, 'ArrowRight', 39);
  await key(rtl, 'ArrowLeft', 40);
  await key(rtl, 'ArrowUp', 41);
  await key(rtl, 'ArrowDown', 40);
  await key(rtl, 'PageUp', 60);
  await key(rtl, 'PageDown', 40);
  await key(rtl, 'Home', 0);
  await key(rtl, 'End', 100);
  // Pointer track picking, document-level drag, release outside the track.
  await page.locator('#reset').click(); await frames(page);
  const box = async id => {
    const track = page.locator(`#${id}-track`);
    await track.scrollIntoViewIfNeeded(); return track.boundingBox();
  };
  const drag = async (id, from, to, target, expected, verticalAxis = false) => {
    const rect = await box(id);
    const point = fraction => verticalAxis ? [rect.x + rect.width / 2, rect.y + rect.height * (1 - fraction)] : [rect.x + rect.width * fraction, rect.y + rect.height / 2];
    await page.mouse.move(...point(from)); await page.mouse.down(); await frames(page);
    assert.equal(await target.getAttribute('data-dragging'), '');
    assert.equal(await page.locator(`#${id}-track`).evaluate(e => e.hasPointerCapture(1)), true, 'native mouse pointer capture');
    await audit();
    await page.mouse.move(...point(to), {steps: 4}); await frames(page);
    await page.mouse.up(); await frames(page);
    assert.equal(await value(target), expected);
    assert.equal(await target.getAttribute('data-dragging'), null);
    assert.equal(await page.locator(`#${id}-track`).evaluate(e => e.hasPointerCapture(1)), false, 'capture released');
    assert.equal(await target.evaluate(e => e === document.activeElement), true);
    await audit();
  };
  await drag('volume', .4, .75, volume, 75);
  let rect = await box('volume');
  await page.mouse.click(rect.x + rect.width * .2, rect.y + rect.height / 2); await frames(page);
  assert.equal(await value(volume), 20); await assertFocus(page, '#volume-item-volume');
  await audit();
  await drag('volume', .2, 1.2, volume, 100);
  rect = await box('volume');
  await page.mouse.move(rect.x + rect.width * .5, rect.y + rect.height / 2);
  await page.mouse.down(); await frames(page);
  assert.equal(await value(volume), 50);
  // A subsequent event processes pending capture into actual capture first.
  await page.mouse.move(rect.x + rect.width * .55, rect.y + rect.height / 2); await frames(page);
  const beforeLostCapture = await value(volume);
  assert.equal(await page.locator('#volume-track').evaluate(e => e.hasPointerCapture(1)), true);
  await page.locator('#volume-track').evaluate(e => e.releasePointerCapture(1));
  // Chromium processes pending capture changes at the next pointer event.
  await page.mouse.move(rect.x + rect.width * .6, rect.y + rect.height / 2); await frames(page);
  assert.equal(await volume.getAttribute('data-dragging'), null, 'lost capture terminates drag');
  assert.equal(await value(volume), beforeLostCapture);
  await page.mouse.up(); await frames(page);
  await audit();
  await drag('price', .2, .95, lower, 70);
  assert.equal(await upper.getAttribute('aria-valuemin'), '80');
  await drag('price', .8, .05, upper, 80);
  await drag('vertical', .5, .25, vertical, 25, true);
  await drag('rtl', .6, .8, rtl, 20);
  const beforeDisabled = await activity();
  rect = await box('disabled');
  await page.mouse.click(rect.x + rect.width * .9, rect.y + rect.height / 2); await frames(page);
  await disabled.focus(); await pressKeys(page, 'ArrowRight', 'ArrowUp', 'Home', 'End', 'PageUp', 'PageDown');
  assert.equal(await value(disabled), 40);
  assert.equal(await activity(), beforeDisabled);
  assert.equal(await disabled.getAttribute('tabindex'), '-1');
  const form = await page.locator('form').evaluate(e => Object.fromEntries(new FormData(e)));
  assert.equal(form['price[minimum]'], '70');
  assert.equal(form['price[maximum]'], '80');
  assert.equal(form['value-4'], undefined, 'disabled fields do not submit');
  await audit();
  // Real touch input, not synthetic PointerEvents: no page panning while dragging.
  const cdp = await context.newCDPSession(page);
  await cdp.send('Emulation.setTouchEmulationEnabled', {enabled: true, maxTouchPoints: 1});
  rect = await box('volume');
  const scroll = await page.evaluate(() => scrollY);
  const touch = (x, y) => ({x, y, radiusX: 1, radiusY: 1, force: 1, id: 1});
  const x = rect.x + rect.width * .3, y = rect.y + rect.height / 2;
  await cdp.send('Input.dispatchTouchEvent', {type: 'touchStart', touchPoints: [touch(x, y)]}); await frames(page);
  await cdp.send('Input.dispatchTouchEvent', {type: 'touchMove', touchPoints: [touch(rect.x + rect.width * .7, y - 45)]}); await frames(page);
  await cdp.send('Input.dispatchTouchEvent', {type: 'touchEnd', touchPoints: []}); await frames(page);
  assert.equal(await value(volume), 70);
  assert.equal(await page.evaluate(() => scrollY), scroll, 'touch slider drag does not scroll page');
  await audit();
  await cdp.detach();
}
