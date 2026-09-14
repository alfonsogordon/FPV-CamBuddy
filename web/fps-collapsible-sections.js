(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const STORE='fpsCollapsedSectionsV1';
function load(){try{return JSON.parse(localStorage.getItem(STORE)||'{}')}catch{return{}}}
function save(v){localStorage.setItem(STORE,JSON.stringify(v))}
function sectionKey(sec,i){return sec.id||sec.dataset.fpsCollapseKey||(`section-${i}`)}
function enabledToggle(sec){return sec.querySelector(':scope > .fps-section-head input[type="checkbox"], :scope > .fps-master input[type="checkbox"]')}
function refreshEnabled(sec){const t=enabledToggle(sec);sec.classList.toggle('fps-section-enabled',!!t?.checked)}
function enhance(sec,i){
 if(sec.dataset.fpsCollapsible==='1')return;const head=sec.querySelector(':scope > .fps-section-head');const body=sec.querySelector(':scope > .fps-section-body');if(!head||!body)return;
 sec.dataset.fpsCollapsible='1';const key=sectionKey(sec,i);sec.dataset.fpsCollapseKey=key;
 const btn=document.createElement('button');btn.type='button';btn.className='fps-collapse-btn';btn.setAttribute('aria-label','Collapse section');btn.title='Collapse / expand';btn.innerHTML='<span class="fps-collapse-chevron">⌄</span>';
 const toggle=head.querySelector('.switch');if(toggle)head.insertBefore(btn,toggle);else head.appendChild(btn);
 const state=load();let collapsed=!!state[key];
 function draw(){body.classList.toggle('fps-section-body-collapsed',collapsed);sec.classList.toggle('fps-section-collapsed',collapsed);btn.setAttribute('aria-expanded',collapsed?'false':'true');btn.setAttribute('aria-label',collapsed?'Expand section':'Collapse section');btn.querySelector('.fps-collapse-chevron').textContent=collapsed?'›':'⌄';refreshEnabled(sec)}
 btn.addEventListener('click',()=>{collapsed=!collapsed;const s=load();s[key]=collapsed;save(s);draw()});
 enabledToggle(sec)?.addEventListener('change',()=>refreshEnabled(sec));draw();
}
function scan(){document.querySelectorAll('.fps-section').forEach(enhance)}
const style=document.createElement('style');style.id='fpsCollapsibleSectionsStyle';style.textContent=`
.fps-section{transition:border-color .18s ease,box-shadow .18s ease,background .18s ease}.fps-section.fps-section-enabled{border-color:rgba(124,58,237,.72)!important;box-shadow:0 0 0 1px rgba(124,58,237,.18),0 0 16px rgba(124,58,237,.08)}.fps-section.fps-section-enabled>.fps-section-head{background:linear-gradient(90deg,rgba(124,58,237,.12),rgba(124,58,237,.025))}.fps-section.fps-section-collapsed>.fps-section-head{border-bottom-color:transparent}.fps-section-body.fps-section-body-collapsed{display:none!important}.fps-collapse-btn{margin-left:auto;margin-right:8px;width:30px;height:30px;display:grid;place-items:center;border:1px solid rgba(167,139,250,.28)!important;border-radius:7px;background:rgba(124,58,237,.06)!important;color:#c4b5fd!important;padding:0!important;min-width:30px!important;cursor:pointer}.fps-collapse-btn:hover{background:rgba(124,58,237,.14)!important;border-color:rgba(167,139,250,.55)!important}.fps-collapse-chevron{font-size:19px;line-height:1;transform:translateY(-1px)}.fps-section-enabled .fps-collapse-btn{border-color:rgba(167,139,250,.48)!important;color:#e9ddff!important}.fps-section-head{gap:8px}
`;document.head.appendChild(style);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(scan,500));else setTimeout(scan,500);
new MutationObserver(()=>scan()).observe(document.documentElement,{childList:true,subtree:true});
})();