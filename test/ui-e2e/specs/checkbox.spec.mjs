import assert from 'node:assert/strict';
import {openExample,frames,pressKeys,assertFocus,ariaSnapshot,axe} from '../lib.mjs';
export async function run(context) {
 const page=await openExample(context,'forms');
 const count=async()=>Number((await page.locator('#activity').textContent()).match(/Toggles: (\d+)/)[1]);
 for(const [id,label] of [['native','Include tasting equipment'],['custom','Include take-home samples']]) {
 const control=page.locator('#'+id+'-control');
 await ariaSnapshot(control, '- checkbox "'+label+'" [checked=mixed]'); await axe(page);
 assert.equal(await control.getAttribute('data-indeterminate'),'');
 await control.focus(); const before=await count(); const scroll=await page.evaluate(()=>scrollY);
 await page.keyboard.down('Space'); await frames(page);
 assert.equal(await count(),before); assert.equal(await page.evaluate(()=>scrollY),scroll);
 await page.keyboard.up('Space'); await frames(page);
 assert.equal(await count(),before+1); assert.equal(await page.evaluate(()=>scrollY),scroll);
 await ariaSnapshot(control,'- checkbox "'+label+'" [checked]'); await axe(page);
 assert.equal(await control.getAttribute('data-checked'),''); assert.equal(await control.getAttribute('data-indeterminate'),null);
 if(id==='native') assert.equal(await control.evaluate(el=>el.indeterminate),false);
 await pressKeys(page,'Space'); await ariaSnapshot(control,'- checkbox "'+label+'"'); await axe(page);
 await pressKeys(page,'Enter'); assert.equal(await count(),before+2,'Enter does not toggle');
 await page.locator('#'+id+'-label').click(); await frames(page);
 await assertFocus(page,'#'+id+'-control'); await ariaSnapshot(control,'- checkbox "'+label+'" [checked]'); await axe(page);
 await control.click(); await frames(page); await ariaSnapshot(control,'- checkbox "'+label+'"'); await axe(page);
 await page.locator('#mixed').click(); await frames(page);
 await ariaSnapshot(control,'- checkbox "'+label+'" [checked=mixed]'); await axe(page);
 await control.click(); await frames(page);
 await ariaSnapshot(control,'- checkbox "'+label+'" [checked]'); await axe(page);
 }
 await page.locator('#custom-control').focus(); const before=await count();
 await pressKeys(page,'Control+Space','Alt+Space','Meta+Space','Home','End','ArrowDown');
 assert.equal(await count(),before,'custom shortcuts/unhandled keys untouched');
 await page.locator('#custom-control').evaluate(el=>{
 for(const type of ['keydown','keyup']){
 const event=new KeyboardEvent(type,{key:' ',isComposing:true,bubbles:true,cancelable:true});
 el.dispatchEvent(event); if(event.defaultPrevented) throw new Error('IME Space intercepted');
 }
 });
 await frames(page); assert.equal(await count(),before,'IME never toggles');
 await page.evaluate(()=>{const el=document.querySelector('#custom-control'); el.click(); el.click();});
 await frames(page); assert.equal(await count(),before+2);
 await ariaSnapshot(page.locator('#custom-control'),'- checkbox "Include take-home samples" [checked]'); await axe(page);
 await page.locator('#submit').click(); await frames(page);
 const entries=JSON.parse(await page.locator('#serialized').textContent());
 assert.ok(entries.some(([k,v])=>k==='equipment'&&v==='provided')); assert.ok(entries.some(([k,v])=>k==='samples'&&v==='included'));
 await page.locator('#native-control').focus(); await pressKeys(page,'Tab'); await assertFocus(page,'#custom-control');
 await pressKeys(page,'Tab'); await assertFocus(page,'#updates-control');
 await pressKeys(page,'Shift+Tab'); await assertFocus(page,'#custom-control');
 await page.locator('#disable').click(); await frames(page);
 for(const [id,label] of [['native','Include tasting equipment'],['custom','Include take-home samples']]){
 await ariaSnapshot(page.locator('#'+id+'-control'),'- checkbox "'+label+'" '+(id==='native'?'[checked=mixed]':'[checked]')+' [disabled]');
 assert.equal(await page.locator('#'+id+'-control').getAttribute('data-disabled'),'');
 }
 await axe(page);
 const disabledCount=await count();
 await page.locator('#custom-control').dispatchEvent('click'); await page.locator('#native-control').dispatchEvent('click'); await frames(page);
 assert.equal(await count(),disabledCount);
 await page.locator('#mixed').click(); await frames(page);
 for(const [id,label] of [['native','Include tasting equipment'],['custom','Include take-home samples']]){
 await ariaSnapshot(page.locator('#'+id+'-control'),'- checkbox "'+label+'" [checked=mixed] [disabled]');
 }
 await axe(page);
 await page.locator('#submit').click(); await frames(page); assert.deepEqual(JSON.parse(await page.locator('#serialized').textContent()),[]);
 await page.locator('#disable').click(); await frames(page);
 await ariaSnapshot(page.locator('#native-control'),'- checkbox "Include tasting equipment" [checked=mixed]'); await axe(page);
}
