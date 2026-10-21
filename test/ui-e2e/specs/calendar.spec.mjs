import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'calendar');
  const cell = (iso, id = 'calendar') => `#${id}-item-${iso}`;
  const activity = () => page.locator('#activity').textContent();
  const reset = async () => {
    await page.locator('#reset').click(); await frames(page);
    await page.locator(cell('2024-01-31')).focus();
  };
  const pointer = async selector => {
    const target = page.locator(selector);
    await target.scrollIntoViewIfNeeded();
    const box = await target.boundingBox();
    await page.mouse.click(box.x + box.width / 2, box.y + box.height / 2);
    await frames(page);
  };
  const oneStop = async (id = 'calendar') => {
    assert.equal(await page.locator(`#${id}-grid [tabindex="0"]`).count(), 1);
  };
  const state = async (iso, name, { disabled = false, selected = false, id = 'calendar' } = {}) => {
    await assertFocus(page, cell(iso, id));
    await oneStop(id);
    await ariaSnapshot(page.locator(cell(iso, id)), `- gridcell "${name}"${disabled ? ' [disabled]' : ''}${selected ? ' [selected]' : ''}: "${Number(iso.slice(-2))}"`);
    await axe(page);
  };

  assert.equal(await page.locator('#calendar-grid').getAttribute('aria-labelledby'), 'calendar-heading');
  assert.equal(await page.locator('#calendar-heading').getAttribute('aria-live'), 'polite');
  assert.equal(await page.locator('#calendar-heading').getAttribute('aria-atomic'), 'true');
  await ariaSnapshot(page.locator('#calendar-heading'), '- heading "January 2024" [level=2]');
  const weekdaySnapshot = [['Monday', 'Mo'], ['Tuesday', 'Tu'], ['Wednesday', 'We'], ['Thursday', 'Th'], ['Friday', 'Fr'], ['Saturday', 'Sa'], ['Sunday', 'Su']]
    .map(([day, short]) => `    - columnheader "${day}": ${short}`).join('\n');
  await ariaSnapshot(page.locator('#calendar-grid thead'), '- rowgroup:\n  - row "Monday Tuesday Wednesday Thursday Friday Saturday Sunday":\n' + weekdaySnapshot);
  assert.equal(await page.locator('#calendar-grid tbody tr').count(), 6);
  assert.equal(await page.locator('#rtl-grid tbody tr').count(), 5);
  assert.equal(await page.locator('#calendar-grid [data-outside-month]').count(), 0);
  assert.equal(await page.locator('#rtl-grid [data-outside-month]').count(), 4);
  assert.equal(await page.locator(cell('2024-01-31')).getAttribute('aria-current'), 'date');
  await reset();
  await state('2024-01-31', '31 January 2024', { selected: true });

  // One roving stop, native Tab/Shift+Tab, and native month/year buttons.
  await page.locator('#calendar-previous-year').focus();
  for (const id of ['calendar-previous-month', 'calendar-next-month', 'calendar-next-year', 'calendar-item-2024-01-31', 'rtl-previous-year']) {
    await pressKeys(page, 'Tab'); await assertFocus(page, '#' + id);
  }
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, cell('2024-01-31'));
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#calendar-next-year');

  // Each APG date-grid row, including disabled days and cross-month movement.
  for (const [key, iso, name, disabled] of [
    ['ArrowLeft', '2024-01-30', '30 January 2024', false],
    ['ArrowRight', '2024-02-01', '1 February 2024', false],
    ['ArrowUp', '2024-01-24', '24 January 2024', false],
    ['ArrowDown', '2024-02-07', '7 February 2024', false],
    ['Home', '2024-01-29', '29 January 2024', false],
    ['End', '2024-02-04', '4 February 2024', true],
    ['PageUp', '2023-12-31', '31 December 2023', true],
    ['PageDown', '2024-02-29', '29 February 2024', false],
    ['Shift+PageUp', '2023-01-31', '31 January 2023', false],
    ['Shift+PageDown', '2025-01-31', '31 January 2025', false],
  ]) {
    await reset(); await pressKeys(page, key);
    await state(iso, name, { disabled });
    const month = name.split(' ').slice(1).join(' ');
    assert.equal(await page.locator('#calendar-heading').textContent(), month, key + ' updates live heading');
    await ariaSnapshot(page.locator('#calendar-heading'), `- heading "${month}" [level=2]`);
  }
  await reset(); await pressKeys(page, 'PageDown', 'Shift+PageDown');
  await state('2025-02-28', '28 February 2025');
  await pressKeys(page, 'Shift+PageUp');
  await state('2024-02-28', '28 February 2024');

  // Rapid intents use the latest model, even before any render.
  await reset();
  await page.locator(cell('2024-01-31')).evaluate(el => {
    for (let n = 0; n < 2; n++) el.dispatchEvent(new KeyboardEvent('keydown', { key: 'ArrowRight', bubbles: true, cancelable: true }));
  });
  await frames(page);
  await state('2024-02-02', '2 February 2024');

  // Keyboard activation and pointer selection emit once; selection does not follow focus.
  await reset(); await pressKeys(page, 'ArrowLeft', 'Enter');
  assert.match(await activity(), /Selections: 1\. Selected date: 2024-01-30\./);
  await state('2024-01-30', '30 January 2024', { selected: true });
  await pressKeys(page, 'ArrowLeft');
  const beforeSpace = await activity();
  const scroll = await page.evaluate(() => scrollY);
  await pressKeys(page, 'Space');
  assert.notEqual(await activity(), beforeSpace);
  assert.equal(await page.evaluate(() => scrollY), scroll, 'Space does not scroll');
  assert.match(await activity(), /Selections: 2\. Selected date: 2024-01-29\./);
  await state('2024-01-29', '29 January 2024', { selected: true });
  await page.locator(cell('2024-01-26')).click(); await frames(page);
  assert.match(await activity(), /Selections: 3\. Selected date: 2024-01-26\./);
  await state('2024-01-26', '26 January 2024', { selected: true });
  const beforeDisabled = await activity();
  await pointer(cell('2024-01-27'));
  await pressKeys(page, 'Enter', 'Space');
  assert.equal(await activity(), beforeDisabled);
  await state('2024-01-27', '27 January 2024', { disabled: true });

  // Browser/AT shortcuts, composition and unhandled keys are not claimed.
  await reset();
  await pressKeys(page, 'Control+ArrowRight', 'Alt+ArrowRight', 'Meta+ArrowRight', 'Control+PageDown', 'Escape', 'x');
  await assertFocus(page, cell('2024-01-31'));
  assert.match(await activity(), /Selections: 0\./);
  assert.equal(await page.locator(cell('2024-01-31')).evaluate(el => el.dispatchEvent(new KeyboardEvent('keydown', { key: 'ArrowRight', isComposing: true, cancelable: true, bubbles: true }))), true);
  await frames(page); await assertFocus(page, cell('2024-01-31'));

  // Previous/next month/year, activated by pointer, Enter and Space. Focus stays on the button.
  for (const [button, key, expected] of [
    ['previous-month', null, 'December 2023'],
    ['next-month', 'Enter', 'February 2024'],
    ['previous-year', 'Space', 'January 2023'],
    ['next-year', 'Enter', 'January 2025'],
  ]) {
    await reset();
    if (key) { await page.locator('#calendar-' + button).focus(); await pressKeys(page, key); }
    else { await page.locator('#calendar-' + button).click(); await frames(page); }
    await assertFocus(page, '#calendar-' + button);
    await ariaSnapshot(page.locator('#calendar-heading'), `- heading "${expected}" [level=2]`);
    await oneStop(); await axe(page);
  }

  // Read-only and wholly unavailable grids still navigate but never change selection.
  for (const option of ['readonly', 'unavailable']) {
    await reset(); await page.locator('#' + option).check(); await frames(page);
    await page.locator(cell('2024-01-31')).focus();
    const before = await activity();
    await pressKeys(page, 'ArrowLeft', 'Enter', 'Space');
    await pointer(cell('2024-01-29'));
    assert.equal(await activity(), before);
    await state('2024-01-29', '29 January 2024', { disabled: option === 'unavailable' });
    if (option === 'readonly') {
      assert.equal(await page.locator('#calendar-grid').getAttribute('aria-readonly'), 'true');
      assert.equal(await page.locator('#calendar-grid').getAttribute('data-readonly'), '');
    } else {
      await ariaSnapshot(page.locator(cell('2024-01-31')), '- gridcell "31 January 2024" [disabled] [selected]: "31"');
    }
  }

  // Inclusive singleton min/max: buttons disabled, outside-bound dates remain reachable.
  await reset(); await page.locator('#single-day').check(); await frames(page);
  for (const [part, name, text] of [['previous-month', 'Previous month', '- Month'], ['next-month', 'Next month', 'Month +'], ['previous-year', 'Previous year', '- Year'], ['next-year', 'Next year', 'Year +']]) {
    await ariaSnapshot(page.locator('#calendar-' + part), `- button "${name}" [disabled]: ${text.startsWith('-') ? JSON.stringify(text) : text}`);
    assert.equal(await page.locator('#calendar-' + part).getAttribute('data-disabled'), '');
    assert.equal(await page.locator('#calendar-' + part).getAttribute('tabindex'), '-1');
    await pointer('#calendar-' + part);
    await page.locator('#calendar-' + part).focus(); await pressKeys(page, 'Enter', 'Space');
    assert.equal(await page.locator('#calendar-heading').textContent(), 'January 2024');
  }
  await page.locator('#reset').focus(); await pressKeys(page, 'Tab');
  await assertFocus(page, cell('2024-01-31'));
  await axe(page);
  await page.locator(cell('2024-01-31')).focus(); await pressKeys(page, 'ArrowRight', 'Enter', 'Space');
  assert.match(await activity(), /Selections: 0\./);
  await state('2024-02-01', '1 February 2024', { disabled: true });
  await pressKeys(page, 'ArrowLeft', 'Enter');
  await state('2024-01-31', '31 January 2024', { selected: true });
  const selectedAtBound = await activity();
  await pressKeys(page, 'ArrowLeft', 'Enter', 'Space');
  assert.equal(await activity(), selectedAtBound);
  await state('2024-01-30', '30 January 2024', { disabled: true });

  // RTL swaps horizontal keys only. Adjacent-month dates are visible and selectable.
  await reset(); await page.locator(cell('2024-01-31', 'rtl')).focus();
  await pressKeys(page, 'ArrowLeft');
  await state('2024-02-01', '1 February 2024', { id: 'rtl' });
  await pressKeys(page, 'ArrowRight');
  await state('2024-01-31', '31 January 2024', { id: 'rtl' });
  assert.equal(await page.locator(cell('2024-02-01', 'rtl')).getAttribute('data-outside-month'), '');
  await page.locator(cell('2024-02-01', 'rtl')).click(); await frames(page);
  await state('2024-02-01', '1 February 2024', { id: 'rtl', selected: true });
  await ariaSnapshot(page.locator('#rtl-heading'), '- heading "February 2024" [level=2]');
}
