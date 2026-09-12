(()=>{
'use strict';
const MODE='freeclinkerDemoMode',CFG='freeclinkerDemoConfig',C3='freeclinkerC3Config',FRESH='fpsSettingsClean20260912v1';
const $=id=>document.getElementById(id),demo=()=>localStorage.getItem(MODE)==='1';
const D={osd1:'{batt}',osd2:'{state} {recdur}',osd3:'{mode} {res} {fps} {eis}',osd4:'{rectf} {rcap}',pilotTpl:'{stateonly} {batt} {rectf}',craftTpl:'',pilotEn:true,craftEn:false,fpvFlash:true,fpvPreArm:true,fpvPreArmText:'CLEAN LENS',fpvCustomDurationSec:'1.0',fpvLowPct:'10',fpvRecLowMin:'5',cameraMatch:'0',wakeGuard:true,stopOnDisarm:true,disarmDelay:'5',debugBle:false,lowPower:true,wifiApEnabled:false,bf45Compat:false};
const IDS=[...Object.keys(D),'fpsCustomEnable','fpvLowBatt','auxChannel'];
const read=k=>{try{return JSON.parse(localStorage.getItem(k)||'{}')}catch{return{}}},write=(k,v)=>localStorage.setItem(k,JSON.stringify(v));
function put(id,v){const e=$(id);if(!e)return;e.type==='checkbox'?e.checked=!!v:e.value=v}
function get(id){const e=$(id);return e?(e.type==='checkbox'?e.checked:e.value):undefined}
function seed(){Object.entries(D).forEach(([k,v])=>put(k,v));put('fpsOsdEnable',false);put('fpsCustomEnable',false);put('fpvLowBatt',false);put('fpsAuxEnable',false);put('auxChannel','0');localStorage.setItem('fpsOsdEnabled','0');localStorage.setItem('fpsOsdMessageCount','4');localStorage.setItem('fpsWarningDestination','1');localStorage.setItem('fpsLegacyTempDestination','pilot');localStorage.setItem('fpsCustomMessageText','CLEAN LENS')}
function capture(){const d={};IDS.forEach(id=>{const v=get(id);if(v!==undefined)d[id]=v});d.fpsOsdEnabled=!!$('fpsOsdEnable')?.checked;d.fpsAuxEnable=!!$('fpsAuxEnable')?.checked;d.fpsOsdMessageCount=4;d.fpsWarningDestination=localStorage.getItem('fpsWarningDestination')||'1';d.fpsLegacyTempDestination=localStorage.getItem('fpsLegacyTempDestination')||'pilot';return d}
function restore(d){if(!d)return;Object.entries(d).forEach(([k,v])=>{if(IDS.includes(k))put(k,v)});if(d.fpsOsdEnabled!==undefined)put('fpsOsdEnable',d.fpsOsdEnabled);if(d.fpsAuxEnable!==undefined)put('fpsAuxEnable',d.fpsAuxEnable)}
function mirror(d){write(C3,{...read(C3),...d,bf45_compat:d.bf45Compat?'true':'false',pilot_tpl:d.pilotTpl||'',craft_tpl:d.craftTpl||'',fpv_flash:d.fpvFlash?'true':'false',fpv_prearm:d.fpvPreArm?'true':'false',fpv_prearm_text:d.fpvPreArmText||'',fpv_prearm_show:String(Math.round((Number(d.fpvCustomDurationSec)||1)*1000)),fpv_low_batt:d.fpvLowBatt?'true':'false',fpv_low_pct:String(d.fpvLowPct||10),fpv_rect_min:String(d.fpvRecLowMin||5)})}
function save(){if(!demo())return;const d=capture();write(CFG,d);mirror(d)}
function builder(id){return $(id)?.closest('.fps-builder')}
function visible(el,on){if(!el)return;el.classList.remove('fps-user-hidden','fps-toggle-hidden','fps-disabled','fps-collapsed','fps-hidden');el.style.display=on?'':'none'}
function featureHead(toggle){return $(toggle)?.closest('.fps-feature-head')||$('fpsWarningsHead')}
function featureRows(toggle){const h=featureHead(toggle);if(!h?.parentElement)return[];return [...h.parentElement.children].filter(x=>x!==h)}
function feature(toggle,on){const e=$(toggle),h=featureHead(toggle);if(e)e.disabled=false;if(h){h.classList.remove('fps-user-hidden','fps-toggle-hidden','fps-hidden','fps-collapsed');h.style.display='';}featureRows(toggle).forEach(r=>{r.classList.remove('fps-user-hidden','fps-toggle-hidden','fps-hidden');r.classList.toggle('fps-collapsed',!on);r.style.display=on?'':'none'});}
function refresh(){const legacy=!!$('bf45Compat')?.checked,osd=!!$('fpsOsdEnable')?.checked;
 // OSD: one active family only; builders keep their element picker/body intact.
 ['osd1','osd2','osd3','osd4'].forEach(id=>visible(builder(id),osd&&!legacy));
 visible(builder('pilotTpl'),osd&&legacy&&!!$('pilotEn')?.checked);visible(builder('craftTpl'),osd&&legacy&&!!$('craftEn')?.checked);
 const method=$('fpsOsdMethod');if(method)method.value=legacy?'legacy':'current';
 // These shared feature masters must always remain visible in both BF modes.
 feature('fpsCustomEnable',!!$('fpsCustomEnable')?.checked);feature('fpvLowBatt',!!$('fpvLowBatt')?.checked);feature('fpsAuxEnable',!!$('fpsAuxEnable')?.checked);
 // Old helpers disable/clear Temporary Message while OFF. Undo that: OFF means hidden body, not lost value.
 const t=$('fpvPreArmText');if(t){t.disabled=false;if(!t.value)t.value=localStorage.getItem('fpsCustomMessageText')||'CLEAN LENS'}
 [$('fpvPreArm'),$('fpvCustomDurationSec'),$('fpvLowPct'),$('fpvRecLowMin')].forEach(e=>{if(e)e.disabled=false});
 // Remove old mode-inactive treatment and unavailable labels.
 document.querySelectorAll('#panel-config .fps-mode-inactive,#panel-config .fps-disabled').forEach(e=>e.classList.remove('fps-mode-inactive','fps-disabled'));
 document.querySelectorAll('#panel-config .fps-unavailable').forEach(e=>e.style.display='none');
}
function rebuild(){['osd1','osd2','osd3','osd4','pilotTpl','craftTpl'].forEach(id=>$(id)?.dispatchEvent(new Event('input',{bubbles:false})));refresh()}
function init(){if(!$('panel-config'))return;
 // New UI revision deliberately seeds the requested profile once, replacing stale/broken demo snapshots from earlier revisions.
 if(!localStorage.getItem(FRESH)){seed();localStorage.setItem(FRESH,'1');if(demo()){write(CFG,capture());mirror(capture())}}
 else if(demo()&&Object.keys(read(CFG)).length)restore(read(CFG));
 rebuild();
 // Older scripts finish after DOMContentLoaded; reassert once without firing their change handlers.
 setTimeout(()=>{if(demo()&&Object.keys(read(CFG)).length)restore(read(CFG));rebuild()},120);
 document.addEventListener('input',e=>{if(!e.target.closest?.('#panel-config'))return;if(e.target.id==='fpvPreArmText'&&e.target.value)localStorage.setItem('fpsCustomMessageText',e.target.value);setTimeout(()=>{refresh();save()},0)},true);
 document.addEventListener('change',e=>{if(!e.target.closest?.('#panel-config'))return;setTimeout(()=>{refresh();save()},0)},true);
 document.addEventListener('click',e=>{if(e.target.closest?.('a'))save()},true);window.addEventListener('pagehide',save);window.addEventListener('beforeunload',save);
 document.querySelectorAll('#panel-config .cfg-field-desc').forEach(e=>{if((e.textContent||'').trim()==='Default: battery percentage')e.style.display='none'});
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,700));else setTimeout(init,700);
})();