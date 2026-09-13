(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const KEY='fpsExperimentalFeatures';
const YELLOW='#f5ff00';
const s=document.createElement('style');
s.textContent=`
.fps-exp-banner{display:none;position:sticky;top:0;z-index:99999;background:${YELLOW};color:#101318;text-align:center;font:900 13px/1.25 system-ui,sans-serif;padding:7px 12px;letter-spacing:.04em;border-bottom:1px solid #b8c000;box-shadow:0 0 14px rgba(245,255,0,.28)}
.fps-exp-banner.show{display:block}.fps-exp-banner strong{font-weight:950}
.fps-test-label{margin-left:7px;color:#8a9200;font-size:10px;font-weight:900;letter-spacing:.06em;white-space:nowrap}.fps-test-label::before{content:'TEST VERSION'}
@media(max-width:650px){.fps-test-label{font-size:8px;margin-left:4px}}
`;
document.head.appendChild(s);
const b=document.createElement('div');
b.className='fps-exp-banner';
b.innerHTML='<strong>EXPERIMENTAL FEATURES ENABLED</strong> — development functionality; bench test before relying on it in flight';
document.body.prepend(b);
const brand=document.querySelector('.brand');
if(brand&&!brand.querySelector('.fps-test-label')){const t=document.createElement('span');t.className='fps-test-label';brand.appendChild(t)}
function sync(){b.classList.toggle('show',localStorage.getItem(KEY)==='1')}
sync();
window.addEventListener('storage',e=>{if(e.key===KEY)sync()});
document.addEventListener('change',()=>setTimeout(sync,0));
})();
