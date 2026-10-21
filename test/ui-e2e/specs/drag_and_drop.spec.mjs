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
}
