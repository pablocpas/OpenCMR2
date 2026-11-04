// Execute the real sector traversal, broad bounds, detailed test and response
// with synthetic geometry. No game assets, renderer or user settings needed.
#include "port/types.h"
#include "port/sys.h"
#include "Car.h"
#include "Sector.h"
#include "SectorObjectEntry.h"
#include "Mesh.h"
#include "MeshCollisionRecord.h"
#include "Collision2D.h"
#include "GraphicsOptions.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <string>
extern int Collision_TestCarAgainstSectorObjects(Car *);
extern int Collision_DoSpheresOverlap(int,int,int*,int*);
extern void Scene_InitFixedMathTables();
extern short *g_sectorListIndexA,*g_sectorListIndexB;
extern unsigned short *g_sectorListCountA;
extern SectorObjectEntry **g_sectorListEntriesA;
extern CollisionBox g_carCollisionBoxes[8];
#define REQUIRE(x) do { if(!(x)){fprintf(stderr,"tree collision line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
struct Result {int hit, speed, contacts, impulseX, impulseZ; unsigned flags; FixVector position;};
static Result Run(int option,int x,int z,unsigned flags,int speed){
    REQUIRE(Sys_SetOption("video.tree_break",option));
    REQUIRE(Sys_GetOption("video.tree_break",-1)==option);
    Car car{};Mesh mesh{};MeshCollisionRecord record{};StageObject object{};
    SectorObjectBounds bounds{};SectorObjectEntry entry{};short indices[]={0};unsigned short counts[]={1};SectorObjectEntry *entries[]={&entry};
    g_sectorCount=1;g_sectorListIndexA=indices;g_sectorListIndexB=nullptr;g_sectorListCountA=counts;g_sectorListEntriesA=entries;
    object.position={x,0,z};object.pMesh=&mesh;object.flags=flags;
    mesh.pCollisionRecords=&record;record.height=0x20000;
    bounds.radius=0x4000;entry.pObject=&object;entry.pBounds=&bounds;
    car.sector=0;car.neighbours.s[0]=car.neighbours.s[1]=car.neighbours.s[2]=-1;
    car.collisionRadius=0x40000;car.speed=speed;car.velocity={speed,0,0};car.pWorld=&car.physicsMatrix;
    car.right.x=car.up.y=car.forward.z=0x10000;car.physicsMatrix.right.x=car.physicsMatrix.up.y=car.physicsMatrix.forward.z=0x10000;
    car.halfExtents={0x8000,0x10000,0x10000};car.previousContactCount=1;car.contactSurfaces[0][0]=0;
    auto &box=g_carCollisionBoxes[0];box={};box.built=1;box.halfWidth=0x8000;box.halfLength=0x10000;box.top=0x10000;box.bottom=-0x10000;
    box.axisA={0x10000,0,0};box.axisB={0,0,0x10000};box.pVertex=&car.position;
    const int signs[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}};
    for(int i=0;i<4;i++)box.points[i]={signs[i][0]*box.halfWidth,0,signs[i][1]*box.halfLength};
    if(x==0x20000){REQUIRE(Collision_DoSpheresOverlap(car.collisionRadius,bounds.radius,(int*)&car.position,(int*)&object.position)!=0);}
    unsigned falls=g_treeFallStateCount;int hit=Collision_TestCarAgainstSectorObjects(&car);
    REQUIRE(g_treeFallStateCount==falls);REQUIRE(object.flags==flags);
    Result result={hit,car.speed,car.pendingContactCount,car.field_0x5c4.x,car.field_0x5c4.z,object.flags,car.position};
    box={};g_sectorListIndexA=nullptr;g_sectorListEntriesA=nullptr;g_sectorListCountA=nullptr;g_sectorCount=0;return result;
}
int main(){
    auto old=std::filesystem::current_path();auto tmp=std::filesystem::temp_directory_path()/("opencmr2-tree-collision-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(tmp);std::filesystem::current_path(tmp);std::ofstream("opencmr2.ini") << "[video]\ntree_break=0\n";Scene_InitFixedMathTables();
    for(int speed:{0x10000,0x20000,0x40000})for(unsigned flags:{0u,(unsigned)STAGE_OBJECT_BROKEN}){
        for(auto point:{FixVector{0x80000,0,0},FixVector{0x20000,0,0x20000},FixVector{0,0,0x12000}}){
            auto a=Run(0,point.x,point.z,flags,speed),b=Run(1,point.x,point.z,flags,speed);
            REQUIRE(a.hit==b.hit && a.speed==b.speed && a.contacts==b.contacts && a.impulseX==b.impulseX && a.impulseZ==b.impulseZ);
            REQUIRE(a.position.x==b.position.x && a.position.z==b.position.z);REQUIRE(a.speed==speed);
            if(point.x==0){REQUIRE(a.hit!=0);REQUIRE(a.contacts==1);}else{REQUIRE(a.hit==0);REQUIRE(a.contacts==0);}
        }
    }
    std::filesystem::current_path(old);std::filesystem::remove_all(tmp);
    puts("tree collision runtime: original contact/near-miss behavior, both option states, low/high speed and stale flags pass");
}
