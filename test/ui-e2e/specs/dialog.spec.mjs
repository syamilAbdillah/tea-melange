import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'dialog');
  const root = key => page.locator(`#${key}-dialog`);
  const settled = async (key, open) => {
    await page.waitForFunction(({key, open}) => {
      const el = document.getElementById(`${key}-dialog`);
      return el.open === open && !el.hasAttribute('data-transition');
    }, {key, open});
    await frames(page);
  };
  const show = async key => { await page.locator(`#open-${key}`).click(); await settled(key, true); };
  const escape = async key => { await pressKeys(page, 'Escape'); await settled(key, false); };
  await ariaSnapshot(root('basic'), '');
  await axe(page);
  const before = await page.locator('#open-basic').boundingBox();
  await show('basic');
  await assertFocus(page, '#collection-name');
  await ariaSnapshot(root('basic'), `
- dialog "Edit collection":
  - heading "Edit collection" [level=2]
  - text: Collection name
  - textbox "Collection name": Field notes
  - button "Save"
  - button "Choose collection"
  - button "Remove opener"
  - button "Close"
  `);
  await axe(page);
  assert.equal(await root('basic').getAttribute('aria-modal'), 'true');
  assert.equal(await root('basic').getAttribute('aria-describedby'), null);
  // Native top-layer containment, including both boundaries. Never cancel Tab.
  for (let i = 0; i < 14; ++i) {
    await pressKeys(page, i < 7 ? 'Tab' : 'Shift+Tab');
    assert.equal(await root('basic').evaluate(el => el.contains(document.activeElement)), true, `Tab cycle ${i} stays inside`);
  }
  const during = await page.locator('#open-basic').boundingBox();
  assert.equal(during.x, before.x, 'scrollbar compensation prevents layout shift');
  const scroll = await page.evaluate(() => scrollY);
  await page.mouse.move(5, 5); await page.mouse.wheel(0, 800); await frames(page);
  assert.equal(await page.evaluate(() => scrollY), scroll, 'background is scroll locked');
  await page.locator('#collection-name').focus();
  for (const key of ['Control+Escape', 'Alt+Escape', 'Meta+Escape', 'ArrowDown', 'Home', 'End']) {
    await pressKeys(page, key);
    assert.equal(await root('basic').evaluate(el => el.open), true, key);
  }
  await pressKeys(page, 'F2');
  assert.match(await page.locator('#activity').textContent(), /keyboard help/);
  await page.locator('#basic-close').focus(); await pressKeys(page, 'Space');
  await settled('basic', false); await assertFocus(page, '#open-basic'); await axe(page);
  assert.equal(await page.evaluate(() => document.documentElement.style.overflow), '');

  await page.locator('#open-basic').focus(); await pressKeys(page, 'Enter'); await settled('basic', true);
  await page.locator('#save').focus(); await pressKeys(page, 'Enter'); await settled('basic', false);
  assert.match(await page.locator('#activity').textContent(), /value=saved/);
  await assertFocus(page, '#open-basic');

  await show('basic');
  await page.locator('#basic-dialog form').evaluate(form => form.method = 'post');
  await page.locator('#save').evaluate(button => button.setAttribute('formmethod', 'dialog'));
  await page.locator('#save').click(); await settled('basic', false);
  assert.match(await page.locator('#activity').textContent(), /value=saved/);
  await page.locator('#basic-dialog form').evaluate(form => form.method = 'dialog');
  await page.locator('#save').evaluate(button => button.removeAttribute('formmethod'));
  await show('basic');
  await page.locator('#basic-dialog form').evaluate(form => form.requestSubmit());
  await page.waitForFunction(() => document.getElementById('activity').textContent.includes('basic closed; value='));
  await settled('basic', false);
  assert.match(await page.locator('#activity').textContent(), /value=;/);
  await assertFocus(page, '#open-basic');

  await show('basic');
  assert.equal(await page.locator('#collection-name').evaluate(el => {
    const event = new KeyboardEvent('keydown', {key: 'Escape', bubbles: true, cancelable: true, isComposing: true});
    return el.dispatchEvent(event);
  }), true, 'composing Escape is untouched');
  await frames(page);
  assert.equal(await root('basic').evaluate(el => el.open), true);
  // A drag from content to backdrop must not dismiss; a real backdrop click does.
  const box = await root('basic').boundingBox();
  await page.mouse.move(box.x + 20, box.y + 20); await page.mouse.down();
  await page.mouse.move(5, 5); await page.mouse.up(); await frames(page);
  assert.equal(await root('basic').evaluate(el => el.open), true);
  await page.mouse.click(box.x + 3, box.y + 3); await frames(page);
  assert.equal(await root('basic').evaluate(el => el.open), true, 'border/content box click is not backdrop');
  await page.mouse.click(5, 5); await settled('basic', false); await assertFocus(page, '#open-basic');

  await show('alert'); await assertFocus(page, '#alert-cancel');
  await ariaSnapshot(root('alert'), `
- alertdialog "Delete collection?":
  - heading "Delete collection?" [level=2]
  - paragraph: This permanently deletes the collection. Cancel keeps it safe.
  - button "Delete collection"
  - button "Cancel"
  `);
  assert.equal(await root('alert').getAttribute('aria-describedby'), 'alert-description');
  await axe(page);
  await page.mouse.click(5, 5); await frames(page);
  assert.equal(await root('alert').evaluate(el => el.open), true, 'alert backdrop default is off');
  for (let i = 0; i < 5; ++i) await pressKeys(page, 'Escape');
  assert.equal(await root('alert').evaluate(el => el.open), true, 'parent rejects repeated Escape without native close-watcher desync');
  assert.match(await page.locator('#activity').textContent(), /requested Escape/);
  await page.locator('#alert-cancel').click(); await settled('alert', false); await assertFocus(page, '#open-alert');
  await page.locator('#guard').check(); await frames(page); await show('alert');
  await page.locator('#alert-cancel').click(); await frames(page);
  assert.equal(await root('alert').evaluate(el => el.open), true);
  await root('alert').evaluate(el => el.close('imperative-guarded'));
  await page.waitForFunction(() => document.getElementById('alert-dialog').matches(':modal'));
  await frames(page);
  assert.match(await page.locator('#activity').textContent(), /requested Native/);
  assert.equal(await root('alert').evaluate(el => el.returnValue), 'imperative-guarded');
  await assertFocus(page, '#alert-cancel');
  await axe(page);
  await page.locator('#confirm').click(); await settled('alert', false);
  assert.match(await page.locator('#activity').textContent(), /deleted=1/); await axe(page);

  await show('alert-backdrop');
  await assertFocus(page, '#alert-backdrop-close');
  assert.match(await root('alert-backdrop').ariaSnapshot(), /alertdialog "Optional backdrop dismissal"/);
  await axe(page);
  await page.mouse.click(5, 5); await settled('alert-backdrop', false);
  await assertFocus(page, '#open-alert-backdrop');
  await axe(page);

  await show('missing');
  await page.mouse.click(5, 5); await frames(page);
  assert.equal(await root('missing').evaluate(el => el.open), true, 'modal backdrop optout');
  await root('missing').evaluate(el => el.close('imperative'));
  // close() mutates DOM synchronously, before its queued close event reaches
  // Tea. Wait for model completion, not that transient native closed state.
  await page.waitForFunction(() => document.getElementById('activity').textContent.includes('missing closed; value=imperative'));
  await settled('missing', false);
  assert.match(await page.locator('#activity').textContent(), /value=imperative/);
  await assertFocus(page, '#open-missing');
  await axe(page);

  await show('basic'); await page.locator('#open-nested').click(); await settled('nested', true);
  await assertFocus(page, '#nested-close'); await axe(page);
  await escape('nested');
  assert.equal(await root('basic').evaluate(el => el.open), true, 'only top dialog closes');
  await assertFocus(page, '#open-nested');
  assert.equal(await page.evaluate(() => document.documentElement.style.overflow), 'hidden', 'nested release keeps outer lock');
  await escape('basic'); await assertFocus(page, '#open-basic');

  for (const [key, focus] of [['empty', '#empty-dialog'], ['missing', '#missing-close'], ['long', '#long-title'], ['label', '#label-close']]) {
    await show(key); await assertFocus(page, focus); await axe(page);
    assert.match(await root(key).ariaSnapshot(), /dialog/);
    if (key === 'label') {
      assert.equal(await root(key).getAttribute('aria-labelledby'), null);
      assert.equal(await root(key).getAttribute('aria-label'), 'Label without a title');
    }
    if (key === 'long') {
      await page.locator('#long-close').scrollIntoViewIfNeeded();
      assert.ok(await root(key).evaluate(el => el.scrollTop) > 0);
    }
    await escape(key); await assertFocus(page, `#open-${key}`); await axe(page);
  }

  // Deliberately transfer focus to an independently owned top-layer modal
  // during Leaving. Closing the older dialog must not steal that focus.
  await page.emulateMedia({reducedMotion: 'no-preference'});
  await show('animated');
  await page.locator('#leave').click();
  await page.waitForFunction(() => document.getElementById('animated-dialog').hasAttribute('data-leave'));
  await page.evaluate(() => {
    const destination = document.createElement('dialog');
    destination.id = 'focus-destination';
    destination.setAttribute('aria-label', 'Independent dialog');
    destination.innerHTML = '<button id="intentional-focus" type="button">Keep focus here</button>';
    document.body.append(destination);
    destination.showModal();
    destination.querySelector('button').focus();
  });
  await settled('animated', false);
  await assertFocus(page, '#intentional-focus');
  await axe(page);
  await page.evaluate(() => {
    const destination = document.getElementById('focus-destination');
    destination.close(); destination.remove();
  });
  await page.emulateMedia({reducedMotion: 'reduce'});

  await show('basic'); await page.locator('#remove-opener').click(); await frames(page);
  await escape('basic');
  assert.equal(await page.locator('#open-basic').count(), 0);
  assert.equal(await page.evaluate(() => document.activeElement.isConnected), true);

  // Real transitions, reversible leave, Escape in enter/leave, and native open
  // retained throughout leaving. Backdrop animation uses the same data flags.
  await page.emulateMedia({reducedMotion: 'no-preference'});
  await show('animated');
  await page.locator('#leave').click();
  await page.waitForFunction(() => {
    const el = document.getElementById('animated-dialog');
    if (!el.hasAttribute('data-leave') || !el.getAnimations({subtree: true}).length) return false;
    el.getAnimations({subtree: true}).forEach(a => a.pause());
    return true;
  });
  assert.equal(await root('animated').getAttribute('data-leave'), '');
  assert.equal(await root('animated').evaluate(el => el.open && el.matches(':modal')), true);
  assert.match(await root('animated').ariaSnapshot(), /dialog "A measured entrance"/);
  await axe(page);
  // Resume and send a real pointer click without Playwright waiting for the
  // moving control to become stable (which would wait until after unmount).
  const reverse = await page.locator('#reverse').boundingBox();
  await root('animated').evaluate(el => el.getAnimations({subtree: true}).forEach(a => a.play()));
  await page.mouse.click(reverse.x + reverse.width / 2, reverse.y + reverse.height / 2);
  await settled('animated', true);
  assert.equal(await root('animated').evaluate(el => el.matches(':modal')), true);
  await pressKeys(page, 'Escape', 'Escape', 'Escape'); await settled('animated', false);
  await assertFocus(page, '#open-animated');
  await page.addStyleTag({content: 'dialog.animated[data-transition] {transition-duration:1200ms}'});
  await page.locator('#open-animated').click();
  await page.waitForFunction(() => {
    const el = document.getElementById('animated-dialog');
    const animations = el.getAnimations({subtree: true});
    if (!el.open || !el.hasAttribute('data-enter') || !animations.some(a => a.currentTime > 300)) return false;
    animations.forEach(a => a.pause());
    return true;
  });
  assert.match(await root('animated').ariaSnapshot(), /dialog "A measured entrance"/);
  await axe(page);
  await root('animated').evaluate(el => el.getAnimations({subtree: true}).forEach(a => a.play()));
  await pressKeys(page, 'Escape', 'Escape');
  assert.equal(await root('animated').evaluate(el => el.open), true, 'Escape during leave retains top layer');
  await settled('animated', false);
  await axe(page);
  // No CSS is necessary for semantics/lifecycle correctness.
  await page.addStyleTag({content: 'dialog,dialog::backdrop {transition:none!important;animation:none!important}'});
  await show('missing'); await escape('missing');
  await ariaSnapshot(root('missing'), ''); await axe(page);
}
