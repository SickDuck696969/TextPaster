#include "config.hpp"
#include <unistd.h>
extern "C" {
u32 __nx_applet_type=AppletType_None;
u32 __nx_fs_num_sessions=1;
void __libnx_initheap(){static u8 heap[0x80000];extern void* fake_heap_start;extern void* fake_heap_end;fake_heap_start=heap;fake_heap_end=heap+sizeof(heap);}
void __appInit(){smInitialize();if(R_SUCCEEDED(setsysInitialize())){SetSysFirmwareVersion v{};if(R_SUCCEEDED(setsysGetFirmwareVersion(&v)))hosversionSet(MAKEHOSVERSION(v.major,v.minor,v.micro));setsysExit();}Result r=fsInitialize();if(R_FAILED(r))diagAbortWithResult(r);fsdevMountSdmc();}
void __appExit(){fsdevUnmountAll();fsExit();smExit();}
}
static bool stopping(){return access(tp::StopPath,F_OK)==0;}
static bool keyboard(u64 expected=0){u64 pid=0;return R_SUCCEEDED(pmdmntGetProcessId(&pid,tp::KeyboardId))&&(!expected||pid==expected);}
// padUpdate only reads the ordinary handheld ring and ignores the Other ID.
// System applets can expose input through these generic controller rings.
static u64 systemButtons(bool& connected){
    u64 held=0;
    for(auto id:{HidNpadIdType_Handheld,HidNpadIdType_Other}){
        HidNpadSystemExtState state{};
        if(hidGetNpadStatesSystemExt(id,&state,1) && (state.attributes&HidNpadAttribute_IsConnected)){
            held|=state.buttons;connected=true;
        }
    }
    return held;
}
int main(){
    tp::dirs();Result r=hidInitialize();if(R_FAILED(r)){return 1;}
    r=pmdmntInitialize();if(R_FAILED(r)){hidExit();return 1;}
    r=hiddbgInitialize();if(R_FAILED(r)){pmdmntExit();hidExit();return 1;}
    padConfigureInput(8,HidNpadStyleSet_NpadStandard | HidNpadStyleTag_NpadSystemExt);
    PadState pad;padInitializeAny(&pad);
    tp::Config c=tp::read();
    u64 lastReload=armGetSystemTick();bool latched=false;
    while(!stopping()){
        u64 now=armGetSystemTick();
        if(now-lastReload>armGetSystemTickFreq()*2){c=tp::read();lastReload=now;}
        padUpdate(&pad);bool connected=pad.active_id_mask||pad.active_handheld;
        u64 held=padGetButtons(&pad)|systemButtons(connected);
        bool match=c.error.empty()&&(held&c.mask)==c.mask;
        // Do not type into the underlying app while overlay controls are in use.
        if(access(tp::OverlayPath,F_OK)==0){latched=true;svcSleepThread(50000000);continue;}
        // Fire on the first poll seeing the complete combo, once per press.
        // Breaking the combo rearms it; holding it never repeats the text.
        if(!match)latched=false;
        if(match&&!latched){
            latched=true;
            tp::Config fresh=tp::read();u64 pid=0;
            if(fresh.error.empty()&&!fresh.text.empty()&&R_SUCCEEDED(pmdmntGetProcessId(&pid,tp::KeyboardId))){
                bool success=true;r=0;
                // Validate the entire string before injecting any key; never send Enter/submit.
                for(unsigned char ch:fresh.text)if(!tp::key(ch).code)success=false;
                if(success)for(unsigned char ch:fresh.text){
                    if(stopping()||access(tp::OverlayPath,F_OK)==0||!keyboard(pid)){success=false;break;}
                    auto k=tp::key(ch);HiddbgKeyboardAutoPilotState state{};state.keys[k.code/64]|=1ULL<<(k.code%64);if(k.shift){state.modifiers=HidKeyboardModifier_Shift;state.keys[225/64]|=1ULL<<(225%64);}
                    r=hiddbgSetKeyboardAutoPilotState(&state);if(R_FAILED(r)){success=false;break;}
                    // sys-botbase uses this unused key bit for a neutral report;
                    // a zero report can leave repeated keys without a release edge.
                    svcSleepThread(fresh.delay*1000000LL);state={};state.keys[3]=1ULL<<59;r=hiddbgSetKeyboardAutoPilotState(&state);if(R_FAILED(r)){success=false;break;}svcSleepThread(fresh.delay*1000000LL);
                }
                hiddbgUnsetKeyboardAutoPilotState();
            }
        }
        svcSleepThread(50000000);
    }
    hiddbgUnsetKeyboardAutoPilotState();hiddbgExit();pmdmntExit();hidExit();remove(tp::StopPath);tp::cleanRuntime();return 0;
}
