#include "SceneryPackFormat.h"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace SceneryFormat;
int main(int argc,char **argv){
    assert(SafePath("trees/a/tree.obj"));for(auto p:{"../secret","/root","C:/x","a/../b","a\\b","a//b","a/"})assert(!SafePath(p));
    assert(Step(25)==65536);assert(Step(50)==32768);assert(Step(0)==0);
    uint64_t clock=0;for(int i=0;i<5;i++){assert(Frame(clock,5,12,0)==0);clock+=65536;}
    assert(Frame(clock,5,12,0)==1);for(int pass=0;pass<100;pass++)assert(Frame(clock,5,12,0)==1);
    clock=0;assert(Frame(clock,5,12,0)==0);assert(Frame(65536ull*60,5,12,0)==0);
    assert(Frame(65536ull*5,5,12,3)==4);
    uint64_t at25=0,at50=0;for(int i=0;i<25;i++)at25+=Step(25);for(int i=0;i<50;i++)at50+=Step(50);
    assert(at25==at50&&Frame(at25,5,12,0)==Frame(at50,5,12,0));
    Entry e;assert(ParseEntry("tree\tfrahit.bfl\ttre_md1.dds\ttrees/a/tree.obj\t\t1",e));
    assert(e.source=="tre_md1");assert(!ParseEntry("tree\tfrahit.bfl\ttre.dds\t../x\t\t1",e));
    assert(!ParseEntry("spectator\tfrahit.bfl\ts.dds\tx.tga\tx.tga\t0",e));
    std::vector<Group> groups;
    std::string obj="v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nvn 0 0 1\nusemtl crown\nf 1/1/1 2/2/1 3/3/1\n";
    assert(ParseOBJ(obj,groups));assert(groups[0].vertices.size()==3);assert(groups[0].vertices[0].v==1);
    assert(!ParseOBJ(obj+"f 100/1/1 2/2/1 3/3/1\n",groups));
    assert(!ParseOBJ(obj+"f 1/1/1 2/2/1 3/3/1 1/1/1\n",groups));
    // Not assert(): CI builds RelWithDebInfo (NDEBUG), which would skip the call itself.
    if(argc>1){std::ifstream in(argv[1]);std::string text((std::istreambuf_iterator<char>(in)),{});
        if(!ParseOBJ(text,groups)){std::cerr<<"ParseOBJ rejected "<<argv[1]<<"\n";return 1;}
        size_t count=0;for(auto &g:groups){count+=g.vertices.size();std::cout<<"material "<<g.material<<"\n";}std::cout<<groups.size()<<" groups, "<<count/3<<" triangles\n";}
}
