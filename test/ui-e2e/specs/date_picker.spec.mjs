import assert from 'node:assert/strict';
import {openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe} from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'date_picker');
  // Unavailable cells fail Playwright's enabled check; click by coordinates once
  // the anchored panel has stopped moving instead of forcing the click.
  const clickWhenStable = async locator => {
    await locator.scrollIntoViewIfNeeded();
    let previous = null;
    for (;;) {
      await frames(page);
      const box = await locator.boundingBox();
      if (previous && box && ['x', 'y', 'width', 'height'].every(k => box[k] === previous[k])) {
        await page.mouse.click(box.x + box.width / 2, box.y + box.height / 2);
        return;
      }
      previous = box;
    }
  };
  const input = name => page.locator(`#${name}-input-control`);
  const panel = name => page.locator(`#${name}-panel`);
  const trigger = name => page.locator(`#${name}-trigger`);
  const active = name => page.locator(`#${name}-panel [role=gridcell][tabindex="0"]`);
  const open = async (name, activation='click') => {
    if (activation === 'click') await trigger(name).click();
    else { await trigger(name).focus(); await pressKeys(page, activation); }
    await page.waitForFunction(name => {
      const p = document.getElementById(name + '-panel');
      return p.matches(':popover-open') && p.contains(document.activeElement) && document.activeElement.matches('[role=gridcell]');
    }, name);
    await page.waitForFunction(name => document.getElementById(name + '-instructions').textContent.length > 0, name);
    assert.equal(await trigger(name).getAttribute('aria-expanded'), 'true');
  };
  const closed = async name => {
    await page.waitForFunction(name => document.getElementById(name + '-panel').hidden, name);
    assert.equal(await trigger(name).getAttribute('aria-expanded'), 'false');
    assert.equal(await trigger(name).getAttribute('aria-controls'), null);
  };
  const escape = async name => { await pressKeys(page, 'Escape'); await closed(name); await assertFocus(page, `#${name}-trigger`); };
  const activity = () => page.locator('#activity').textContent();
  const focusDate = async (name, label) => assert.equal(await active(name).getAttribute('aria-label'), label);
  const type = async (name, value, commit='Enter') => { await input(name).fill(value); await pressKeys(page, commit); };

  const instructions = 'Cursor keys can navigate dates. Home and End move to the first and last day of the week. Page Up and Page Down change month; add Shift to change year. Enter or Space selects. Escape closes.';
  const marchSnapshot = [
    '- dialog "March 2026":',
    '  - heading "March 2026" [level=2]',
    ...['Previous year', 'Previous month', 'Next month', 'Next year'].map(label => `  - button "${label}"`),
    '  - grid "March 2026":',
    '    - rowgroup:',
    '      - row "Sunday Monday Tuesday Wednesday Thursday Friday Saturday":',
    ...['Sunday:Su', 'Monday:Mo', 'Tuesday:Tu', 'Wednesday:We', 'Thursday:Th', 'Friday:Fr', 'Saturday:Sa'].map(day => {
      const [full, short] = day.split(':'); return `        - columnheader "${full}": ${short}`;
    }),
    '    - rowgroup:',
    ...Array.from({length:6}, (_, week) => {
      const days = Array.from({length:7}, (_, column) => week * 7 + column + 1);
      const names = days.filter(day => day <= 31).map(day => `${day} March 2026`).join(' ');
      return [`      - row${names ? ` "${names}"` : ''}:`, ...days.map(day => day > 31 ? '        - gridcell [disabled]' : `        - gridcell "${day} March 2026"${day === 5 ? ' [selected]' : ''}: "${day}"`)];
    }).flat(),
    '  - button "Cancel"',
    `  - paragraph: ${instructions}`,
  ].join('\n');
  await ariaSnapshot(trigger('basic'), '- button "Choose date, selected date is March 5, 2026": Choose date');
  await ariaSnapshot(panel('basic'), '');
  await axe(page);
  for (const activation of ['Enter', 'Space', 'click']) {
    await open('basic', activation);
    await focusDate('basic', '5 March 2026');
    await assertFocus(page, await active('basic').evaluate(e => '#' + e.id));
    assert.equal(await panel('basic').getAttribute('role'), 'dialog');
    assert.equal(await panel('basic').getAttribute('aria-modal'), 'true');
    assert.equal(await panel('basic').getAttribute('aria-labelledby'), 'basic-calendar-heading');
    assert.equal(await trigger('basic').getAttribute('aria-controls'), 'basic-panel');
    await ariaSnapshot(panel('basic'), marchSnapshot);
    await axe(page);
    await escape('basic');
  }
  await open('basic');
  const group = page.locator('#basic-group');
  await page.waitForFunction(() => {
    const group = document.getElementById('basic-group').getBoundingClientRect();
    const panel = document.getElementById('basic-panel').getBoundingClientRect();
    return Math.abs(panel.left - group.left) <= 1;
  });
  const [groupRect, panelRect, triggerRect] = await Promise.all([
    group.boundingBox(), panel('basic').boundingBox(), trigger('basic').boundingBox(),
  ]);
  assert.ok(Math.abs(panelRect.x - groupRect.x) <= 1, 'popup aligns with the input group');
  assert.ok(Math.abs(panelRect.x - triggerRect.x) > 20, 'popup does not align with the trigger');
  await escape('basic');
  // Put the same input group at the bottom-right: placement must flip above
  // and shift left to stay within the viewport, rather than track the button.
  await group.evaluate(e => { e.style.cssText = 'position:fixed;right:8px;bottom:8px;width:340px;z-index:1'; });
  await open('basic');
  await page.waitForFunction(() => {
    const group = document.getElementById('basic-group').getBoundingClientRect();
    const panel = document.getElementById('basic-panel').getBoundingClientRect();
    return panel.bottom <= group.top && panel.right <= innerWidth - 7 && panel.left >= 7;
  });
  const edgeGroup = await group.boundingBox();
  const edgePanel = await panel('basic').boundingBox();
  assert.match(await panel('basic').getAttribute('data-placement'), /^top/);
  assert.ok(edgePanel.y + edgePanel.height <= edgeGroup.y, 'flips above input group');
  assert.ok(edgePanel.x < edgeGroup.x - 20, 'shifts away from overflowing start alignment');
  assert.ok(edgePanel.x >= 7 && edgePanel.x + edgePanel.width <= page.viewportSize().width - 7, 'shift keeps panel inside viewport padding');
  await axe(page);
  await escape('basic');
  await group.evaluate(e => e.removeAttribute('style'));
  await open('basic');
  const keys = [
    ['ArrowLeft', '4 March 2026'], ['ArrowRight', '5 March 2026'],
    ['ArrowUp', '26 February 2026'], ['ArrowDown', '5 March 2026'],
    ['Home', '1 March 2026'], ['End', '7 March 2026'],
    ['PageUp', '7 February 2026'], ['PageDown', '7 March 2026'],
    ['Shift+PageUp', '7 March 2025'], ['Shift+PageDown', '7 March 2026'],
  ];
  for (const [key, date] of keys) { await pressKeys(page, key); await focusDate('basic', date); await assertFocus(page, await active('basic').evaluate(e => '#' + e.id)); }
  for (const key of ['Control+ArrowLeft', 'Alt+ArrowDown', 'Meta+Home', 'Control+Escape', 'Alt+Escape', 'Meta+Escape']) await pressKeys(page, key);
  await focusDate('basic', '7 March 2026');
  assert.equal(await active('basic').evaluate(e => e.dispatchEvent(new KeyboardEvent('keydown', {key:'Escape',isComposing:true,bubbles:true,cancelable:true}))), true);
  // Verify event propagation with document listeners, not cancelBubble after dispatch.
  const flags = await active('basic').evaluate(e => {
    const reached=[]; const listener=event => reached.push(event.key);
    document.addEventListener('keydown',listener);
    const tab=new KeyboardEvent('keydown',{key:'Tab',bubbles:true,cancelable:true}); e.dispatchEvent(tab);
    const other=new KeyboardEvent('keydown',{key:'F6',bubbles:true,cancelable:true}); e.dispatchEvent(other);
    document.removeEventListener('keydown',listener);
    return {reached,tab:tab.defaultPrevented,other:other.defaultPrevented};
  });
  assert.deepEqual(flags,{reached:['Tab','F6'],tab:false,other:false});
  // Both native Tab boundaries wrap synchronously via focus sentinels.
  await page.locator('#basic-cancel').focus(); await pressKeys(page,'Tab'); await assertFocus(page,'#basic-calendar-previous-year');
  await pressKeys(page,'Shift+Tab'); await assertFocus(page,'#basic-cancel');
  for(let i=0;i<16;i++) { await pressKeys(page,'Tab'); assert.equal(await panel('basic').evaluate(p=>p.contains(document.activeElement)),true); }
  // Native navigation buttons retain focus, both Enter and Space.
  await page.locator('#basic-calendar-next-month').focus(); await pressKeys(page,'Enter');
  await assertFocus(page,'#basic-calendar-next-month');
  assert.equal(await page.locator('#basic-calendar-heading').textContent(),'April 2026');
  await pressKeys(page,'Space'); assert.equal(await page.locator('#basic-calendar-heading').textContent(),'May 2026');
  await escape('basic');
  await open('basic'); await pressKeys(page,'ArrowRight','Enter'); await closed('basic');
  await assertFocus(page,'#basic-trigger'); assert.equal(await input('basic').inputValue(),'2026-03-06');
  assert.match(await activity(),/Closed\(Selection\)/);
  assert.equal(await trigger('basic').getAttribute('aria-label'),'Choose date, selected date is March 6, 2026');
  await open('basic'); await pressKeys(page,'ArrowRight','Space'); await closed('basic');
  assert.equal(await input('basic').inputValue(),'2026-03-07'); await assertFocus(page,'#basic-trigger');
  await axe(page);

  await type('basic','2026-04-13'); assert.equal(await input('basic').inputValue(),'2026-04-13');
  await open('basic'); await focusDate('basic','13 April 2026'); await escape('basic');
  await type('basic','not a date'); assert.equal(await input('basic').getAttribute('aria-invalid'),'true');
  assert.equal(await page.locator('#basic-input-error').textContent(),'Enter a valid date.');
  assert.match(await activity(),/Invalid\(not a date\)/); await axe(page);
  await type('basic',''); assert.equal(await trigger('basic').getAttribute('aria-label'),'Choose date');
  assert.equal(await input('basic').getAttribute('aria-invalid'),null);
  await open('basic'); await focusDate('basic','5 March 2026'); await escape('basic');
  await type('custom','17/04/2026','Tab');
  assert.equal(await page.locator('input[name=custom-date]').inputValue(),'2026-04-17');
  await open('custom'); await focusDate('custom','17 April 2026');
  await active('custom').click(); await closed('custom'); assert.equal(await input('custom').inputValue(),'17/04/2026');
  await type('range','2026-03-01'); assert.match(await page.locator('#range-input-error').textContent(),/before/); await axe(page);
  await type('range','2026-03-28'); assert.match(await page.locator('#range-input-error').textContent(),/after/);
  await type('range','2026-03-07'); assert.match(await page.locator('#range-input-error').textContent(),/unavailable/);
  await open('range'); await focusDate('range','5 March 2026');
  await pressKeys(page,'ArrowRight','ArrowRight'); assert.equal(await active('range').getAttribute('aria-disabled'),'true');
  await pressKeys(page,'Enter','Space'); assert.equal(await trigger('range').getAttribute('aria-expanded'),'true');
  await clickWhenStable(active('range')); assert.equal(await trigger('range').getAttribute('aria-expanded'),'true');
  await page.locator('#range-cancel').focus(); await pressKeys(page, 'Tab');
  await assertFocus(page, await active('range').evaluate(e => '#' + e.id));
  await pressKeys(page, 'Shift+Tab'); await assertFocus(page, '#range-cancel');
  await axe(page); await escape('range');

  await open('confirm'); await pressKeys(page,'ArrowRight','Enter');
  assert.equal(await input('confirm').inputValue(),'2026-03-05');
  assert.equal(await trigger('confirm').getAttribute('aria-expanded'),'true');
  await axe(page); await page.locator('#confirm-cancel').click(); await closed('confirm'); await assertFocus(page,'#confirm-trigger');
  assert.equal(await input('confirm').inputValue(),'2026-03-05');
  await open('confirm'); await focusDate('confirm','5 March 2026'); await pressKeys(page,'ArrowRight','Enter');
  await escape('confirm'); assert.equal(await input('confirm').inputValue(),'2026-03-05');
  await open('confirm'); await focusDate('confirm','5 March 2026'); await pressKeys(page,'ArrowRight','Space');
  await page.locator('#confirm-confirm').click(); await closed('confirm'); await assertFocus(page,'#confirm-trigger');
  assert.equal(await input('confirm').inputValue(),'2026-03-06'); assert.match(await activity(),/Closed\(Confirm\)/);

  await open('rtl'); await pressKeys(page,'ArrowLeft'); await focusDate('rtl','6 March 2026');
  await pressKeys(page,'ArrowRight'); await focusDate('rtl','5 March 2026'); await axe(page); await escape('rtl');
  await open('readonly'); await pressKeys(page,'ArrowRight','Enter','Space');
  assert.equal(await trigger('readonly').getAttribute('aria-expanded'),'true');
  assert.equal(await input('readonly').inputValue(),'2026-03-05');
  assert.equal(await panel('readonly').locator('[role=grid]').getAttribute('aria-readonly'),'true');
  await axe(page); await escape('readonly');
  assert.equal(await trigger('disabled').isDisabled(),true); assert.equal(await input('disabled').isDisabled(),true);
  await trigger('disabled').evaluate(e=>e.click()); await frames(page); assert.equal(await trigger('disabled').getAttribute('aria-expanded'),'false');
  await axe(page);

  await open('basic'); await page.locator('#outside').click(); await closed('basic'); await assertFocus(page,'#outside');
  assert.match(await activity(),/Closed\(Outside\)/);
  await open('basic'); await input('basic').click(); await closed('basic');
  await assertFocus(page, '#basic-input-control');
  assert.match(await activity(),/Closed\(Outside\)/);
  await axe(page);
  await open('basic'); await page.locator('h1').click(); await closed('basic');
  assert.notEqual(await page.evaluate(()=>document.activeElement.id),'basic-trigger');
  await type('form','2026-08-19'); await page.locator('#submit').click(); await frames(page);
  assert.equal(await page.locator('#submitted').textContent(),'2026-08-19');
  assert.deepEqual(await page.locator('#booking').evaluate(form=>[...new FormData(form)]),[['form-date','2026-08-19']]);
  await type('form','invalid');
  assert.deepEqual(await page.locator('#booking').evaluate(form=>[...new FormData(form)]),[['form-date','2026-08-19']]);
  await axe(page);

  await page.locator('#open-modal').click(); await page.waitForFunction(()=>document.querySelector('#details-dialog').open);
  await open('nested');
  const escapeFlags=await active('nested').evaluate(e=>{
    let reached=false; const listener=()=>{reached=true;}; document.addEventListener('keydown',listener);
    const event=new KeyboardEvent('keydown',{key:'Escape',bubbles:true,cancelable:true}); e.dispatchEvent(event);
    document.removeEventListener('keydown',listener); return {reached,prevented:event.defaultPrevented};
  });
  assert.deepEqual(escapeFlags,{reached:false,prevented:true});
  await closed('nested'); await assertFocus(page,'#nested-trigger');
  assert.equal(await page.locator('#details-dialog').evaluate(e=>e.open),true);
  await open('nested'); await axe(page); await escape('nested');
  assert.equal(await page.locator('#details-dialog').evaluate(e=>e.open),true);
  await pressKeys(page,'Escape'); await page.waitForFunction(()=>!document.querySelector('#details-dialog').open); await assertFocus(page,'#open-modal');
  await axe(page);
  await open('basic');
  // Unmount without an outside click first; all top-layer leases must dispose.
  await page.locator('#unmount').evaluate(e=>e.click()); await frames(page);
  assert.equal(await page.locator(':popover-open').count(),0);
  assert.equal(await page.locator('[popover]').count(),0); await axe(page);
  await page.locator('#reset').click(); await frames(page);
  await ariaSnapshot(panel('basic'),''); await axe(page);
}
