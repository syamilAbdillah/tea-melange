import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';
export async function run(context) {
  const page = await openExample(context, 'disclosure');
  const button = page.locator('#details-button');
  const check = async (open, mounted = true) => {
    await ariaSnapshot(button, '- button "Storage details"' + (open ? ' [expanded]' : ''));
    assert.equal(await button.getAttribute('aria-expanded'), String(open));
    assert.equal(await button.getAttribute(open ? 'data-open' : 'data-closed'), '');
    assert.equal(await button.getAttribute(open ? 'data-closed' : 'data-open'), null);
    assert.equal(await button.getAttribute('aria-controls'), mounted ? 'details-panel' : null);
    assert.equal(await page.locator('#details-panel').count(), mounted ? 1 : 0);
    if (mounted) {
      assert.equal(await page.locator('#details-panel').evaluate(el => el.hidden), !open);
      assert.equal(await page.locator('#details-panel').getAttribute(open ? 'data-open' : 'data-closed'), '');
      await ariaSnapshot(page.locator('#details-panel'), open ? '- paragraph: Your files stay available while you are offline.\n- link "Continue reading":\n  - /url: "#end"' : '');
    }
    await axe(page);
  };
  await check(false);
  await page.locator('#keep').focus(); await pressKeys(page, 'Tab'); await assertFocus(page, '#details-button');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#first-button');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#details-button');
  await pressKeys(page, 'Enter'); await check(true); await assertFocus(page, '#details-button');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#details-link');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#details-button');
  const scroll = await page.evaluate(() => scrollY);
  await page.keyboard.down('Space'); await frames(page); await check(true);
  assert.equal(await page.evaluate(() => scrollY), scroll);
  await page.keyboard.up('Space'); await frames(page); await check(false); await assertFocus(page, '#details-button');
  assert.equal(await page.evaluate(() => scrollY), scroll);
  await button.click(); await frames(page); await check(true); await assertFocus(page, '#details-button');
  await button.click(); await frames(page); await check(false);
  await page.locator('#keep').uncheck(); await frames(page); await check(false, false);
  await button.click(); await frames(page); await check(true);
  await button.click(); await frames(page); await check(false, false); await assertFocus(page, '#details-button');
  // Two clicks before a draw are two intents, not two assignments of stale state.
  await button.evaluate(el => { el.click(); el.click(); }); await frames(page); await check(false, false);
  await page.locator('#keep').check(); await frames(page); await check(false);
  for (const [id, name] of [['first','Access'],['second','Retention'],['third','Recovery']]) {
    await page.locator('#' + id + '-button').focus();
    await pressKeys(page, 'Enter');
    await assertFocus(page, '#' + id + '-button');
    for (const [other, label] of [['first','Access'],['second','Retention'],['third','Recovery']]) {
      await ariaSnapshot(page.locator('#' + other + '-button'), `- button "${label}"${other === id ? ' [expanded]' : ''}`);
      assert.equal(await page.locator('#' + other + '-panel').evaluate(el => el.hidden), other !== id);
    }
    assert.equal(await page.locator('h3 > button[aria-expanded="true"]').count(), 1);
    await axe(page);
  }
  await pressKeys(page, 'Space');
  assert.equal(await page.locator('button[aria-expanded="true"]').count(), 0); await axe(page);
  // Optional accordion arrow navigation is not installed, native page keys remain.
  await pressKeys(page, 'ArrowUp', 'ArrowDown', 'Home', 'End'); await assertFocus(page, '#third-button');
}
