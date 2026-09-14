(()=>{
'use strict';
function init(){
 const hero=document.querySelector('.cta-row');
 if(hero&&!document.querySelector('.fps-socials')){
  const s=document.createElement('div');
  s.className='fps-socials';
  s.innerHTML='<a href="https://www.youtube.com/@FPSteVe" target="_blank" rel="noopener">YouTube · @FPSteVe ↗</a><a href="https://www.instagram.com/fpvsteve/" target="_blank" rel="noopener">Instagram · @fpvsteve ↗</a>';
  hero.after(s);
 }
 const foot=document.querySelector('.foot-links');
 if(foot&&!foot.querySelector('[href*="youtube.com/@FPSteVe"]')){
  foot.insertAdjacentHTML('beforeend','<a href="https://www.youtube.com/@FPSteVe" target="_blank" rel="noopener">YouTube</a><a href="https://www.instagram.com/fpvsteve/" target="_blank" rel="noopener">Instagram</a>');
 }

 if(!document.querySelector('#fpsExperimentalFeatures')){
  const normalFeatures=[...document.querySelectorAll('section')].find(sec=>sec.querySelector('.section-head h2')?.textContent.trim()==='Features');
  if(normalFeatures){
   const exp=document.createElement('section');
   exp.id='fpsExperimentalFeatures';
   exp.className='fps-exp-features';
   exp.innerHTML=`
    <div class="section-head">
     <div class="fps-exp-kicker">V1.0.2 · EXPERIMENTAL</div>
     <h2>Experimental Features</h2>
     <p>New V1.0.2 development features currently available only in the experimental firmware and configurator. Bench-test before flight.</p>
    </div>
    <div class="fps-exp-grid">
     <div class="fps-exp-card"><h3>Multi Cam Coordinator <span class="fps-exp-badge">NEW</span></h3><p>One C3 can coordinate supported GoPro, DJI, Sony, Blackmagic, Insta360 and Caddx camera backends. Cameras that join or reconnect are brought into line with the currently requested START/STOP state.</p></div>
     <div class="fps-exp-card"><h3>Multi-GoPro Support <span class="fps-exp-badge">NEW</span></h3><p>The experimental GoPro backend supports up to eight application slots instead of the old six-camera cap. Real simultaneous BLE capacity still depends on ESP32 resources and needs hardware validation.</p></div>
     <div class="fps-exp-card"><h3>Multi-Camera OSD <span class="fps-exp-badge">NEW</span></h3><p>See aggregate camera state directly in Betaflight with compact states such as <strong>REC 2/2</strong> and <strong>PART 1/2</strong>, plus the new <strong>{cams}</strong> token.</p></div>
     <div class="fps-exp-card"><h3>Advanced BLE TX Power <span class="fps-exp-badge">NEW</span></h3><p>Optional Idle → Arm Boost → Armed → Disarm Boost → Idle BLE power profile, with selectable -12 to +9 dBm levels and configurable boost durations.</p></div>
     <div class="fps-exp-card"><h3>Multi-Camera Power Latch <span class="fps-exp-badge">NEW</span></h3><p>Optionally leave the normal BLE behaviour untouched until more than one camera has actually been detected. Once triggered, the multi-camera power profile remains active until reboot.</p></div>
     <div class="fps-exp-card"><h3>Camera-Count Status LED <span class="fps-exp-badge">NEW</span></h3><p>With Multi Cam active, multiple connected cameras are shown as a repeating pulse count on the status LED, while a single connected camera keeps the normal indication.</p></div>
     <div class="fps-exp-card"><h3>Experimental Multi Cam Settings <span class="fps-exp-badge">UI</span></h3><p>All V1.0.2 Multi Cam and advanced BLE power controls are grouped together in one experimental settings area so the normal V1 configuration remains uncluttered.</p></div>
     <div class="fps-exp-card"><h3>Single-GoPro Path Preserved <span class="fps-exp-badge fps-exp-safe">ISOLATED</span></h3><p>Leave Multi Cam disabled and the normal single-camera GoPro path remains in use, including the existing arm/disarm recording flow and GoPro Burst Slo-Mo AUX behaviour.</p></div>
     <div class="fps-exp-card"><h3>Hardware Validation <span class="fps-exp-badge fps-exp-next">NEXT</span></h3><p>Two-camera operation, 3+ scaling, mixed camera families, reconnect behaviour, staged BLE power, REC/PART counts and LED pulses are implemented but still need real-hardware acceptance testing.</p></div>
    </div>
    <div class="fps-exp-actions"><a class="btn primary" href="flash.html">Flash V1.0.2 Experimental →</a><a class="btn" href="config.html">Open Experimental Config</a><a class="btn" href="https://github.com/alfonsogordon/freeclinker/blob/experimental/EXPERIMENTAL_V1.0.2.md" target="_blank" rel="noopener">V1.0.2 Notes</a></div>`;
   normalFeatures.parentNode.insertBefore(exp,normalFeatures);
  }
 }

 const style=document.createElement('style');
 style.textContent=`
 .fps-socials{display:flex;justify-content:center;gap:14px;flex-wrap:wrap;margin-top:15px;font-size:12px}.fps-socials a{padding:5px 9px;border-bottom:1px solid #6d3fc0}.fps-socials a:hover{text-decoration:none;color:#d8c4ff}
 .fps-exp-features{padding-top:8px}.fps-exp-features .section-head{margin-bottom:20px}.fps-exp-kicker{display:inline-block;margin-bottom:8px;padding:4px 9px;border:1px solid #b9c000;border-radius:5px;background:rgba(245,255,0,.11);color:#f5ff00;font-size:10px;font-weight:900;letter-spacing:.12em}.fps-exp-features .section-head h2{color:#f5ff00}.fps-exp-features .section-head p{max-width:760px;margin:8px auto 0;color:#b9bd9f}
 .fps-exp-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:13px}.fps-exp-card{background:linear-gradient(145deg,#15180f,#0e151f 55%);border:1px solid #6f7500;border-radius:8px;padding:17px 18px;box-shadow:0 0 18px rgba(245,255,0,.035)}.fps-exp-card h3{font-size:13px;color:#f2f6dc;margin-bottom:6px}.fps-exp-card p{font-size:11px;color:#aeb49c}.fps-exp-card strong{color:#e8efc6}.fps-exp-badge{display:inline-block;margin-left:5px;padding:2px 5px;border-radius:4px;background:#f5ff00;color:#101318;font-size:8px;font-weight:950;letter-spacing:.06em;vertical-align:1px}.fps-exp-badge.fps-exp-safe{background:#9de8b3}.fps-exp-badge.fps-exp-next{background:#e5c38e}.fps-exp-actions{display:flex;justify-content:center;gap:10px;flex-wrap:wrap;margin-top:18px}
 @media(max-width:760px){.fps-exp-grid{grid-template-columns:1fr 1fr}}@media(max-width:500px){.fps-exp-grid{grid-template-columns:1fr}.fps-exp-card{padding:15px}}
 `;
 document.head.appendChild(style);
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();
