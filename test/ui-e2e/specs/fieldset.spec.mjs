import assert from 'node:assert/strict';
import {openExample,frames,pressKeys,assertFocus,ariaSnapshot,axe} from '../lib.mjs';
const snapshot=disabled=>{
 const d=disabled?' [disabled]':'';
 return `- group "Tasting preferences":
  - text: Tasting preferences
  - paragraph: Disabling this fieldset excludes every preference from form submission, including custom controls.
  - text: Your name
  - textbox "Your name"${d}
  - paragraph: Use the name we should put on your reservation.
  - text: Tasting notes
  - textbox "Tasting notes"${d}
  - paragraph: Tell us about dietary needs or a tea you would like to try.
  - checkbox "Include tasting equipment" [checked=mixed]${d}
  - text: Include tasting equipment
  - paragraph: Mixed becomes checked on activation. This is a native checkbox.
  - checkbox "Include take-home samples" [checked=mixed]${d}
  - paragraph: "A custom checkbox: Space toggles on release. Its nested label is clickable."
  - text: Tasting updates
  - switch "Tasting updates"${d}: "Off"
  - paragraph: An ordinary button activation toggles the switch. The label stays constant.
  - text: First tea
  - combobox "First tea"${d}:
    - option "Choose a tea" [disabled] [selected]
    - option "Sencha"${d}
    - option "Sold out" [disabled]
  - paragraph: Choose one tea for the tasting.
  - text: Extra teas
  - listbox "Extra teas"${d}:
    - option "Sencha"${d} [selected]
    - option "Sold out" [disabled]
    - group${d}:
      - option "Roasted oolong"${d}
      - option "Assam"${d}
  - paragraph: Choose any extra teas. Use your platform's multiple-selection keys.`;
};
export async function run(context){
 const page=await openExample(context,'forms');
 const group=page.locator('#preferences-root');
 await ariaSnapshot(group,snapshot(false));await axe(page);
 assert.equal(await group.getAttribute('aria-describedby'),'preferences-description');
 await page.locator('#options-reordered').focus();await pressKeys(page,'Tab');await assertFocus(page,'#name-control');
 await pressKeys(page,'Shift+Tab');await assertFocus(page,'#options-reordered');
 await page.locator('#disable').click();await frames(page);
 assert.equal(await group.evaluate(el=>el.disabled),true);assert.equal(await group.getAttribute('data-disabled'),'');
 await ariaSnapshot(group,snapshot(true));await axe(page);
 await page.locator('#options-reordered').focus();await pressKeys(page,'Tab');await assertFocus(page,'#submit');
 await pressKeys(page,'Shift+Tab');await assertFocus(page,'#options-reordered');
 await page.locator('#submit').click();await frames(page);
 assert.deepEqual(JSON.parse(await page.locator('#serialized').textContent()),[]);
 await page.locator('#disable').click();await frames(page);
 await ariaSnapshot(group,snapshot(false));await axe(page);
 await page.locator('#validate').click();await frames(page);
 assert.equal(await group.getAttribute('data-invalid'),'');
 await ariaSnapshot(page.locator('#name-control'),'- textbox "Your name" [invalid]');
 await ariaSnapshot(page.locator('#notes-control'),'- textbox "Tasting notes" [invalid]');
 await axe(page);
 await page.locator('#validate').click();await frames(page);assert.equal(await group.getAttribute('data-invalid'),null);
 await ariaSnapshot(group,snapshot(false));await axe(page);
 await page.locator('#name-control').fill('Guest');await page.locator('#tea-control').selectOption('sencha');
 await page.locator('#submit').click();await frames(page);
 const data=JSON.parse(await page.locator('#serialized').textContent());
 assert.ok(data.some(([k,v])=>k==='name'&&v==='Guest'));assert.ok(data.some(([k,v])=>k==='tea'&&v==='sencha'));
}
