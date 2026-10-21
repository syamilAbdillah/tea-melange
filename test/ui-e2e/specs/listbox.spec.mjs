import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'listbox');
  const button = id => page.locator(`#${id}-button`);
  const popup = id => page.locator(`#${id}-listbox`);
  const active = async (id, key) => {
    assert.equal(await popup(id).getAttribute('aria-activedescendant'), key === null ? null : `${id}-item-${key}`);
    if (key !== null) assert.equal(await page.locator(`#${id}-item-${key}`).getAttribute('data-active'), '');
  };
  const opened = async id => {
    await page.waitForFunction(id => document.activeElement?.id === id + '-listbox', id);
    assert.equal(await button(id).getAttribute('aria-expanded'), 'true');
    assert.equal(await popup(id).evaluate(el => el.hidden), false);
  };
  const closed = async id => {
    await page.waitForFunction(id => document.getElementById(id + '-listbox').hidden, id);
    assert.equal(await button(id).getAttribute('aria-expanded'), 'false');
    assert.equal(await popup(id).getAttribute('aria-activedescendant'), null);
    assert.equal(await popup(id).evaluate(el => el.matches(':popover-open')), false);
  };
  const open = async (id, key = 'ArrowDown') => {
    await button(id).scrollIntoViewIfNeeded();
    await button(id).focus();
    await pressKeys(page, key);
    await opened(id);
  };
  const escape = async id => { await pressKeys(page, 'Escape'); await closed(id); await assertFocus(page, `#${id}-button`); };
  const activity = () => page.locator('#activity').textContent();

  await ariaSnapshot(button('single'), '- button "Fruit Choose fruit": Choose fruit');
  await ariaSnapshot(popup('single'), '');
  await ariaSnapshot(button('disabled'), '- button "Disabled Choose fruit" [disabled]: Choose fruit');
  assert.equal(await button('invalid').getAttribute('aria-invalid'), 'true');
  await axe(page);

  // Every trigger opening key, selected precedence, and no native click replay.
  for (const key of ['Enter', 'Space', 'ArrowDown']) {
    await open('single', key); await active('single', 'apple');
    await assertFocus(page, '#single-listbox'); await escape('single');
  }
  await open('single', 'ArrowUp'); await active('single', 'pear'); await escape('single');
  await open('single');
  await ariaSnapshot(popup('single'), `- listbox "Fruit":
  - group "Orchard":
    - text: Orchard
    - option "Apple"
    - option "Apricot" [disabled]
    - option "Banana"
  - group "More fruit":
    - text: More fruit
    - option "Black currant"
    - option "Blueberry"
    - option "Cherry"
    - option "Date"
    - option "Elderberry"
    - option "Fig"
    - option "Grape"
    - option "Kiwi"
    - option "Lemon"
    - option "Mango"
    - option "Orange"
    - option "Pear"`);
  await axe(page);
  const unchanged = await activity();
  await pressKeys(page, 'ArrowDown'); await active('single', 'banana');
  await pressKeys(page, 'ArrowUp'); await active('single', 'apple');
  await pressKeys(page, 'ArrowUp'); await active('single', 'apple');
  await pressKeys(page, 'End', 'ArrowDown'); await active('single', 'pear');
  await pressKeys(page, 'Home'); await active('single', 'apple');
  await pressKeys(page, 'PageDown'); await active('single', 'kiwi');
  await pressKeys(page, 'PageUp'); await active('single', 'apple');
  assert.equal(await activity(), unchanged, 'Navigation never commits');
  await pressKeys(page, 'ArrowDown', 'Enter'); await closed('single');
  await assertFocus(page, '#single-button');
  await ariaSnapshot(button('single'), '- button "Fruit Banana": Banana');
  await axe(page);
  for (const key of ['ArrowUp', 'ArrowDown']) {
    await open('single', key); await active('single', 'banana'); await escape('single');
  }

  // Real timestamps: repeated-letter cycling, growing prefixes, embedded Space,
  // failed-prefix fallback and >=500ms buffer expiry (not synthetic clock edits).
  await open('single', 'b'); await active('single', 'black');
  await page.keyboard.press('b', { delay: 25 }); await frames(page); await active('single', 'blue');
  await pressKeys(page, 'Home');
  await page.keyboard.type('bl', { delay: 25 }); await frames(page); await active('single', 'black');
  await page.keyboard.type('ack ', { delay: 25 }); await frames(page);
  await active('single', 'black'); assert.equal(await button('single').getAttribute('aria-expanded'), 'true');
  await page.keyboard.type('c', { delay: 25 }); await frames(page); await active('single', 'black');
  await page.waitForTimeout(550); // The public typeahead timeout is the behavior under test.
  await page.keyboard.press('Space'); await frames(page); await closed('single');
  await ariaSnapshot(button('single'), '- button "Fruit Black currant": Black currant');
  await open('single');
  await page.keyboard.type('b', { delay: 25 }); await frames(page); await active('single', 'blue');
  await page.waitForTimeout(550);
  await page.keyboard.type('a', { delay: 25 }); await frames(page); await active('single', 'apple');
  for (const key of ['Control+ArrowDown', 'Alt+ArrowUp', 'Meta+Home']) await pressKeys(page, key);
  await active('single', 'apple');
  await popup('single').dispatchEvent('keydown', { key: 'End', isComposing: true }); await frames(page);
  await active('single', 'apple');
  await escape('single');

  // Tab/Shift+Tab close without committing and retain native focus traversal.
  await open('single'); await pressKeys(page, 'Home', 'Tab'); await closed('single');
  await assertFocus(page, '#multi-button');
  await ariaSnapshot(button('single'), '- button "Fruit Black currant": Black currant');
  await open('multi'); await pressKeys(page, 'Shift+Tab'); await closed('multi');
  await assertFocus(page, '#multi-button');

  // Pointer movement, not mouseenter; down preserves the list's DOM focus.
  await button('single').click(); await opened('single');
  await page.locator('#single-item-cherry').hover(); await frames(page); await active('single', 'cherry');
  const option = await page.locator('#single-item-cherry').boundingBox();
  await page.mouse.move(option.x + option.width / 2, option.y + option.height / 2);
  await page.mouse.down(); await frames(page); await assertFocus(page, '#single-listbox');
  await page.mouse.up(); await frames(page); await closed('single'); await assertFocus(page, '#single-button');
  await ariaSnapshot(button('single'), '- button "Fruit Cherry": Cherry');
  await button('single').click(); await opened('single');
  await page.locator('#single-item-apricot').click({ force: true }); await frames(page);
  assert.equal(await button('single').getAttribute('aria-expanded'), 'true');
  await page.locator('#outside').click(); await closed('single'); await assertFocus(page, '#outside');
  await button('single').click(); await opened('single');
  await page.locator('#plain-outside').click(); await closed('single');
  assert.notEqual(await page.evaluate(() => document.activeElement?.id), 'single-button', 'Nonfocusable outside click must not return focus');
  await button('single').click(); await opened('single');
  await button('single').click(); await closed('single'); await assertFocus(page, '#single-button');

  // Multiple toggles with no modifier requirement, tags and selected snapshots.
  await open('multi'); await pressKeys(page, 'Space', 'ArrowDown', 'Enter');
  await opened('multi'); await active('multi', 'banana');
  assert.equal(await popup('multi').getAttribute('aria-multiselectable'), 'true');
  await ariaSnapshot(page.locator('#multi-item-apple'), '- option "Apple" [selected]');
  await ariaSnapshot(page.locator('#multi-item-banana'), '- option "Banana" [selected]');
  await axe(page);
  await pressKeys(page, 'Enter');
  assert.equal(await page.locator('#multi-item-banana').getAttribute('aria-selected'), 'false');
  await escape('multi'); await page.locator('#tag-apple').click(); await frames(page);
  await ariaSnapshot(button('multi'), '- button "Tags Choose fruit": Choose fruit'); await axe(page);

  // Readonly browses but refuses both keyboard and pointer changes; invalid,
  // disabled, empty and RTL states all get unfiltered accessibility checks.
  const beforeReadonly = await activity();
  await open('readonly'); await active('readonly', 'banana');
  await pressKeys(page, 'End', 'Enter', 'Home', 'Space'); await opened('readonly');
  await page.locator('#readonly-item-cherry').click(); await frames(page); await opened('readonly');
  await ariaSnapshot(button('readonly'), '- button "Read only Banana" [expanded]: Banana');
  assert.equal(await popup('readonly').getAttribute('aria-readonly'), 'true');
  assert.equal(await activity(), beforeReadonly); await axe(page); await escape('readonly');
  await open('invalid'); assert.equal(await popup('invalid').getAttribute('aria-invalid'), 'true'); await axe(page); await escape('invalid');
  assert.equal(await button('disabled').isDisabled(), true);
  await open('empty'); await active('empty', null); await pressKeys(page, 'Home', 'End', 'PageUp', 'PageDown', 'Enter', 'Space');
  await opened('empty'); await ariaSnapshot(popup('empty'), '- listbox "Empty":\n  - option "No options available" [disabled]'); await axe(page); await escape('empty');
  await open('rtl'); await pressKeys(page, 'ArrowDown'); await active('rtl', 'banana');
  await pressKeys(page, 'ArrowLeft', 'ArrowRight'); await active('rtl', 'banana'); await axe(page); await escape('rtl');
  await open('wrap'); await pressKeys(page, 'ArrowUp'); await active('wrap', 'pear'); await pressKeys(page, 'ArrowDown'); await active('wrap', 'apple'); await escape('wrap');

  // Browser form serialization: one input per selected key, not the display text.
  assert.deepEqual(await page.locator('#fruit-form').evaluate(form => new FormData(form).getAll('fruits')), ['banana', 'cherry']);
  await open('form'); await pressKeys(page, 'Home', 'Enter', 'Escape'); await closed('form');
  assert.deepEqual(await page.locator('#fruit-form').evaluate(form => new FormData(form).getAll('fruits')), ['apple', 'banana', 'cherry']);
  await page.locator('#submit').click(); await frames(page);
  assert.equal(await page.locator('#submission').textContent(), 'Submitted: banana, cherry, apple');

  // A 200-row scroll container: page steps, endpoint clamping and active visibility.
  await open('long'); await pressKeys(page, 'PageDown'); await active('long', 'i010');
  await pressKeys(page, 'PageUp'); await active('long', 'i000');
  await pressKeys(page, 'End'); await active('long', 'i199');
  assert.equal(await popup('long').evaluate(el => el.scrollTop > 0), true);
  const inView = async () => popup('long').evaluate(el => {
    const option = document.getElementById(el.getAttribute('aria-activedescendant'));
    const p = el.getBoundingClientRect(), r = option.getBoundingClientRect();
    return r.top >= p.top && r.bottom <= p.bottom;
  });
  assert.equal(await inView(), true);
  await pressKeys(page, 'Home');
  await page.keyboard.type('Item 150', { delay: 25 }); await frames(page); await active('long', 'i150');
  assert.equal(await inView(), true); await axe(page); await escape('long');

  // Collection repair while open, with no outside-click dismissal from the controls.
  await open('dynamic'); await pressKeys(page, 'End');
  await page.locator('#remove-active').evaluate(el => el.click()); await frames(page); await active('dynamic', 'orange');
  await page.locator('#empty-dynamic').evaluate(el => el.click()); await frames(page); await active('dynamic', null);
  await pressKeys(page, 'Enter'); await opened('dynamic'); await axe(page);
  await page.locator('#restore-dynamic').evaluate(el => el.click()); await frames(page); await active('dynamic', 'apple'); await escape('dynamic');

  // Rapid intents share one stale DOM view; update alone decides each toggle.
  await button('single').scrollIntoViewIfNeeded();
  await button('single').focus();
  await button('single').evaluate(el => { el.click(); el.click(); el.click(); });
  await opened('single');
  await button('single').evaluate(el => { el.click(); el.click(); });
  await opened('single'); await escape('single');

  // Correctness without the example stylesheet (including native hidden).
  await page.evaluate(() => { for (const sheet of document.styleSheets) sheet.disabled = true; });
  await open('single'); await pressKeys(page, 'Home', 'ArrowDown'); await active('single', 'banana');
  await axe(page); await escape('single');
  await page.evaluate(() => { for (const sheet of document.styleSheets) sheet.disabled = false; });

  // Real leaving transitions retain Anchor/top-layer until Left and reverse.
  await page.emulateMedia({ reducedMotion: 'no-preference' });
  await page.addStyleTag({ content: '.popup[data-transition] { transition-duration: 1000ms; }' });
  await open('single');
  await page.waitForFunction(() => !document.querySelector('#single-listbox[data-transition]'));
  await page.keyboard.press('Escape');
  await page.waitForFunction(() => {
    const panel = document.querySelector('#single-listbox[data-leave]');
    const animations = panel?.getAnimations() ?? [];
    if (!animations.length) return false;
    animations.forEach(animation => animation.pause());
    return true;
  });
  assert.equal(await popup('single').evaluate(el => el.hidden), false);
  assert.equal(await popup('single').evaluate(el => el.matches(':popover-open')), true);
  assert.equal(await popup('single').evaluate(el => el.inert), true);
  await ariaSnapshot(popup('single'), '');
  await axe(page);
  await button('single').click(); await opened('single');
  await page.waitForFunction(() => !document.querySelector('#single-listbox[data-transition]'));
  await escape('single'); await axe(page);

  // Full runtime shutdown while a transition/positioning/focus lease is active.
  await button('single').click(); await opened('single');
  await page.evaluate(async () => { const { app } = await import('/examples/ui/listbox/main.mjs'); app.shutdown(); });
  await frames(page); await frames(page);
  assert.equal(await page.locator(':popover-open').count(), 0);
  await assertFocus(page, '#single-button');
}
