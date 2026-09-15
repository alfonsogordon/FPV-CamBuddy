(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const STORE='fpsCollapsedMenusV8';
function load(){try{return JSON.parse(localStorage.getItem(STORE)||'{}')}catch{return{}}}
function save(v){localStorage.setItem(STORE,JSON.stringify(v))}
function enabled(root,toggle){root.classList.toggle('fps-menu-enabled',!!toggle?.checked)}
function addChevron(host,key,getTargets,toggle,root,opts={}){
 if(!host||host.querySelector(':scope > .fps-menu-collapse-arrow'))return;
 const {defaultCollapsed=false,followToggle=false}=opts;
 const btn=document.createElement('button');btn.type='button';btn.className='fps-menu-collapse-arrow';btn.setAttribute('aria-label','Collapse section');
 const saved=load();let collapsed=Object.prototype.hasOwnProperty.call(saved,key)?!!saved[key]:defaultCollapsed;
 function persist(){const s=load();s[key]=collapsed;save(s)}
 function draw(){for(const el of getTargets())if(el)el.classList.toggle('fps-menu-collapsed-target',collapsed);root.classList.toggle('fps-menu-collapsed',collapsed);btn.classList.toggle('is-collapsed',collapsed);btn.setAttribute('aria-expanded',collapsed?'false':'true');btn.title=collapsed?'Expand section':'Collapse section';enabled(root,toggle)}
 btn.addEventListener('click',e=>{e.preventDefault();e.stopPropagation();collapsed=!collapsed;persist();draw()});host.appendChild(btn);
 if(toggle)toggle.addEventListener('change',()=>{if(followToggle){collapsed=!toggle.checked;persist()}draw()});draw();
}
function slug(v){return String(v||'settings').toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'')}
function cardHeader(card){return card?.querySelector(':scope > .card-header')||null}
function enhanceOsd(){
 const master=document.getElementById('fpsOsdMaster'),top=master?.closest('.fps-top'),card=top?.closest('.config-card'),head=cardHeader(card);
 if(!master||!top||!card||!head||card.dataset.fpsOsdCollapse==='1')return;
 card.dataset.fpsOsdCollapse='1';card.dataset.fpsMenuCollapse='1';card.classList.add('fps-menu-collapse-root','fps-osd-collapse-root');
 addChevron(head,'osd-main',()=>{const version=top.querySelector(':scope > .fps-version');const rest=[...card.children].filter(x=>x!==head&&x!==top);return [version,...rest,document.getElementById('fpsIntegratedPreview')].filter(Boolean)},master,card,{defaultCollapsed:false,followToggle:false});
}
function enhanceModeSwitchByText(){
 const heads=[...document.querySelectorAll('.card-header')];const head=heads.find(h=>/camera\s*mode\s*switch/i.test(h.textContent||''));const root=head?.parentElement;if(!head||!root||root.dataset.fpsModeSwitchCollapse==='1')return;
 root.dataset.fpsModeSwitchCollapse='1';root.dataset.fpsMenuCollapse='1';root.classList.add('fps-menu-collapse-root');addChevron(head,'camera-mode-switch',()=>[...root.children].filter(x=>x!==head),null,root,{defaultCollapsed:false});
}
function enhanceCard(card){if(!card||card.dataset.fpsMenuCollapse==='1'||card.id==='fpsIntegratedPreview')return;const head=cardHeader(card);if(!head)return;const title=head.querySelector('h1,h2,h3,strong')?.textContent||head.textContent||'settings';card.dataset.fpsMenuCollapse='1';card.classList.add('fps-menu-collapse-root');addChevron(head,'card-'+slug(title),()=>[...card.children].filter(x=>x!==head),null,card,{defaultCollapsed:false})}
function enhanceSection(sec){
 const head=sec?.querySelector(':scope > .fps-section-head'),body=sec?.querySelector(':scope > .fps-section-body'),toggle=head?.querySelector('input[type="checkbox"]');if(!sec||!head||!body||sec.dataset.fpsMenuCollapse==='1')return;
 sec.dataset.fpsMenuCollapse='1';sec.classList.add('fps-menu-collapse-root');addChevron(head,'section-'+(toggle?.id||slug(head.textContent)),()=>[body],toggle,sec,{defaultCollapsed:false,followToggle:false});
}
function scan(){enhanceOsd();enhanceModeSwitchByText();document.querySelectorAll('.config-card').forEach(enhanceCard);document.querySelectorAll('.fps-section').forEach(enhanceSection)}
const style=document.createElement('style');style.id='fpsCollapsibleSectionsStyle';style.textContent=`
.fps-menu-collapse-root{transition:border-color .18s ease,box-shadow .18s ease,background .18s ease,opacity .18s ease}.fps-menu-collapse-root.fps-menu-enabled{border-color:rgba(124,58,237,.78)!important;box-shadow:0 0 0 1px rgba(124,58,237,.2),0 0 16px rgba(124,58,237,.08)}.fps-menu-collapse-root.fps-menu-enabled.fps-menu-collapsed{background:rgba(124,58,237,.045)!important;opacity:.86}.fps-osd-collapse-root.fps-menu-enabled.fps-menu-collapsed{background:rgba(124,58,237,.035)!important;box-shadow:0 0 0 1px rgba(124,58,237,.22),0 0 14px rgba(124,58,237,.07)!important}.fps-menu-collapsed-target{display:none!important}
.fps-menu-collapse-arrow{margin-left:auto!important;width:30px!important;height:30px!important;min-width:30px!important;padding:0!important;border:0!important;background:transparent!important;position:relative!important;border-radius:7px!important;box-shadow:none!important;flex:0 0 auto!important}.fps-menu-collapse-arrow:hover{background:rgba(124,58,237,.12)!important}.fps-menu-collapse-arrow::before{content:'';position:absolute;left:10px;top:8px;width:8px;height:8px;border-right:2px solid #b8a7e8;border-bottom:2px solid #b8a7e8;transform:rotate(45deg);transition:transform .16s ease,top .16s ease,border-color .16s ease}.fps-menu-collapse-arrow.is-collapsed::before{transform:rotate(-45deg);top:10px}.fps-menu-enabled .fps-menu-collapse-arrow::before{border-color:#c4b5fd}.card-header,.fps-section-head{display:flex;align-items:center;gap:8px}.card-header .fps-menu-collapse-arrow,.fps-section-head .fps-menu-collapse-arrow{margin-left:auto!important}
`;document.head.appendChild(style);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(scan,600));else setTimeout(scan,600);new MutationObserver(()=>scan()).observe(document.documentElement,{childList:true,subtree:true});
})();