(()=>{
'use strict';
const $=id=>document.getElementById(id);
const DEMO='freeclinkerDemoMode';
const DEFAULT_TPL=['','{batt}','{state} {recdur}','{mode} {res} {fps} {eis}','{rectf} {rcap}'];
let syncing=false,readTimer=null;

function legacyMode(){return $('fpsBfMode')?.value==='legacy'}
function connected(){return !!$('connectBtn')?.disabled}
function targetCode(value){
  if(legacyMode()) return value==='craft'?2:1;
  const n=parseInt(value,10);
  return n>=1&&n<=4?n:1;
}
function stripTarget(value){
  const s=String(value||'');
  const m=s.match(/^@([1-4]):(.*)$/s);
  return m?{target:parseInt(m[1],10),text:m[2]}:{target:1,text:s};
}
function setStatus(text,error=false){
  const e=$('fpsApplyStatus'); if(!e)return;
  e.textContent=text||'';
  e.classList.toggle('fps-apply-error',!!error);
}
function activeDestinationCount(){
  if(legacyMode()) return Number(!!$('fpsPilotMaster')?.checked)+Number(!!$('fpsCraftMaster')?.checked);
  return [1,2,3,4].filter(n=>$('fpsMsg'+n)?.checked).length;
}
async function cmd(text){
  if(typeof sendCommand!=='function') throw new Error('Serial command interface unavailable');
  await sendCommand(text);
}
function fire(id,type='change'){
  const e=$(id); if(e)e.dispatchEvent(new Event(type,{bubbles:true}));
}

async function disableHardwareOsd(showStatus=true){
  if(localStorage.getItem(DEMO)==='1'||!connected())return;
  if(showStatus)setStatus('Disabling OSD…');
  try{
    for(let n=1;n<=4;n++)await cmd(`set osd${n} {off}`);
    await cmd('set pilot_en 0');
    await cmd('set craft_en 0');
    await cmd('set fpv_state_mode 0');
    await cmd('set fpv_error 0');
    await cmd('set fpv_ready 0');
    await cmd('set fpv_record 0');
    await cmd('set fpv_flash 0');
    await cmd('set fpv_prearm 0');
    await cmd('set fpv_prearm_text @1:');
    await cmd('set fpv_low_batt 0');
    await cmd('set fpv_rect_warn 0');
    await cmd('set fpv_hot_warn 0');
    document.dispatchEvent(new CustomEvent('fps-settings-applied'));
    if(showStatus){setStatus('OSD disabled on C3 ✓');setTimeout(()=>setStatus(''),1400)}
  }catch(e){setStatus('Disable failed — '+e.message,true)}
}

async function applyParity(){
  if(localStorage.getItem(DEMO)==='1'){
    setStatus('Saved in Demo ✓');
    document.dispatchEvent(new CustomEvent('fps-settings-applied'));
    setTimeout(()=>setStatus(''),1200);
    return;
  }
  if(!connected()){setStatus('Connect a C3 first',true);return}
  if(!$('fpsOsdMaster')?.checked){await disableHardwareOsd();return}
  if(activeDestinationCount()===0&&($('fpsTempMaster')?.checked||$('fpsWarnMaster')?.checked)){
    setStatus('Enable an OSD destination first',true);return;
  }

  const legacy=legacyMode();
  const pilotOn=!!$('fpsPilotMaster')?.checked;
  const craftOn=!!$('fpsCraftMaster')?.checked;
  const recOnly=!!$('fpsRecOnly')?.checked;
  const flash=!!$('fpvFlash')?.checked;
  const tempOn=!!$('fpsTempMaster')?.checked;
  const warnOn=!!$('fpsWarnMaster')?.checked;
  const tempTarget=targetCode($('fpsTempDest')?.value);
  const warnTarget=targetCode($('fpsWarnDest')?.value);
  const tempText=String($('fpvPreArmText')?.value||'CLEAN LENS').replace(/^@[1-4]:/,'');
  const tempMs=Math.max(100,Math.round((Number($('fpvCustomDurationSec')?.value)||1)*1000));
  const lowPct=Math.max(0,Math.min(100,parseInt($('fpvLowPct')?.value,10)||0));
  const lowMins=Math.max(0,Math.min(999,parseInt($('fpvRecLowMin')?.value,10)||0));

  setStatus('Applying…');
  try{
    for(let n=1;n<=4;n++){
      const enabled=!!$('fpsMsg'+n)?.checked;
      const tpl=enabled?String($('osd'+n)?.value||DEFAULT_TPL[n]):'{off}';
      await cmd(`set osd${n} ${tpl}`);
    }

    await cmd(`set bf45_compat ${legacy?1:0}`);
    await cmd(`set pilot_en ${pilotOn?1:0}`);
    await cmd(`set pilot_tpl ${String($('pilotTpl')?.value||'').trim()||'{off}'}`);
    await cmd(`set craft_en ${craftOn?1:0}`);
    await cmd(`set craft_tpl ${String($('craftTpl')?.value||'').trim()||'{off}'}`);

    // fpv_state_mode is the persisted, authoritative OSD master flag.
    await cmd('set fpv_state_mode 1');
    await cmd('set fpv_error 1');
    await cmd('set fpv_err_text {state}');
    await cmd('set fpv_ready 1');
    await cmd('set fpv_ready_text {state}');
    await cmd('set fpv_record 1');
    await cmd(`set fpv_record_text ${recOnly?'@ARM:{state}':'{state}'}`);
    await cmd(`set fpv_flash ${flash?1:0}`);

    await cmd(`set fpv_prearm ${tempOn&&$('fpvPreArm')?.checked?1:0}`);
    await cmd(`set fpv_prearm_text @${tempTarget}:${tempOn?tempText:''}`);
    await cmd(`set fpv_prearm_show ${tempMs}`);
    await cmd('set fpv_prearm_int 3000');

    await cmd(`set fpv_low_batt ${warnOn?1:0}`);
    await cmd(`set fpv_low_pct ${lowPct}`);
    await cmd('set fpv_low_rdyflash 1');
    await cmd('set fpv_low_rectext 1');
    await cmd(`set fpv_low_text @${warnTarget}:BATT LOW`);
    await cmd(`set fpv_rect_warn ${warnOn?1:0}`);
    await cmd(`set fpv_rect_min ${lowMins}`);
    await cmd('set fpv_rect_ready 1');
    await cmd('set fpv_rect_record 1');
    await cmd('set fpv_rect_text REC LOW');
    await cmd(`set fpv_hot_warn ${warnOn?1:0}`);
    await cmd('set fpv_hot_ready 1');
    await cmd('set fpv_hot_record 1');
    await cmd('set fpv_hot_text CAM HOT');

    document.dispatchEvent(new CustomEvent('fps-settings-applied'));
    setStatus('Applied to C3 ✓');
    setTimeout(()=>setStatus(''),1600);
  }catch(e){
    setStatus('Apply failed — '+e.message,true);
  }
}

function setSelectTarget(id,target){
  const s=$(id); if(!s)return;
  const value=legacyMode()?(target===2?'craft':'pilot'):String(target);
  if([...s.options].some(o=>o.value===value)){s.value=value;s.dispatchEvent(new Event('change',{bubbles:true}))}
}

function syncMetadataFromDevice(){
  syncing=true;
  try{
    const bf=$('bf45Compat');
    if($('fpsBfMode')&&bf){$('fpsBfMode').value=bf.checked?'legacy':'current';fire('fpsBfMode')}
    if($('fpsPilotMaster')&&$('pilotEn')){$('fpsPilotMaster').checked=$('pilotEn').checked;fire('fpsPilotMaster')}
    if($('fpsCraftMaster')&&$('craftEn')){$('fpsCraftMaster').checked=$('craftEn').checked;fire('fpsCraftMaster')}

    // Do not infer the OSD master from templates/warnings. The board already
    // persists fpv_state_mode, so Read Settings must reflect that exact value.
    const osdOn=!!$('fpvStateMode')?.checked;
    if($('fpsOsdMaster')){$('fpsOsdMaster').checked=osdOn;fire('fpsOsdMaster')}

    for(let n=1;n<=4;n++){
      const input=$('osd'+n),master=$('fpsMsg'+n); if(!input||!master)continue;
      const wire=String(input.value||'').trim();
      const off=!wire||wire==='{off}';
      master.checked=!off;
      if(off)input.value=DEFAULT_TPL[n];
      input.dispatchEvent(new Event('input',{bubbles:true}));
      master.dispatchEvent(new Event('change',{bubbles:true}));
    }

    const recordMeta=String($('fpvRecordText')?.value||'');
    if($('fpsRecOnly')){$('fpsRecOnly').checked=recordMeta.startsWith('@ARM:');fire('fpsRecOnly')}

    const p=stripTarget($('fpvPreArmText')?.value);
    if($('fpvPreArmText')){$('fpvPreArmText').value=p.text||'CLEAN LENS';fire('fpvPreArmText','input')}
    if($('fpsTempMaster')){$('fpsTempMaster').checked=!!p.text;fire('fpsTempMaster')}
    setSelectTarget('fpsTempDest',p.target);
    if($('fpvCustomDurationSec')&&$('fpvPreArmShow')){
      const ms=Math.max(100,parseInt($('fpvPreArmShow').value,10)||1000);
      $('fpvCustomDurationSec').value=(ms/1000).toFixed(ms%1000===0?0:1);
      fire('fpvCustomDurationSec','input');
    }

    const w=stripTarget($('fpvLowText')?.value);
    if($('fpsWarnMaster')&&$('fpvLowBatt')){$('fpsWarnMaster').checked=$('fpvLowBatt').checked;fire('fpsWarnMaster')}
    setSelectTarget('fpsWarnDest',w.target);

    if($('fpsAuxMaster')&&$('auxChannel')){
      $('fpsAuxMaster').checked=parseInt($('auxChannel').value,10)>0;
      fire('fpsAuxMaster');
    }

    fire('fpvFlash'); fire('fpvPreArm'); fire('fpvLowPct','input'); fire('fpvRecLowMin','input');
    setStatus('Read from C3 ✓');
    document.dispatchEvent(new CustomEvent('fps-config-read-complete'));
    setTimeout(()=>setStatus(''),1200);
  }finally{syncing=false}
}

function scheduleReadSync(){
  clearTimeout(readTimer);
  readTimer=setTimeout(syncMetadataFromDevice,180);
}

function installReadCompletionHook(){
  // The native parser is the canonical board -> native-control path. Wrap it
  // only to know when the stream of [cfg] lines has gone quiet, then translate
  // those already-parsed values into the friendly FPSteVe controls.
  if(typeof parseConfigLine!=='function'||parseConfigLine.__fpsWrapped)return;
  const original=parseConfigLine;
  const wrapped=function(line){
    original(line);
    if(/^\[cfg\]\s+\w+\s*=/.test(String(line||''))) scheduleReadSync();
  };
  wrapped.__fpsWrapped=true;
  parseConfigLine=wrapped;
}

function init(){
  const apply=$('fpsApplyAll');
  if(apply) apply.onclick=applyParity;
  installReadCompletionHook();

  document.addEventListener('change',e=>{
    if(e.target?.id==='fpsCraftMaster'&&e.target.checked){
      const tpl=$('craftTpl');
      if(tpl&&(!String(tpl.value||'').trim()||String(tpl.value).trim()==='{off}')){
        tpl.value='{res} {fps}';
        tpl.dispatchEvent(new Event('input',{bubbles:true}));
      }
    }
  });

  // Fallback for browsers where the global parser cannot be wrapped.
  $('readBtn')?.addEventListener('click',()=>setTimeout(()=>{
    if(connected())syncMetadataFromDevice();
  },1200));
}

if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,450));
else setTimeout(init,450);
})();
