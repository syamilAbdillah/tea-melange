import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'combobox');
  const input = id => page.locator(`#${id}-control`);
  const popup = id => page.locator(`#${id}-popup`);
  const value = id => input(id).inputValue();
  const expanded = id => input(id).getAttribute('aria-expanded');
  const active = id => input(id).evaluate(el => document.getElementById(el.getAttribute('aria-activedescendant'))?.textContent ?? null);
  const visible = async id => {
    await page.waitForFunction(id => {
      const p = document.getElementById(id + '-popup');
      return !p.hidden && p.matches(':popover-open') && getComputedStyle(p).visibility === 'visible';
    }, id);
  };
  const closed = async id => {
    await page.waitForFunction(id => document.getElementById(id + '-popup').hidden, id);
    assert.equal(await expanded(id), 'false');
    assert.equal(await input(id).getAttribute('aria-controls'), null);
    assert.equal(await input(id).getAttribute('aria-activedescendant'), null);
    assert.equal(await popup(id).evaluate(el => el.matches(':popover-open')), false);
  };
  const fill = async (id, text) => { await input(id).fill(text); await frames(page); };
  const focus = async id => { await input(id).scrollIntoViewIfNeeded(); await input(id).focus(); };
  const escape = async id => { await pressKeys(page, 'Escape'); await closed(id); };
  const tagButtons = () => page.locator('[id^="tags-chip-"]');
  const results = (id, text) => page.waitForFunction(({id, text}) => document.getElementById(id + '-live').textContent === text, {id, text});

  await ariaSnapshot(input('country'), '- combobox "Country or territory"');
  await ariaSnapshot(popup('country'), '');
  assert.equal(await popup('country').locator('[role=option]').count(), 250);
  await axe(page); // Closed, disabled, invalid, read-only states all present.
  assert.equal(await input('disabled').isDisabled(), true);
  assert.equal(await input('readonly').getAttribute('readonly'), '');
  assert.equal(await input('invalid').getAttribute('aria-errormessage'), 'invalid-error');

  // Down opens with first; Up wraps; Alt opens without moving active.
  await focus('country'); await pressKeys(page, 'ArrowDown'); await visible('country');
  assert.equal(await active('country'), 'Andorra');
  assert.equal(await input('country').getAttribute('aria-controls'), 'country-popup');
  await assertFocus(page, '#country-control');
  await pressKeys(page, 'ArrowUp'); assert.equal(await active('country'), 'Kosovo');
  await pressKeys(page, 'ArrowDown'); assert.equal(await active('country'), 'Andorra');
  await results('country', '250 results available');
  await axe(page);
  await escape('country'); await pressKeys(page, 'Alt+ArrowDown'); await visible('country');
  assert.equal(await active('country'), null);
  await pressKeys(page, 'Alt+ArrowUp'); await closed('country');
  await pressKeys(page, 'ArrowUp'); await visible('country'); assert.equal(await active('country'), 'Kosovo');
  await escape('country');

  // Parent filtering and exact open/closed ARIA snapshots, selected state.
  await fill('country', 'Can'); await visible('country'); await results('country', '1 results available');
  assert.equal(await active('country'), null);
  await ariaSnapshot(input('country'), '- combobox "Country or territory" [expanded]: Can');
  await ariaSnapshot(popup('country'), '- listbox "Country or territory":\n  - option "Canada"');
  await pressKeys(page, 'ArrowDown'); assert.equal(await active('country'), 'Canada');
  await axe(page);
  await pressKeys(page, 'Enter'); await closed('country'); assert.equal(await value('country'), 'Canada');
  await ariaSnapshot(input('country'), '- combobox "Country or territory": Canada');
  await pressKeys(page, 'Alt+ArrowDown'); await visible('country');
  await ariaSnapshot(popup('country'), '- listbox "Country or territory":\n  - option "Canada" [selected]');
  await axe(page);
  await escape('country');
  await fill('country', 'unmatched'); await results('country', 'No results');
  assert.equal(await popup('country').locator('[role=option]').count(), 0);
  await axe(page);
  // Tab closes without selecting and applies restricted revert via blur.
  await pressKeys(page, 'Tab'); await closed('country'); await assertFocus(page, '#tags-control');
  assert.equal(await value('country'), 'Canada');
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#country-control');
  assert.equal(await expanded('country'), 'false');

  // Native editing/caret keys, shortcuts, same-frame intents and clear Escape.
  await fill('country', 'Canada');
  await pressKeys(page, 'Home'); assert.equal(await input('country').evaluate(el => el.selectionStart), 0);
  await pressKeys(page, 'End'); assert.equal(await input('country').evaluate(el => el.selectionStart), 6);
  await pressKeys(page, 'ArrowLeft'); assert.equal(await input('country').evaluate(el => el.selectionStart), 5);
  await pressKeys(page, 'ArrowRight'); assert.equal(await input('country').evaluate(el => el.selectionStart), 6);
  await pressKeys(page, 'Backspace'); assert.equal(await value('country'), 'Canad');
  await page.keyboard.type('a'); await frames(page);
  await pressKeys(page, 'Control+ArrowDown', 'Meta+ArrowDown', 'Alt+Enter');
  assert.equal(await active('country'), null);
  await escape('country'); await pressKeys(page, 'Escape'); assert.equal(await value('country'), '');
  await input('country').evaluate(el => {
    for (let i = 0; i < 3; i++) el.dispatchEvent(new KeyboardEvent('keydown', {key: 'ArrowDown', bubbles: true, cancelable: true}));
  });
  await frames(page); assert.equal(await active('country'), 'Afghanistan');
  // The active option, including the last in a large list, is scrolled after draw.
  await escape('country'); await pressKeys(page, 'ArrowUp'); await visible('country'); await frames(page);
  assert.equal(await popup('country').evaluate(p => {
    const a = document.getElementById(document.querySelector('#country-control').getAttribute('aria-activedescendant'));
    return a.getBoundingClientRect().top >= p.getBoundingClientRect().top && a.getBoundingClientRect().bottom <= p.getBoundingClientRect().bottom;
  }), true);
  await escape('country');

  // Pointermove, pointerdown blur suppression, click selection, outside focus.
  await fill('country', 'Jap'); await visible('country');
  await popup('country').getByRole('option', {name: 'Japan', exact: true}).hover(); await frames(page);
  assert.equal(await active('country'), 'Japan');
  await popup('country').getByRole('option', {name: 'Japan', exact: true}).click(); await closed('country');
  assert.equal(await value('country'), 'Japan'); await assertFocus(page, '#country-control');
  await page.locator('#country-toggle').click(); await visible('country'); await assertFocus(page, '#country-control');
  await page.locator('#country-toggle').click(); await closed('country');
  await fill('country', 'bogus'); await input('tags').click(); await closed('country');
  assert.equal(await value('country'), 'Japan'); await assertFocus(page, '#tags-control');

  // Multiple suggestions, removable chips, free-form tags and Backspace.
  await fill('tags', 'Can'); await visible('tags'); await pressKeys(page, 'ArrowDown', 'Enter');
  assert.equal(await expanded('tags'), 'true'); assert.equal(await value('tags'), '');
  await tagButtons().getByRole('button', {name: 'Remove Canada', exact: true}).waitFor();
  await fill('tags', 'Jap'); await pressKeys(page, 'ArrowDown', 'Enter');
  await tagButtons().getByRole('button', {name: 'Remove Japan', exact: true}).waitFor();
  await axe(page);
  await pressKeys(page, 'Backspace'); assert.equal(await tagButtons().getByRole('button', {name: 'Remove Japan', exact: true}).count(), 0);
  await tagButtons().getByRole('button', {name: 'Remove Canada', exact: true}).click(); await assertFocus(page, '#tags-control');
  await fill('tags', 'Weekend trip'); await pressKeys(page, 'Enter');
  await tagButtons().getByRole('button', {name: 'Remove Weekend trip', exact: true}).waitFor();
  assert.equal(await value('tags'), '');
  await pressKeys(page, 'Escape'); await closed('tags');

  // Simulated remote response: immediate busy + polite loading, latest query wins.
  await fill('remote', 'Ca');
  await page.waitForFunction(() => document.querySelector('#remote-popup').getAttribute('aria-busy') === 'true');
  await results('remote', 'Loading');
  await axe(page);
  await fill('remote', 'Jap'); await results('remote', '1 results available'); await visible('remote');
  assert.equal(await popup('remote').getAttribute('aria-busy'), 'false');
  await ariaSnapshot(popup('remote'), '- listbox "Search destinations":\n  - option "Japan"');
  await axe(page);
  await escape('remote');

  // Inline completion range, Backspace-first policy, native suffix replacement.
  await focus('inline'); await page.keyboard.type('Can'); await frames(page); await visible('inline');
  await page.waitForFunction(() => document.querySelector('#inline-control').value === 'Canada');
  assert.deepEqual(await input('inline').evaluate(el => [el.selectionStart, el.selectionEnd]), [3, 6]);
  await ariaSnapshot(input('inline'), '- combobox "Inline completion" [expanded]: Canada');
  await axe(page);
  await pressKeys(page, 'Backspace'); assert.equal(await value('inline'), 'Can');
  await page.keyboard.type('a'); await frames(page);
  assert.equal(await value('inline'), 'Canada');
  await pressKeys(page, 'Enter'); await closed('inline'); assert.equal(await value('inline'), 'Canada');

  // Composition events suppress query/selection until compositionend.
  await focus('none');
  await input('none').evaluate(el => {
    el.dispatchEvent(new CompositionEvent('compositionstart', {bubbles: true}));
    el.value = 'Jap';
    el.dispatchEvent(new InputEvent('input', {data: 'Jap', isComposing: true, bubbles: true}));
    for (const key of ['Enter', 'ArrowDown', 'Escape']) {
      if (!el.dispatchEvent(new KeyboardEvent('keydown', {key, isComposing: true, bubbles: true, cancelable: true}))) throw new Error('IME key claimed');
    }
  });
  await frames(page); assert.equal(await expanded('none'), 'false'); assert.equal(await value('none'), 'Jap');
  await input('none').evaluate(el => el.dispatchEvent(new CompositionEvent('compositionend', {data: 'Jap', bubbles: true})));
  await visible('none'); await results('none', '1 results available');
  assert.equal(await input('none').getAttribute('aria-autocomplete'), 'none');
  // No active Enter submits text, but prevents form submit while open.
  const before = await page.locator('#activity').textContent();
  await pressKeys(page, 'Enter'); await closed('none');
  assert.match(await page.locator('#activity').textContent(), /Submitted: Jap/);
  assert.equal((await page.locator('#activity').textContent()).split('Form submissions: ')[1], before.split('Form submissions: ')[1]);
  // This form has several text inputs and no submit button: Chromium does not
  // implicitly submit it, but closed Enter must remain unprevented.
  assert.equal(await input('none').evaluate(el => el.dispatchEvent(new KeyboardEvent('keydown', {key: 'Enter', bubbles: true, cancelable: true}))), true);
  await axe(page);

  await fill('clear', 'Can'); await pressKeys(page, 'ArrowDown', 'Enter');
  await fill('clear', 'invalid'); await input('readonly').click(); await closed('clear'); assert.equal(await value('clear'), '');
  // Read-only can navigate but never selects or clears.
  await pressKeys(page, 'ArrowDown'); await visible('readonly'); await pressKeys(page, 'Enter');
  assert.equal(await value('readonly'), ''); await pressKeys(page, 'Escape'); await closed('readonly');
  await fill('rtl', 'Can'); await visible('rtl'); await pressKeys(page, 'Home', 'End');
  assert.equal(await input('rtl').evaluate(el => el.selectionStart), 3);
  assert.match(await popup('rtl').getAttribute('data-placement'), /-start$/); await axe(page); await escape('rtl');

  // Layering: each claimed Escape is consumed by Combobox, not modal Dialog.
  await page.locator('#show-modal').click();
  await page.waitForFunction(() => document.querySelector('#destination-dialog').open);
  await assertFocus(page, '#nested-control');
  await fill('nested', 'Can'); await visible('nested');
  await axe(page);
  await pressKeys(page, 'Escape'); await closed('nested');
  assert.equal(await page.locator('#destination-dialog').evaluate(el => el.open), true);
  assert.equal(await value('nested'), 'Can');
  await pressKeys(page, 'Escape'); assert.equal(await value('nested'), '');
  assert.equal(await page.locator('#destination-dialog').evaluate(el => el.open), true);
  await pressKeys(page, 'Escape'); // Nothing claimed: ancestor handles this one.
  await page.waitForFunction(() => !document.querySelector('#destination-dialog').open);
  await assertFocus(page, '#show-modal'); await axe(page);

  // Real leave keeps Anchor promoted until Animation.Left; rapid reversal.
  await page.emulateMedia({reducedMotion: 'no-preference'});
  await focus('country'); await page.locator('#country-toggle').click(); await visible('country');
  await page.waitForFunction(() => !document.querySelector('#country-popup').hasAttribute('data-transition'));
  await page.locator('#country-toggle').evaluate(el => {el.click(); el.click(); el.click();});
  await page.waitForFunction(() => document.querySelector('#country-popup').hasAttribute('data-leave'));
  assert.equal(await popup('country').evaluate(el => !el.hidden && el.matches(':popover-open')), true);
  await page.locator('#country-toggle').click(); await visible('country');
  await page.waitForFunction(() => !document.querySelector('#country-popup').hasAttribute('data-transition'));
  assert.equal(await expanded('country'), 'true');
  await escape('country'); await axe(page);
}
