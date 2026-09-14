(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const YELLOW='#f5ff00';
const s=document.createElement('style');
s.textContent=`
.fps-exp-banner{position:sticky;top:0;z-index:99999;background:${YELLOW};color:#101318;text-align:center;font:950 13px/1.25 system-ui,sans-serif;padding:8px 12px;letter-spacing:.055em;border-bottom:2px solid #a8b000;box-shadow:0 0 16px rgba(245,255,0,.34)}
.fps-exp-banner strong{font-weight:1000}.fps-test-label{margin-left:7px;padding:2px 6px;border-radius:4px;background:${YELLOW};color:#101318;font-size:10px;font-weight:950;letter-spacing:.07em;white-space:nowrap}.fps-test-label::before{content:'EXPERIMENTAL'}
@media(max-width:650px){.fps-test-label{font-size:8px;margin-left:4px}.fps-exp-banner{font-size:11px}}
`;
document.head.appendChild(s);
function dedupeBanner(){
 const banners=[...document.querySelectorAll('.fps-exp-banner')];
 let keep=banners.find(b=>b.dataset.fpsCanonical==='1')||banners[0];
 if(!keep){keep=document.createElement('div');keep.className='fps-exp-banner';document.body.prepend(keep)}
 keep.dataset.fpsCanonical='1';
 keep.innerHTML='<strong>⚠ EXPERIMENTAL V1.0.2 TEST ENVIRONMENT</strong> — development firmware and controls; bench test before flight';
 banners.forEach(b=>{if(b!==keep)b.remove()});
}
dedupeBanner();
const observer=new MutationObserver(()=>dedupeBanner());
observer.observe(document.body,{childList:true,subtree:false});
window.addEventListener('load',()=>{dedupeBanner();setTimeout(()=>{dedupeBanner();observer.disconnect()},1500)});
const brand=document.querySelector('.brand');
if(brand&&!brand.querySelector('.fps-test-label')){const t=document.createElement('span');t.className='fps-test-label';brand.appendChild(t)}
})();
