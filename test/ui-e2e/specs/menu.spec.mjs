import assert from 'node:assert/strict';
import { openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe } from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'menu');
  const root = id => `#${id}-menu`;
  const sub = (id, key) => `#${id}-submenu-item-${key}`;
  const button = id => `#${id}-button`;
  const item = (id, key) => `#${id}-item-${key}`;
  const active = async (selector, id, key) => assert.equal(await page.locator(selector).getAttribute('aria-activedescendant'), `${id}-item-${key}`);
  const focused = async selector => {
    await page.waitForFunction(s => document.activeElement === document.querySelector(s), selector);
    await assertFocus(page, selector);
  };
  const open = async (id = 'file', key = 'ArrowDown') => {
    await page.locator(button(id)).focus(); await pressKeys(page, key); await focused(root(id));
    assert.equal(await page.locator(root(id)).evaluate(el => el.hidden), false);
    assert.equal(await page.locator(button(id)).getAttribute('aria-expanded'), 'true');
  };
  const closed = async id => {
    await page.waitForFunction(s => document.querySelector(s).hidden, root(id));
    assert.equal(await page.locator(button(id)).getAttribute('aria-expanded'), 'false');
    assert.equal(await page.locator(root(id)).evaluate(el => el.matches(':popover-open')), false);
  };
  const check = async () => {
    // Every rendered idref resolves; no duplicate ids, including submenu/label namespaces.
    await page.evaluate(() => {
      const ids = [...document.querySelectorAll('[id]')].map(el => el.id);
      if (new Set(ids).size !== ids.length) throw Error('duplicate ids');
      for (const el of document.querySelectorAll('[aria-controls],[aria-labelledby],[aria-activedescendant]')) {
        for (const name of ['aria-controls', 'aria-labelledby', 'aria-activedescendant'])
          for (const id of (el.getAttribute(name) || '').split(' ').filter(Boolean))
            if (!document.getElementById(id)) throw Error('unresolved ' + id);
      }
    });
    await axe(page);
  };
  const fileSnapshot = `- menu "File actions":
  - menuitem "New file"
  - menuitem "Copy"
  - menuitem "Delete" [disabled]
  - separator
  - menuitem "Share"
  - menuitem "Export"`;
  await ariaSnapshot(page.locator(button('file')), '- button "File actions"');
  await check();
  for (const key of ['Enter', 'Space', 'ArrowDown']) {
    await open('file', key); await active(root('file'), 'file', 'new');
    await ariaSnapshot(page.locator(root('file')), fileSnapshot); await check();
    await pressKeys(page, 'Escape'); await closed('file'); await focused(button('file'));
  }
  await open('file', 'ArrowUp'); await active(root('file'), 'file', 'export');
  await pressKeys(page, 'ArrowDown'); await active(root('file'), 'file', 'new');
  await pressKeys(page, 'ArrowUp'); await active(root('file'), 'file', 'export');
  await pressKeys(page, 'Home', 'ArrowDown', 'ArrowDown'); await active(root('file'), 'file', 'delete');
  const beforeDisabled = await page.locator('#activity').textContent();
  await pressKeys(page, 'Enter', 'Space');
  assert.equal(await page.locator('#activity').textContent(), beforeDisabled);
  await check();
  await pressKeys(page, 'End'); await active(root('file'), 'file', 'export');
  await pressKeys(page, 'c'); await active(root('file'), 'file', 'copy');
  await pressKeys(page, 'o'); await active(root('file'), 'file', 'copy');
  await pressKeys(page, 'Control+Home', 'Alt+End', 'Meta+ArrowDown'); await active(root('file'), 'file', 'copy');
  await page.locator(root('file')).evaluate(el => el.dispatchEvent(new KeyboardEvent('keydown', {key:'End', isComposing:true, bubbles:true, cancelable:true})));
  await frames(page); await active(root('file'), 'file', 'copy');
  await pressKeys(page, 'Enter'); await closed('file'); await focused(button('file'));
  assert.match(await page.locator('#activity').textContent(), /Selected copy/); await check();

  // Native Tab default starts at the trigger, not at a floating subtree's DOM position.
  await open(); await pressKeys(page, 'Tab'); await closed('file'); await focused(button('view'));
  await open(); await pressKeys(page, 'Shift+Tab'); await closed('file'); await focused('#before');

  await open('view');
  await ariaSnapshot(page.locator(root('view')), `- menu "View":
  - menuitemcheckbox "Status bar" [checked]
  - menuitemcheckbox "Grid"
  - group "Density":
    - text: Density
    - menuitemradio "Comfortable" [checked]
    - menuitemradio "Compact"`);
  await check();
  await pressKeys(page, 'Space');
  assert.equal(await page.locator(item('view','status')).getAttribute('aria-checked'), 'false');
  await focused(root('view')); await check();
  await pressKeys(page, 'End', 'Space');
  assert.equal(await page.locator(item('view','compact')).getAttribute('aria-checked'), 'true');
  assert.equal(await page.locator(item('view','comfortable')).getAttribute('aria-checked'), 'false');
  await check(); await pressKeys(page, 'Enter'); await closed('view'); await focused(button('view'));
  await open('view'); await pressKeys(page, 'Enter'); await closed('view');

  // Arbitrary-depth submenu navigation, all opening gestures and RTL mirrors.
  for (const id of ['file', 'rtl']) {
    const forward = id === 'rtl' ? 'ArrowLeft' : 'ArrowRight';
    const back = id === 'rtl' ? 'ArrowRight' : 'ArrowLeft';
    for (const opener of [forward, 'Enter', 'Space']) {
      await open(id); await pressKeys(page, 'End', 'ArrowUp', opener);
      await focused(sub(id,'share')); await active(sub(id,'share'), id, 'link');
      assert.equal(await page.locator(item(id,'share')).getAttribute('aria-expanded'), 'true');
      assert.equal(await page.locator(sub(id,'share')).getAttribute('data-placement'), id === 'rtl' ? 'left-start' : 'right-start');
      await ariaSnapshot(page.locator(sub(id,'share')), '- menu "Share":\n  - menuitem "Copy link"\n  - menuitem "Team"'); await check();
      await pressKeys(page, 'ArrowDown', forward); await focused(sub(id,'team'));
      await ariaSnapshot(page.locator(sub(id,'team')), '- menu "Team":\n  - menuitem "Design team"\n  - menuitem "Engineering team"'); await check();
      await pressKeys(page, 'Escape'); await focused(sub(id,'share')); await active(sub(id,'share'), id, 'team');
      await pressKeys(page, back); await focused(root(id)); await active(root(id), id, 'share');
      await pressKeys(page, forward); await focused(sub(id,'share'));
      await pressKeys(page, 'Enter'); await closed(id); await focused(button(id));
      assert.match(await page.locator('#activity').textContent(), /Selected link/);
    }
  }

  // Pointermove (not enter) activates, disabled click is inert, and hover delay
  // gives a grace interval across adjacent rows while travelling diagonally.
  await page.locator(button('file')).click(); await focused(root('file'));
  await page.locator(item('file','delete')).hover(); await frames(page); await active(root('file'), 'file', 'delete');
  const disabledBox = await page.locator(item('file','delete')).boundingBox();
  await page.mouse.click(disabledBox.x + disabledBox.width / 2, disabledBox.y + disabledBox.height / 2); await frames(page);
  assert.equal(await page.locator(button('file')).getAttribute('aria-expanded'), 'true');
  await page.locator(item('file','share')).hover(); await focused(sub('file','share'));
  await page.locator(item('file','copy')).hover(); await frames(page);
  assert.equal(await page.locator(item('file','share')).getAttribute('aria-expanded'), 'true');
  await page.locator(item('file','team')).hover(); await focused(sub('file','team')); await check();
  await page.locator(item('file','design')).click(); await closed('file'); await focused(button('file'));
  assert.match(await page.locator('#activity').textContent(), /Selected design/);
  await open();
  await page.locator(root('file')).dispatchEvent('pointerleave'); await frames(page);
  assert.equal(await page.locator(root('file')).getAttribute('aria-activedescendant'), null);
  await page.locator('#before').click(); await closed('file'); await focused('#before'); await check();
  // Nonfocusable outside target must not move focus back to the trigger.
  await open(); await page.locator('h1').click(); await closed('file');
  assert.notEqual(await page.evaluate(() => document.activeElement.id), 'file-button');

  // Same-frame intents use update's latest model, including reverse-leave.
  await page.locator(button('file')).evaluate(el => { el.click(); el.click(); el.click(); });
  await focused(root('file')); await pressKeys(page, 'Escape'); await closed('file');
  await page.emulateMedia({ reducedMotion: 'no-preference' });
  await open('animated'); await check();
  await pressKeys(page, 'Escape');
  await page.locator(button('animated')).click(); await focused(root('animated'));
  await check(); await pressKeys(page, 'Escape'); await closed('animated'); await focused(button('animated')); await check();
  await page.waitForFunction(() => document.querySelectorAll(':popover-open').length === 0);

  // A claimed Escape must not reach an enclosing modal's native cancel action.
  // Reuse the mounted example inside a real modal dialog, not a dialog polyfill.
  await page.evaluate(() => {
    const dialog = document.createElement('dialog');
    dialog.id = 'menu-modal';
    dialog.setAttribute('aria-label', 'Menu host');
    document.body.append(dialog);
    dialog.append(document.querySelector('#file-button').closest('section'));
    window.menuEscapeEvents = 0;
    document.addEventListener('keydown', event => {
      if (event.key === 'Escape') window.menuEscapeEvents++;
    });
    dialog.showModal();
  });
  await open(); await ariaSnapshot(page.locator(root('file')), fileSnapshot); await check();
  await pressKeys(page, 'Escape'); await closed('file'); await focused(button('file'));
  assert.equal(await page.locator('#menu-modal').evaluate(el => el.open), true);
  assert.equal(await page.evaluate(() => window.menuEscapeEvents), 0);
  await check();
  await pressKeys(page, 'Escape');
  await page.waitForFunction(() => !document.querySelector('#menu-modal').open);
  assert.equal(await page.evaluate(() => window.menuEscapeEvents), 1);
}
