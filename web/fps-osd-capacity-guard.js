(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const LIMIT=16;
const BASE={Status:'state',Battery:'batt','Recording Duration':'recdur',Mode:'mode',Resolution:'res',FPS:'fps',Stabilisation:'eis','Time Remaining':'rectf','Storage Remaining':'rcap'};
const MAX={state:7,batt:5,recdur:5,mode:9,res:4,fps:3,eis:3,rectf:6,rcap:5};
const SIMPLE={state:3,stateonly:3,batt:4,recdur:5,mode:5,res:2,fps:2,eis:2,rectf:5,rcap:5};
const TOKEN_RE=/\{[^}]+\}/g;
function advanced(){return !!window.fpsMultiCamOsd?.advancedOn?.()}
function idMode(){return localStorage.getItem('fpsAdvancedMultiCamOsdIdentifier')==='tag'?'t':'n'}
function parse(raw){const s=String(raw||'').replace(/^\{|\}$/g,''),m=s.match(/^([a-z0-9]+)@(\d+)([nt])$/i);return m?{base:m[1],source:Number(m[2]),mode:m[3]}:{base:s.replace(/@.*$/,''),source:null,mode:null}}
function tokenLen(tok){const p=parse(tok);if(!advanced()&&p.source===null)return SIMPLE[p.base]||MAX[p.base]||8;let n=MAX[p.base]||8;if(p.source!==null){const identifiable=p.source>0||p.base==='batt'||p.base==='rectf';if(identifiable)n+=1+(p.mode==='t'?7:2)}return n}
function lengthFor(tpl){const a=String(tpl||'').match(TOKEN_RE)||[];return a.reduce((n,t)=>n+tokenLen(t),0)+Math.max(0,a.length-1)}
function inputFor(builder){return builder?.querySelector('input[id],textarea[id]')||null}
function tokenFor(builder,base){if(!advanced())return `{${base}}`;const source=Number(builder.querySelector('.fps-multiosd-source')?.value)||0;return `{${base}@${source}${idMode()}}`}
function candidate(builder,btn){const input=inputFor(builder),base=BASE[btn?.textContent?.trim()];if(!input||!base)return null;const add=tokenFor(builder,base),existing=input.value.match(TOKEN_RE)||[];if(existing.includes(add))return input.value;return [...existing,add].join(' ')}
function ensureBadge(builder){let badge=builder.querySelector(':scope > .fps-osd-capacity-guard');if(!badge){badge=document.createElement('span');badge.className='fps-osd-capacity-guard';const add=builder.querySelector('.fps-add');add?.insertAdjacentElement('afterend',badge)}return badge}
function refresh(builder){const input=inputFor(builder);if(!input)return;const used=lengthFor(input.value),badge=ensureBadge(builder);if(badge){badge.textContent=`${used} / ${LIMIT}`;badge.classList.toggle('over',used>LIMIT);badge.title=advanced()?`Worst-case Betaflight text length: ${used} of ${LIMIT} characters.`:`Typical displayed length: ${used} of ${LIMIT} characters. Simple mode keeps the proven default templates available.`}let any=false;for(const btn of builder.querySelectorAll('.fps-menu button')){const c=candidate(builder,btn);if(c===null)continue;const overflow=lengthFor(c)>LIMIT,wasCapacityBlocked=btn.dataset.fpsCapacityBlocked==='1';if(overflow){if(!wasCapacityBlocked)btn.dataset.fpsCapacityPrevDisabled=btn.disabled?'1':'0';btn.dataset.fpsCapacityBlocked='1';btn.disabled=true;btn.dataset.fpsCapacityPrevTitle=btn.title||'';btn.title=`Won't fit: this OSD box is limited to ${LIMIT} displayed characters.`}else{if(wasCapacityBlocked){btn.dataset.fpsCapacityBlocked='0';if(!advanced()&&btn.dataset.fpsCapacityPrevDisabled!=='1')btn.disabled=false;if(btn.dataset.fpsCapacityPrevTitle!==undefined)btn.title=btn.dataset.fpsCapacityPrevTitle}if(!btn.disabled)any=true}}const add=builder.querySelector('.fps-add');if(add){add.disabled=!any;add.title=any?'':`This OSD box is full (${LIMIT} displayed characters max). Remove an element to add another.`}}
function refreshAll(){document.querySelectorAll('.fps-builder').forEach(refresh)}
document.addEventListener('click',e=>{const btn=e.target.closest?.('.fps-menu button');if(!btn)return;const builder=btn.closest('.fps-builder'),c=candidate(builder,btn);if(c!==null&&lengthFor(c)>LIMIT){e.preventDefault();e.stopImmediatePropagation();refresh(builder)}},true);
document.addEventListener('input',e=>{const b=e.target.closest?.('.fps-builder');if(b)setTimeout(()=>refresh(b),0)},true);
document.addEventListener('change',e=>{if(e.target.matches?.('.fps-multiosd-source,#fpsMultiOsdIdentifier,#fpsAdvancedMultiOsd,#fpsBfMode,#fpsMultiCamSync'))setTimeout(refreshAll,0)},true);
let queued=false;new MutationObserver(()=>{if(queued)return;queued=true;requestAnimationFrame(()=>{queued=false;refreshAll()})}).observe(document.documentElement,{childList:true,subtree:true});
const style=document.createElement('style');style.textContent=`.fps-osd-capacity-guard{display:inline-block;margin-left:8px;font:700 10px/1 monospace;color:#a78bfa;vertical-align:middle}.fps-osd-capacity-guard.over{color:#ff5c6c}.fps-builder .fps-add:disabled{opacity:.42}`;document.head.appendChild(style);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(refreshAll,650));else setTimeout(refreshAll,650);
})();