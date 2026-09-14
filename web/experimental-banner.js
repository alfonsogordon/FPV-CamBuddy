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
if(!document.querySelector('.fps-exp-banner')){const b=document.createElement('div');b.className='fps-exp-banner';b.innerHTML='<strong>⚠ EXPERIMENTAL V1.0.2 TEST ENVIRONMENT</strong> — development firmware and controls; bench test before flight';document.body.prepend(b)}
const brand=document.querySelector('.brand');
if(brand&&!brand.querySelector('.fps-test-label')){const t=document.createElement('span');t.className='fps-test-label';brand.appendChild(t)}
})();
