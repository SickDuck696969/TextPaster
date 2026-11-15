#include "config.hpp"
#include <cassert>
#include <iostream>
tp::Config readText(const std::string& s){std::istringstream stream(s);return tp::parse(stream);}
int main(){
    auto c=readText("combo=L+ZR\ndelay_ms=40\ntext=  #hello=[a];  \n");
    assert(c.error.empty()&&c.legacy&&c.selected==1&&c.entries.size()==1&&c.text=="  #hello=[a];  ");
    c.entries.push_back({7,"Email","name@example.com"});c.selected=7;
    std::ostringstream output;tp::serialize(output,c);auto round=readText(output.str());
    assert(round.error.empty()&&!round.legacy&&round.entries.size()==2&&round.selected==7&&round.text=="name@example.com");
    assert(round.entries[0].value=="  #hello=[a];  ");
    assert(tp::nextId(round)==2);
    auto bom=readText(std::string("\xEF\xBB\xBF")+"version=2\r\ncombo=L+ZR\r\nselected=0\r\n");
    assert(bom.error.empty()&&bom.entries.empty()&&bom.text.empty());
    assert(!readText("version=2\nselected=5\n").error.empty());
    assert(!readText("version=2\nselected=1\n[text:1]\nname=A\ntext=a\n[text:1]\nname=B\ntext=b\n").error.empty());
    assert(!readText("version=2\nselected=0\n[text:0]\n").error.empty());
    assert(!readText("version=2\nselected=1\n[text:1]\nname=A\ntext=\n").error.empty());
    assert(!readText("version=2\nselected=0\ntext=legacy\n").error.empty());
    assert(!readText("version=2\nselected=1\n[text:1]\nname=A\ntext=\t\n").error.empty());
    assert(!readText(std::string(32769,'#')).error.empty());
    tp::Config full;full.entries.clear();
    for(unsigned i=1;i<=32;++i)full.entries.push_back({i,"Name",std::string(512,'a')});
    tp::validate(full);assert(full.error.empty());std::ostringstream max;tp::serialize(max,full);
    assert(readText(max.str()).error.empty());full.entries.push_back({33,"Extra","x"});tp::validate(full);assert(!full.error.empty());
    round.entries.erase(round.entries.begin()+1);round.selected=round.entries.front().id;tp::validate(round);assert(round.error.empty()&&round.text=="  #hello=[a];  ");
    round.entries.clear();round.selected=0;tp::validate(round);assert(round.error.empty()&&round.text.empty());
    std::cout<<"Config tests passed\n";
}
