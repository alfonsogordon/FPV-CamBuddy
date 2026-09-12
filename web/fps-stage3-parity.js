(()=>{
'use strict';
const $=id=>document.getElementById(id);
const DEMO='freeclinkerDemoMode';

function legacyMode(){return $('fpsBfMode')?.value==='legacy'}
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

async function applyParity(){
  // Demo owns no hardware state; keep the existing Demo save behaviour.
  if(localStorage.getItem(DEMO)==='1'){
    setStatus('Saved in Demo ✓');
    setTimeout(()=>setStatus(''),1200);
    return;
  }
  if($('connectBtn')?.disabled){setStatus('Connect a C3 first',true);return}
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
    // Current-BF destinations are disabled by writing a blank template. This
    // also clears a previously visible Custom Message immediately on the FC.
    for(let n=1;n<=4;n++){
      const enabled=!!$('fpsMsg'+n)?.checked;
      const tpl=enabled?String($('osd'+n)?.value||''):'';
      await cmd(`set osd${n} ${tpl}`);
    }

    await cmd(`set bf45_compat ${legacy?1:0}`);
    await cmd(`set pilot_en ${pilotOn?1:0}`);
    await cmd(`set pilot_tpl ${String($('pilotTpl')?.value||'')}`);
    await cmd(`set craft_en ${craftOn?1:0}`);
    await cmd(`set craft_tpl ${String($('craftTpl')?.value||'')}`);

    // The existing persisted recording-state template carries one tiny metadata
    // prefix for REC-only. The firmware strips it before rendering; this keeps
    // backward-compatible NVS/CLI storage without another migration-only field.
    await cmd('set fpv_state_mode 1');
    await cmd('set fpv_error 1');
    await cmd('set fpv_err_text {state}');
    await cmd('set fpv_ready 1');
    await cmd('set fpv_ready_text {state}');
    await cmd('set fpv_record 1');
    await cmd(`set fpv_record_text ${recOnly?'@ARM:{state}':'{state}'}`);
    await cmd(`set fpv_flash ${flash?1:0}`);

    // Target metadata uses @N: on the existing persisted text field. In legacy
    // mode N=1 means Pilot Name and N=2 means Craft Name; in current mode it is
    // Custom Message 1-4. The renderer removes the prefix before display.
    await cmd(`set fpv_prearm ${tempOn&&$('fpvPreArm')?.checked?1:0}`);
    await cmd(`set fpv_prearm_text ${tempOn?`@${tempTarget}:${tempText}`:''}`);
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
  const recordMeta=String($('fpvRecordText')?.value||'');
  const recOnly=$('fpsRecOnly');
  if(recOnly){recOnly.checked=recordMeta.startsWith('@ARM:');recOnly.dispatchEvent(new Event('change',{bubbles:true}))}

  const p=stripTarget($('fpvPreArmText')?.value);
  if($('fpvPreArmText')){
    $('fpvPreArmText').value=p.text;
    $('fpvPreArmText').dispatchEvent(new Event('input',{bubbles:true}));
  }
  setSelectTarget('fpsTempDest',p.target);

  const w=stripTarget($('fpvLowText')?.value);
  setSelectTarget('fpsWarnDest',w.target);
}

function init(){
  const apply=$('fpsApplyAll');
  if(apply) apply.onclick=applyParity;

  // Requested legacy default: when Craft Name is first enabled, use Resolution
  // + FPS (e.g. 4K 60). Existing user choices are never overwritten.
  document.addEventListener('change',e=>{
    if(e.target?.id==='fpsCraftMaster'&&e.target.checked){
      const tpl=$('craftTpl');
      if(tpl&&!String(tpl.value||'').trim()){
        tpl.value='{res} {fps}';
        tpl.dispatchEvent(new Event('input',{bubbles:true}));
      }
    }
  });

  // `show` writes the raw persisted metadata into the old native controls.
  // Convert it back to the clean Stage-3 UI representation after a device read.
  $('readBtn')?.addEventListener('click',()=>setTimeout(syncMetadataFromDevice,700));
}

if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,450));
else setTimeout(init,450);
})();
