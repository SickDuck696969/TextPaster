#pragma once
#include <tesla.hpp>
#include "config.hpp"

class KeyboardCanvas : public tsl::elm::CustomDrawer {
    std::function<void(s32,s32)> tap;
public:
    KeyboardCanvas(std::function<void(tsl::gfx::Renderer*,s32,s32,s32,s32)> draw,
                   std::function<void(s32,s32)> touch):CustomDrawer(draw),tap(touch){}
    bool onTouch(tsl::elm::TouchEvent event,s32 x,s32 y,s32,s32,s32 initialX,s32 initialY) override {
        if(!inBounds(x,y))return false;
        if(event==tsl::elm::TouchEvent::Release && abs(x-initialX)<18 && abs(y-initialY)<18)tap(x,y);
        return true;
    }
};

class TextEditor : public tsl::Gui {
    tp::Config original;
    unsigned entryId=0;bool renameOnly=false;size_t limit=512;
    std::string draft,notice;
    size_t cursor=0;
    int page=0,selected=0;
    s32 originX=0,originY=0,cell=0;
    std::string characters() const {
        if(page==0)return "abcdefghijklmnopqrstuvwxyz0123456789.,!?";
        if(page==1)return "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?";
        std::string result;
        for(char ch=33;ch<=126;++ch)if(!((ch>='0'&&ch<='9')||(ch>='A'&&ch<='Z')||(ch>='a'&&ch<='z')))result+=ch;
        result.resize(40,' ');return result;
    }
    void insert(char ch){if(draft.size()>=limit){notice="Character limit reached";return;}draft.insert(cursor,1,ch);++cursor;notice.clear();}
    void erase(){if(cursor){draft.erase(--cursor,1);notice.clear();}}
    void save(){
        if(draft.empty()){notice="Enter text first";return;}
        // Read again to preserve shortcut/timing edits made externally while editing.
        auto fresh=tp::read();
        if(!fresh.error.empty()){notice=fresh.error;return;}
        if(!tp::printable(draft,limit)){notice="Use printable ASCII characters";return;}
        if(entryId){
            auto* target=tp::entry(fresh,entryId);auto* before=tp::entry(original,entryId);
            if(!target||!before){notice="Text was removed";return;}
            if((renameOnly?target->name:target->value)!=(renameOnly?before->name:before->value)){notice="Text changed; reopen editor";return;}
            if(renameOnly)target->name=draft;else target->value=draft;
        }else{
            if(fresh.entries.size()>=32){notice="Maximum 32 texts";return;}
            unsigned id=tp::nextId(fresh);fresh.entries.push_back({id,"Text "+std::to_string(id),draft});fresh.selected=id;
        }
        if(!tp::write(fresh)){notice="Could not save";return;}
        tsl::goBack();
    }
    void activate(){
        if(selected<40){insert(characters()[selected]);return;}
        switch(selected-40){
            case 0:insert(' ');break;case 1:erase();break;
            case 2:page=page==1?0:1;break;case 3:page=page==2?0:2;break;
            case 4:draft.clear();cursor=0;break;
            case 5:if(cursor)--cursor;break;case 6:if(cursor<draft.size())++cursor;break;
            case 7:cursor=draft.size();break;case 8:tsl::goBack();break;case 9:save();break;
        }
    }
public:
    TextEditor(unsigned id=0,bool rename=false):original(tp::read()),entryId(id),renameOnly(rename),limit(rename?48:512){
        notice=original.error;
        if(id){auto* e=tp::entry(original,id);if(e)draft=rename?e->name:e->value;else notice="Text no longer exists";}
        cursor=draft.size();
    }
    tsl::elm::Element* createUI() override {
        auto* frame=new tsl::elm::OverlayFrame(renameOnly?"Rename":entryId?"Edit text":"Add text","A Select   B Cancel   + Save");
        frame->setContent(new KeyboardCanvas([this](tsl::gfx::Renderer* r,s32 x,s32 y,s32 w,s32){
            auto white=r->a(tsl::Color{15,15,15,15}),dim=r->a(tsl::Color{9,10,11,15});
            originX=x+8;originY=y+126;cell=(w-16)/10;
            r->drawRect(x+8,y+8,w-16,70,r->a(tsl::Color{2,3,4,15}));
            const size_t visible=std::max(8,(w-32)/13);
            const size_t start=cursor>=visible?cursor-visible+1:0;
            std::string preview=draft;
            preview.insert(cursor,"|");
            r->drawString(preview.substr(start,visible),true,x+16,y+48,20,white);
            r->drawString(std::to_string(draft.size())+" / "+std::to_string(limit),false,x+12,y+104,17,dim);
            auto keys=characters();
            for(int i=0;i<50;++i){
                int col=i<40?i%10:(i-40)%5;
                int row=i<40?i/10:4+(i-40)/5;
                int width=i<40?cell:cell*2;
                s32 left=originX+col*width,top=originY+row*46;
                r->drawRect(left+1,top+1,width-3,42,r->a(i==selected?tsl::Color{1,8,8,15}:tsl::Color{3,3,4,15}));
                const char* actions[]={"Space","Del","Aa","#+=","Clear","<",">","End","Cancel","Save"};
                std::string label=i<40?std::string(1,keys[i]):actions[i-40];
                r->drawString(label,false,left+5,top+28,i<40?20:15,white);
            }
            r->drawString("X Delete   Y Layout   L/R Cursor",false,x+10,originY+302,16,dim);
            if(!notice.empty())r->drawString(notice,false,x+10,originY+334,16,white,w-20);
        },[this](s32 x,s32 y){
            if(cell<=0||x<originX||x>=originX+cell*10||y<originY||y>=originY+276)return;
            int row=(y-originY)/46;
            selected=row<4?row*10+(x-originX)/cell:40+(row-4)*5+(x-originX)/(cell*2);
            activate();
        }));return frame;
    }
    bool handleInput(u64 down,u64,const HidTouchState&,HidAnalogStickState,HidAnalogStickState) override {
        if(down&HidNpadButton_B){tsl::goBack();return true;}
        if(down&HidNpadButton_Plus){save();return true;}
        if(down&HidNpadButton_X){erase();return true;}
        if(down&HidNpadButton_Y){page=(page+1)%3;return true;}
        if(down&HidNpadButton_L){if(cursor)--cursor;return true;}
        if(down&HidNpadButton_R){if(cursor<draft.size())++cursor;return true;}
        if(down&HidNpadButton_A){activate();return true;}
        if(down&HidNpadButton_Left){selected=(selected+49)%50;return true;}
        if(down&HidNpadButton_Right){selected=(selected+1)%50;return true;}
        if(down&HidNpadButton_Down){selected=selected<30?selected+10:selected<40?40+(selected-30)/2:selected<45?selected+5:(selected-45)*2;return true;}
        if(down&HidNpadButton_Up){selected=selected<10?45+selected/2:selected<40?selected-10:selected<45?30+(selected-40)*2:selected-5;return true;}
        return false;
    }
};
