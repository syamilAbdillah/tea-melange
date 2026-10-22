import assert from 'node:assert/strict';
import {openExample, frames, pressKeys, assertFocus, axe, ariaSnapshot} from '../lib.mjs';

export async function run(context) {
  const page = await openExample(context, 'showcase');
  // A genuine keyboard journey: no locator.focus(), fill(), or click() below.
  const tabTo = async selector => {
    const target = page.locator(selector);
    for (let i = 0; i < 65; i++) {
      if (await target.evaluate(e => e === document.activeElement)) return;
      await pressKeys(page, 'Tab');
    }
    assert.fail('Could not Tab to ' + selector);
  };
  const type = async (selector, value) => {
    await tabTo(selector);
    await pressKeys(page, 'ControlOrMeta+A');
    await page.keyboard.insertText(value);
    await frames(page);
  };
  const hidden = async selector => page.waitForFunction(selector => document.querySelector(selector).hidden, selector);
  const expanded = async selector => page.waitForFunction(selector => document.querySelector(selector).getAttribute('aria-expanded') === 'true', selector);
  const modalOpen = () => page.waitForFunction(() => document.querySelector('#archive-dialog').open);
  const modalStillOpen = async () => assert.equal(await page.locator('#archive-dialog').evaluate(e => e.open), true);
  const dateOpen = async key => {
    await tabTo(`#${key}-date-trigger`);
    await pressKeys(page, 'Enter');
    await page.waitForFunction(key => document.querySelector(`#${key}-date-panel [role=gridcell]`) && document.activeElement.matches(`#${key}-date-panel [role=gridcell]`), key);
  };

  for (const id of ['identity', 'preferences', 'checklist', 'help', 'archive-section']) {
    assert.equal(await page.locator('#' + id).isVisible(), true, id);
  }
  assert.equal(await page.locator('#audit-section').count(), 1); // retained, hidden tab
  assert.equal(await page.getByRole('meter').count(), 1);
  assert.equal(await page.getByRole('progressbar').getAttribute('aria-valuenow'), '100');
  await axe(page);

  await tabTo('#help-tip-trigger');
  await page.waitForFunction(() => !document.querySelector('#help-tip-tooltip').hidden);
  await tabTo('#preview-tip-trigger');
  await page.waitForFunction(() => !document.querySelector('#preview-tip-tooltip').hidden);
  await hidden('#help-tip-tooltip'); // parent enforces grouped exclusivity
  await pressKeys(page, 'Escape');
  await hidden('#preview-tip-tooltip');

  await type('#workspace-name-control', 'Coastal survey');
  await type('#workspace-notes-control', 'Bring sample labels and a waterproof notebook.');
  await tabTo('#region-control');
  await pressKeys(page, 'ArrowDown', 'Tab');
  assert.equal(await page.locator('#region-control').inputValue(), 'us');
  await type('#workspace-owner-control', 'Jo');
  await pressKeys(page, 'ArrowDown', 'Enter');
  assert.equal(await page.locator('#workspace-owner-control').inputValue(), 'Jo Rivera');
  await tabTo('#workspace-team-button');
  await pressKeys(page, 'ArrowDown');
  await expanded('#workspace-team-button');
  await pressKeys(page, 'ArrowDown', 'Enter');
  await hidden('#workspace-team-listbox');
  assert.equal(await page.locator('#workspace-team-button').textContent(), 'Operations');
  await dateOpen('workspace');
  await pressKeys(page, 'ArrowRight', 'Enter');
  await hidden('#workspace-date-panel');
  assert.equal(await page.locator('#workspace-date-input-control').inputValue(), '2026-03-06');
  await tabTo('#digest-control');
  await pressKeys(page, 'Space');
  assert.equal(await page.locator('#digest-control').isChecked(), false);
  await tabTo('[role=switch]');
  await pressKeys(page, 'Space');
  assert.equal(await page.locator('[role=switch]').getAttribute('aria-checked'), 'true');
  await tabTo('[role=radio][tabindex="0"]');
  await pressKeys(page, 'ArrowRight');
  assert.equal(await page.getByRole('radio', {name:'Daily digest'}).getAttribute('aria-checked'), 'true');
  await tabTo('[role=slider]');
  await pressKeys(page, 'ArrowRight');
  assert.equal(await page.getByRole('slider').getAttribute('aria-valuenow'), '76');
  await tabTo('#save');
  await pressKeys(page, 'Enter');
  await page.locator('#notifications-polite').getByText('Changes saved for Coastal survey.').waitFor();
  assert.equal(await page.locator('#notifications-polite').getAttribute('aria-live'), 'polite');
  assert.equal(await page.locator('#notifications-assertive').textContent(), '');
  assert.match(await page.locator('#saved-summary').textContent(), /Coastal survey \/ Jo Rivera \/ operations \/ 2026-03-06/);
  await ariaSnapshot(page.locator('#save'), '- button "Save changes"');

  // Keyboard sorting applies Moved to the parent's keyed collection.
  await tabTo('.tasks li:first-child button');
  await pressKeys(page, 'Space', 'ArrowDown', 'Space');
  assert.match(await page.locator('.tasks li').nth(1).textContent(), /Review field survey/);
  await tabTo('#faq-button');
  await pressKeys(page, 'Enter');
  assert.equal(await page.locator('#faq-panel').isVisible(), true);

  await tabTo('#open-archive');
  await pressKeys(page, 'Enter');
  await modalOpen();
  await page.waitForFunction(() => document.activeElement.id === 'cancel-archive');
  await assertFocus(page, '#cancel-archive');
  assert.equal(await page.locator('#archive-dialog').getAttribute('role'), 'alertdialog');
  await axe(page);

  // Each inner Escape consumes only its own layer. The outer modal must stay
  // natively open, not merely retain an attribute or an application flag.
  await tabTo('#review-team-button');
  await pressKeys(page, 'ArrowDown');
  await expanded('#review-team-button');
  await pressKeys(page, 'Escape');
  await hidden('#review-team-listbox');
  await assertFocus(page, '#review-team-button');
  await modalStillOpen();

  await tabTo('#review-owner-control');
  await page.keyboard.insertText('Ada');
  await frames(page);
  await pressKeys(page, 'ArrowDown');
  await expanded('#review-owner-control');
  await pressKeys(page, 'Escape');
  await hidden('#review-owner-popup');
  await assertFocus(page, '#review-owner-control');
  await modalStillOpen();

  await dateOpen('review');
  await pressKeys(page, 'Escape');
  await hidden('#review-date-panel');
  await assertFocus(page, '#review-date-trigger');
  await modalStillOpen();

  await tabTo('#review-actions-button');
  await pressKeys(page, 'ArrowDown', 'ArrowRight');
  await page.waitForFunction(() => [...document.querySelectorAll('#archive-dialog [role=menu]')].filter(e => !e.hidden).length === 2);
  await pressKeys(page, 'Escape'); // submenu only
  await page.waitForFunction(() => [...document.querySelectorAll('#archive-dialog [role=menu]')].filter(e => !e.hidden).length === 1);
  await modalStillOpen();
  await pressKeys(page, 'Escape'); // root menu only
  await page.waitForFunction(() => [...document.querySelectorAll('#archive-dialog [role=menu]')].every(e => e.hidden));
  await assertFocus(page, '#review-actions-button');
  await modalStillOpen();

  await tabTo('#review-tip-trigger');
  await page.waitForFunction(() => !document.querySelector('#review-tip-tooltip').hidden);
  await pressKeys(page, 'Escape');
  await hidden('#review-tip-tooltip');
  await assertFocus(page, '#review-tip-trigger');
  await modalStillOpen();
  await pressKeys(page, 'Escape'); // now the modal itself
  await page.waitForFunction(() => !document.querySelector('#archive-dialog').open);
  await page.waitForFunction(() => document.activeElement.id === 'open-archive');

  await hidden('#archive-dialog');
  await frames(page); // flush native close delivery before the next activation

  // Confirming the destructive action updates the workspace and announces via
  // the already-mounted success lane, never the assertive error lane.
  await pressKeys(page, 'Enter');
  await modalOpen();
  await page.waitForFunction(() => document.activeElement.id === 'cancel-archive');
  await tabTo('#confirm-archive');
  await pressKeys(page, 'Enter');
  await page.waitForFunction(() => !document.querySelector('#archive-dialog').open);
  await page.getByRole('heading', {name:'Field station / Archived', exact:true}).waitFor();
  await page.locator('#notifications-polite').getByText(/Workspace archived/).waitFor();
  assert.equal(await page.locator('#notifications-assertive').textContent(), '');

  // Smoke the non-form compositions with trusted pointer interaction too.
  await page.getByRole('tab', {name:'Activity', exact:true}).click();
  await page.locator('#audit-section').waitFor({state:'visible'});
  await page.locator('#jump').click();
  await page.getByRole('listitem').filter({hasText:'Entry 10000:'}).waitFor();
  assert.ok(await page.locator('.audit-row').count() < 30, 'bounded virtual DOM');
  await page.getByRole('tab', {name:'Settings', exact:true}).click();
  await page.locator('#brief-input').setInputFiles({name:'field-brief.txt', mimeType:'text/plain', buffer:Buffer.from('Sample labels ready.')});
  await page.getByText('field-brief.txt', {exact:true}).waitFor();
  await page.locator('#filters-trigger').click();
  await page.getByRole('dialog', {name:'View options', exact:true}).waitFor();
  await axe(page);
  await pressKeys(page, 'Escape');
  await hidden('#filters-panel');

  // Both color schemes and a narrow viewport retain accessible contrast and
  // avoid a horizontal document scrollbar. No timing sleeps or axe exclusions.
  await page.emulateMedia({colorScheme:'dark'});
  await page.setViewportSize({width:390, height:844});
  await frames(page);
  assert.equal(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth), true);
  await axe(page);
}
