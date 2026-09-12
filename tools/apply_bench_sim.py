from pathlib import Path

p=Path('src/msp_serial.h')
s=p.read_text()
needle='    bool isArmed() const { return _armed; }\n'
insert='''    bool isArmed() const { return _armed; }\n    // USB bench simulation uses the same state/callback path as MSP responses.\n    // RAM-only: reboot clears it and it can never arm a real flight controller.\n    void simulateArmState(bool armed) {\n        if (armed) _hasArmedSinceBoot = true;\n        if (_armed == armed) return;\n        _armed = armed;\n        if (_armCb) _armCb(armed);\n    }\n    void simulateAuxSwitch(bool high) {\n        if (_auxHigh == high) return;\n        _auxHigh = high;\n        if (_auxSwitchCb) _auxSwitchCb(high);\n    }\n'''
assert needle in s
p.write_text(s.replace(needle,insert,1))

p=Path('src/config_manager.cpp')
s=p.read_text()
needle='static constexpr const char *KEY_FPV_PREARM_INT = "fpv_prearm_int";\n'
insert=needle+'''\n// Implemented in main.cpp. These bench hooks feed the normal MSPSerial state\n// handlers; they do not transmit an ARM command to the FC.\nextern void benchSimArm(bool armed);\nextern void benchSimAux(bool high);\nextern void benchSimOff();\nextern void benchSimStatus(Stream &out);\n'''
assert needle in s
s=s.replace(needle,insert,1)
needle='''    if (strcmp(line, "record start") == 0 || strcmp(line, "record stop") == 0) {\n'''
insert='''    if (strcmp(line, "sim arm 1") == 0) { benchSimArm(true); out.println("[sim] armed=1"); return; }\n    if (strcmp(line, "sim arm 0") == 0) { benchSimArm(false); out.println("[sim] armed=0"); return; }\n    if (strcmp(line, "sim aux high") == 0) { benchSimAux(true); out.println("[sim] aux=high"); return; }\n    if (strcmp(line, "sim aux low") == 0) { benchSimAux(false); out.println("[sim] aux=low"); return; }\n    if (strcmp(line, "sim off") == 0) { benchSimOff(); out.println("[sim] off"); return; }\n    if (strcmp(line, "sim status") == 0) { benchSimStatus(out); return; }\n\n'''+needle
assert needle in s
s=s.replace(needle,insert,1)
needle='''        out.println("  record stop                - stop camera recording now");\n'''
insert=needle+'''        out.println("  sim arm <0|1>              - bench-test FC arm state through normal camera handler");\n        out.println("  sim aux <low|high>         - bench-test configured AUX camera action");\n        out.println("  sim status                 - show RAM-only bench simulation state");\n        out.println("  sim off                    - disarm and leave bench simulation");\n'''
assert needle in s
p.write_text(s.replace(needle,insert,1))

p=Path('src/main.cpp')
s=p.read_text()
needle='''static bool     pendingStop = false;\nstatic uint32_t disarmMs   = 0;\n'''
insert=needle+'''\nstatic bool benchSimActive = false;\nstatic bool benchSimAuxHigh = false;\n'''
assert needle in s
s=s.replace(needle,insert,1)
needle='''void setup() {\n'''
insert='''void benchSimArm(bool armed) {\n    benchSimActive = true;\n    DBG_SERIAL.printf("[sim] FC %s (USB bench only)\\n", armed ? "ARM" : "DISARM");\n    mspSerial.simulateArmState(armed);\n}\n\nvoid benchSimAux(bool high) {\n    benchSimActive = true;\n    benchSimAuxHigh = high;\n    DBG_SERIAL.printf("[sim] AUX %s (USB bench only)\\n", high ? "HIGH" : "LOW");\n    mspSerial.simulateAuxSwitch(high);\n}\n\nvoid benchSimOff() {\n    if (mspSerial.isArmed()) mspSerial.simulateArmState(false);\n    if (benchSimAuxHigh) mspSerial.simulateAuxSwitch(false);\n    benchSimAuxHigh = false;\n    benchSimActive = false;\n}\n\nvoid benchSimStatus(Stream &out) {\n    out.printf("[sim] active=%s armed=%s aux=%s camera_connected=%s recording=%s\\n",\n               benchSimActive ? "yes" : "no", mspSerial.isArmed() ? "yes" : "no",\n               benchSimAuxHigh ? "high" : "low",\n               (activeCamera && activeCamera->isConnected()) ? "yes" : "no",\n               currentCamera.recording ? "yes" : "no");\n}\n\n'''+needle
assert needle in s
p.write_text(s.replace(needle,insert,1))

p=Path('web/test.html')
s=p.read_text()
s=s.replace('background:linear-gradient(115deg,#2a1c3b,#14101f 60%,#05080d)','background:linear-gradient(115deg,#241535,#100b18 60%,#05080d)')
s=s.replace("await send('record start')","await send('sim arm 1')",1)
s=s.replace("await send('record stop');await send('status')","await send('sim arm 0');await send('status')",1)
s=s.replace("if(live)timer=setTimeout(async()=>{await send('sim arm 0');await send('status')},d);else rec=false","if(live){await send('sim arm 0')}else rec=false")
s=s.replace('Development bench preview. Exact supplied DVR image and firmware simulation controls are being added on this branch.','USB bench preview. ARM/DISARM now feeds the same firmware handler used by Betaflight MSP; it cannot arm the flight controller or motors.')
p.write_text(s)
print('bench simulation patch applied')
