import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'popover');
  const trigger = name => page.locator(`#${name}-trigger`);
  const panel = name => page.locator(`#${name}-panel`);
  const open = async name => {
    await trigger(name).click();
    await page.waitForFunction(id => {
      const p = document.getElementById(id);
      return p.matches(':popover-open') && getComputedStyle(p).visibility === 'visible';
    }, name + '-panel');
    await frames(page);
    assert.equal(await trigger(name).getAttribute('aria-expanded'), 'true');
  };
  const closed = async name => {
    await page.waitForFunction(id => document.getElementById(id).hidden, name + '-panel');
    assert.equal(await trigger(name).getAttribute('aria-expanded'), 'false');
    assert.equal(await trigger(name).getAttribute('aria-controls'), null);
    assert.equal(await panel(name).evaluate(p => p.matches(':popover-open')), false);
  };
  const escape = async name => { await pressKeys(page, 'Escape'); await closed(name); await assertFocus(page, `#${name}-trigger`); };
  const activity = () => page.locator('#activity').textContent();
  const filterSnapshot = `- dialog "Filter specimens":
  - form "Specimen filters":
    - text: Common name
    - searchbox "Common name"
    - text: Growing conditions
    - combobox "Growing conditions":
      - option "All conditions" [selected]
      - option "Shade"
    - checkbox "In stock only"
    - text: In stock only
  - button "Done"`;

  await ariaSnapshot(trigger('filter'), '- button "Filter specimens"');
  await ariaSnapshot(panel('filter'), '');
  await axe(page);

  // Every trigger is immediately followed by its panel, including in top layer.
  for (const name of ['filter', 'plants', 'tools', 'animated', 'clipped', 'first', 'element', 'persistent']) {
    assert.equal(await trigger(name).evaluate(t => t.nextElementSibling.id), name + '-panel');
    assert.equal(await panel(name).getAttribute('popover'), 'manual');
  }
  await trigger('filter').focus();
  await pressKeys(page, 'Enter');
  await page.waitForFunction(() => getComputedStyle(document.querySelector('#filter-panel')).visibility === 'visible');
  await assertFocus(page, '#filter-trigger');
  await ariaSnapshot(trigger('filter'), '- button "Filter specimens" [expanded]');
  await ariaSnapshot(panel('filter'), filterSnapshot);
  assert.equal(await trigger('filter').getAttribute('aria-haspopup'), 'dialog');
  assert.equal(await panel('filter').getAttribute('aria-modal'), null);
  assert.equal(await trigger('filter').getAttribute('aria-controls'), 'filter-panel');
  await axe(page);

  // Native Tab order into the top-layer panel; no focus trap in either direction.
  await pressKeys(page, 'Tab'); await assertFocus(page, '#filter-search');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#filter-trigger');
  assert.equal(await trigger('filter').getAttribute('aria-expanded'), 'true');
  for (const id of ['filter-search', 'filter-kind', 'filter-stock', 'filter-close', 'after-filter']) {
    await pressKeys(page, 'Tab'); await assertFocus(page, '#' + id);
  }
  await closed('filter');
  assert.match(await activity(), /filter Closed\(Focus_out\)/);
  await axe(page);
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#filter-trigger');
  // Space toggles on keyup, without scrolling.
  const scroll = await page.evaluate(() => scrollY);
  await page.keyboard.down('Space'); await frames(page);
  assert.equal(await trigger('filter').getAttribute('aria-expanded'), 'false');
  assert.equal(await page.evaluate(() => scrollY), scroll);
  await page.keyboard.up('Space'); await frames(page);
  await page.waitForFunction(() => getComputedStyle(document.querySelector('#filter-panel')).visibility === 'visible');
  assert.equal(await trigger('filter').getAttribute('aria-expanded'), 'true');
  await assertFocus(page, '#filter-trigger');
  await escape('filter'); // Escape from trigger.
  assert.match(await activity(), /filter Closed\(Escape\)/);

  // Modified shortcuts and IME are never claimed; panel inputs keep native keys.
  await open('filter');
  await page.locator('#filter-search').focus();
  await page.locator('#filter-search').fill('fern');
  await pressKeys(page, 'Home', 'End', 'ArrowLeft', 'ArrowRight', 'ArrowDown', 'ArrowUp');
  assert.equal(await page.locator('#filter-search').inputValue(), 'fern');
  // Native search fields may clear on Escape chords; Popover must not claim them.
  await pressKeys(page, 'Control+Escape', 'Alt+Escape', 'Meta+Escape');
  assert.equal(await trigger('filter').getAttribute('aria-expanded'), 'true');
  assert.equal(await page.locator('#filter-search').evaluate(el => el.dispatchEvent(new KeyboardEvent('keydown', { key: 'Escape', isComposing: true, bubbles: true, cancelable: true }))), true);
  await frames(page);
  assert.equal(await trigger('filter').getAttribute('aria-expanded'), 'true');
  await escape('filter'); // Escape from content.
  await open('filter');
  await trigger('filter').click(); await closed('filter');
  assert.match(await activity(), /filter Closed\(Trigger\)/);
  await assertFocus(page, '#filter-trigger');

  // Outside pointerdown preserves the clicked destination; programmatic focus
  // outside and blur-to-body close without returning focus to the trigger.
  await open('filter'); await page.locator('#filter-search').focus();
  await page.locator('#after-filter').click(); await closed('filter');
  await assertFocus(page, '#after-filter');
  assert.match(await activity(), /filter Closed\(Outside\)/);
  await open('filter'); await page.locator('#filter-search').focus();
  await page.locator('h1').click(); await closed('filter');
  assert.notEqual(await page.evaluate(() => document.activeElement.id), 'filter-trigger');
  assert.match(await activity(), /filter Closed\(Outside\)/);
  await open('filter'); await page.locator('#filter-search').focus();
  await page.locator('#after-filter').focus(); await closed('filter');
  await assertFocus(page, '#after-filter');
  assert.match(await activity(), /filter Closed\(Focus_out\)/);
  await open('filter'); await page.locator('#filter-search').focus();
  await page.locator('#filter-search').evaluate(el => el.blur()); await closed('filter');
  assert.equal(await page.evaluate(() => document.activeElement.localName), 'body');
  await open('filter');
  await page.locator('#filter-close').focus(); await pressKeys(page, 'Enter'); await closed('filter');
  await assertFocus(page, '#filter-trigger');
  assert.match(await activity(), /filter Closed\(Programmatic\)/);

  // Group: focus on peer triggers does not flicker; opening a peer is exclusive.
  await open('plants');
  assert.equal(await trigger('plants').getAttribute('aria-haspopup'), null);
  assert.equal(await panel('plants').getAttribute('role'), null);
  await ariaSnapshot(panel('plants'), `- navigation "Plants links":
  - link "Ferns":
    - /url: "#collection"
  - link "Browse the collection":
    - /url: "#collection"
- button "Done"`);
  await axe(page);
  await page.locator('#plants-close').focus(); await pressKeys(page, 'Tab'); await assertFocus(page, '#tools-trigger');
  assert.equal(await trigger('plants').getAttribute('aria-expanded'), 'true');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#plants-close');
  await trigger('tools').focus();
  await pressKeys(page, 'Enter'); await closed('plants');
  assert.equal(await trigger('tools').getAttribute('aria-expanded'), 'true');
  await pressKeys(page, 'Tab'); await assertFocus(page, '#tools-link');
  await axe(page);
  await escape('tools');

  for (const [name, target] of [['first', 'first-target'], ['element', 'element-target']]) {
    await open(name); await page.waitForFunction(id => document.activeElement.id === id, target);
    await assertFocus(page, '#' + target);
    assert.equal(await panel(name).getAttribute('aria-labelledby'), name + '-trigger');
    await ariaSnapshot(panel(name), `- region "${name === 'first' ? 'Focus the first field' : 'Focus a specific field'}":
  - paragraph: Notes stay in the document's natural reading and Tab order.
  - text: Field notes
  - textbox "Field notes"
  - button "Done"`);
    await axe(page);
    await escape(name);
  }
  await open('persistent'); await page.locator('#open-filter').focus(); await frames(page);
  assert.equal(await trigger('persistent').getAttribute('aria-expanded'), 'true');
  await axe(page);
  await trigger('persistent').focus(); await escape('persistent');

  // Flip near viewport bottom. Only scroll the example: do not alter Anchor.
  await page.setViewportSize({ width: 1000, height: 420 });
  await trigger('clipped').evaluate(t => t.scrollIntoView({ block: 'end' }));
  await open('clipped');
  const placement = await panel('clipped').getAttribute('data-placement');
  assert.match(placement, /^top-/);
  await axe(page);
  await escape('clipped');
  // Restore roomy viewport and prove hit testing outside overflow:hidden +
  // transform, while retaining the panel's original DOM sibling relationship.
  await page.setViewportSize({ width: 1100, height: 900 });
  await trigger('clipped').evaluate(t => t.scrollIntoView({ block: 'start' }));
  await open('clipped');
  const escaped = await panel('clipped').evaluate(p => {
    const r = p.getBoundingClientRect(), c = p.closest('.clip').getBoundingClientRect();
    const x = r.left + r.width / 2, y = r.bottom - 12;
    return { outside: y > c.bottom || y < c.top, hit: p.contains(document.elementFromPoint(x, y)), top: p.matches(':popover-open'), sibling: p.previousElementSibling.id };
  });
  assert.deepEqual(escaped, { outside: true, hit: true, top: true, sibling: 'clipped-trigger' });
  await page.locator('#clipped-target').click(); await assertFocus(page, '#clipped-target');
  await escape('clipped');

  // Real transition reversal, including Leaving and immediate batched toggles.
  await page.emulateMedia({ reducedMotion: 'no-preference' });
  await open('animated');
  await page.waitForFunction(() => !document.querySelector('#animated-panel').hasAttribute('data-transition'));
  const arrow = await panel('animated').locator('.arrow').evaluate(a => ({ x: parseFloat(a.style.left), width: a.parentElement.getBoundingClientRect().width }));
  assert.ok(arrow.x >= 0 && arrow.x <= arrow.width - 12);
  await trigger('animated').click();
  await page.waitForFunction(() => document.querySelector('#animated-panel').hasAttribute('data-leave'));
  assert.equal(await panel('animated').evaluate(p => p.hidden), false);
  assert.equal(await trigger('animated').getAttribute('aria-controls'), 'animated-panel');
  assert.equal(await panel('animated').evaluate(p => p.matches(':popover-open')), true);
  await panel('animated').evaluate(p => p.getAnimations().forEach(a => a.pause()));
  await axe(page);
  await trigger('animated').click();
  await panel('animated').evaluate(p => p.getAnimations().forEach(a => a.play()));
  await page.waitForFunction(() => !document.querySelector('#animated-panel').hasAttribute('data-transition'));
  assert.equal(await trigger('animated').getAttribute('aria-expanded'), 'true');
  assert.equal(await panel('animated').evaluate(p => p.hidden), false);
  await axe(page);
  await trigger('animated').evaluate(t => { for (let i = 0; i < 100; i++) t.click(); });
  await frames(page);
  assert.equal(await trigger('animated').getAttribute('aria-expanded'), 'true');
  await trigger('animated').evaluate(t => { for (let i = 0; i < 101; i++) t.click(); });
  await closed('animated');
  await axe(page);
  await page.emulateMedia({ reducedMotion: 'reduce' });

  // Programmatic controls are silent, and unmount disposes promoted panels.
  await page.locator('#open-filter').click();
  await page.waitForFunction(() => document.querySelector('#filter-trigger').getAttribute('aria-expanded') === 'true');
  await assertFocus(page, '#open-filter');
  await page.locator('#close-filter').click(); await closed('filter');
  await assertFocus(page, '#close-filter');
  const styles = await page.evaluate(() => [document.documentElement.getAttribute('style'), document.body.getAttribute('style')]);
  await open('animated');
  await page.locator('#unmount').click(); await frames(page);
  assert.equal(await page.locator('[popover]').count(), 0);
  assert.equal(await page.locator(':popover-open').count(), 0);
  assert.deepEqual(await page.evaluate(() => [document.documentElement.getAttribute('style'), document.body.getAttribute('style')]), styles);
  await assertFocus(page, '#unmount');
  await axe(page);
  await page.locator('#mount').click(); await frames(page);
  await ariaSnapshot(trigger('filter'), '- button "Filter specimens"');
  await ariaSnapshot(panel('filter'), '');
  await axe(page);
}
