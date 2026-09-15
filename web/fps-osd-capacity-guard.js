(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const LIMIT=16;
const BASE={Status:'state',Battery:'batt','Recording Duration':'recdur',Mode:'mode',Resolution:'res',FPS:'fps',Stabilisation:'eis','Time Remaining':'rectf','Storage Remaining':'rcap'};
const TOKEN_RE=/\{[^}]+\}/g;
const $=id=>document.getElementById(id);
function api(){return window.fpsMultiCamOsd||null}
function advanced(){return !!api()?.advancedOn?.()}
function multi(){return !!api()?.multiOn?.()}
function idMode(){return localStorage.getItem('fpsAdvancedMultiCamOsdIdentifier')==='tag'?'t':'n'}
function aggregateState(){const a=api()?.aggregate?.();const count=Number(a?.count)||0;if(!count)return'ERR';const live=[...(api()?.cameras?.values?.()||[])].filter(c=>c.connected),recording=live.filter(c=>c.recording).length;if(!recording)return count>1?`RDY (${count})`:'RDY';if(recording===count)return count>1?`REC (${count})`:'REC';return`PART ${recording}/${count}`}
function legacyLength(tpl){const batt=Math.max(0,Math.min(100,Number($('fpsPreviewBattery')?.value)||69)),remain=Math.max(0,Math.min(999,Number($('fpsPreviewRemain')?.value)||45));const map={state:'RDY',stateonly:'RDY',batt:`B:${batt}`,recdur:'00:42',mode:'VIDEO',res:'4K',fps:'60',eis:'HS',rectf:`T:${remain}m`,rcap:'128GB'};return String(tpl||'').replace(TOKEN_RE,t=>{const raw=t.slice(1,-1).replace(/@.*$/,'');return map[raw]??'---'}).replace(/\s+/g,' ').trim().length}
function lengthFor(tpl){const a=api();if(advanced()&&a?.renderTemplate){return a.renderTemplate(tpl,{aggregateState:aggregateState(),elapsed:'00:42'},{truncate:false}).length}return legacyLength(tpl)}
function inputFor(builder){return builder?.querySelector('input[id],textarea[id]')||null}
function normalizeAuto(input){if(!advanced()||!input)return;const next=String(input.value||'').replace(/\{([a-z0-9]+)@0[nt]\}/gi,'{$1}');if(next!==input.value)input.value=next}
function tokenFor(builder,base){if(!advanced())return`{${base}}`;const source=Number(builder.querySelector('.fps-multiosd-source')?.value)||0;return source>0?`{${base}@${source}${idMode()}}`:`{${base}}`}
function candidate(builder,btn){const input=inputFor(builder),base=BASE[btn?.textContent?.trim()];if(!input||!base)return null;const add=tokenFor(builder,base),existing=input.value.match(TOKEN_RE)||[];if(existing.includes(add))return input.value;const current=String(input.value||'').trim();return current?`${current} ${add}`:add}
function fits(tpl){return lengthFor(tpl)<=LIMIT}
function ensureBadge(builder){let badge=builder.querySelector(':scope > .fps-osd-capacity-guard');if(!badge){badge=document.createElement('span');badge.className='fps-osd-capacity-guard';builder.querySelector('.fps-add')?.insertAdjacentElement('afterend',badge)}return badge}
function modeText(){if(!advanced())return multi()?'Current default Multi Cam output':'Current Single Cam output';return localStorage.getItem('fpsAdvancedMultiCamOsdIdentifier')==='tag'?'Advanced Multi Cam · saved tags':'Advanced Multi Cam · C# IDs'}
function refresh(builder){const input=inputFor(builder);if(!input)return;normalizeAuto(input);const used=lengthFor(input.value),badge=ensureBadge(builder);badge.textContent=`${used} / ${LIMIT}`;badge.classList.toggle('over',used>LIMIT);badge.title=`${modeText()}: ${used} of ${LIMIT} characters using the same renderer as the live preview.`;let any=false;for(const btn of builder.querySelectorAll('.fps-menu button')){const c=candidate(builder,btn);if(c===null)continue;const overflow=!fits(c);if(overflow){btn.dataset.fpsCapacityBlocked='1';btn.disabled=true;btn.title=`Won't fit with the current ${modeText()} settings (${LIMIT} characters max).`}else{if(btn.dataset.fpsCapacityBlocked==='1'){btn.dataset.fpsCapacityBlocked='0';if(btn.dataset.fpsAdvancedBlocked!=='1')btn.disabled=false;if(btn.dataset.fpsAdvancedBlocked!=='1')btn.title=''}if(!btn.disabled)any=true}}const add=builder.querySelector('.fps-add');if(add){add.disabled=!any;add.title=any?'':`No remaining OSD element fits with the current ${modeText()} settings.`}}
function refreshAll(){document.querySelectorAll('.fps-builder').forEach(refresh)}
document.addEventListener('click',e=>{const btn=e.target.closest?.('.fps-menu button');if(!btn)return;const b=btn.closest('.fps-builder'),c=candidate(b,btn);if(c!==null&&!fits(c)){e.preventDefault();e.stopImmediatePropagation();refresh(b)}},true);
document.addEventListener('input',e=>{if(e.target.matches?.('.fps-cam-label-edit input,#fpsPreviewBattery,#fpsPreviewRemain,.fps-multiosd-cam input'))setTimeout(refreshAll,0);const b=e.target.closest?.('.fps-builder');if(b){normalizeAuto(inputFor(b));setTimeout(()=>refresh(b),0)}},true);
document.addEventListener('change',e=>{if(e.target.matches?.('.fps-multiosd-source,#fpsMultiOsdIdentifier,#fpsAdvancedMultiOsd,#fpsBfMode,#fpsMultiCamSync,#fpsExperimentalMaster,#fpsOsdMaster,#fpsPreviewCameraCount,.fps-multiosd-cam input'))setTimeout(refreshAll,0)},true);
document.addEventListener('fps-camera-registry-update',()=>setTimeout(refreshAll,0));document.addEventListener('fps-camera-label-update',()=>setTimeout(refreshAll,0));window.addEventListener('fps-multiosd-preview',()=>setTimeout(refreshAll,0));
let queued=false;new MutationObserver(()=>{if(queued)return;queued=true;requestAnimationFrame(()=>{queued=false;refreshAll()})}).observe(document.documentElement,{childList:true,subtree:true});
const style=document.createElement('style');style.textContent=`.fps-osd-capacity-guard{display:inline-block;margin-left:8px;font:700 10px/1 monospace;color:#a78bfa;vertical-align:middle}.fps-osd-capacity-guard.over{color:#ff5c6c}.fps-builder .fps-add:disabled{opacity:.42}.fps-multiosd-capacity,.fps-multiosd-builder-note{display:none!important}`;document.head.appendChild(style);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(refreshAll,650));else setTimeout(refreshAll,650);
})();