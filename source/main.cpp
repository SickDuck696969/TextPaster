#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include "config.hpp"
#include "texts.hpp"
static Result launchServiceResult=0;

class Shortcuts : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override {
        auto* frame=new tsl::elm::OverlayFrame("Shortcut","TextPaster");auto* list=new tsl::elm::List;
        for(const char* name:{"L+ZR","ZL+R","L+R","L+ZL","R+ZR","ZL+ZR"}){
            auto* item=new tsl::elm::ListItem(name);
            item->setClickListener([item,name](u64 keys){
                if(!(keys&HidNpadButton_A))return false;
                auto config=tp::read();
                if(!config.error.empty()){item->setValue("Config error");return true;}
                config.combo=name;
                if(tp::write(config)){tsl::goBack();}
                else item->setValue("Save failed");
                return true;
            });list->addItem(item);
        }
        frame->setContent(list);return frame;
    }
};
class Gui : public tsl::Gui {
    tsl::elm::ListItem *state=nullptr,*combo=nullptr,*texts=nullptr;
    u64 ticks=0,feedbackUntil=0;
    void feedback(const std::string& text){state->setValue(text);feedbackUntil=armGetSystemTick()+armGetSystemTickFreq()*3;}
public:
    tsl::elm::Element* createUI() override {
        auto* frame=new tsl::elm::OverlayFrame("TextPaster","1.1.1 | SickDuck");auto* list=new tsl::elm::List;
        state=new tsl::elm::ListItem("Enabled",tp::running()?"On":"Off");
        state->setClickListener([this](u64 keys){
            if(!(keys&HidNpadButton_A))return false;
            if(tp::running()){
                tp::dirs();std::ofstream f(tp::StopPath);f<<"stop";f.close();feedback(f?"Stopping...":"Could not stop");
            }else{
                if(R_FAILED(launchServiceResult)){feedback("pm:shell unavailable");return true;}
                auto config=tp::read();if(!config.error.empty()){feedback(config.error);return true;}
                remove(tp::StopPath);NcmProgramLocation loc{tp::ProgramId,NcmStorageId_None};u64 pid=0;
                Result rc=pmshellLaunchProgram(0,&loc,&pid);char msg[64];
                snprintf(msg,sizeof(msg),R_SUCCEEDED(rc)?"Starting...":"Start failed: 0x%08X",rc);feedback(msg);
            }return true;
        });list->addItem(state);
        auto initial=tp::read();auto* active=tp::entry(initial,initial.selected);
        texts=new tsl::elm::ListItem("Saved texts",active?active->name:"None");
        texts->setClickListener([](u64 keys){if(!(keys&HidNpadButton_A))return false;tsl::changeTo<SavedTexts>();return true;});list->addItem(texts);
        combo=new tsl::elm::ListItem("Shortcut",tp::read().combo);
        combo->setClickListener([](u64 keys){if(!(keys&HidNpadButton_A))return false;tsl::changeTo<Shortcuts>();return true;});list->addItem(combo);
        frame->setContent(list);return frame;
    }
    void update() override {
        if(!state)return;
        u64 now=armGetSystemTick();
        if(now-ticks>armGetSystemTickFreq()/2){
            ticks=now;
            if(now>=feedbackUntil)state->setValue(tp::running()?"On":"Off");
            auto current=tp::read();combo->setValue(current.combo);auto* active=tp::entry(current,current.selected);texts->setValue(active?active->name:"None");
        }
    }
};
class TextPasterOverlay : public tsl::Overlay {
    void initServices() override {
        tsl::hlp::doWithSmSession([]{launchServiceResult=pmshellInitialize();});
        tp::dirs();bool safeToCreate=tp::recoverAndClean();
        struct stat st{};if(safeToCreate&&stat(tp::ConfigPath,&st)!=0)tp::write(tp::Config{});
        // Shorten our old presets; preserve user-defined shortcuts and text.
        auto config=tp::read();
        if(config.error.empty()&&config.legacy)tp::write(config);
        if(config.error.empty()&&(config.combo=="ZL+ZR+R3"||config.combo=="L+R+X"||config.combo=="ZL+ZR+Y")){
            config.combo="L+ZR";tp::write(config);
        }
    }
    void onShow() override {tp::dirs();std::ofstream flag(tp::OverlayPath);flag<<"visible";}
    void onHide() override {remove(tp::OverlayPath);tp::cleanRuntime();}
    void exitServices() override {remove(tp::OverlayPath);tp::cleanRuntime();if(R_SUCCEEDED(launchServiceResult))pmshellExit();}
    std::unique_ptr<tsl::Gui> loadInitialGui() override {return initially<Gui>();}
};
int main(int argc,char** argv){return tsl::loop<TextPasterOverlay>(argc,argv);}
