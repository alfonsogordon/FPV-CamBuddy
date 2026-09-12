#pragma once
#include <Arduino.h>
#include "telemetry.h"
#define MSP_STATUS 101
#define MSP_RC 105
#define MSP2_SET_TEXT 0x3007
#define MSP_TEXT_PILOT_NAME 1
#define MSP_TEXT_CRAFT_NAME 2
#define MSP_TEXT_CUSTOM_1 7
#define MSP_TEXT_CUSTOM_2 8
#define MSP_TEXT_CUSTOM_3 9
#define MSP_TEXT_CUSTOM_4 10
#define MSP2_CAMERA_BATTERY 0x3001
struct __attribute__((packed)) MSP2CameraPayload{uint8_t percent,camera_mode,recording,temp_over,eis_mode;uint16_t record_time;uint32_t remain_cap_mb,remain_time;};
static_assert(sizeof(MSP2CameraPayload)==15,"Camera payload size mismatch");
using ArmCallback=void(*)(bool);using AuxSwitchCallback=void(*)(bool);
class MSPSerial{
public:
 void begin(HardwareSerial&);void update();void sendCameraStatus(const CameraData&);void sendCustomOSD1(const CameraData&,const char*);void sendCustomOSD2(const CameraData&,const char*);void sendCustomOSD3(const CameraData&,const char*);void sendCustomOSD4(const CameraData&,const char*);void sendCustomOSDWarnings(uint8_t,const CameraData&,const char*);uint8_t warningTarget()const{return _fpvWarningTarget;}void sendPilotName(const CameraData&,const char*);void sendCraftName(const CameraData&,const char*);
 void setFpvDisplayOptions(bool sm,bool ee,const char*et,bool re,const char*rt,bool rce,const char*rct,bool fr,bool lbe,uint8_t lbp,bool lbr,bool lbrec,const char*lbt,bool lre,uint16_t lrm,bool lrr,bool lrrec,const char*lrt,bool he,bool hr,bool hrec,const char*ht,bool pae,const char*pat,uint16_t pas,uint16_t pai){_fpvStateMode=sm;_fpvErrorEnabled=ee;strlcpy(_fpvErrorText,et?et:"",sizeof(_fpvErrorText));_fpvReadyEnabled=re;strlcpy(_fpvReadyText,rt?rt:"",sizeof(_fpvReadyText));_fpvRecordingEnabled=rce;strlcpy(_fpvRecordingText,rct?rct:"",sizeof(_fpvRecordingText));_fpvRecFlash=fr;_fpvLowBatteryEnabled=lbe;_fpvLowBatteryPct=lbp>100?100:lbp;_fpvLowBatteryReadyFlash=lbr;_fpvLowBatteryRecText=lbrec;const char*clean=lbt?lbt:"";_fpvWarningTarget=1;if(clean[0]=='@'&&clean[1]>='1'&&clean[1]<='4'&&clean[2]==':'){_fpvWarningTarget=(uint8_t)(clean[1]-'0');clean+=3;}strlcpy(_fpvLowBatteryText,clean,sizeof(_fpvLowBatteryText));_fpvLowRecEnabled=lre;_fpvLowRecMinutes=lrm;_fpvLowRecReady=lrr;_fpvLowRecRecording=lrrec;strlcpy(_fpvLowRecText,lrt?lrt:"",sizeof(_fpvLowRecText));_fpvHotEnabled=he;_fpvHotReady=hr;_fpvHotRecording=hrec;strlcpy(_fpvHotText,ht?ht:"",sizeof(_fpvHotText));_fpvPreArmEnabled=pae;strlcpy(_fpvPreArmText,pat?pat:"",sizeof(_fpvPreArmText));_fpvPreArmShowMs=pas<100?100:pas;_fpvPreArmIntervalMs=pai<_fpvPreArmShowMs?_fpvPreArmShowMs:pai;}
 bool isArmed()const{return _armed;}void simulateArmState(bool a){if(a)_hasArmedSinceBoot=true;if(_armed==a)return;_armed=a;if(_armCb)_armCb(a);}void simulateAuxSwitch(bool h){if(_auxHigh==h)return;_auxHigh=h;if(_auxSwitchCb)_auxSwitchCb(h);}void setArmCallback(ArmCallback c){_armCb=c;}void setAuxChannel(uint8_t);void setAuxSwitchCallback(AuxSwitchCallback c){_auxSwitchCb=c;}
private:
 void sendFrame(uint16_t,const uint8_t*,uint16_t,char='>');void sendRequest(uint16_t);void sendCustomText(uint8_t,const char*);void sendCustomOSD(uint8_t,const CameraData&,const char*,const char*=nullptr);void feedByte(uint8_t);void processResponse();void handleStatusResponse();void handleRcResponse();enum class RxState:uint8_t{IDLE,HDR_X,HDR_DIR,FLAG,CMD_LO,CMD_HI,SZ_LO,SZ_HI,PAYLOAD,CRC};static constexpr uint8_t RX_BUF_SIZE=32;RxState _rxState=RxState::IDLE;uint8_t _rxBuf[RX_BUF_SIZE]{};uint16_t _rxCmd=0,_rxSize=0,_rxPos=0;uint8_t _rxCrc=0;static uint8_t crc8DvbS2(uint8_t,uint8_t);static uint8_t crc8DvbS2Buf(uint8_t,const uint8_t*,uint16_t);HardwareSerial*_serial=nullptr;bool _armed=false;ArmCallback _armCb=nullptr;uint32_t _lastPollMs=0;uint8_t _auxChannel=0;bool _auxHigh=false;AuxSwitchCallback _auxSwitchCb=nullptr;
 bool _fpvStateMode=true,_fpvErrorEnabled=true;char _fpvErrorText[32]="{state}";bool _fpvReadyEnabled=true;char _fpvReadyText[32]="{state} B:{batn} T:{rect}";bool _fpvRecordingEnabled=true;char _fpvRecordingText[32]="{state}";bool _fpvRecFlash=true,_fpvLowBatteryEnabled=true;uint8_t _fpvLowBatteryPct=10;bool _fpvLowBatteryReadyFlash=true,_fpvLowBatteryRecText=true;char _fpvLowBatteryText[32]="BATT LOW";uint8_t _fpvWarningTarget=1;bool _fpvLowRecEnabled=true;uint16_t _fpvLowRecMinutes=5;bool _fpvLowRecReady=true,_fpvLowRecRecording=true;char _fpvLowRecText[32]="REC LOW";bool _fpvHotEnabled=true,_fpvHotReady=true,_fpvHotRecording=true;char _fpvHotText[32]="CAM HOT";bool _fpvPreArmEnabled=true;char _fpvPreArmText[32]="";uint16_t _fpvPreArmShowMs=1000,_fpvPreArmIntervalMs=3000;bool _hasArmedSinceBoot=false;
};
