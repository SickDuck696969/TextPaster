#pragma once
#include <switch.h>
#include <string>
#include <fstream>
#include <algorithm>
#include <vector>
#include <sstream>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
namespace tp {
constexpr u64 ProgramId=0x4200000000545053ULL;
constexpr u64 KeyboardId=0x0100000000001008ULL;
constexpr const char* ConfigPath="sdmc:/config/TextPaster/config.ini";
constexpr const char* RuntimeDir="sdmc:/switch/.overlays/TextPaster.runtime";
constexpr const char* OverlayPath="sdmc:/switch/.overlays/TextPaster.runtime/overlay.flag";
constexpr const char* StopPath="sdmc:/switch/.overlays/TextPaster.runtime/stop.flag";
struct Button { const char* name; u64 bit; };
inline constexpr Button buttons[]={{"L",HidNpadButton_L},{"R",HidNpadButton_R},{"ZL",HidNpadButton_ZL},{"ZR",HidNpadButton_ZR},{"A",HidNpadButton_A},{"B",HidNpadButton_B},{"X",HidNpadButton_X},{"Y",HidNpadButton_Y},{"PLUS",HidNpadButton_Plus},{"MINUS",HidNpadButton_Minus},{"UP",HidNpadButton_Up},{"DOWN",HidNpadButton_Down},{"LEFT",HidNpadButton_Left},{"RIGHT",HidNpadButton_Right},{"L3",HidNpadButton_StickL},{"R3",HidNpadButton_StickR}};
struct TextEntry { unsigned id=1; std::string name="Text 1",value="Hello from TextPaster"; };
struct Config { std::string combo="L+ZR",text="Hello from TextPaster",error; u64 mask=0; int delay=40; unsigned selected=1; bool legacy=false; std::vector<TextEntry> entries{TextEntry{}}; };
inline TextEntry* entry(Config& c,unsigned id){for(auto& e:c.entries)if(e.id==id)return &e;return nullptr;}
inline bool printable(const std::string& s,size_t max){if(s.empty()||s.size()>max)return false;for(unsigned char ch:s)if(ch<32||ch>126)return false;return true;}
inline bool number(const std::string& s,unsigned& n){if(s.empty()||s.size()>9||s.find_first_not_of("0123456789")!=std::string::npos)return false;n=static_cast<unsigned>(strtoul(s.c_str(),nullptr,10));return true;}
inline unsigned nextId(const Config& c){for(unsigned n=1;;++n){bool found=false;for(auto& e:c.entries)if(e.id==n)found=true;if(!found)return n;}}

inline u64 comboMask(const std::string& value){u64 result=0;size_t start=0;int count=0;while(start<value.size()){auto end=value.find('+',start);auto key=value.substr(start,end==std::string::npos?end:end-start);bool found=false;for(auto b:buttons)if(key==b.name && !(result&b.bit)){result|=b.bit;found=true;++count;break;}if(!found)return 0;if(end==std::string::npos)break;start=end+1;if(start==value.size())return 0;}return count>=2?result:0;}
inline void dirs(){mkdir("sdmc:/config",0777);mkdir("sdmc:/config/TextPaster",0777);mkdir("sdmc:/switch",0777);mkdir("sdmc:/switch/.overlays",0777);mkdir(RuntimeDir,0777);}
inline void cleanRuntime(){
#ifdef _WIN32
    _rmdir(RuntimeDir);
#else
    rmdir(RuntimeDir);
#endif
}
inline void validate(Config& c){
    c.error.clear();c.mask=comboMask(c.combo);c.text.clear();
    if(!c.mask)c.error="Invalid shortcut";
    else if(c.delay<20||c.delay>200)c.error="delay_ms must be 20..200";
    else if(c.entries.size()>32)c.error="Maximum 32 texts";
    for(size_t i=0;i<c.entries.size()&&c.error.empty();++i){
        auto& e=c.entries[i];
        if(!e.id||!printable(e.name,48)||!printable(e.value,512))c.error="Invalid saved text or name";
        for(size_t j=0;j<i;++j)if(c.entries[j].id==e.id)c.error="Duplicate text ID";
    }
    if(!c.error.empty())return;
    if(c.entries.empty()){if(c.selected)c.error="Invalid selected text";return;}
    auto* active=entry(c,c.selected);
    if(!active)c.error="Selected text does not exist";else c.text=active->value;
}
inline Config parse(std::istream& f){
    Config c;c.entries.clear();c.selected=0;
    std::string line,legacyText;bool legacySeen=false,version=false,selection=false,first=true;
    unsigned section=0;size_t total=0;
    while(std::getline(f,line)){
        total+=line.size()+1;if(total>32768){c.error="Config exceeds 32 KiB";return c;}
        if(first&&line.compare(0,3,"\xEF\xBB\xBF")==0)line.erase(0,3);
        first=false;
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty()||line[0]=='#'||line[0]==';')continue;
        if(line[0]=='['){
            unsigned id=0;
            if(line.size()<8||line.rfind("[text:",0)!=0||line.back()!=']'||!number(line.substr(6,line.size()-7),id)||!id||entry(c,id)||c.entries.size()>=32){c.error="Invalid text section";return c;}
            c.entries.push_back({id,"",""});section=id;continue;
        }
        auto equal=line.find('=');if(equal==std::string::npos){c.error="Invalid config line";return c;}
        auto key=line.substr(0,equal),value=line.substr(equal+1);
        if(section){auto* e=entry(c,section);if(key=="name")e->name=value;else if(key=="text")e->value=value;else {c.error="Unknown text setting";return c;}}
        else if(key=="version"){if(value!="2"){c.error="Unsupported config version";return c;}version=true;}
        else if(key=="combo")c.combo=value;
        else if(key=="delay_ms"){unsigned n=0;if(!number(value,n)||n>200){c.error="Invalid delay_ms";return c;}c.delay=n;}
        else if(key=="selected"){if(!number(value,c.selected)){c.error="Invalid selection";return c;}selection=true;}
        else if(key=="text"){legacyText=value;legacySeen=true;}
        else {c.error="Unknown config setting";return c;}
    }
    if(!f.eof()){c.error="Cannot read config";return c;}
    if(!version){
        if(!c.entries.empty()||selection){c.error="Missing version=2";return c;}
        c.legacy=true;c.entries.push_back({1,"Text 1",legacySeen?legacyText:"Hello from TextPaster"});c.selected=1;
    }else if(legacySeen||!selection){c.error="Invalid version 2 config";return c;}
    validate(c);return c;
}
inline Config read(){
    Config c;struct stat size{};if(stat(ConfigPath,&size)==0&&size.st_size>32768){c.error="Config exceeds 32 KiB";return c;}
    std::ifstream f(ConfigPath);if(!f){c.error="Cannot read config.ini";return c;}return parse(f);
}
inline void serialize(std::ostream& f,const Config& c){
    f<<"version=2\ncombo="<<c.combo<<"\ndelay_ms="<<c.delay<<"\nselected="<<c.selected<<"\n";
    for(auto& e:c.entries)f<<"\n[text:"<<e.id<<"]\nname="<<e.name<<"\ntext="<<e.value<<"\n";
}
inline bool write(const Config& input){
    Config c=input;validate(c);if(!c.error.empty())return false;
    dirs();std::string tmp=std::string(RuntimeDir)+"/config.tmp",bak=std::string(RuntimeDir)+"/config.bak";
    std::ofstream f(tmp);serialize(f,c);
    f.close();if(!f){remove(tmp.c_str());return false;}
    struct stat st{};bool old=stat(ConfigPath,&st)==0;
    if(old){remove(bak.c_str());if(rename(ConfigPath,bak.c_str())){remove(tmp.c_str());return false;}}
    if(rename(tmp.c_str(),ConfigPath)){if(old)rename(bak.c_str(),ConfigPath);remove(tmp.c_str());cleanRuntime();return false;}
    remove(bak.c_str());cleanRuntime();return true;
}
// Recover an interrupted save before creating defaults. Only remove known old
// artifacts after a valid config is available; never discard the sole saved copy.
inline bool recoverAndClean(){
    struct stat st{};
    const char* copies[]={"sdmc:/switch/.overlays/TextPaster.runtime/config.bak","sdmc:/config/TextPaster/config.ini.bak","sdmc:/switch/.overlays/TextPaster.runtime/config.tmp","sdmc:/config/TextPaster/config.ini.tmp"};
    if(stat(ConfigPath,&st)!=0){
        for(auto path:copies){std::ifstream f(path);if(!f)continue;auto c=parse(f);f.close();if(c.error.empty()&&rename(path,ConfigPath)==0)break;}
    }
    if(read().error.empty())for(auto path:copies)remove(path);
    for(auto name:{"status.txt","diagnostics.txt","overlay.log","stop.flag","overlay.flag"}){
        std::string path="sdmc:/config/TextPaster/";path+=name;remove(path.c_str());
    }
    cleanRuntime();
    if(stat(ConfigPath,&st)!=0)for(auto path:copies)if(stat(path,&st)==0)return false;
    return true;
}
inline bool running(){u64 pid=0;return R_SUCCEEDED(pmdmntGetProcessId(&pid,ProgramId));}
struct Key { unsigned code=0; bool shift=false; };
inline constexpr Key key(unsigned char c){if(c>='a'&&c<='z')return {unsigned(4+c-'a'),false};if(c>='A'&&c<='Z')return {unsigned(4+c-'A'),true};if(c>='1'&&c<='9')return {unsigned(30+c-'1'),false};if(c=='0')return {39,false};if(c==' ')return {44,false};const char* normal="-=[]\\;\x27`,./";const char* shifted="_+{}|:\x22~<>?";const unsigned codes[]={45,46,47,48,49,51,52,53,54,55,56};for(unsigned i=0;i<11;++i){if(c==normal[i])return {codes[i],false};if(c==shifted[i])return {codes[i],true};}const char* digits="!@#$%^&*()";for(unsigned i=0;i<10;++i)if(c==digits[i])return {i==9?39:30+i,true};return {};}
}

namespace tp {
constexpr bool checkKeyMap(){
    for(unsigned c=32;c<=126;++c){auto k=key(c);if(k.code==0||k.code>=256)return false;
        for(unsigned other=32;other<c;++other){auto o=key(other);if(o.code==k.code && o.shift==k.shift)return false;}}
    return key(10).code==0 && key(13).code==0 && key(128).code==0;
}
static_assert(checkKeyMap(),"ASCII typing map must cover every printable character uniquely and reject control characters");
}
