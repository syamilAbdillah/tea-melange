import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

const names = {
  poster: 'Edition poster', cover: 'Book cover', proof: 'Proof awaiting approval',
  labels: 'Parcel labels', sample: 'Reference sample',
  tissue: 'Tissue paper', sleeve: 'Print sleeve', backing: 'Backing board',
  corners: 'Corner protectors', wrap: 'Paper wrap', seal: 'Studio seal',
  note: 'Thank-you note', invoice: 'Invoice', mailer: 'Rigid mailer',
  tape: 'Paper tape', address: 'Address label', stamp: 'Postage stamp',
  cyan: 'Cyan ink', magenta: 'Magenta ink', yellow: 'Yellow ink', black: 'Black ink',
  red: 'Red ink', blue: 'Blue ink', green: 'Green ink', white: 'White ink',
  etching: 'Etching', linocut: 'Linocut', screen: 'Screen print',
};
const initial = {
  board: { queue: ['poster', 'cover', 'proof'], making: ['labels'], archive: ['sample'] },
  packing: { packing: ['tissue', 'sleeve', 'backing', 'corners', 'wrap', 'seal', 'note', 'invoice', 'mailer', 'tape', 'address', 'stamp'] },
  horizontal: { upper: ['cyan', 'magenta', 'yellow'], lower: ['black'] },
  rtl: { upper: ['red', 'blue', 'green'], lower: ['white'] },
  'rtl-board': { first: ['etching', 'linocut'], second: ['screen'] },
};
const listNames = {
  board: { queue: 'Queue', making: 'Making', archive: 'Archive (locked)' },
  packing: { packing: 'Packing order' },
  horizontal: { upper: 'Upper shelf', lower: 'Lower shelf' },
  rtl: { upper: 'RTL upper shelf', lower: 'RTL lower shelf' },
  'rtl-board': { first: 'First RTL column', second: 'Second RTL column' },
};
const section = (page, id) => page.locator(`#${id}-example`);
const item = (page, id, key) => section(page, id).locator(`[data-key="${key}"]`);
const handle = (page, id, key) => item(page, id, key).locator('button');
const list = (page, id, key) => section(page, id).locator(`[data-container="${key}"]`);
const live = (page, id) => section(page, id).locator('[aria-live]');
const handleSelector = (id, key) => `#${id}-example [data-key="${key}"] button`;

async function snapshot(page, id, state = initial[id]) {
  for (const [key, keys] of Object.entries(state)) {
    const yaml = [`- list "${listNames[id][key]}":`, ...keys.flatMap(key => [
      '  - listitem:',
      `    - text: ${names[key]}`,
      `    - button "Move ${names[key]}"${['proof', 'sample'].includes(key) ? ' [disabled]' : ''}`,
    ])].join('\n');
    await ariaSnapshot(list(page, id, key), yaml);
    assert.deepEqual(await list(page, id, key).locator('li').evaluateAll(nodes => nodes.map(node => node.dataset.key)), keys);
  }
  assert.equal(await page.locator('[role="listbox"], [role="option"], [aria-grabbed], [aria-dropeffect]').count(), 0);
}
async function state(page, id, expected = initial[id]) {
  await snapshot(page, id, expected);
  await axe(page);
}
async function focus(page, id, key) { await handle(page, id, key).focus(); await frames(page); }
async function idle(page, id) {
  assert.equal(await section(page, id).locator('li[data-dragging]').count(), 0);
  assert.equal(await section(page, id).locator('[data-over]').count(), 0);
}
async function lifted(page, id, key) {
  assert.equal(await item(page, id, key).getAttribute('data-dragging'), '');
  assert.equal(await handle(page, id, key).getAttribute('data-dragging'), '');
}
async function target(page, id, container, position) {
  assert.equal(await section(page, id).locator('[data-preview]').textContent(), `Drop in ${container} at position ${position}.`);
  assert.equal(await list(page, id, container).getAttribute('data-over'), '');
}
async function beginMouse(page, locator) {
  await locator.scrollIntoViewIfNeeded();
  const box = await locator.boundingBox();
  const point = { x: box.x + box.width / 2, y: box.y + box.height / 2 };
  await page.mouse.move(point.x, point.y);
  await page.mouse.down(); await frames(page);
  return point;
}
async function moveMouse(page, x, y) {
  await page.mouse.move(x, y, { steps: 8 }); await frames(page);
}
async function dimensions(locator) {
  // Compare document geometry: an idle arrow may legitimately scroll the viewport.
  return locator.evaluate(node => {
    const { x, y, width, height } = node.getBoundingClientRect();
    return { x: x + scrollX, y: y + scrollY, width, height };
  });
}

async function nativeClicks(context) {
  const page = await openExample(context, 'drag_and_drop');
  await page.evaluate(() => {
    window.dropClicks = [];
    window.dropActions = 0;
    window.addEventListener('click', event => window.dropClicks.push(event), true);
    document.querySelector('#board-container-queue').addEventListener('click', () => window.dropActions++);
    document.querySelector('#board-example [data-key="poster"] .item-label').addEventListener('click', () => window.dropActions++);
    const link = document.createElement('a');
    link.id = 'packing-action'; link.href = '#packing-action-destination'; link.textContent = 'Read packing details';
    link.addEventListener('click', () => window.dropActions++);
    document.querySelector('#packing-example [data-key="tissue"]').append(link);
  });
  const clicks = () => page.evaluate(() => window.dropClicks.map(event => ({ target: event.target.id, prevented: event.defaultPrevented, trusted: event.isTrusted, detail: event.detail })));
  await beginMouse(page, handle(page, 'board', 'poster'));
  const cover = await item(page, 'board', 'cover').boundingBox();
  await moveMouse(page, cover.x + cover.width / 2, cover.y + cover.height / 2);
  await page.mouse.up(); await frames(page);
  assert.ok((await clicks()).some(event => event.target === 'board-container-queue' && event.trusted && event.prevented), 'Native ancestor-retargeted drop click is prevented');
  assert.equal(await page.evaluate(() => window.dropActions), 0, 'Container and delegated actions must not run after dragging');
  await axe(page);

  const label = item(page, 'board', 'poster').locator('.item-label');
  const point = await beginMouse(page, label);
  await moveMouse(page, point.x + 10, point.y + 2);
  await lifted(page, 'board', 'poster');
  await pressKeys(page, 'Escape');
  await idle(page, 'board');
  await page.mouse.up(); await frames(page);
  assert.equal(await page.evaluate(() => window.dropActions), 0, 'Escape retains capture suppression until the original pointer release/click');
  assert.ok((await clicks()).at(-1).prevented);
  await axe(page);

  const packingStart = await beginMouse(page, handle(page, 'packing', 'tissue'));
  await moveMouse(page, packingStart.x + 10, packingStart.y + 2);
  await lifted(page, 'packing', 'tissue');
  await pressKeys(page, 'Escape');
  await page.mouse.up(); await frames(page);
  await idle(page, 'packing');
  await page.locator('#packing-action').click(); await frames(page);
  assert.equal(await page.evaluate(() => window.dropActions), 1, 'A later legitimate handle-only card action is never swallowed');
  assert.equal(new URL(page.url()).hash, '#packing-action-destination');
  assert.equal((await clicks()).at(-1).prevented, false);
  await ariaSnapshot(page.locator('#packing-action'), '- link "Read packing details":\n  - /url: "#packing-action-destination"');
  await axe(page);
}

async function realTouch(context) {
  const page = await openExample(context, 'drag_and_drop');
  const cdp = await context.newCDPSession(page);
  await cdp.send('Emulation.setTouchEmulationEnabled', { enabled: true, maxTouchPoints: 1 });
  await page.evaluate(() => {
    window.dragTouchEvents = [];
    for (const type of ['pointerdown', 'pointercancel', 'touchmove', 'touchend']) {
      // Observe browser input after consumer handlers, without affecting gesture ownership.
      window.addEventListener(type, event => window.dragTouchEvents.push({
        type, trusted: event.isTrusted, prevented: event.defaultPrevented,
        pointerType: event.pointerType, time: performance.now(),
      }), { passive: true });
    }
  });
  const events = () => page.evaluate(() => window.dragTouchEvents);
  const resetEvents = () => page.evaluate(() => { window.dragTouchEvents = []; });
  const send = (type, point) => cdp.send('Input.dispatchTouchEvent', {
    type, touchPoints: point ? [{ ...point, id: 1 }] : [],
  });
  const begin = async locator => {
    await locator.scrollIntoViewIfNeeded();
    // No consumer CSS may hide a broken nonpassive touchmove fallback.
    assert.deepEqual(await locator.evaluate(node => {
      const values = [];
      for (; node; node = node.parentElement) values.push(getComputedStyle(node).touchAction);
      return [...new Set(values)];
    }), ['auto']);
    const box = await locator.boundingBox();
    const point = { x: box.x + box.width / 2, y: box.y + box.height / 2 };
    await resetEvents();
    await send('touchStart', point); await frames(page);
    assert.ok((await events()).some(event => event.type === 'pointerdown' && event.pointerType === 'touch' && event.trusted));
    return point;
  };
  try {
    // A real short press/click is not a lift or committed move.
    await begin(handle(page, 'board', 'poster'));
    await send('touchEnd'); await frames(page);
    const shortPress = await events();
    assert.ok(shortPress.find(event => event.type === 'touchend').time - shortPress.find(event => event.type === 'pointerdown').time < 250);
    await idle(page, 'board'); await state(page, 'board');
    assert.equal(await live(page, 'board').textContent(), '');
    assert.equal(await page.locator('#board-moves').textContent(), 'Committed moves: 0');

    // Exercise both the handle and whole-card path with native browser gesture arbitration.
    let expected = initial.board;
    for (const source of [handle(page, 'board', 'poster'), item(page, 'board', 'poster').locator('.item-label')]) {
      const start = await begin(source);
      // Only the intentional long-press uses real time: readiness has no public DOM state.
      await page.waitForTimeout(300); // default touch_delay = 250ms
      await frames(page); await idle(page, 'board');
      const scrollBefore = await page.evaluate(() => scrollY);
      const originalBox = await dimensions(item(page, 'board', 'poster'));
      // The FIRST movement crosses both the 6px module threshold and Chromium's pan slop.
      await send('touchMove', { x: start.x, y: start.y - 40 }); await frames(page);
      await lifted(page, 'board', 'poster');
      assert.equal(await live(page, 'board').textContent(), `Picked up Edition poster. Position ${expected.queue.indexOf('poster') + 1} of 3.`);
      const firstMove = (await events()).filter(event => event.type === 'touchmove');
      assert.equal(firstMove.length, 1);
      assert.ok(firstMove[0].time - (await events()).find(event => event.type === 'pointerdown').time >= 250);
      assert.ok(firstMove[0].trusted && firstMove[0].prevented, 'The first native touchmove must be owned after the delay');
      assert.equal((await events()).filter(event => event.type === 'pointercancel').length, 0);
      assert.equal(await page.evaluate(() => scrollY), scrollBefore);
      assert.deepEqual(await dimensions(item(page, 'board', 'poster')), originalBox);
      await state(page, 'board', expected);

      const cover = await item(page, 'board', 'cover').boundingBox();
      const toEnd = expected.queue[0] === 'poster';
      await send('touchMove', { x: cover.x + cover.width / 2, y: cover.y + cover.height * (toEnd ? .8 : .2) });
      await frames(page); await target(page, 'board', 'queue', toEnd ? 2 : 1);
      await state(page, 'board', expected);
      await send('touchEnd'); await frames(page);
      expected = { ...initial.board, queue: toEnd ? ['cover', 'poster', 'proof'] : initial.board.queue };
      await idle(page, 'board'); await state(page, 'board', expected);
      assert.equal(await live(page, 'board').textContent(), 'Dropped.');
      await assertFocus(page, handleSelector('board', 'poster'));
      const gesture = await events();
      assert.ok(gesture.filter(event => event.type === 'touchmove').every(event => event.trusted && event.prevented));
      assert.equal(gesture.filter(event => event.type === 'pointercancel').length, 0);
      assert.ok(gesture.some(event => event.type === 'touchend' && event.trusted));
    }
    assert.equal(await page.locator('#board-moves').textContent(), 'Committed moves: 2');

    // Before the delay, movement abandons the candidate and leaves native scrolling alone.
    const start = await begin(handle(page, 'board', 'poster'));
    const scrollBefore = await page.evaluate(() => scrollY);
    await send('touchMove', { x: start.x, y: start.y - 60 }); await frames(page);
    await idle(page, 'board');
    await send('touchMove', { x: start.x, y: start.y - 140 }); await frames(page);
    await send('touchEnd'); await frames(page);
    await page.waitForFunction(before => scrollY > before + 20, scrollBefore);
    const scrolling = await events();
    assert.ok(scrolling.some(event => event.type === 'touchmove' && event.trusted));
    assert.ok(scrolling.find(event => event.type === 'touchmove').time - scrolling.find(event => event.type === 'pointerdown').time < 250, 'Movement must occur before the long-press delay');
    assert.ok(scrolling.filter(event => event.type === 'touchmove').every(event => !event.prevented), 'Pre-delay movement must not suppress native scrolling');
    assert.ok(scrolling.some(event => event.type === 'pointercancel' && event.trusted), 'Chromium owns the scrolling gesture');
    await idle(page, 'board'); await state(page, 'board');
    assert.equal(await page.locator('#board-moves').textContent(), 'Committed moves: 2');
  } finally {
    await cdp.detach();
  }
}

export async function run(context) {
  const page = await openExample(context, 'drag_and_drop');
  // Record real browser default handling, without replacing any event or collection.
  await page.evaluate(() => {
    window.dragKeys = [];
    document.addEventListener('keydown', event => window.dragKeys.push({ key: event.key, prevented: event.defaultPrevented }));
  });
  for (const id of Object.keys(initial)) await snapshot(page, id);
  await axe(page);
  assert.equal(await live(page, 'packing').getAttribute('aria-live'), 'assertive');
  assert.equal(await live(page, 'board').getAttribute('aria-live'), 'polite');
  assert.equal(await handle(page, 'board', 'proof').isDisabled(), true);
  assert.equal(await handle(page, 'board', 'sample').isDisabled(), true);
  assert.equal(await list(page, 'board', 'archive').getAttribute('data-disabled'), '');
  const describedBy = await handle(page, 'board', 'poster').getAttribute('aria-describedby');
  assert.match(await page.locator(`[id="${describedBy}"]`).textContent(), /Space or Enter/);

  // Idle arrows, modifier shortcuts, composition and repeated activations do not lift.
  await focus(page, 'board', 'poster');
  await pressKeys(page, 'ArrowDown', 'Control+Space', 'Alt+Enter', 'Meta+Enter');
  await handle(page, 'board', 'poster').dispatchEvent('keydown', { key: 'Enter', isComposing: true, bubbles: true });
  await frames(page); await idle(page, 'board');
  assert.equal(await page.evaluate(() => window.dragKeys.find(event => event.key === 'ArrowDown').prevented), false);

  // Space lift/drop; vertical arrows change preview only, never item geometry/order.
  const originalBox = await dimensions(item(page, 'board', 'poster'));
  await page.keyboard.down('Space'); await frames(page);
  await page.keyboard.down('Space'); await frames(page); // real repeat:true
  await page.keyboard.up('Space'); await frames(page);
  await lifted(page, 'board', 'poster');
  assert.equal(await live(page, 'board').textContent(), 'Picked up Edition poster. Position 1 of 3.');
  await state(page, 'board');
  await pressKeys(page, 'ArrowDown');
  await target(page, 'board', 'queue', 2);
  assert.equal(await live(page, 'board').textContent(), 'Edition poster moved to position 2 of 3 in Queue.');
  assert.deepEqual(await dimensions(item(page, 'board', 'poster')), originalBox);
  await state(page, 'board');
  await pressKeys(page, 'ArrowUp'); await target(page, 'board', 'queue', 1);
  await pressKeys(page, 'Shift+ArrowRight'); await target(page, 'board', 'making', 1);
  await state(page, 'board');
  await pressKeys(page, 'Shift+ArrowRight'); // disabled archive cannot be a target
  await target(page, 'board', 'making', 1);
  await pressKeys(page, 'Shift+ArrowLeft', 'ArrowDown', 'Space');
  const reordered = { queue: ['cover', 'poster', 'proof'], making: ['labels'], archive: ['sample'] };
  await idle(page, 'board'); await state(page, 'board', reordered);
  assert.equal(await live(page, 'board').textContent(), 'Dropped.');
  await assertFocus(page, handleSelector('board', 'poster'));
  assert.equal(await page.locator('#board-moves').textContent(), 'Committed moves: 1');

  // Enter lift/drop, cross-container commit and focus restoration onto the moved handle.
  await pressKeys(page, 'Enter', 'Shift+ArrowRight', 'Enter');
  const transferred = { queue: ['cover', 'proof'], making: ['labels', 'poster'], archive: ['sample'] };
  await state(page, 'board', transferred); await idle(page, 'board');
  await assertFocus(page, handleSelector('board', 'poster'));
  await pressKeys(page, 'Space', 'ArrowUp', 'Escape');
  await state(page, 'board', transferred);
  assert.equal(await live(page, 'board').textContent(), 'Cancelled, Edition poster returned to position 2.');
  await assertFocus(page, handleSelector('board', 'poster'));

  // Tab and Shift+Tab cancel without preventDefault or stealing the browser's focus.
  await focus(page, 'board', 'cover'); await pressKeys(page, 'Space', 'Tab');
  await idle(page, 'board'); await assertFocus(page, handleSelector('board', 'labels'));
  await state(page, 'board', transferred);
  await pressKeys(page, 'Enter', 'Shift+Tab');
  await idle(page, 'board'); await assertFocus(page, handleSelector('board', 'cover'));
  const tabEvents = await page.evaluate(() => window.dragKeys.filter(event => event.key === 'Tab'));
  assert.equal(tabEvents.length, 2); assert.ok(tabEvents.every(event => !event.prevented));
  await state(page, 'board', transferred);

  // Both horizontal arrows, both cross-axis directions, and RTL reversal.
  for (const [id, key, forward, backward] of [
    ['horizontal', 'cyan', 'ArrowRight', 'ArrowLeft'], ['rtl', 'red', 'ArrowLeft', 'ArrowRight'],
  ]) {
    await focus(page, id, key); await pressKeys(page, 'Enter', forward);
    await target(page, id, 'upper', 2); await state(page, id);
    await pressKeys(page, backward); await target(page, id, 'upper', 1);
    await pressKeys(page, 'Shift+ArrowDown'); await target(page, id, 'lower', 1);
    await state(page, id);
    await pressKeys(page, 'Shift+ArrowUp', forward, 'Enter');
    const expected = structuredClone(initial[id]);
    [expected.upper[0], expected.upper[1]] = [expected.upper[1], expected.upper[0]];
    await state(page, id, expected); await assertFocus(page, handleSelector(id, key));
    await pressKeys(page, 'Space', backward, 'Escape');
    await state(page, id, expected); await idle(page, id);
  }
  await focus(page, 'rtl-board', 'etching');
  await pressKeys(page, 'Space', 'ArrowDown', 'ArrowUp', 'Shift+ArrowLeft');
  await target(page, 'rtl-board', 'second', 1); await state(page, 'rtl-board');
  await pressKeys(page, 'Shift+ArrowRight'); await target(page, 'rtl-board', 'first', 1);
  await pressKeys(page, 'Shift+ArrowLeft', 'Space');
  await state(page, 'rtl-board', { first: ['linocut'], second: ['etching', 'screen'] });
  await assertFocus(page, handleSelector('rtl-board', 'etching'));

  // Programmatic cancellation and custom announcements/configuration.
  await focus(page, 'packing', 'tissue'); await pressKeys(page, 'Enter', 'ArrowDown', 'Enter');
  const packed = { packing: ['sleeve', 'tissue', ...initial.packing.packing.slice(2)] };
  assert.equal(await live(page, 'packing').textContent(), 'Packing order saved.');
  await state(page, 'packing', packed);
  await pressKeys(page, 'Space', 'ArrowUp');
  await page.locator('#packing-cancel').click(); await frames(page);
  await idle(page, 'packing'); await assertFocus(page, handleSelector('packing', 'tissue'));
  await state(page, 'packing', packed);

  // Reset for real pointer geometry: threshold, stable preview, same-list insertion.
  await page.reload(); await page.locator('#app > *').waitFor(); await frames(page);
  const start = await beginMouse(page, handle(page, 'board', 'poster'));
  await moveMouse(page, start.x + 2, start.y + 2);
  await idle(page, 'board'); await state(page, 'board');
  const before = await dimensions(item(page, 'board', 'poster'));
  const cover = await item(page, 'board', 'cover').boundingBox();
  await moveMouse(page, cover.x + cover.width / 2, cover.y + cover.height * .8);
  await lifted(page, 'board', 'poster'); await target(page, 'board', 'queue', 2);
  assert.deepEqual(await dimensions(item(page, 'board', 'poster')), before);
  await state(page, 'board');
  await page.mouse.up(); await frames(page);
  await state(page, 'board', reordered); await assertFocus(page, handleSelector('board', 'poster'));

  // Whole-card pointer start and a genuine cross-container hit-tested drop.
  await beginMouse(page, item(page, 'board', 'poster').locator('.item-label'));
  const making = await list(page, 'board', 'making').boundingBox();
  await moveMouse(page, making.x + making.width / 2, making.y + making.height - 8);
  await target(page, 'board', 'making', 2); await state(page, 'board', reordered);
  await page.mouse.up(); await frames(page);
  await state(page, 'board', transferred); await assertFocus(page, handleSelector('board', 'poster'));

  // Disabled source cannot lift; disabled destination rejects a real pointer drop.
  await beginMouse(page, handle(page, 'board', 'proof'));
  await moveMouse(page, making.x + 40, making.y + 40);
  await idle(page, 'board'); await page.mouse.up(); await frames(page);
  await beginMouse(page, handle(page, 'board', 'poster'));
  const archive = await list(page, 'board', 'archive').boundingBox();
  await moveMouse(page, archive.x + archive.width / 2, archive.y + archive.height / 2);
  await lifted(page, 'board', 'poster');
  assert.equal(await section(page, 'board').locator('[data-over]').count(), 0);
  await state(page, 'board', transferred);
  await page.mouse.up(); await frames(page);
  await idle(page, 'board'); await state(page, 'board', transferred);
  assert.equal(await page.locator('#board-moves').textContent(), 'Committed moves: 2');

  // Handle-only policy ignores item-body drags, then honors its configured threshold.
  const bodyStart = await beginMouse(page, item(page, 'packing', 'tissue').locator('.item-label'));
  await moveMouse(page, bodyStart.x, bodyStart.y + 45);
  await idle(page, 'packing'); await page.mouse.up(); await frames(page);
  await state(page, 'packing');
  const packingStart = await beginMouse(page, handle(page, 'packing', 'tissue'));
  await moveMouse(page, packingStart.x + 5, packingStart.y);
  await idle(page, 'packing');
  await moveMouse(page, packingStart.x + 12, packingStart.y);
  await lifted(page, 'packing', 'tissue'); await state(page, 'packing');

  // Stationary pointer at the nearest scrollable area's edge continues auto-scroll.
  const scrollArea = list(page, 'packing', 'packing');
  const area = await scrollArea.boundingBox();
  const windowScroll = await page.evaluate(() => ({ x: scrollX, y: scrollY }));
  const scrollBefore = await scrollArea.evaluate(node => node.scrollTop);
  await moveMouse(page, area.x + area.width / 2, area.y + area.height - 8);
  await page.waitForFunction(({ selector, before }) => document.querySelector(selector).scrollTop > before + 60,
    { selector: '#packing-example .scroll-area', before: scrollBefore });
  assert.deepEqual(await page.evaluate(() => ({ x: scrollX, y: scrollY })), windowScroll, 'Only the nearest scrollable list scrolls');
  await state(page, 'packing');
  await page.keyboard.press('Escape'); await frames(page);
  await idle(page, 'packing'); await assertFocus(page, handleSelector('packing', 'tissue'));
  await page.mouse.up(); await frames(page);
  await state(page, 'packing');

  await nativeClicks(context);
  await realTouch(context);
}
