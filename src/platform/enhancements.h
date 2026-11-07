// Optional enhancements are per launch. Never save selections into the profile.
#ifndef OPENCMR2_ENHANCEMENTS_H
#define OPENCMR2_ENHANCEMENTS_H
#include <string>
#include <map>
#include <cstdlib>
#include <utility>
struct EnhancementOptions {
    std::string captureDir;int captureInterval=5,captureCount=7;
    std::string replayExportDir;   // --replay-export: fixed-step PNG+WAV dump of the first replay played, MP4 when ffmpeg is on PATH
    std::string trees, spectators, names, cars, modsDir, carNames, roads, textures;
    bool installDependencies=false,prepareOnly=false;
    std::string treeCoverage;int spectatorGain=100;   // experimental pack options: --tree-coverage reviewed|all, --spectator-gain 100..400 (percent)
    std::string testPreset;
    std::map<std::string,int> overrides;
    bool Consume(int &i,int argc,char **argv,std::string &error) {
        std::string key=argv[i];
        auto equal=key.find('=');
        std::string inlineValue;
        if(equal!=key.npos){inlineValue=key.substr(equal+1);key.resize(equal);}
        if(equal==key.npos && key=="--asset-upgrades"){trees="shell";treeCoverage="all";spectators="animated";textures="uprez";roads="finland1";return true;}
        if(equal==key.npos && key=="--prepare-only"){prepareOnly=true;return true;}
        if(equal==key.npos && key=="--install-enhancement-deps"){installDependencies=true;return true;}
        if(key!="--camera-roll"&&key!="--sun-glare"&&key!="--textures"&&key!="--roads"&&key!="--tree-wind"&&key!="--car-names"&&key!="--trees"&&key!="--spectators"&&key!="--driver-names"&&key!="--cars"&&key!="--mods-dir"&&key!="--effect"&&key!="--attract-capture"&&key!="--capture-interval"&&key!="--capture-count"&&key!="--speed-fov"&&key!="--tunnel-vision"&&key!="--tunnel-lines"&&key!="--speed-motes-stretch"&&key!="--speed-motes-amount"&&key!="--speed-motes-size"&&key!="--dof-blur-max-px"&&key!="--speed-streak"&&key!="--speed-hold"&&key!="--tree-coverage"&&key!="--spectator-gain"&&key!="--cockpit-eye-shift"&&key!="--tree-coverage"&&key!="--spectator-gain"&&key!="--camera-move-chase"&&key!="--camera-move-cockpit"&&key!="--camera-move-car"&&key!="--fov-dolly-curve"&&key!="--tire-debris"&&key!="--tree-rustle"&&key!="--spectator-irregular"&&key!="--tire-track-length"&&key!="--tire-track-history"&&key!="--camera-move-type"&&key!="--fov-preview"&&key!="--dof-focus-car"&&key!="--drone-speed-drop"&&key!="--drone-distance"&&key!="--speed-fov-cockpit"&&key!="--cockpit-blur-clear"&&key!="--test-preset"&&key!="--fog-amount"&&key!="--fog-glow"&&key!="--fog-clear"&&key!="--fog-lights"&&key!="--fog-cull"&&key!="--fog-probe"&&key!="--fog-spatial"&&key!="--brush-volume"&&key!="--fog-evolve"&&key!="--air-motes-amount"&&key!="--snow-glint"&&key!="--cockpit-door-extend"&&key!="--cockpit-extend-rear"&&key!="--cockpit-extend-probe"&&key!="--dump-interior"&&key!="--cockpit-mesh"&&key!="--windshield-crack-overlay"&&key!="--windshield-dirt-fade-cracked"&&key!="--tunnel-track"&&key!="--diag-replay-glass"&&key!="--replay-export")return false;
        if(equal==std::string::npos && i+1>=argc){error="Missing value for "+key;return true;}
        std::string value=equal==std::string::npos?argv[++i]:inlineValue;
        if(key=="--camera-roll"){char*end=nullptr;long n=strtol(value.c_str(),&end,10);if(value.empty()||*end||n<0||n>100)error="--camera-roll expects 0..100";else overrides["video.camera_roll"]=(int)n;}
        if(key=="--sun-glare") {char*end=nullptr;long n=strtol(value.c_str(),&end,10);if(value.empty()||*end||n<0||n>100)error="--sun-glare expects0..100";else overrides["video.sun_glare"]=(int)n;}
        if(key=="--textures"){if(value!="original"&&value!="france1"&&value!="uprez")error="--textures expects original, france1 or uprez";else textures=value;}
        if(key=="--roads"){if(value!="original"&&value!="curved"&&value!="finland1")error="--roads expects original, curved or finland1";else roads=value;}
        if(key=="--tree-wind"){if(value!="on"&&value!="off")error="--tree-wind expects on or off";else overrides["video.tree_wind"]=value=="on";}
        if(key=="--car-names"){if(value!="original"&&value!="skin")error="--car-names expects original or skin";else carNames=value;}
        if(key=="--trees") {if(value!="original"&&value!="shell"&&value!="crossed")error="--trees expects original, shell or crossed";else trees=value;}
        if(key=="--tree-coverage"){if(value!="reviewed"&&value!="all")error="--tree-coverage expects reviewed or all";else treeCoverage=value;}
        if(key=="--spectator-gain"){char *end=nullptr;long n=strtol(value.c_str(),&end,10);if(value.empty()||*end||n<100||n>400)error="--spectator-gain expects 100..400";else spectatorGain=(int)n;}
        if(key=="--spectators"){if(value!="original"&&value!="animated")error="--spectators expects original or animated";else spectators=value;}
        if(key=="--driver-names"){if(value!="original"&&value!="real"&&value!="sidecar")error="--driver-names expects original, real or sidecar";else names=value;}
        if(key=="--effect"){
            auto at=value.find('=');std::string name=value.substr(0,at),v=at==value.npos?"":value.substr(at+1);
            static const char *allowed[]={"dust_plume","body_dirt","dust_volume","volumetric_clouds","spatial_aa","metalfx_spatial","motion_blur","bloom","particles","shadows","depth_of_field","god_rays","color_grading","temporal_aa","speed_motes","debug_object_mask","vapor_trails","speed_lines","tunnel_lines","fog","air_motes"};
            bool known=false;for(auto key:allowed)if(name==key)known=true;
            if(!known||(v!="on"&&v!="off"))error="--effect expects a listed NAME=on/off";
            else overrides["video."+name]=v=="on";
        }
        if(key=="--speed-fov"||key=="--speed-motes-stretch"||key=="--speed-motes-amount"||key=="--speed-motes-size"||key=="--dof-blur-max-px"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            int hi=key=="--speed-fov"?500:key=="--speed-motes-stretch"?100:key=="--dof-blur-max-px"?40:key=="--speed-motes-size"?800:300,lo=key=="--dof-blur-max-px"?1:key=="--speed-motes-size"?50:0;
            if(value.empty()||*end||n<lo||n>hi)error=key+" expects "+std::to_string(lo)+".."+std::to_string(hi);
            else overrides[key=="--speed-fov"?"video.speed_fov":key=="--speed-motes-stretch"?"video.speed_motes_stretch":key=="--dof-blur-max-px"?"video.dof_blur_max_px":key=="--speed-motes-size"?"video.speed_motes_size":"video.speed_motes_amount"]=(int)n;
        }
        if(key=="--cockpit-eye-shift"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);   // signed: negative moves the eye the other way
            if(value.empty()||*end||n<-100||n>100)error=key+" expects -100..100";else overrides["video.cockpit_eye_shift"]=(int)n;
        }
        if(key=="--camera-move-type"){   // N=V, N one of the camera types 1 2 3 4 5 6 10, V -200..200 (0.05 model unit steps at full speed); replaces the family value for that type when not 0
            auto eq=value.find('=');int n=-1;long v=0;bool ok=eq!=std::string::npos;
            if(ok){char *end=nullptr;n=(int)strtol(value.substr(0,eq).c_str(),&end,10);ok=!*end&&(n==1||n==2||n==3||n==4||n==5||n==6||n==10);std::string vs=value.substr(eq+1);char *e2=nullptr;v=strtol(vs.c_str(),&e2,10);ok=ok&&!vs.empty()&&!*e2&&v>=-200&&v<=200;}
            if(!ok)error="--camera-move-type expects TYPE=VALUE, TYPE 1 2 3 4 5 6 or 10, VALUE -200..200";else overrides["video.camera_move_type"+std::to_string(n)]=(int)v;
        }
        if(key=="--tire-track-history"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||n<0||n>64)error="--tire-track-history expects 0..64";else overrides["video.tire_track_history"]=(int)n;
        }
        if(key=="--tire-track-length"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||n<0||n>1000)error="--tire-track-length expects 0..1000";else overrides["video.tire_track_length"]=(int)n;
        }
        if(key=="--tree-rustle"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||n<0||n>300)error="--tree-rustle expects 0..300";else overrides["video.tree_rustle"]=(int)n;
        }
        if(key=="--spectator-irregular"){
            if(value!="0"&&value!="1")error=key+" expects 0 or 1";else overrides["video.spectator_irregular"]=value=="1"?1:0;
        }
        if(key=="--tire-debris"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||n<0||n>2000)error="--tire-debris expects 0..2000";else overrides["video.tire_debris"]=(int)n;
        }
        if(key=="--drone-speed-drop"||key=="--drone-distance"||key=="--speed-fov-cockpit"||key=="--cockpit-blur-clear"||key=="--tunnel-vision"||key=="--tunnel-lines"){   // wins over opencmr2.ini, so a value saved there earlier cannot hide the default
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            int lo=key=="--drone-distance"?50:0,hi=key=="--drone-speed-drop"?200:key=="--drone-distance"?150:key=="--speed-fov-cockpit"?100:key=="--tunnel-vision"?200:key=="--tunnel-lines"?1:60;
            if(value.empty()||*end||n<lo||n>hi)error=key+" expects "+std::to_string(lo)+".."+std::to_string(hi);
            else overrides[key=="--drone-speed-drop"?"video.drone_speed_drop":key=="--drone-distance"?"video.drone_distance":key=="--speed-fov-cockpit"?"video.speed_fov_cockpit":key=="--tunnel-vision"?"video.tunnel_vision":key=="--tunnel-lines"?"video.tunnel_lines":"video.cockpit_blur_clear"]=(int)n;
        }
        if(key=="--fov-dolly-curve"){
            if(value!="0"&&value!="1")error="--fov-dolly-curve expects 0 or 1";else overrides["video.fov_dolly_curve"]=value=="1"?1:0;
        }
        if(key=="--camera-move-chase"||key=="--camera-move-cockpit"||key=="--camera-move-car"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);   // signed: positive forward, negative back, 0.05 model units per step
            if(value.empty()||*end||n<-200||n>200)error=key+" expects -200..200";else overrides[key=="--camera-move-chase"?"video.camera_move_chase":key=="--camera-move-cockpit"?"video.camera_move_cockpit":"video.camera_move_car"]=(int)n;
        }
        if(key=="--speed-hold"||key=="--dof-focus-car"){
            // 0/1. --speed-hold 0 makes --test-preset overdo run its speed effects (FOV, tunnel, streak) at the real car speed instead of
            // pretending full speed; --dof-focus-car 0 keeps the fixed focus plane under the preset. An explicit flag always wins.
            if(value!="0"&&value!="1")error=key+" expects 0 or 1";else overrides[key=="--speed-hold"?"video.speed_fov_hold":"video.dof_focus_car"]=value=="1"?1:0;
        }
        if(key=="--fov-preview"){
            // Debug view of the absolute video.fov number (legacy scale, higher = wider; not a measured angle): 0 = off, else 20..150.
            // Per launch only. Speed FOV % is a different, relative setting.
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||(n!=0&&(n<20||n>150)))error="--fov-preview expects 0 (off) or 20..150";else overrides["video.fov_preview"]=(int)n;
        }
        if(key=="--speed-streak"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||n<0||n>100)error="--speed-streak expects 0..100";else overrides["video.speed_streak"]=(int)n;
        }
        if(key=="--fog-cull"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||n<0||n>1)error="--fog-cull expects 0 or 1";else overrides["video.fog_cull"]=(int)n;
        }
        if(key=="--fog-probe"){
            // One-shot fog depth readback diagnostic (video.fog_probe, SDL_GPU only); see docs/ATMOSPHERE.md.
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||n<0||n>1)error="--fog-probe expects 0 or 1";else overrides["video.fog_probe"]=(int)n;
        }
        if(key=="--brush-volume"){   // EXPERIMENTAL Greece1 brush-wall extrusion (visual only, away from the road, collision untouched); default 0
            if(value!="0"&&value!="1")error="--brush-volume expects 0 or 1";else overrides["video.brush_volume"]=value=="1"?1:0;
        }
        if(key=="--fog-spatial"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||n<0||n>1)error="--fog-spatial expects 0 or 1";else overrides["video.fog_spatial"]=(int)n;
        }
        if(key=="--fog-evolve"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            // signed on the command line (negative clears toward the finish), stored 0..200 neutral 100
            if(value.empty()||*end||n<-100||n>100)error="--fog-evolve expects -100..100 (negative clears, positive thickens)";else overrides["video.fog_evolve"]=(int)n+100;
        }
        if(key=="--fog-amount"||key=="--fog-glow"||key=="--fog-clear"||key=="--fog-lights"||key=="--air-motes-amount"||key=="--snow-glint"||key=="--snow-material"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            int hi=key=="--fog-glow"?100:key=="--air-motes-amount"?500:200;
            if(value.empty()||*end||n<0||n>hi)error=key+" expects 0.."+std::to_string(hi);
            else overrides[key=="--fog-amount"?"video.fog_amount":key=="--fog-clear"?"video.fog_clear":key=="--fog-lights"?"video.fog_lights":key=="--fog-glow"?"video.fog_glow":key=="--snow-glint"?"video.snow_glint":key=="--snow-material"?"video.snow_material":"video.air_motes_amount"]=(int)n;
        }
        if(key=="--cockpit-door-extend"){
            // OpenCMR2 prototype: extends the Subaru cockpit door card rearward by N/100 model units (0 = off, default).
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||n<0||n>80)error="--cockpit-door-extend expects 0..80 (hundredths of a model unit, 0 = off)";
            else overrides["video.cockpit_door_extend"]=(int)n;
        }
        if(key=="--cockpit-extend-rear"){
            // OpenCMR2 prototype: draws door-card and floor strips continuing the Subaru cockpit rearward by N/100 model units (0 = off, default).
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);
            if(value.empty()||*end||n<0||n>150)error="--cockpit-extend-rear expects 0..150 (hundredths of a model unit, 0 = off)";
            else overrides["video.cockpit_extend_rear"]=(int)n;
        }
        if(key=="--cockpit-extend-probe"){
            // One-shot visibility diagnostic for the cockpit extension (video.cockpit_extend_probe); log only.
            if(value!="0"&&value!="1")error="--cockpit-extend-probe expects 0 or 1";else overrides["video.cockpit_extend_probe"]=value=="1"?1:0;
        }
        if(key=="--dump-interior"){
            // One-shot: write the car's cockpit model as <user dir>/interior_dump.obj (scaffold for modelling add-on pieces). 0 = off, default.
            if(value!="0"&&value!="1")error="--dump-interior expects 0 or 1";
            else overrides["video.dump_interior"]=value=="1"?1:0;
        }
        if(key=="--cockpit-mesh"){
            // Draw the add-on mesh <user dir>/cockpit_mesh.obj inside the cockpit (hot reloaded on file change). 0 = off, default.
            if(value!="0"&&value!="1")error="--cockpit-mesh expects 0 or 1";
            else overrides["video.cockpit_mesh"]=value=="1"?1:0;
        }
        if(key=="--diag-replay-glass"){
            // Diagnostic (default 0): in replays, log every glass/part break-flag transition and the
            // saved-damage restore at replay init (Sys_Log), to trace "glass broken from 0:00".
            if(value!="0"&&value!="1")error="--diag-replay-glass expects 0 or 1";
            else overrides["video.diag_replay_glass"]=value=="1"?1:0;
        }
        if(key=="--windshield-dirt-fade-cracked"){
            // Dim the windshield dirt film to half strength on cracked (state 1) glass. 0 = off, default (full dirt on cracked glass).
            if(value!="0"&&value!="1")error="--windshield-dirt-fade-cracked expects 0 or 1";
            else overrides["video.windshield_dirt_fade_cracked"]=value=="1"?1:0;
        }
        if(key=="--windshield-crack-overlay"){
            // OpenCMR2 prototype: draw the front window's damaged/broken glass texture over cockpits with no interior glass part (0 = off, default).
            if(value!="0"&&value!="1")error="--windshield-crack-overlay expects 0 or 1";
            else overrides["video.windshield_crack_overlay"]=value=="1"?1:0;
        }
        if(key=="--tunnel-track"){
            // OpenCMR2 (default 1): in the cockpit view the tunnel-vision clear zone and the
            // speed-streak/lines centre follow the stage route ahead (eased, clamped to the
            // central frame half). 0 = the old fixed screen-centre behaviour.
            if(value!="0"&&value!="1")error="--tunnel-track expects 0 or 1";
            else overrides["video.tunnel_track"]=value=="1"?1:0;
        }
        if(key=="--test-preset"){
            bool ok=!value.empty();size_t begin=0;int parts=0;
            for(size_t i=0;ok&&i<=value.size();++i)if(i==value.size()||value[i]==','){
                std::string part=value.substr(begin,i-begin);begin=i+1;++parts;
                if(part!="overdo"&&part!="atmosphere"&&part!="off")ok=false;
                else if(part=="off"&&(parts>1||i<value.size()))ok=false;
            }
            if(!ok)error="--test-preset expects overdo, atmosphere or off (comma-separated to combine)";else testPreset=value;
        }
        if(key=="--attract-capture"){if(value.empty())error="--attract-capture needs an output folder";else captureDir=value;}
        if(key=="--replay-export"){
            // OpenCMR2: export the first replay played after launch as fixed-step PNG frames + a WAV of the
            // mixed game audio (25 sim steps/sec, one frame and one audio slice per step), then assemble
            // replay.mp4 with ffmpeg when it is on PATH (the exact command is always written to the folder).
            if(value.empty())error="--replay-export needs an output folder";else replayExportDir=value;
        }
        if(key=="--capture-interval"||key=="--capture-count"){
            char *end=nullptr;long n=strtol(value.c_str(),&end,10);bool interval=key=="--capture-interval";
            if(value.empty()||*end||n<1||n>(interval?60:120))error=key+(interval?" expects 1..60 seconds":" expects 1..120 shots");
            else (interval?captureInterval:captureCount)=(int)n;
        }
        if(key=="--cars"){if(value.empty())error="--cars needs a nonempty alternative list";else cars=value;}
        if(key=="--mods-dir"){if(value.empty())error="--mods-dir needs a folder";else modsDir=value;}

        return true;
    }
    // Exaggerated diagnostic look (per launch, nothing saved). Explicit flags always win.
    void ApplyTestPreset(){
        size_t begin=0;
        for(size_t i=0;i<=testPreset.size();++i)if(i==testPreset.size()||testPreset[i]==','){
            std::string part=testPreset.substr(begin,i-begin);begin=i+1;
            if(part=="overdo"){
                static const std::pair<const char*,int> kOverdo[]={{"video.speed_motes",0},
                    {"video.speed_fov",100},{"video.speed_fov_hold",1},{"video.depth_of_field",0},{"video.dof_blur",100},{"video.dof_blur_max_px",16},{"video.tunnel_vision",130},{"video.tunnel_lines",1},{"video.motion_blur",0},{"video.motion_blur_amount",60},{"video.speed_streak",100},{"video.dof_focus_car",1}};
                for(auto&e:kOverdo)if(!overrides.count(e.first))overrides[e.first]=e.second;
            } else if(part=="atmosphere"){
                // The requested exaggerated atmosphere starting look, separate from overdo so
                // --all-mods keeps its established appearance. Per launch, nothing saved.
                static const std::pair<const char*,int> kAtmosphere[]={
                    {"video.fog",1},{"video.fog_amount",65},{"video.fog_glow",100},{"video.fog_clear",60},{"video.fog_lights",40},{"video.fog_spatial",1},{"video.fog_evolve",60},{"video.air_motes",1},{"video.air_motes_amount",500},{"video.snow_glint",100},
                    {"video.god_rays",1},{"video.god_rays_amount",80}};
                for(auto&e:kAtmosphere)if(!overrides.count(e.first))overrides[e.first]=e.second;
            }
        }
    }
    void Apply(){
        if(!textures.empty())overrides["video.texture_uprez"]=textures=="uprez";
        ApplyTestPreset();
        if(!trees.empty()&&!overrides.count("video.tree_wind"))overrides["video.tree_wind"]=trees!="original";
        if(!carNames.empty())overrides["video.skin_car_names"]=carNames=="skin";
        // A generated pack contains both kinds; activate only requested kinds.
        if(NeedsPack()&&trees.empty())overrides["video.tree_overrides"]=0;
        if(NeedsPack()&&spectators.empty())overrides["video.spectator_overrides"]=0;
        if(!trees.empty())overrides["video.tree_overrides"]=trees!="original";
        if(!spectators.empty())overrides["video.spectator_overrides"]=spectators!="original";
        if(trees=="shell"||trees=="crossed"||spectators=="animated")overrides["video.scenery_overrides"]=1;
        if(!names.empty()){overrides["video.driver_name_overrides"]=names!="original";overrides["video.real_driver_names"]=names=="real";}
    }
    bool NeedsPack()const{return trees=="shell"||trees=="crossed"||spectators=="animated";}
};
inline const char *EnhancementHelp(){return
"OpenCMR2 options (per launch, no saved selection changes):\n"
"  Valued enhancement flags accept --name value or --name=value.\n"
"  --asset-upgrades            Opt-in broader trees/bushes + 2Dcrowds + ground2x + Finland1 roads\n"
"  --data PATH                 Owned installed PC game data\n"
"  --user-dir PATH             Explicit DIFFERENT saves/settings profile\n"
"  --scenery-dir PATH          Existing read-only sidecar pack\n"
"  --trees original|shell|crossed\n"
"                              Prepare/cache selected experimental tree mode\n"
"                              Only3high-source mappings; shell is tiered;\n"
"                              crossed is fixed cutouts, not full3Dtrees\n"
"  --tree-coverage reviewed|all Experimental: all 64 individual trees + 10 bushes, adaptive crown/stem\n"
"                              (default reviewed = the 3 mappings); needs --trees shell|crossed; native unverified\n"
"  --spectator-gain 100..400   Experimental: percent of spectator sway/arm swing (default 100); needs --spectators animated\n"
"  --textures original|france1|uprez  Exact-source opaque ground2x filter (uprez: Finland1 + France1)\n"
"                              uprez: Finland1/France1 ground + forest/hedge/fence cutout2x; no new detail\n"
"                              Other stages stay original; native unverified\n"
"  --roads original|curved|finland1      finland1: visual-only curves, original physics/route retained\n"
"                              Exact original source only; others stay original\n"
"                              curved: legacy Ballon paired research; FIN unchanged; native unverified\n"
"                              Requires Python3.10+ with numpy; no auto download\n"
"  --tree-wind on|off          2D foliage sway, fixed bases; default on with trees\n"
"                              Hash-verified individual tree cards only\n"
"  --spectators original|animated\n"
"                             Four limb poses on10France atlases;38others subtle sway\n"
"  --driver-names original|real|sidecar\n"
"                              Real=15extracted display names, no save edits\n"
"                              Sidecar=existing validated driver-names.tsv\n"
"  --cars LIST                 Comma-separated owned ZIP alternatives (below)\n"
"                              original=use unchanged selected data\n"
"  --car-names skin|original   Selected skin labels (default skin); display only\n"
"                              Original restores labels, not replacement assets\n"
"  --mods-dir PATH             Folder containing original downloaded ZIPs\n"
"                              Required with --cars, no assets downloaded\n"
"                              Hash-checked replacement models/info/textures\n"
"                              Not native-tested; may include physics changes\n"
"  --install-enhancement-deps   Allow uv to create an isolated Python3.12\n"
"                              environment and install Pillow/numpy (network)\n"
"  --effect NAME=on|off        Per-launch existing optional GPU effects:\n"
"                              dust_plume,body_dirt,dust_volume,volumetric_clouds,spatial_aa,metalfx_spatial,\n"
"                              motion_blur,bloom,particles,shadows,\n"
"                              depth_of_field,god_rays,color_grading,temporal_aa,speed_motes,fog,air_motes\n"
"                              Requested is not proof of backend support;\n"
"                              see runtime log. temporal_aa = PARTIAL stage (SDL_GPU only,\n"
"                              camera reprojection, no object velocity), default off.\n"
"  --speed-fov 0..500          Widen the regular cameras' view with speed (percent); default 0 = off\n"
"  --drone-speed-drop 0..100    Drone height reduction at speed (default 100, 0 disables)\n"
"  --drone-distance 50..150     Drone camera distance (default 50; saved locked preset wins)\n"
"  --sun-glare 0..100          Restrained sun halo/eye response;0 off, default 60, capped for readability\n"
"  --camera-roll 0..100        Optional damped corner roll for drone/chase, default 30, 100 = up to four degrees\n"
"  --speed-fov-cockpit 0..100  Cockpit camera: share of --speed-fov it uses, percent (default 25; overrides opencmr2.ini)\n"
"  --tunnel-vision 0..200      Tunnel vision strength, percent (default 130; overrides opencmr2.ini; --test-preset overdo sets 130 too)\n"
"  --brush-volume 0|1          EXPERIMENTAL Greece1 roadside brush walls get depth away from the road (visual only, collision unchanged, needs --textures uprez/asset pack; default 0 = original flat quads; real-stage direction not yet verified)\n"
"  --tunnel-lines 0|1          Radial lines inside the tunnel-vision zone (default 1; overrides opencmr2.ini)\n"
"  --tunnel-track 0|1          Tunnel vision + streak centre follows the road ahead, cockpit only (default 1)\n"
"  --cockpit-blur-clear 0..60  Cockpit: tenths of a world unit; interior nearer than this stays out of the tunnel-vision blur (default 0 = off: the interior fills the blur zone, so any value above 0 hides tunnel vision in the cockpit; 16 = the old 1.6)\n"
"  --speed-streak 0..100       Directional radial speed streak (SDL_GPU only); default 0 = off\n""  --speed-hold 0|1            Hold speed effects at full speed (overdo sets 1); 0 = real car speed\n"
"  Replay keys (no flag): while a replay plays, , slower  . faster  Space pause or resume  Tab next camera (last stop: free camera). In chase cameras (4/5/6, including drone), arrows orbit and R resets the active camera to its normal pose. Free-camera controls are unchanged. A small bar shows replay time, speed and camera.\n"
"    Free camera (replay only, works paused): W/S forward/back, A/D left/right, Q/E down/up, Shift fast, arrow keys turn and tilt, R back to the chase pose, Tab leaves it.\n"
"  --fov-preview 0|20..150     Debug: try an absolute video.fov number (legacy scale, 70 = original, higher = wider, not an angle); F10 row too; 0 = off\n"
"  --dof-focus-car 0|1         Focus depth of field on the followed car (SDL_GPU; overdo sets 1); default 0 = fixed plane\n"

"  --speed-motes-stretch 0..100 Stretch speed motes into radial streaks; needs --effect speed_motes=on\n"
"  --speed-motes-amount 0..300 Percent of the normal mote spawn rate (live cap 64 at 100, up to 192 at 300)\n"
"  --speed-motes-size 50..800  Mote size in percent (default 100)\n"
"  --dof-blur-max-px 1..40     Blur radius in pixels when dof_blur is 100 (default 4)\n"
"  --camera-move-chase|--camera-move-cockpit|--camera-move-car -200..200  Move that camera family along its forward axis by this many 0.05 model unit steps at full speed, blending from 0 standing still; + forward, - back (default 0; visual only)\n"
"  --camera-move-type TYPE=VALUE  Per camera type (1 2 3 4 5 6 10; repeatable): same as the family move, replaces the family value for that type when not 0\n"
"  --tire-track-history 0..64  Optional long-lived tyre tracks: keeps N x 1024 points per wheel (default 0 = off, original ring; replaces the ring drawing when on). Old marks are dropped when full, so this is long, not unlimited. Draws only marks near the car, at most 1000 segments per wheel. Visual only\n"
"  --tire-track-length 0..1000  Optional longer tyre tracks: skid-mark points kept per wheel for the viewed car (default 0 = original 200; 5x longer at 1000; marks never time out in this port, only this ring wraps; visual only, costs triangles per frame)\n"
"  --tree-rustle 0..300         Leaf rustle on the 3D replacement trees (percent, default 100, 0 = off): the crown texture shifts a little in gusts while the geometry is unchanged; needs --tree-wind on and --trees shell or crossed; the 2D original trees are not affected\n"
"  --spectator-irregular 0|1    1 (default) varies crowd sway tempo and phase along each card and per placement instead of one tempo; 0 = previous uniform sway; needs --spectators animated\n"
"  --tire-debris 0..400  Optional stronger wheel debris: average chips per wheel per effects step x100 on loose or wet ground above 10 km/h (default 0 = original; visual only, max 4 per wheel step)\n"
"  --fov-dolly-curve 0|1  Experimental: 1 spreads the chase dolly (same 1.0 to 0.70 limits) linearly over widening 1.0 to 2.0 instead of reaching 0.70 at about x1.3 (default 0 = current curve; visual only)\n"
"  --cockpit-eye-shift -100..100  Cockpit camera: move the eye along its forward axis as the view widens (default 0 = off; sign tuned natively; visual only)\n"
"  --fog-amount 0..200         Atmosphere fog density percent (needs --effect fog=on or video.fog; default 65; 100 = heavy)\n"
"  --fog-glow 0..100           Sun-ward brightening of the fog, percent (default 60)\n"
"  --air-motes-amount 0..300   Floating dust motes percent (needs --effect air_motes=on; default 100; live cap 48 at 100)\n"
"  --fog-lights 0..200         Fog brightening inside head/rear light beams, percent (default 100)\n"
"  --fog-probe 0|1             One-shot fog depth readback diagnostic on the next fogged frame (SDL_GPU only; logs depth vs expected fog vs actual shift with a verdict; default 0)\n"
"  --snow-glint 0..200         Ice-crystal glints on snowbanks and falling snow, percent (default 100)\n"
"  --dump-interior 0|1       One-shot: write the cockpit model to <user dir>/interior_dump.obj as a scaffold for modelling add-on pieces (default 0)\n"
"  --cockpit-mesh 0|1        Draw the add-on mesh <user dir>/cockpit_mesh.obj in the cockpit; reloads when the file changes (default 0)\n"
"  --diag-replay-glass 0|1   Diagnostic: in replays, log part break-flag transitions and the saved-damage restore (default 0; log only, no behaviour change)\n"
"  --windshield-dirt-fade-cracked 0|1\n                             Dim the windshield dirt film to half strength on cracked (state 1) glass (default 0; broken glass always hides the dirt)\n"
"  --cockpit-extend-probe 0|1  One-shot cockpit extension visibility diagnostic: logs where the door-card edge and extension strips land on screen (default 0; log only)\n"
"  --test-preset overdo        Exaggerated diagnostic look for speed lines/streak, FOV, DoF, tunnel vision 130 with lines (legacy motes off; add --effect speed_motes=on)\n"
"  --test-preset atmosphere    The accepted atmosphere set plus the effects under test: fog 100 with full sun\n"
"                              glow, near clear 60, spatial pockets, clearing toward the finish, fog light\n"
"                              response, snow glints, air motes 500%%, god rays 80 (combine: overdo,atmosphere)\n"
"                              Explicit flags override it; nothing is saved. fog_cull stays opt-in (experimental, unmeasured)\n"
"  --attract-capture DIR       Start the attract demo at once, save PNGs of the frame at\n"
"                              stage-clock 0, N, 2N... s into DIR (+ capture.csv), then quit\n"
"  --replay-export DIR         Export the first replay played this launch into DIR: one PNG per sim step\n"
"                              (25 fps, every step captured however long saving takes) plus the mixed game\n"
"                              audio as WAV; replay.mp4 is assembled when ffmpeg is on PATH (the exact\n"
"                              command is saved as export-command.txt either way)\n"
"  --capture-interval SEC      Seconds of stage clock between shots (1..60, default 5)\n"
"  --capture-count N           Number of shots (1..120, default 7 = 0..30 s)\n"
"  --prepare-only              Prepare/validate selections, then exit\n"
"  --help, --help-enhancements  Show this list without starting the game\n"
"Car alternatives (slot replacement, NOT native-tested):\n"
"  205-dakar-1987 OR 205-finland-1986\n"
"  subaru-spain-1996 OR subaru-corsica-1998 OR subaru-xgames-2006\n"
"  xsara-turkey-2006, fabia-redbull, focus-norway-2007,\n"
"  focus-montecarlo-2006, 307-montecarlo-2006, lancer-finland-2001\n"
"HQ Cars/CMR2.5/V6 automatic import not supported; assets not bundled.\n"
"Scenery preparation requires supported original archives plus Python3.10+\n"
"with Pillow/numpy, or uv. Existing converter .venv is reused. Without the\n"
"explicit install flag no dependency or Python download is allowed. Generated\n"
"packs are private, hash-checked, versioned and never overwrite old packs.\n"
"Unspecified scenery kind stays original when generating a new pack.\n"
"No flags: existing settings/sidecars unchanged. Original means this launch\n"
"only. Explicit mode flags override settings without rewriting the INI.\n";}
#endif
