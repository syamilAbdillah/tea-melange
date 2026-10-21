import assert from 'node:assert/strict';
import {openExample, frames, pressKeys, assertFocus, ariaSnapshot, axe} from '../lib.mjs';
export async function run(context) {
 const page=await openExample(context,'forms');
 const field=page.locator('#name-control');
 await ariaSnapshot(field,'- textbox "Your name"'); await axe(page);
 assert.equal(await page.locator('#name-label').getAttribute('for'),'name-control');
 assert.equal(await field.getAttribute('aria-describedby'),'name-description');
 assert.equal(await field.getAttribute('aria-errormessage'),null);
 await page.locator('#name-label').click(); await assertFocus(page,'#name-control');
 await field.fill('Tea'); await frames(page);
 await ariaSnapshot(field,'- textbox "Your name": Tea'); await axe(page);
 await pressKeys(page,'End','Home','ArrowRight','Shift+End');
 await pressKeys(page,'Backspace'); await page.keyboard.type('asting'); await frames(page);
 assert.equal(await field.inputValue(),'Tasting');
 await pressKeys(page,'Control+a'); await page.keyboard.type('Guest'); await frames(page);
 assert.equal(await field.inputValue(),'Guest');

 await pressKeys(page,'Tab'); await assertFocus(page,'#notes-control');
 assert.match(await page.locator('#activity').textContent(),/Changes: [1-9]/);
 assert.match(await page.locator('#activity').textContent(),/Blurs: [1-9]/);
 await pressKeys(page,'Shift+Tab'); await assertFocus(page,'#name-control');
 await page.locator('#readonly').click(); await frames(page);
 assert.equal(await field.getAttribute('data-readonly'),'');
 assert.equal(await field.evaluate(el=>el.readOnly),true);
 await field.focus(); const before=await field.inputValue(); await page.keyboard.type('ignored'); await frames(page);
 assert.equal(await field.inputValue(),before);
 await ariaSnapshot(field, '- textbox "Your name": '+before.replace(/\n/g,' ')); await axe(page);
 await page.locator('#validate').click(); await frames(page);
 assert.equal(await field.getAttribute('aria-invalid'),'true');
 assert.equal(await field.getAttribute('data-invalid'),'');
 assert.equal(await field.getAttribute('aria-describedby'),'name-description name-error');
 assert.equal(await field.getAttribute('aria-errormessage'),'name-error');
 assert.equal(await page.locator('#name-error').isVisible(),true);
 await ariaSnapshot(field,'- textbox "Your name" [invalid]: '+before.replace(/\n/g,' ')); await axe(page);
 await page.locator('#disable').click(); await frames(page);
 await ariaSnapshot(field,'- textbox "Your name" [disabled] [invalid]: '+before.replace(/\n/g,' ')); await axe(page);
 await page.locator('#submit').click(); await frames(page);
 assert.deepEqual(JSON.parse(await page.locator('#serialized').textContent()),[]);
 await page.locator('#disable').click(); await page.locator('#readonly').click(); await page.locator('#validate').click(); await frames(page);
 assert.equal(await field.getAttribute('aria-invalid'),null); assert.equal(await field.getAttribute('aria-errormessage'),null);
 assert.equal(await field.getAttribute('data-readonly'),null);
 await field.evaluate(el=>{
 el.dispatchEvent(new CompositionEvent('compositionstart',{bubbles:true,data:''}));
 el.value='Composed';
 el.dispatchEvent(new InputEvent('input',{bubbles:true,data:'Composed',isComposing:true,inputType:'insertCompositionText'}));
 el.dispatchEvent(new CompositionEvent('compositionend',{bubbles:true,data:'Composed'}));
 for(const type of ['keydown','keyup']){
 const event=new KeyboardEvent(type,{key:'Enter',isComposing:true,bubbles:true,cancelable:true});
 el.dispatchEvent(event); if(event.defaultPrevented) throw new Error('IME key intercepted');
 }
 });
 await frames(page); assert.equal(await field.inputValue(),'Composed');
 await ariaSnapshot(field,'- textbox "Your name": Composed'); await axe(page);
 await field.fill('Submitted'); await page.locator('#submit').click(); await frames(page);
 assert.ok(JSON.parse(await page.locator('#serialized').textContent()).some(([key,value])=>key==='name'&&value==='Submitted'));
 await field.focus(); await page.keyboard.press('Enter'); await frames(page);
 assert.ok(JSON.parse(await page.locator('#serialized').textContent()).some(([key,value])=>key==='name'&&value==='Submitted'),'native implicit Enter submit');
 await assertFocus(page,'#name-control');
 await ariaSnapshot(field,'- textbox "Your name": Submitted'); await axe(page);
}
