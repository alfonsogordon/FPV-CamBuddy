(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const LIMIT=16;
const BASE={Status:'state',Battery:'batt','Recording Duration':'recdur',Mode:'mode',Resolution:'res',FPS:'fps',Stabilisation:'eis','Time Remaining':'rectf','Storage Remaining':'rcap'};
const TOKEN_RE=/\{[^}]+\}/g;
const clean=v=>String(v||'').replace(/[\[\]\x00-\x1f\x7f]/g,'').trim().replace(/\s+/g,' ').slice(0,7);
const $=id=>document.getElementById(id);
function advanced(){return !!window.fpsMultiCamOsd?.advancedOn?.()}
function multi(){return !!window.fpsMultiCamOsd?.multiOn?.()}
function idMode(){return localStorage.getItem('fpsAdvancedMultiCamOsdIdentifier')==='tag'?'t':'n'}
function parse(raw){const s=String(raw||'').replace(/^\{|\}$/g,''),m=s.match(/^([a-z0-9]+)@(\d+)([nt])$/i);return m?{base:m[1],source:Number(m[2]),mode:m[3]}:{base:s.replace(/@.*$/,''),source:null,mode:null}}
function cams(){const m=window.fpsMultiCamOsd?.cameras;if(!m?.values)return[];return [...m.values()].filter(c=>Number(c?.number)>0)}
function identifierLen(source,mode){const list=cams(),one=source>0?list.find(c=>Number(c.number)===source):null;const len=c=>mode==='t'?(clean(c?.label).length||('C'+Math.max(1,Number(c?.number)||1)).length):('C'+Math.max(1,Number(c?.number)||1)).length;if(one)return len(one);if(source>0)return ('C'+source).length;return list.length?Math.max(...list.map(len)):2}
function simpleValue(base){
 const batt=Math.max(0,Math.min(100,Number($('fpsPreviewBattery')?.value)||69));
 const remain=Math.max(0,Math.min(999,Number($('fpsPreviewRemain')?.value)||45));
 const count=Math.max(0,Number($('fpsPreviewCameraCount')?.value)||1);
 if(base==='state'||base==='stateonly')return multi()&&count>1?`RDY (${count})`:'RDY';
 if(base==='batt')return `B:${batt}`;
 if(base==='rectf')return `T:${remain}m`;
 if(base==='recdur')return '00:42';
 if(base==='mode')return 'VIDEO';
 if(base==='res')return '4K';
 if(base==='fps')return '60';
 if(base==='eis')return 'HS';
 if(base==='rcap')return '128GB';
 return '---';
}
function advancedBaseValue(base,source){const c=source>0?cams().find(x=>Number(x.number)===source):null;if(c&&window.fpsMultiCamOsd?.resolvePreviewToken){const mode=idMode()==='t'?'t':'n',v=window.fpsMultiCamOsd.resolvePreviewToken(`${base}@${source}${mode}`,{});if(v!=null)return String(v)}return null}
function tokenValue(tok){const p=parse(tok);if(!advanced()||p.source===null||p.source===0)return simpleValue(p.base);const live=advancedBaseValue(p.base,p.source);if(live!==null)return live;const suffix=1+identifierLen(p.source,p.mode||idMode());const base={state:3,stateonly:3,batt:4,recdur:5,mode:5,res:2,fps:2,eis:2,rectf:5,rcap:5}[p.base]||3;return 'X'.repeat(base+suffix)}
function lengthFor(tpl){const rendered=String(tpl||'').replace(TOKEN_RE,t=>tokenValue(t));return rendered.replace(/\s+/g,' ').trim().length}
function inputFor(builder){return builder?.querySelector('input[id],textarea[id]')||null}
function normalizeAuto(input){if(!advanced()||!input)return false;const next=String(input.value||'').replace(/\{([a-z0-9]+)@0[nt]\}/gi,'{$1}');if(next===input.value)return false;input.value=next;return true}
function tokenFor(builder,base){if(!advanced())return `{${base}}`;const source=Number(builder.querySelector('.fps-multiosd-source')?.value)||0;return source>0?`{${base}@${source}${idMode()}}`:`{${base}}`}
function candidate(builder,btn){const input=inputFor(builder),base=BASE[btn?.textContent?.trim()];if(!input||!base)return null;const add=tokenFor(builder,base),existing=input.value.match(TOKEN_RE)||[];if(existing.includes(add))return input.value;const current=String(input.value||'').trim();return current?`${current} ${add}`:add}
function ensureBadge(builder){let badge=builder.querySelector(':scope > .fps-osd-capacity-guard');if(!badge){badge=document.createElement('span');badge.className='fps-osd-capacity-guard';builder.querySelector('.fps-add')?.insertAdjacentElement('afterend',badge)}return badge}
function modeText(){if(!advanced())return multi()?'Current default Multi Cam output':'Current Single Cam output';return idMode()==='t'?'Advanced Multi Cam · saved tags':'Advanced Multi Cam · C# IDs'}
function refresh(builder){const input=inputFor(builder);if(!input)return;normalizeAuto(input);const used=lengthFor(input.value),badge=ensureBadge(builder);if(badge){badge.textContent=`${used} / ${LIMIT}`;badge.classList.toggle('over',used>LIMIT);badge.title=`${modeText()}: ${used} of ${LIMIT} characters with the current preview/configuration.`}let any=false;for(const btn of builder.querySelectorAll('.fps-menu button')){const c=candidate(builder,btn);if(c===null)continue;const overflow=lengthFor(c)>LIMIT,was=btn.dataset.fpsCapacityBlocked==='1';if(overflow){if(!was){btn.dataset.fpsCapacityPrevDisabled=btn.disabled?'1':'0';btn.dataset.fpsCapacityPrevTitle=btn.title||''}btn.dataset.fpsCapacityBlocked='1';btn.disabled=true;btn.title=`Won't fit with the current ${modeText()} settings (${LIMIT} characters max).`}else{if(was){btn.dataset.fpsCapacityBlocked='0';if(btn.dataset.fpsCapacityPrevDisabled!=='1')btn.disabled=false;btn.title=btn.dataset.fpsCapacityPrevTitle||''}else if(btn.disabled&&String(btn.title||'').startsWith('Would exceed Betaflight')){btn.disabled=false;btn.title=''}if(!btn.disabled)any=true}}const add=builder.querySelector('.fps-add');if(add){add.disabled=!any;add.title=any?'':`No remaining OSD element fits with the current ${modeText()} settings.`}}
function refreshAll(){document.querySelectorAll('.fps-builder').forEach(refresh)}
document.addEventListener('click',e=>{const btn=e.target.closest?.('.fps-menu button');if(!btn)return;const b=btn.closest('.fps-builder'),c=candidate(b,btn);if(c!==null&&lengthFor(c)>LIMIT){e.preventDefault();e.stopImmediatePropagation();refresh(b)}},true);
document.addEventListener('input',e=>{if(e.target.matches?.('.fps-cam-label-edit input,#fpsPreviewBattery,#fpsPreviewRemain,.fps-multiosd-cam input'))setTimeout(refreshAll,0);const b=e.target.closest?.('.fps-builder');if(b){normalizeAuto(inputFor(b));setTimeout(()=>refresh(b),0)}},true);
document.addEventListener('change',e=>{if(e.target.matches?.('.fps-multiosd-source,#fpsMultiOsdIdentifier,#fpsAdvancedMultiOsd,#fpsBfMode,#fpsMultiCamSync,#fpsExperimentalMaster,#fpsOsdMaster,#fpsPreviewCameraCount,.fps-multiosd-cam input'))setTimeout(refreshAll,0)},true);
document.addEventListener('fps-camera-registry-update',()=>setTimeout(refreshAll,0));
let queued=false;new MutationObserver(()=>{if(queued)return;queued=true;requestAnimationFrame(()=>{queued=false;refreshAll()})}).observe(document.documentElement,{childList:true,subtree:true});
const style=document.createElement('style');style.textContent=`.fps-osd-capacity-guard{display:inline-block;margin-left:8px;font:700 10px/1 monospace;color:#a78bfa;vertical-align:middle}.fps-osd-capacity-guard.over{color:#ff5c6c}.fps-builder .fps-add:disabled{opacity:.42}.fps-multiosd-capacity,.fps-multiosd-builder-note{display:none!important}`;document.head.appendChild(style);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(refreshAll,650));else setTimeout(refreshAll,650);
})();