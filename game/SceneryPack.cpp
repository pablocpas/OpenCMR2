#include "SceneryMotion.h"
#include "SceneryMip.h"
#include "ParallelFor.h"
#include "OrderedBatch.h"
#include "CrowdMesh.h"
#include "SceneryPack.h"
#include <cmath>
#include "CrowdSelection.h"
#include "ScenerySHA256.h"
#include "SceneryPackFormat.h"
#include "ScenerySourceFit.h"
#include "FixedStrip.h"
#include "France1Textures.h"
#include "FixedStripSources.inc"
#include "Gre01BrushSources.inc"
#include "RallyRoute.h"
#include "BrushRelief.h"
#include "Graphics.h"
#include "Texture.h"
#include "Mesh.h"
#include "MeshVertex.h"
#include "Sector.h"
#include "port/sys.h"
#include "port/gfx.h"
#include <map>
#include <unordered_map>
#include <set>
#ifdef rand
#undef rand
#endif
#pragma push_macro("rand")
#undef rand
#include <algorithm>
#pragma pop_macro("rand")
#include <cstring>
#include <limits>
using namespace SceneryFormat;
namespace {
std::map<GenericFile*,std::string> archives;
std::map<const Texture*,std::string> sourceIdentities;
struct Identity {std::string archive,source,hash;};
std::vector<Identity> identities;
std::map<std::string,std::set<std::string>> windIdentities;
std::set<std::string> windLogged,treeOriginalLogged;
std::set<std::string> unmatched;
struct PackTexture { GfxTexture *surface=nullptr; int width=0,height=0; };
std::vector<GfxTexture*> ownedTextures;
uint64_t animationTime=0;
const StageObject *diagnosticObject=nullptr;
bool diagnosticArticulated=false;
unsigned diagnosticFrame=0,diagnosticChanges=0;uint64_t diagnosticTime=0;
static unsigned long long timeWindMathNs=0,timeSubmitNs=0;
static unsigned long long windBatchSmall=0,windBatchLarge=0,windBatchTrees=0;
// Timers. depth counters keep a flush that runs inside a draw from being counted twice.
static unsigned long long timeReplacementNs=0,timeOtherNs=0,timeCalls=0;static int drawDepth=0,replacementDepth=0;static std::vector<GfxVertex> windScratch;
unsigned long long crowdDrawsTotal=0,crowdTrianglesTotal=0,treeTrianglesTotal=0,treeRustledVerticesTotal=0;
unsigned loggedSecond=0,treeReplacementDraws=0,treeWindDraws=0,crowdDraws=0;unsigned long long crowdTriangles=0;
std::unordered_map<const Mesh*,CrowdMesh::Template<GfxVertex>> crowdMeshes;

struct Part { std::vector<GfxVertex> vertices; PackTexture *texture=nullptr; std::vector<SceneryMotion::WindVertex> wind; float windBottom=0,windHeight=0; };
struct PendingTree{GfxMatrix m;std::vector<Part> *parts;SceneryMotion::WindFrame frame;};
static OrderedBatch<PendingTree,std::vector<std::vector<GfxVertex>>> pendingTrees;
struct Asset { std::map<const Mesh*,std::vector<Part>> fitted; int sourceWidth=0,sourceHeight=0; SceneryFormat::Entry entry; std::string failure; std::set<std::string> loggedFailures; bool attempted=false,ready=false; std::vector<Part> parts; std::vector<PackTexture*> frames; };
std::vector<Asset> assets;
struct FixedPart {bool ready=false;std::vector<FixedStrip::Vertex> vertices;std::vector<GfxVertex> scratch;};
struct BrushPart {bool ready=false;float world[16]={};std::vector<BrushRelief::Vertex> vertices;std::vector<unsigned short> indices;std::vector<GfxVertex> scratch;MeshVertexF corners[4]={};};
std::map<const MeshPart*,FixedPart> fixedParts;
std::map<const MeshPart*,BrushPart> brushParts;
bool loaded=false,identityContract=false;
bool treeLogged=false,spectatorLogged=false;
std::string root;
std::map<std::string,PackTexture> textures;
bool Read(const std::string &path,std::string &out) {
    SysFile *f=nullptr;
    if(Sys_HasSceneryDirectory()){
        // Paths already carry the configured relative pack prefix. Remove only
        // that exact prefix when reading from the independently selected root.
        std::string prefix=Sys_GetOptionText("video.scenery_pack","scenery-pack");if(!prefix.empty()&&prefix.back()!='/')prefix+='/';
        if(path.compare(0,prefix.size(),prefix)!=0)return false;
        std::string relative=path.substr(prefix.size());if(!SafePath(relative))return false;
        f=Sys_OpenSceneryFile(relative.c_str());
    }else f=Sys_OpenFile(path.c_str(),SYS_FILE_READ);
    if(!f)return false;
    DWORD n=Sys_GetFileSize(f);if(n>16*1024*1024){Sys_CloseFile(f);return false;}
    out.resize(n);bool ok=Sys_ReadFile(f,out.data(),n)==n;Sys_CloseFile(f);return ok;
}
std::string Dir(const std::string &s){auto at=s.find_last_of('/');return at==s.npos?"":s.substr(0,at+1);}
PackTexture *Load(const std::string &relative,bool tree=false){
    const std::string cacheKey=(tree?"tree:":"atlas:")+relative;
    auto cached=textures.find(cacheKey);if(cached!=textures.end())return &cached->second;
    if(!SafePath(relative))return nullptr;
    std::string bytes,path=root+relative;if(!Read(path,bytes)||bytes.size()<18)return nullptr;
    // Decoder accepts uncompressed truecolour only. Validate dimensions/size first.
    const unsigned char *b=(const unsigned char*)bytes.data();unsigned w=b[12]|b[13]<<8,h=b[14]|b[15]<<8;
    if(b[1]!=0||b[2]!=2||b[16]!=32||w==0||h==0||w>4096||h>4096||bytes.size()<18u+b[0]+size_t(w)*h*4)return nullptr;
    if(b[4]||b[5]||b[6]||b[7]||b[8]||b[9]||b[10]||b[11]||b[17]!=8)return nullptr;
    unsigned levels=1;if(tree)for(unsigned x=w,y=h;x>1||y>1;x=std::max(1u,x/2),y=std::max(1u,y/2))++levels;
    GfxTexture *surface=Gfx_CreateTexture(w,h,GFX_FORMAT_BGRA8,levels,0);if(!surface)return nullptr;
    std::vector<uint8_t> pixels(size_t(w)*h*4);
    // TGA rows begin at bottom; renderer UV v=0 is top, as native TGA path.
    for(unsigned y=0;y<h;y++)memcpy(pixels.data()+y*w*4,b+18+b[0]+(h-1-y)*w*4,w*4);
    unsigned opaque=0;for(size_t i=3;i<pixels.size();i+=4)opaque+=pixels[i]>=128;
    double coverage=double(opaque)/(w*h);unsigned mw=w,mh=h;
    for(unsigned level=0;level<levels;level++){
        GfxLockedRect lock={};if(!Gfx_LockTexture(surface,0,level,&lock)){Gfx_DestroyTexture(surface);return nullptr;}
        for(unsigned y=0;y<mh;y++)memcpy((char*)lock.pixels+y*lock.pitch,pixels.data()+y*mw*4,mw*4);
        Gfx_UnlockTexture(surface,0,level);
        if(level+1<levels){pixels=SceneryMip::Down(pixels,mw,mh,coverage);mw=std::max(1u,mw/2);mh=std::max(1u,mh/2);}
    }
    ownedTextures.push_back(surface);
    auto &t=textures[cacheKey];t={surface,(int)w,(int)h};return &t;
}
void Init(){
    if(loaded)return;loaded=true;
    root=Sys_GetOptionText("video.scenery_pack","scenery-pack");if(!root.empty()&&root.back()!='/')root+='/';
    std::string text;if(!Read(root+"manifest.tsv",text))return;
    std::istringstream in(text);std::string line;
    while(std::getline(in,line)) { SceneryFormat::Entry e;if(ParseEntry(line,e)){
        bool duplicate=false;for(auto &a:assets)if(a.entry.archive==e.archive&&a.entry.source==e.source)duplicate=true;
        if(duplicate){Sys_Log("scenery pack: duplicate mapping rejected %s/%s",e.archive.c_str(),e.source.c_str());assets.clear();return;}
        if(assets.size()>=2048)return;Asset a;a.entry=e;assets.push_back(std::move(a));
    }}
    if(Read(root+"source-identities.tsv",text)) {
        identityContract=true;
        std::istringstream rows(text);
        while(std::getline(rows,line)) {auto p=Split(line,'\t');
            if(p.size()!=3||p[0].find('/')==p[0].npos||p[2].size()!=64||identities.size()>=2048)continue;
            if(p[2].find_first_not_of("0123456789abcdef")!=p[2].npos)continue;
            auto slash=p[0].find('/');identities.push_back({p[0].substr(0,slash),Stem(p[0].substr(slash+1)),p[2]});
        }
    }
    if(Read(root+"tree-wind-identities.tsv",text)){
        std::istringstream rows(text);while(std::getline(rows,line)){auto p=Split(line,'\t');
            if(p.size()!=3||p[2].size()!=64||p[2].find_first_not_of("0123456789abcdef")!=p[2].npos)continue;
            auto slash=p[0].find('/');if(slash!=p[0].npos)windIdentities[Stem(p[0].substr(slash+1))].insert(p[2]);
        }
    }
    Sys_Log("scenery pack: %u mappings, %u verified-byte identity routes",(unsigned)assets.size(),(unsigned)identities.size());
}
bool Reject(Asset &a,const char *reason){a.failure=reason;return false;}
void LogRejection(Asset &a){
    if(!a.loggedFailures.insert(a.failure).second)return;
    Sys_Log("options: scenery rejected %s/%s asset=%s: %s; keeping original",a.entry.archive.c_str(),
        a.entry.source.c_str(),a.entry.asset.c_str(),a.failure.c_str());
}
bool Prepare(Asset &a){
    if(a.attempted)return a.ready;a.attempted=true;
    if(a.entry.kind=="spectator"){
        int w=0,h=0;for(auto &path:a.entry.frames){PackTexture *t=Load(path);if(!t)return Reject(a,"spectator frame missing, invalid TGA or texture allocation failed");
            if(w&&(t->width!=w||t->height!=h))return Reject(a,"spectator frame dimensions disagree");w=t->width;h=t->height;a.frames.push_back(t);}
        return a.ready=!a.frames.empty();
    }
    std::string text;if(!Read(root+a.entry.asset,text))return Reject(a,"tree OBJ missing or unreadable");
    // Legacy cylindrical/repeat assets are deliberately rejected: they must
    // be regenerated with the source-canvas contract, not silently distorted.
    { std::istringstream header(text);std::string hash,version;
      if(!(header>>hash>>version>>a.sourceWidth>>a.sourceHeight)||hash!="#"||version!="opencmr2-source-canvas-1"||
         a.sourceWidth<1||a.sourceHeight<1||a.sourceWidth>4096||a.sourceHeight>4096)return Reject(a,"missing/invalid source-canvas-1 header; regenerate pack with current converter into a NEW folder"); }
    std::vector<Group> groups;if(!ParseOBJ(text,groups))return Reject(a,"tree OBJ geometry invalid");
    std::istringstream obj(text);std::string line,tag,mtl;
    while(std::getline(obj,line)){std::istringstream in(line);in>>tag;if(tag=="mtllib")in>>mtl;}
    if(!SafePath(mtl)||!Read(root+Dir(a.entry.asset)+mtl,text))return Reject(a,"tree material missing, unreadable or unsafe path");
    std::map<std::string,std::string> maps;std::string material;
    std::istringstream input(text);
    while(std::getline(input,line)){std::istringstream in(line);in>>tag;
        if(tag=="newmtl")in>>material;else if(tag=="map_Kd"){std::string p;in>>p;if(!SafePath(p))return Reject(a,"unsafe material texture path");maps[material]=Dir(a.entry.asset)+Dir(mtl)+p;}}
    for(auto &g:groups){auto it=maps.find(g.material);if(it==maps.end())return Reject(a,"tree material has no texture mapping");
        Part p;p.texture=Load(it->second,true);if(!p.texture)return Reject(a,"tree texture missing, invalid TGA or texture allocation failed");
        for(auto &v:g.vertices)p.vertices.push_back({v.x,v.y,v.z,v.nx,v.ny,v.nz,0xffffffff,0,v.u,v.v,0,0});
        a.parts.push_back(std::move(p));}
    return a.ready=true;
}
void State(const GfxMatrix &m,bool tree=false){
    Gfx_PushState();Gfx_SetTransform(GFX_TRANSFORM_WORLD,&m);Gfx_SetLighting(FALSE);
    Gfx_SetDepthTest(TRUE);Gfx_SetDepthWrite(TRUE);Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetAlphaBlend(TRUE);Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA);Gfx_SetDestBlend(GFX_BLEND_INVSRCALPHA);
    Gfx_SetAlphaTest(TRUE);Gfx_SetAlphaFunc(GFX_CMP_GREATER);Gfx_SetAlphaRef(127);
    Gfx_SetStageColorOp(0,GFX_TOP_MODULATE);Gfx_SetStageColorArg1(0,GFX_TA_TEXTURE);Gfx_SetStageColorArg2(0,GFX_TA_DIFFUSE);
    Gfx_SetStageAlphaOp(0,GFX_TOP_MODULATE);Gfx_SetStageAlphaArg1(0,GFX_TA_TEXTURE);Gfx_SetStageAlphaArg2(0,GFX_TA_DIFFUSE);
    Gfx_SetStageMinFilter(0,GFX_FILTER_POINT);Gfx_SetStageMagFilter(0,GFX_FILTER_POINT);Gfx_SetStageMipFilter(0,GFX_MIPFILTER_NONE);
    if(tree){Gfx_SetStageMinFilter(0,GFX_FILTER_LINEAR);Gfx_SetStageMagFilter(0,GFX_FILTER_LINEAR);Gfx_SetStageMipFilter(0,GFX_MIPFILTER_LINEAR);}
    Gfx_SetStageTexCoordIndex(0,0);Gfx_SetStageTextureTransform(0,0);Gfx_SetStageAddress(0,GFX_ADDRESS_WRAP);
    Gfx_SetStageColorOp(1,GFX_TOP_DISABLE);Gfx_SetStageAlphaOp(1,GFX_TOP_DISABLE);
}
}
void SceneryPack_LogStatus(){
    if(!Sys_GetOption("video.scenery_overrides",1)){
        Sys_Log("options: scenery=original (overrides disabled)");return;
    }
    std::string folder=Sys_GetOptionText("video.scenery_pack","scenery-pack");
    if(!Sys_HasSceneryDirectory()&&!SceneryFormat::SafePath(folder)){
        Sys_Log("options: scenery=original (invalid relative pack root)");return;
    }
    std::string text;
    if(!Read(folder+"/manifest.tsv",text)){
        Sys_Log("options: scenery=original (manifest missing/unreadable)");return;
    }
    std::istringstream input(text);std::string line;unsigned count=0;
    while(std::getline(input,line)){SceneryFormat::Entry entry;if(ParseEntry(line,entry))count++;}
    Sys_Log("options: scenery manifest=%u parsed mappings; NOT active yet, resources checked lazily at stage draw",count);
}
void SceneryPack_RegisterSource(Texture *texture,const void *bytes,unsigned size){
    if(!texture)return;
    sourceIdentities.erase(texture); // allocator may reuse a released texture address
    if(!bytes||size<1||size>16*1024*1024||!Sys_GetOption("video.scenery_overrides",1))return;
    Init();std::string stem=Stem(texture->name);bool candidate=false;
    for(auto &a:assets)if(a.entry.source==stem){candidate=true;break;}
    if(windIdentities.count(stem))candidate=true;
    if(candidate)sourceIdentities[texture]=SceneryIdentity::SHA256(bytes,size);
}
void SceneryPack_RegisterArchive(GenericFile *file,const char *path){if(file&&path)archives[file]=Leaf(path);}
struct ReactCar{float x=0,z=0,speed=0;bool valid=false;uint64_t at=0;};static ReactCar reactCars[8];
void SceneryPack_Clear(){pendingTrees.Clear();fixedParts.clear();crowdMeshes.clear();assets.clear();windIdentities.clear();windLogged.clear();treeOriginalLogged.clear();archives.clear();sourceIdentities.clear();identities.clear();unmatched.clear();for(auto t:ownedTextures)Gfx_DestroyTexture(t);ownedTextures.clear();textures.clear();loaded=false;identityContract=false;treeLogged=false;spectatorLogged=false;root.clear();animationTime=0;for(auto &c:reactCars)c=ReactCar();diagnosticObject=nullptr;diagnosticArticulated=false;diagnosticFrame=diagnosticChanges=loggedSecond=treeReplacementDraws=treeWindDraws=0;diagnosticTime=0;}
void SceneryPack_Advance(int step){if(step>0)animationTime+=(unsigned)step;}
void SceneryPack_SetReactionSource(int slot,int x,int z,int speed,bool valid){if(slot<0||slot>=8)return;reactCars[slot]={x/65536.f,z/65536.f,speed/65536.f,valid,animationTime};}
static bool treeBatch=false;
static float batchEye[4]={0,0,0,0}; // xyz + valid flag, set by SceneryPack_BeginBatch(eye)
static void FlushTrees();
static ParallelWork::Pool &treePool(){
    // video.tree_threads: 0 = automatic (cores, at most 8), 1 = no worker threads. Read once.
    static ParallelWork::Pool pool((unsigned)std::max(0,Sys_GetOption("video.tree_threads",0)));return pool;
}
static bool DrawReplacement(StageObject *object,const float *billboard,unsigned textureCount,Texture **textureBuffer){
    if(!object||!object->pMesh||!billboard||Sys_GetOption("video.scenery_overrides",1)==0)return false;
    Mesh *mesh=object->pMesh;
    // Only replace single-source cutouts, never a composite stage mesh.
    if(!(mesh->flags&2)||!mesh->pTriangles||mesh->triangleCount<1||!mesh->pVertexData||mesh->field_0x10<1)return false;
    int texture=mesh->pTriangles[0].textures[0];if(texture<0||(unsigned)texture>=textureCount)return false;
    for(int i=1;i<mesh->triangleCount;i++)if(mesh->pTriangles[i].textures[0]!=texture)return false;
    Texture *source=textureBuffer[texture];if(!source)return false;
    auto archive=archives.find((GenericFile*)source->pArchive);
    Init();Asset *asset=nullptr;std::string stem=Stem(source->name);
    auto identity=sourceIdentities.find(source);
    // Source-index archives are preparation containers, not necessarily the
    // runtime stage archive. Loose textures have no pArchive at all. Route
    // these only by exact original DDS bytes, never by an ambiguous filename.
    if(identity!=sourceIdentities.end())for(auto &id:identities)if(id.source==stem&&id.hash==identity->second)
        for(auto &a:assets)if(a.entry.archive==id.archive&&a.entry.source==stem){asset=&a;break;}
    // Legacy packs retain their strict archive+stem contract, but new packs
    // with identity records never bypass a failing byte check.
    if(!identityContract&&archive!=archives.end())
        for(auto &a:assets)if(a.entry.archive==archive->second&&a.entry.source==stem){asset=&a;break;}
    if(!asset){
        for(auto &a:assets)if(a.entry.source==stem&&unmatched.insert(stem).second)
            Sys_Log("options: scenery source unmatched %s runtime archive=%s; source bytes unverified or pack lacks matching identity; keeping original",stem.c_str(),archive==archives.end()?"loose/unregistered":archive->second.c_str());
        return false;
    }
    if(asset->entry.kind=="tree"&&Sys_GetOption("video.tree_overrides",1)==0)return false;
    if(asset->entry.kind=="spectator"&&Sys_GetOption("video.spectator_overrides",1)==0)return false;
    if(!Prepare(*asset)){
        LogRejection(*asset);
        return false;
    }
    auto v=(MeshVertexF*)mesh->pVertexData;
    float bottom=v[0].y,top=v[0].y;
    for(int i=1;i<mesh->field_0x10;i++){bottom=std::min(bottom,v[i].y);top=std::max(top,v[i].y);}
    float height=top-bottom;if(!std::isfinite(height)||height<=0)return false;
    if(asset->entry.kind=="spectator"){
        // The stage-effects update owns time. Drawing and reflection passes never
        // advance it. Stable placement phase does not consume physics RNG.
        unsigned phase=((unsigned)object->position.x*1664525u)^(unsigned)object->position.z;
        unsigned frame=Frame(animationTime,asset->entry.ticks,(unsigned)asset->frames.size(),phase);
        // Every identity-verified supported card moves, including neighbours on
        // multi-person atlases. Authored limb poses remain in addition to sway.
        bool selected=true; // Requested full supported placement coverage.
        if(!selected)frame=0;
        // Any nearby rally car excites the crowd. A car stops counting after ~2s of authored
        // time without a live-race refresh (menus, pauses, replays that do not update).
        float excitement=0;
        if(Sys_GetOption("video.reactive_spectators",1)!=0)for(const auto &c:reactCars)if(c.valid&&animationTime-c.at<65536ull*50)
            excitement=std::max(excitement,SceneryMotion::Excitement(object->position.x/65536.f-c.x,object->position.z/65536.f-c.z,c.speed));
        PackTexture *t=asset->frames[frame];
        bool irregular=Sys_GetOption("video.spectator_irregular",1)!=0;
        float left=v[0].x,right=v[0].x;for(int i=1;i<mesh->field_0x10;i++){left=std::min(left,v[i].x);right=std::max(right,v[i].x);}
        float span=right-left;
        // Reused across draws and passes (reflections draw the same card): no allocation once warm.
        static std::vector<GfxVertex> vertices;vertices.clear();
        if(!irregular){
            for(int i=0;i<mesh->triangleCount;i++)for(int j=0;j<3;j++){
                unsigned ix=mesh->pTriangles[i].vertexIndex[j];if(ix>=(unsigned)mesh->field_0x10)return false;
                auto &p=v[ix];vertices.push_back({p.x+SceneryMotion::ReactiveOffset(p.y,bottom,height,animationTime,phase,excitement),p.y,p.z,p.nx,p.ny,p.nz,p.diffuseColour,p.specularColour,p.textureCoordinates[0][0],p.textureCoordinates[0][1],0,0});}
        }else{
            // Each source triangle is cut into 16 once per mesh (silhouette and texture mapping unchanged) so the sway can
            // differ along the card: one card carries several people, one tempo for all looks like unison.
            auto &tpl=crowdMeshes[mesh];
            if(!tpl.Matches(mesh->triangleCount,mesh->field_0x10,bottom,height,left,span)){
                tpl=CrowdMesh::Template<GfxVertex>();tpl.verts.reserve((size_t)mesh->triangleCount*48);
                for(int i=0;i<mesh->triangleCount;i++){
                    GfxVertex tri[3];
                    for(int j=0;j<3;j++){
                        unsigned ix=mesh->pTriangles[i].vertexIndex[j];if(ix>=(unsigned)mesh->field_0x10){crowdMeshes.erase(mesh);return false;}
                        auto &p=v[ix];tri[j]={p.x,p.y,p.z,p.nx,p.ny,p.nz,p.diffuseColour,p.specularColour,p.textureCoordinates[0][0],p.textureCoordinates[0][1],0,0};
                    }
                    CrowdMesh::Tessellate16(tri[0],tri[1],tri[2],tpl.verts);
                }
                tpl.triangles=mesh->triangleCount;tpl.sourceVertices=mesh->field_0x10;tpl.bottom=bottom;tpl.height=height;tpl.left=left;tpl.span=span;
            }
            vertices.resize(tpl.verts.size());
            for(size_t k=0;k<tpl.verts.size();k++){
                GfxVertex g=tpl.verts[k];
                float u=span>0?(g.x-left)/span:0;
                g.x+=SceneryMotion::ReactivePersonOffset(g.y,bottom,height,u,animationTime,phase,excitement);
                vertices[k]=g;
            }
        }
        ++crowdDraws;crowdTriangles+=vertices.size()/3;++crowdDrawsTotal;crowdTrianglesTotal+=vertices.size()/3;
        if(!spectatorLogged){Sys_Log("options: spectator sidecar active (first successful override draw)");spectatorLogged=true;}
        FlushTrees();State(*(const GfxMatrix*)billboard);Gfx_SetTexture(0,t->surface);Gfx_DrawPrimitive(GFX_TRIANGLELIST,vertices.data(),vertices.size());Gfx_PopState();
        if(selected&&(!diagnosticObject||(asset->frames.size()==4&&!diagnosticArticulated))){diagnosticObject=object;diagnosticFrame=frame;diagnosticChanges=0;diagnosticArticulated=asset->frames.size()==4;}
        if(diagnosticObject==object){
            if(diagnosticFrame!=frame)++diagnosticChanges;
            diagnosticFrame=frame;diagnosticTime=animationTime;
            unsigned second=(unsigned)(animationTime/(25ull*65536));
            if(second!=loggedSecond&&second<=5){
                Sys_Log("options: spectator animation tick=%llu frame=%u/%u bound-frame-changes=%u texture=%s crowd-draws=%u crowd-triangles=%llu (counted since the previous log line; counts, not timing)",(unsigned long long)(animationTime/65536),frame,(unsigned)asset->frames.size(),diagnosticChanges,asset->entry.frames[frame].c_str(),crowdDraws,crowdTriangles);
                loggedSecond=second;crowdDraws=0;crowdTriangles=0;
            }
        }
        return true;
    }
    if(source->width!=asset->sourceWidth||source->height!=asset->sourceHeight){
        Reject(*asset,"source texture dimensions differ from tree header");LogRejection(*asset);return false;
    }
    auto fitted=asset->fitted.find(mesh);
    if(fitted==asset->fitted.end()){
        ScenerySourceFit::Fit fit;
        if(!ScenerySourceFit::Build(v,mesh->field_0x10,fit)){
            asset->fitted.emplace(mesh,std::vector<Part>{});Reject(*asset,"source card is cropped, nonplanar or invalid for full-canvas fit");LogRejection(*asset);return false;
        }
        std::vector<Part> parts=asset->parts;
        for(auto &part:parts)for(auto &q:part.vertices){float xyz[3];
            float u=.5f+q.x*asset->sourceHeight/asset->sourceWidth;
            float sv=1.f-q.y;
            fit.Map(u,sv,q.z,xyz);
            if(!std::isfinite(xyz[0])||!std::isfinite(xyz[1])||!std::isfinite(xyz[2])){
                asset->fitted.emplace(mesh,std::vector<Part>{});Reject(*asset,"source-fit produced nonfinite geometry");LogRejection(*asset);return false;
            }
            q.x=xyz[0];q.y=xyz[1];q.z=xyz[2];
        }
        fitted=asset->fitted.emplace(mesh,std::move(parts)).first;
    }
    if(fitted->second.empty())return false;
    ++treeReplacementDraws;
    // Apply placement basis exactly once. Source-fit positions are already in
    // original mesh units, not normalized geometry multiplied by height again.
    const FixMatrix &f=object->matrix;
    GfxMatrix m={f.right.x/65536.f,f.right.y/65536.f,f.right.z/65536.f,0,
        f.up.x/65536.f,f.up.y/65536.f,f.up.z/65536.f,0,
        f.forward.x/65536.f,f.forward.y/65536.f,f.forward.z/65536.f,0,
        f.position.x/65536.f,f.position.y/65536.f,f.position.z/65536.f,1};
    if(!treeLogged){Sys_Log("options: tree sidecar active (source-fitted successful override draw)");treeLogged=true;}
    unsigned phase=((unsigned)object->position.x*1664525u)^(unsigned)object->position.z;
    // Distance falloff (video.tree_wind_radius, world units, default 150, 0 =
    // unlimited): wind amplitude fades from full at 60% of the radius to zero
    // at the radius, where the tree draws STATIC - the per-vertex wind math is
    // skipped entirely. At forest depth most trees sit past the radius, which
    // is where the user-visible 14-22 fps replays spent ~11 ms/frame of wind
    // math (todo: rainy replay slowdown). The fade band avoids a sway pop.
    float windScale=1.f;
    if(batchEye[3]>0.5f){
        int radius=Sys_GetOption("video.tree_wind_radius",150);
        if(radius>0){
            float dx=object->position.x/65536.f-batchEye[0],dy=object->position.y/65536.f-batchEye[1],dz=object->position.z/65536.f-batchEye[2];
            float d=std::sqrt(dx*dx+dy*dy+dz*dz);
            float full=radius*0.6f;
            if(d>=radius)windScale=0.f;
            else if(d>full)windScale=(radius-d)/(radius-full);
        }
    }
    const bool windOn=Sys_GetOption("video.tree_wind",0)!=0&&windScale>0.005f;
    SceneryMotion::WindFrame windFrame={};
    if(windOn){double t0=Sys_GetTicksPrecise();windFrame=SceneryMotion::MakeWindFrame(height,animationTime,phase,Sys_GetOption("video.tree_rustle",100)/100.f);windFrame.ampScale=windScale;timeWindMathNs+=(unsigned long long)((Sys_GetTicksPrecise()-t0)*1e6);}
    if(!windOn){
        FlushTrees(); // keep the recorded order: anything deferred before this tree goes first
        State(m,true);
        for(auto &p:fitted->second){
            treeTrianglesTotal+=p.vertices.size()/3;
            Gfx_SetTexture(0,p.texture->surface);Gfx_DrawPrimitive(GFX_TRIANGLELIST,p.vertices.data(),p.vertices.size());
        }
        Gfx_PopState();return true;
    }
    // Wind: the vertex math of every part is a pure function of the part, the per-tree frame and the cached
    // per-vertex terms. Build the caches here, on the render thread, then either run the math now or defer
    // it to FlushTrees so a batch of trees can be computed on several threads. The draws are always recorded
    // on the render thread, in the order the trees were drawn.
    for(auto &p:fitted->second){
        treeTrianglesTotal+=p.vertices.size()/3;
        if(p.wind.size()!=p.vertices.size()||p.windBottom!=bottom||p.windHeight!=height){
            FlushTrees(); // no pending job may read a cache that is about to change
            double t0=Sys_GetTicksPrecise();
            p.wind.resize(p.vertices.size());
            for(size_t k=0;k<p.vertices.size();k++)p.wind[k]=SceneryMotion::MakeWindVertex(p.vertices[k].u0,p.vertices[k].v0,p.vertices[k].y,bottom,height);
            p.windBottom=bottom;p.windHeight=height;
            timeWindMathNs+=(unsigned long long)((Sys_GetTicksPrecise()-t0)*1e6);
        }
        treeRustledVerticesTotal+=p.vertices.size();
    }
    pendingTrees.Push({m,&fitted->second,windFrame});
    if(!treeBatch)FlushTrees();
    return true;
}

void FlushTrees(){
    if(pendingTrees.Empty())return;
    const size_t batchSize=pendingTrees.Size();
    if(batchSize<8)++windBatchSmall;else ++windBatchLarge;windBatchTrees+=batchSize;
    double t0=Sys_GetTicksPrecise();
    // Workers touch only their own output slot and read the shared, now immutable, part data.
    pendingTrees.ComputeAll(treePool(),[](const PendingTree &t,std::vector<std::vector<GfxVertex>> &out){
        out.resize(t.parts->size());
        for(size_t k=0;k<t.parts->size();k++){
            const Part &p=(*t.parts)[k];out[k].resize(p.vertices.size());
            SceneryMotion::WindPart(p.vertices.data(),out[k].data(),p.wind.data(),p.vertices.size(),t.frame);
        }
    },8); // Keep tiny batches on caller: no worker wake/wait for 1..7 trees.
    double t1=Sys_GetTicksPrecise();
    pendingTrees.RecordAll([](const PendingTree &t,std::vector<std::vector<GfxVertex>> &out){
        State(t.m,true);
        for(size_t k=0;k<t.parts->size();k++){Gfx_SetTexture(0,(*t.parts)[k].texture->surface);Gfx_DrawPrimitive(GFX_TRIANGLELIST,out[k].data(),out[k].size());}
        Gfx_PopState();
    });
    timeWindMathNs+=(unsigned long long)((t1-t0)*1e6);timeSubmitNs+=(unsigned long long)((Sys_GetTicksPrecise()-t1)*1e6);
    // A flush from inside a draw is already inside that draw's timers; one from the batch loop is not.
    unsigned long long dt=(unsigned long long)((Sys_GetTicksPrecise()-t0)*1e6);
    if(replacementDepth==0)timeReplacementNs+=dt;
    if(drawDepth==0)timeOtherNs+=dt;
}
void SceneryPack_BeginBatch(const float *eye){
    treeBatch=true;
    if(eye){batchEye[0]=eye[0];batchEye[1]=eye[1];batchEye[2]=eye[2];batchEye[3]=1.f;}
    else batchEye[3]=0.f;
}
void SceneryPack_BeginBatch(){SceneryPack_BeginBatch(nullptr);}
void SceneryPack_FlushBatch(){FlushTrees();}
void SceneryPack_EndBatch(){
    FlushTrees();treeBatch=false;
    static uint64_t loggedAt=0;static unsigned long long small=0,large=0,trees=0;
    if(animationTime<loggedAt){loggedAt=animationTime;small=windBatchSmall;large=windBatchLarge;trees=windBatchTrees;}
    if(animationTime-loggedAt>=65536ull*25){
        if(windBatchTrees!=trees)Sys_Log("tree wind batches: %llu small(serial), %llu large(pool), %llu trees since last log",windBatchSmall-small,windBatchLarge-large,windBatchTrees-trees);
        loggedAt=animationTime;small=windBatchSmall;large=windBatchLarge;trees=windBatchTrees;
    }
}

bool SceneryPack_GetAnimationStatus(unsigned long long *tick,unsigned *frame,unsigned *changes){
    if(!diagnosticObject)return false;
    if(tick)*tick=(unsigned long long)(diagnosticTime/65536);
    if(frame)*frame=diagnosticFrame;
    if(changes)*changes=diagnosticChanges;
    return true;
}

extern "C" double Sys_GetTicksPrecise(void);
void SceneryPack_GetTimeSplit(unsigned long long *windMathNs,unsigned long long *submitNs){if(windMathNs)*windMathNs=timeWindMathNs;if(submitNs)*submitNs=timeSubmitNs;}
void SceneryPack_GetTimeTotals(unsigned long long *replacementNs,unsigned long long *totalNs,unsigned long long *calls){
 if(replacementNs)*replacementNs=timeReplacementNs;if(totalNs)*totalNs=timeOtherNs;if(calls)*calls=timeCalls;
}
// Diagnostics: wall-clock time spent inside the scenery sidecar draws (whole-tree and card paths), nanoseconds since launch.
// The clock reads are themselves a small cost; this build is for measuring, not for the final default.
struct DrawTimer{double t0=Sys_GetTicksPrecise();DrawTimer(){++drawDepth;}~DrawTimer(){--drawDepth;timeOtherNs+=(unsigned long long)((Sys_GetTicksPrecise()-t0)*1e6);++timeCalls;}};
bool SceneryPack_Draw(StageObject *object,const float *billboard){
    DrawTimer drawTimer;
    {double t0=Sys_GetTicksPrecise();++replacementDepth;bool done=DrawReplacement(object,billboard,CGraphics::m_textureCount,CGraphics::GetTextureBuffer());--replacementDepth;timeReplacementNs+=(unsigned long long)((Sys_GetTicksPrecise()-t0)*1e6);if(done)return true;}
    if(!object||!object->pMesh||!billboard||!Sys_GetOption("video.scenery_overrides",1))return false;
    auto mesh=object->pMesh;
    if(!(mesh->flags&2)||!mesh->pTriangles||mesh->triangleCount<1||mesh->triangleCount>64||!mesh->pVertexData||mesh->field_0x10<3||mesh->field_0x10>64)return false;
    int index=mesh->pTriangles[0].textures[0];if(index<0||(unsigned)index>=CGraphics::m_textureCount)return false;
    for(int i=1;i<mesh->triangleCount;i++)if(mesh->pTriangles[i].textures[0]!=index)return false;
    auto source=CGraphics::GetTextureBuffer()[index];if(!source||!source->pSurface)return false;
    Init();auto stem=Stem(source->name);auto wind=windIdentities.find(stem);
    if(wind==windIdentities.end()){
        if(stem.compare(0,4,"tre_")==0&&treeOriginalLogged.insert(stem).second)Sys_Log("options: tree original %s: not covered by verified replacement/wind routes",stem.c_str());
        return false;
    }
    auto id=sourceIdentities.find(source);
    if(!Sys_GetOption("video.tree_wind",0)||id==sourceIdentities.end()||!wind->second.count(id->second))return false;
    auto vertices=(MeshVertexF*)mesh->pVertexData;float bottom=vertices[0].y,top=bottom;
    for(int i=0;i<mesh->field_0x10;i++){bottom=std::min(bottom,vertices[i].y);top=std::max(top,vertices[i].y);}
    float height=top-bottom;if(!std::isfinite(height)||height<=0)return false;
    float phase=((unsigned)object->position.x^(unsigned)object->position.z)%6283*.001f;

    std::vector<GfxVertex> out;
    for(int i=0;i<mesh->triangleCount;i++)for(int j=0;j<3;j++){
        unsigned ix=mesh->pTriangles[i].vertexIndex[j];if(ix>=(unsigned)mesh->field_0x10)return false;
        auto &v=vertices[ix];float motion=SceneryMotion::Offset(v.y,bottom,height,animationTime,(unsigned)(phase*1000),true);
        out.push_back({v.x+motion,v.y,v.z,v.nx,v.ny,v.nz,v.diffuseColour,v.specularColour,v.textureCoordinates[0][0],v.textureCoordinates[0][1],0,0});
    }
    FlushTrees();State(*(const GfxMatrix*)billboard);Gfx_SetTexture(0,source->pSurface);Gfx_DrawPrimitive(GFX_TRIANGLELIST,out.data(),out.size());Gfx_PopState();
    ++treeWindDraws;
    if(windLogged.insert(stem).second)Sys_Log("options: tree wind active %s (original2Dcard, fixed base; NOT3Dreplacement)",stem.c_str());
    return true;
}

void SceneryPack_GetFrameTotals(unsigned long long *crowdDraws,unsigned long long *crowdTris,unsigned long long *treeTris,unsigned long long *treeUvVertices){
 if(crowdDraws)*crowdDraws=crowdDrawsTotal;if(crowdTris)*crowdTris=crowdTrianglesTotal;if(treeTris)*treeTris=treeTrianglesTotal;if(treeUvVertices)*treeUvVertices=treeRustledVerticesTotal;
}

void SceneryPack_GetTreeStatus(unsigned *replacements,unsigned *wind,unsigned *originalSources){
 if(replacements)*replacements=treeReplacementDraws;
 if(wind)*wind=treeWindDraws;
 if(originalSources)*originalSources=(unsigned)treeOriginalLogged.size();
}

// Called AFTER the original material/state binding. No world transform, material,
// culling, lighting, alpha mode, collision pointer or source vertex is changed.
bool SceneryPack_DrawFixedPart(Mesh *mesh,MeshPart *part){
 if(!mesh||!part||!Sys_GetOption("video.scenery_overrides",1)||!Sys_GetOption("video.tree_overrides",1)||
    !Sys_GetOption("video.texture_uprez",0)||!(mesh->flags&0x800)||mesh->flags&2||part->texture<0||unsigned(part->texture)>=CGraphics::m_textureCount)return false;
 Texture*source=CGraphics::GetTextureBuffer()[part->texture];if(!source||!source->pSurface||!mesh->pVertexData||!part->pData)return false;
 auto name=Stem(source->name);if(name.compare(0,5,"tresh")&&name.compare(0,6,"tre_w2")&&name.compare(0,6,"tre_wl")&&name.compare(0,7,"bsh_st1")&&name.compare(0,7,"bsh_sh1")&&name.compare(0,6,"tre_s1"))return false;
 // DDS identities are added by the private texture helper, independently of the
 // individual billboard pack. A same-named modified source never enters here.
 if(!France1Textures_IsVerifiedCutout(source))return false;
 // Greece1 brush walls (gre01 temp.sht stage-object bsh_sh1* parts): the flat card gets
 // draw-time depth - interior alpha relief plus a taller darkened back layer
 // ("skybreak"). Exact-source only: part hash + verified cutout + live dial.
 if(!name.compare(0,7,"bsh_sh1")){
  int reliefDial=Sys_GetOption("video.brush_relief",100);
  if(reliefDial>0){
   GfxMatrix wmat;Gfx_GetTransform(GFX_TRANSFORM_WORLD,&wmat);const float*W=&wmat._11;
   auto bfound=brushParts.find(part);
   if(bfound!=brushParts.end()&&Sys_GetOption("video.brush_volume",0)!=0&&memcmp(bfound->second.world,W,sizeof(float)*16)!=0){brushParts.erase(bfound);bfound=brushParts.end();}   // shared mesh drawn under another node: rebuild for this placement
   if(bfound==brushParts.end()){
    BrushPart data;
    if(part->indexCount<6||part->indexCount>12000||part->minIndex<0)return false;
    auto*v=(const MeshVertexF*)mesh->pVertexData;
    std::vector<unsigned short>indices;std::string original;
    for(int i=0;i<part->indexCount;i++){
     unsigned j=unsigned(part->pData[i])+part->minIndex;if(j>=unsigned(mesh->field_0x10))return false;
     indices.push_back(j);original.append((const char*)&v[j].x,12);original.append((const char*)v[j].textureCoordinates[0],8);
    }
    auto hash=SceneryIdentity::SHA256(original.data(),original.size());bool known=false;for(auto h:gre01BrushSources)if(hash==h){known=true;break;}
    if(known){
     // One flat quad (2 triangles, 4 corners in UV-corner order) per part.
     unsigned uniq[4];int used=0;
     for(auto j:indices){bool f=false;for(int k=0;k<used;k++)if(uniq[k]==j)f=true;if(!f){if(used==4){used=-1;break;}uniq[used++]=j;}}
     if(used==4){
      MeshVertexF c[4];bool ok=true;
      for(int k=0;k<4;k++){
       float tu=v[uniq[k]].textureCoordinates[0][0],tv=v[uniq[k]].textureCoordinates[0][1];
       if((tu!=0&&tu!=1)||(tv!=0&&tv!=1)){ok=false;break;}
       c[(int(tu)+2*int(tv))]=v[uniq[k]];   // slot by UV corner
      }
      if(ok){
       float e1[3]={c[1].x-c[0].x,c[1].y-c[0].y,c[1].z-c[0].z},e2[3]={c[2].x-c[0].x,c[2].y-c[0].y,c[2].z-c[0].z};
       float n[3]={e1[1]*e2[2]-e1[2]*e2[1],e1[2]*e2[0]-e1[0]*e2[2],e1[0]*e2[1]-e1[1]*e2[0]};
       float nl=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
       const unsigned char*alpha=nullptr;int aw=0,ah=0;
       if(nl>1e-5f&&France1Textures_CutoutAlpha(source,&alpha,&aw,&ah)){
        n[0]/=nl;n[1]/=nl;n[2]/=nl;
        // Road-facing edge stays fixed; all depth (relief, back card, volume
        // slices) goes AWAY from the route. Side comes from the stage route
        // polyline, never from the quad's winding. No route match = no volume.
        static const RallyRouteNode*routeKey=nullptr;static int routeKeyCount=0;static std::vector<float>routeXZ;
        if(g_routeNodes&&g_routeNodeCount>0&&(routeKey!=g_routeNodes||routeKeyCount!=g_routeNodeCount)){
         routeXZ.clear();for(int i=0;i<g_routeNodeCount;i++){routeXZ.push_back(g_routeNodes[i].position.x/65536.f);routeXZ.push_back(g_routeNodes[i].position.z/65536.f);}
         routeKey=g_routeNodes;routeKeyCount=g_routeNodeCount;
        }
        static int volumeLogs=0;
        bool volumeOn=Sys_GetOption("video.brush_volume",0)!=0;   // experimental, off by default
        float qaL[3]={c[0].x,c[0].y,c[0].z},qbL[3]={c[1].x,c[1].y,c[1].z},qa[3],qb[3],nW[3],awayW[3]={0,0,0},away[3]={0,0,0};
        bool rigid=BrushRelief::RigidWorld(W);
        bool routeOk=false;
        if(volumeOn&&rigid&&g_routeNodes&&g_routeNodeCount>0){
         // Wall quads are object-local: take them to world space (the route's space) first.
         BrushRelief::PointToWorld(W,qaL,qa);BrushRelief::PointToWorld(W,qbL,qb);BrushRelief::DirToWorld(W,n,nW);
         routeOk=BrushRelief::AwayFromRoad(qa,qb,nW,routeXZ.data(),g_routeNodeCount,60.f,awayW);
         if(routeOk)BrushRelief::DirToLocal(W,awayW,away);   // back to local so Build() offsets are along away-from-road
        }
        static unsigned extruded=0,declined=0;
        if(volumeOn&&volumeLogs<4){volumeLogs++;Sys_Log("scenery: gre01 brush wall local mid (%.1f,%.1f,%.1f) world mid (%.1f,%.1f) rigid=%d world normal (%.2f,%.2f) away world (%.2f,%.2f) -> %s",(qaL[0]+qbL[0])*.5f,(qaL[1]+qbL[1])*.5f,(qaL[2]+qbL[2])*.5f,(qa[0]+qb[0])*.5f,(qa[2]+qb[2])*.5f,(int)rigid,nW[0],nW[2],awayW[0],awayW[2],routeOk?"extruded":"declined");}
        if(routeOk){
         data.ready=BrushRelief::Build(c,away,alpha,aw,ah,reliefDial,true,data.vertices,BrushRelief::kVolumeSlices);
         BrushRelief::Indices(true,data.indices,BrushRelief::kVolumeSlices);
         for(int k=0;k<4;k++)data.corners[k]=c[k];memcpy(data.world,W,sizeof(float)*16);
         if(++extruded==1)Sys_Log("scenery: gre01 brush volume active (road-facing edge fixed, extruded away from route; visual only)");
        }else if(!volumeOn&&Sys_GetOption("video.brush_relief_legacy",0)!=0){   // legacy relief + back card along the UNVERIFIED quad-winding normal (may point toward the road); opt-in only
         data.ready=BrushRelief::Build(c,n,alpha,aw,ah,reliefDial,true,data.vertices);
         BrushRelief::Indices(true,data.indices);
         for(int k=0;k<4;k++)data.corners[k]=c[k];
        }else if(!volumeOn){/* default: original flat wall, nothing drawn differently */}else if(++declined==1)Sys_Log("scenery: gre01 brush volume declined for a wall (no route nodes, none within 60 units, wall not vertical, ambiguous side, or non-rigid world matrix; %d route nodes); keeping the flat original",g_routeNodeCount);
       }
      }
     }
    }
    memcpy(data.world,W,sizeof(float)*16);
    bfound=brushParts.emplace(part,std::move(data)).first;
   }
   auto&bdata=bfound->second;
   if(bdata.ready){
    bdata.scratch.resize(bdata.indices.size());
    auto*live=(const MeshVertexF*)mesh->pVertexData;(void)live;
    for(size_t i=0;i<bdata.indices.size();i++){
     auto&q=bdata.vertices[bdata.indices[i]];
     // Live diffuse bilinear from the quad's four corner vertices (runtime
     // lighting/fog rewrites original colours; read them live, FixedStrip pattern).
     float r=0,g=0,b=0,a=0;
     const MeshVertexF*corners=bdata.corners;
     float wC[4]={q.bary[0],q.bary[1],(1-q.u)*q.bary[2],q.u*q.bary[2]};
     for(int k=0;k<4;k++){
      auto*cb=(const unsigned char*)&corners[k].diffuseColour;
      b+=cb[0]*wC[k];g+=cb[1]*wC[k];r+=cb[2]*wC[k];a+=cb[3]*wC[k];
     }
     r*=q.shade;g*=q.shade;b*=q.shade;
     DWORD col=((DWORD)(a+.5f)<<24)|((DWORD)(r+.5f)<<16)|((DWORD)(g+.5f)<<8)|(DWORD)(b+.5f);
     bdata.scratch[i]={q.x,q.y,q.z,0,1,0,col,0,q.u,q.v,0,0};
    }
    Gfx_DrawPrimitive(GFX_TRIANGLELIST,bdata.scratch.data(),bdata.scratch.size());
    return true;
   }
  }
 }
 auto found=fixedParts.find(part);
 if(found==fixedParts.end()){
  FixedPart data;std::vector<unsigned short>indices;std::string original;
  if(part->indexCount<6||part->indexCount>12000||part->minIndex<0)return false;
  auto*v=(const MeshVertexF*)mesh->pVertexData;
  for(int i=0;i<part->indexCount;i++){
   unsigned j=unsigned(part->pData[i])+part->minIndex;if(j>=unsigned(mesh->field_0x10))return false;
   indices.push_back(j);original.append((const char*)&v[j].x,12);original.append((const char*)v[j].textureCoordinates[0],8);
  }
  auto hash=SceneryIdentity::SHA256(original.data(),original.size());bool known=false;for(auto h:fixedStripSources)if(hash==h){known=true;break;}
  if(known)data.ready=FixedStrip::Build(v,mesh->field_0x10,indices.data(),indices.size(),data.vertices,name.compare(0,5,"tresh")!=0);
  if(data.ready&&fixedParts.empty())Sys_Log("scenery: fixed source strip relief active %s (%zu triangles in first part); visual only, not individual3Dtrees",name.c_str(),data.vertices.size()/3);
  found=fixedParts.emplace(part,std::move(data)).first;
 }
 auto&data=found->second;if(!data.ready)return false;
 data.scratch.resize(data.vertices.size());float wind=Sys_GetOption("video.tree_wind",0)?std::sin(float(animationTime%(65536ull*25*3600))/(65536.f*25.f)*1.2f)*.05f:0;
 for(size_t i=0;i<data.vertices.size();i++){auto&q=data.vertices[i];auto v=q.source;
  // Runtime lighting/fog updates rewrite original colours. Read them live.
  auto*live=(const MeshVertexF*)mesh->pVertexData;
  for(int colour=0;colour<2;colour++){auto*dst=(unsigned char*)&v.diffuseColour+colour*4;for(int k=0;k<4;k++){float value=0;for(int b=0;b<3;b++)value+=((const unsigned char*)&live[q.indices[b]].diffuseColour)[colour*4+k]*q.bary[b];dst[k]=(unsigned char)(value+.5f);}}
 data.scratch[i]={std::max(q.lo[0],std::min(q.hi[0],v.x+wind*q.weight)),v.y,v.z,v.nx,v.ny,v.nz,v.diffuseColour,v.specularColour,v.textureCoordinates[0][0],v.textureCoordinates[0][1],v.textureCoordinates[1][0],v.textureCoordinates[1][1]};}
 Gfx_DrawPrimitive(GFX_TRIANGLELIST,data.scratch.data(),data.scratch.size());return true;
}
