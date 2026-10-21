import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';
export async function run(context) {
  const page = await openExample(context, 'meter');
  const reading = page.locator('#reading');
  const choose = async (id, value) => { await page.locator(id).selectOption(value); await frames(page); };
  await ariaSnapshot(page.locator('#default'), '- meter "Default range"');
  assert.equal(await page.locator('#default').getAttribute('aria-valuenow'), '0.5');
  for (const [optimum, states] of [
    ['0', ['optimum','optimum','suboptimal','suboptimal','critical']],
    ['50', ['suboptimal','optimum','optimum','optimum','suboptimal']],
    ['100', ['critical','suboptimal','suboptimal','optimum','optimum']]
  ]) {
    await choose('#optimum', optimum);
    for (const [i, value] of ['0','25','50','75','100'].entries()) {
      await choose('#value', value);
      await ariaSnapshot(reading, '- meter "Storage used"');
      assert.equal(await reading.getAttribute('aria-valuenow'), value);
      assert.equal(await reading.getAttribute('aria-valuemin'), '0');
      assert.equal(await reading.getAttribute('aria-valuemax'), '100');
      assert.equal(await reading.getAttribute('aria-valuetext'), value + ' units');
      assert.equal(await reading.getAttribute('data-state'), states[i]);
      assert.equal(await reading.evaluate(el => el.style.getPropertyValue('--tea-meter-percent')), value + '%');
      const width = await reading.evaluate(el => parseFloat(getComputedStyle(el, '::before').width) / el.clientWidth);
      assert.ok(Math.abs(width - Number(value)/100) < 0.01, 'visible percentage');
      await axe(page);
    }
  }
  for (const [value, expected] of [['-20','0'],['120','100'],['nan','0'],['inf','100'],['-inf','0']]) {
    await choose('#value', value);
    assert.equal(await reading.getAttribute('aria-valuenow'), expected);
    await ariaSnapshot(reading, '- meter "Storage used"'); await axe(page);
  }
  await page.locator('#format').uncheck(); await frames(page);
  assert.equal(await reading.getAttribute('aria-valuetext'), null);
  await ariaSnapshot(reading, '- meter "Storage used"'); await axe(page);
  await page.locator('#format').focus(); await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#optimum');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#format');
  await pressKeys(page, 'Tab');
  assert.equal(await reading.evaluate(el => el === document.activeElement), false, 'display never enters tab order');
  const before = await reading.getAttribute('aria-valuenow');
  await reading.click(); await frames(page);
  assert.equal(await reading.getAttribute('aria-valuenow'), before, 'pointer does not change a meter');
}
