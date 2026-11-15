#pragma once
#include "editor.hpp"

class DeleteText : public tsl::Gui {
    unsigned id;
public:
    explicit DeleteText(unsigned value):id(value){}
    tsl::elm::Element* createUI() override {
        auto* frame=new tsl::elm::OverlayFrame("Delete text?","TextPaster");auto* list=new tsl::elm::List;
        auto* cancel=new tsl::elm::ListItem("Cancel");
        cancel->setClickListener([](u64 k){if(!(k&HidNpadButton_A))return false;tsl::goBack();return true;});list->addItem(cancel);
        auto* remove=new tsl::elm::ListItem("Delete");
        remove->setClickListener([this,remove](u64 k){
            if(!(k&HidNpadButton_A))return false;
            auto c=tp::read();if(!c.error.empty()){remove->setValue("Config error");return true;}
            c.entries.erase(std::remove_if(c.entries.begin(),c.entries.end(),[this](const tp::TextEntry& e){return e.id==id;}),c.entries.end());
            if(c.selected==id)c.selected=c.entries.empty()?0:c.entries.front().id;
            if(tp::write(c))tsl::goBack(2);else remove->setValue("Save failed");return true;
        });list->addItem(remove);frame->setContent(list);return frame;
    }
};
class TextActions : public tsl::Gui {
    unsigned id;
    std::string snapshot;u64 lastCheck=0;
public:
    explicit TextActions(unsigned value):id(value){}
    tsl::elm::Element* createUI() override {
        auto c=tp::read();auto* entry=tp::entry(c,id);
        snapshot=entry?entry->name+"\n"+entry->value+"\n"+std::to_string(c.selected):"";
        auto* frame=new tsl::elm::OverlayFrame(entry?entry->name:"Saved text","TextPaster");auto* list=new tsl::elm::List;
        if(entry)list->addItem(new tsl::elm::ListItem(entry->value));
        auto* use=new tsl::elm::ListItem("Use for paste",c.selected==id?"Selected":"");
        use->setClickListener([this,use](u64 k){if(!(k&HidNpadButton_A))return false;auto current=tp::read();
            if(!current.error.empty()||!tp::entry(current,id)){use->setValue("Text unavailable");return true;}
            current.selected=id;if(tp::write(current))tsl::goBack(2);else use->setValue("Save failed");return true;});list->addItem(use);
        auto* edit=new tsl::elm::ListItem("Edit text");edit->setClickListener([this](u64 k){if(!(k&HidNpadButton_A))return false;tsl::changeTo<TextEditor>(id,false);return true;});list->addItem(edit);
        auto* rename=new tsl::elm::ListItem("Rename");rename->setClickListener([this](u64 k){if(!(k&HidNpadButton_A))return false;tsl::changeTo<TextEditor>(id,true);return true;});list->addItem(rename);
        auto* remove=new tsl::elm::ListItem("Delete");remove->setClickListener([this](u64 k){if(!(k&HidNpadButton_A))return false;tsl::changeTo<DeleteText>(id);return true;});list->addItem(remove);
        frame->setContent(list);return frame;
    }
    void update() override {
        u64 now=armGetSystemTick();if(now-lastCheck<armGetSystemTickFreq()/2)return;lastCheck=now;
        auto c=tp::read();if(!c.error.empty())return;auto* e=tp::entry(c,id);
        if(!e){tsl::goBack();return;}
        if(e->name+"\n"+e->value+"\n"+std::to_string(c.selected)!=snapshot)tsl::swapTo<TextActions>(id);
    }
};
class SavedTexts : public tsl::Gui {
    std::string snapshot;
    static std::string signature(const tp::Config& c){std::string s=std::to_string(c.selected)+"\n";for(auto& e:c.entries)s+=std::to_string(e.id)+"\n"+e.name+"\n"+e.value+"\n";return s;}
    u64 lastCheck=0;
public:
    tsl::elm::Element* createUI() override {
        auto c=tp::read();snapshot=signature(c);
        auto* frame=new tsl::elm::OverlayFrame("Saved texts","TextPaster");auto* list=new tsl::elm::List;
        auto* add=new tsl::elm::ListItem("Add text");
        add->setClickListener([add](u64 k){if(!(k&HidNpadButton_A))return false;auto c=tp::read();
            if(!c.error.empty()){add->setValue("Config error");return true;}
            if(c.entries.size()>=32){add->setValue("Limit: 32");return true;}tsl::changeTo<TextEditor>(0,false);return true;});list->addItem(add);
        if(!c.error.empty())list->addItem(new tsl::elm::ListItem(c.error));
        for(auto& e:c.entries){
            auto* item=new tsl::elm::ListItem(e.name+" ? "+e.value,c.selected==e.id?"Selected":"");unsigned id=e.id;
            item->setClickListener([id](u64 k){if(!(k&HidNpadButton_A))return false;tsl::changeTo<TextActions>(id);return true;});list->addItem(item);
        }
        frame->setContent(list);return frame;
    }
    void update() override {
        u64 now=armGetSystemTick();if(now-lastCheck<armGetSystemTickFreq()/2)return;lastCheck=now;
        auto c=tp::read();if(c.error.empty()&&signature(c)!=snapshot)tsl::swapTo<SavedTexts>();
    }
};
