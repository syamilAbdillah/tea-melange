import assert from 'node:assert/strict';
import {openExample,frames,pressKeys,assertFocus,ariaSnapshot,axe} from '../lib.mjs';
const single=(selected=null,state='')=>`- combobox "First tea"${state}:
  - option "Choose a tea" [disabled]${selected?'':' [selected]'}
  - option "Sencha"${state.includes('disabled')?' [disabled]':''}${selected==='sencha'?' [selected]':''}
  - option "Sold out" [disabled]`;
const multi=(a=false,b=false,c=false,state='')=>`- listbox "Extra teas"${state}:
  - option "Sencha"${a?' [selected]':''}
  - option "Sold out" [disabled]
  - group:
    - option "Roasted oolong"${b?' [selected]':''}
    - option "Assam"${c?' [selected]':''}`;
export async function run(context){
 const page=await openExample(context,'forms');const control=page.locator('#tea-control'),multiple=page.locator('#extras-control');
 const values=()=>multiple.evaluate(el=>Array.from(el.selectedOptions,o=>o.value));
 await ariaSnapshot(control,single());await ariaSnapshot(multiple,multi(true));await axe(page);
 await page.locator('#tea-label').click();await assertFocus(page,'#tea-control');await pressKeys(page,'Escape');
 for(const [key,value] of [['ArrowDown','sencha'],['ArrowDown','oolong / roasted'],['ArrowUp','sencha'],['End','assam'],['Home','sencha'],['r','oolong / roasted']]){
 await pressKeys(page,key);assert.equal(await control.inputValue(),value,key);await ariaSnapshot(control,single(value));await axe(page);
 }
 await pressKeys(page,'Alt+ArrowDown');await ariaSnapshot(control,single(true));await axe(page);
 await pressKeys(page,'Escape');assert.equal(await control.inputValue(),'oolong / roasted');await assertFocus(page,'#tea-control');
 await pressKeys(page,'Space','ArrowDown','Enter');assert.equal(await control.inputValue(),'assam');
 await ariaSnapshot(control,single(true));await axe(page);
 await pressKeys(page,'Escape','Tab');await assertFocus(page,'#extras-control');await pressKeys(page,'Shift+Tab');await assertFocus(page,'#tea-control');
 await control.selectOption('oolong / roasted');await frames(page);
 await page.locator('#options-reordered').click();await frames(page);
 assert.equal(await control.inputValue(),'oolong / roasted','reordering never changes the selected key');
 await axe(page);
 await page.locator('#options-full').click();await frames(page);await multiple.focus();
 await pressKeys(page,'Home');assert.deepEqual(await values(),['sencha']);await ariaSnapshot(multiple,multi(true));await axe(page);
 await pressKeys(page,'ArrowDown');assert.deepEqual(await values(),['oolong / roasted']);await ariaSnapshot(multiple,multi(false,true));await axe(page);
 await pressKeys(page,'Shift+ArrowDown');assert.deepEqual(await values(),['oolong / roasted','assam']);await ariaSnapshot(multiple,multi(false,true,true));await axe(page);
 await pressKeys(page,'Control+ArrowUp');assert.deepEqual(await values(),['oolong / roasted','assam']);
 await pressKeys(page,'Space');assert.deepEqual(await values(),['assam']);await ariaSnapshot(multiple,multi(false,false,true));await axe(page);
 await pressKeys(page,'End','Home','a');assert.deepEqual(await values(),['assam']);
 await multiple.selectOption(['sencha','oolong / roasted']);await frames(page);
 await ariaSnapshot(multiple,multi(true,true));await axe(page);
 await page.locator('#submit').click();await frames(page);
 const data=JSON.parse(await page.locator('#serialized').textContent());
 assert.deepEqual(data.filter(([k])=>k==='extras').map(([,v])=>v),['sencha','oolong / roasted']);
 assert.ok(data.some(([k,v])=>k==='tea'&&v==='oolong / roasted'));
 await page.locator('#options-empty').click();await frames(page);
 await ariaSnapshot(control,'- combobox "First tea":\n  - option "Choose a tea" [disabled] [selected]');
 await ariaSnapshot(multiple,'- listbox "Extra teas"');await axe(page);
 await page.locator('#options-disabled').click();await frames(page);
 await ariaSnapshot(control,'- combobox "First tea":\n  - option "Choose a tea" [disabled] [selected]');
 await ariaSnapshot(multiple,'- listbox "Extra teas":\n  - group [disabled]:\n    - option "Sencha" [disabled]');await axe(page);
 await control.focus();await pressKeys(page,'ArrowDown','End','Home');assert.equal(await control.inputValue(),'');
 await page.locator('#options-single').click();await frames(page);
 await ariaSnapshot(control,'- combobox "First tea":\n  - option "Choose a tea" [disabled] [selected]\n  - option "Only tea"');await axe(page);
 await control.focus();await pressKeys(page,'ArrowDown');assert.equal(await control.inputValue(),'only');
 await ariaSnapshot(control,'- combobox "First tea":\n  - option "Choose a tea" [disabled]\n  - option "Only tea" [selected]');await axe(page);
 await page.locator('#options-full').click();await page.locator('#validate').click();await frames(page);
 await ariaSnapshot(control,single(false,' [invalid]'));await ariaSnapshot(multiple,multi(false,false,false,' [invalid]'));await axe(page);
 assert.equal(await control.getAttribute('aria-errormessage'),'tea-error');assert.equal(await control.getAttribute('aria-describedby'),'tea-description tea-error');
 await page.locator('#validate').click();await page.locator('#disable').click();await frames(page);
 await ariaSnapshot(control,single(false,' [disabled]'));await axe(page);
 await page.locator('#submit').click();await frames(page);assert.deepEqual(JSON.parse(await page.locator('#serialized').textContent()),[]);
 await page.locator('#disable').click();await frames(page);await ariaSnapshot(control,single());await axe(page);
}
