import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';
export async function run(context) {
  const page = await openExample(context, 'progress');
  const reading = page.locator('#reading');
  const choose = async value => { await page.locator('#value').selectOption(value); await frames(page); };
  await ariaSnapshot(page.locator('#default'), '- progressbar "Default range"');
  assert.equal(await page.locator('#default').getAttribute('aria-valuenow'), null);
  for (const [value, expected] of [['0','0'],['25','25'],['100','100'],['-20','0'],['120','100'],['nan','0'],['inf','100'],['-inf','0']]) {
    await choose(value);
    await ariaSnapshot(reading, '- progressbar "Import progress"');
    assert.equal(await reading.getAttribute('aria-valuenow'), expected);
    assert.equal(await reading.getAttribute('aria-valuemin'), '0');
    assert.equal(await reading.getAttribute('aria-valuemax'), '100');
    assert.equal(await reading.getAttribute('aria-valuetext'), expected + ' units');
    assert.equal(await reading.getAttribute('data-state'), expected === '100' ? 'complete' : 'loading');
    assert.equal(await reading.evaluate(el => el.style.getPropertyValue('--tea-progress-percent')), expected + '%');
    assert.equal(await page.locator('#loaded').getAttribute('aria-busy'), expected === '100' ? 'false' : 'true');
    assert.equal(await page.locator('#loaded').getAttribute('aria-describedby'), 'reading');
    await axe(page);
  }
  await choose('none');
  await ariaSnapshot(reading, '- progressbar "Import progress"');
  for (const attr of ['aria-valuenow', 'aria-valuetext']) assert.equal(await reading.getAttribute(attr), null);
  assert.equal(await reading.getAttribute('data-state'), 'indeterminate');
  assert.equal(await reading.evaluate(el => el.style.getPropertyValue('--tea-progress-percent')), '');
  assert.equal(await page.locator('#loaded').getAttribute('aria-busy'), 'true');
  await axe(page);
  await page.locator('#format').uncheck(); await frames(page); await axe(page);
  await choose('50');
  assert.equal(await reading.getAttribute('aria-valuetext'), null);
  await ariaSnapshot(reading, '- progressbar "Import progress"'); await axe(page);
  await page.locator('#format').focus(); await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#value');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#format');
  await pressKeys(page, 'Tab');
  assert.equal(await reading.evaluate(el => el === document.activeElement), false);
  await reading.click(); await frames(page);
  assert.equal(await reading.getAttribute('aria-valuenow'), '50', 'pointer does not change progress');
}
