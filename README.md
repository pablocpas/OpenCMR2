<p align="center">
  <img src="docs/logo.jpeg" width="320" alt="OpenCMR2 logo">
</p>

# OpenCMR2

[![Build](https://github.com/pablocpas/OpenCMR2/actions/workflows/build.yml/badge.svg)](https://github.com/pablocpas/OpenCMR2/actions/workflows/build.yml)

OpenCMR2 is a work-in-progress native port of **Colin McRae Rally 2.0** (PC, 2000)
to modern platforms, built on the [CMR2Decomp](https://github.com/pablocpas/CMR2Decomp)
decompilation. It runs the original game code natively on Linux, Windows,
macOS and Android. There is no emulation and no Wine or DirectX wrapper: DirectDraw,
Direct3D 7, DirectSound, DirectInput, DirectPlay and Bink are replaced with
SDL3, the SDL GPU API (Vulkan, Direct3D 12, Metal) or OpenGL 3.3 and the
port's own code.

https://github.com/user-attachments/assets/3c7322dc-576b-4fb0-8d6a-4d17de6fe17d

*Colin McRae Rally 2.0 running on Linux with OpenCMR2.*

> [!WARNING]
> This repository contains no game assets. You need your own copy of the
> original PC version of Colin McRae Rally 2.0 to play.

## Status

Everything tested so far works: the intro videos, the menus, the stages and
network races over LAN. The project is still in development, so you may find
bugs. If you do, please [report them](https://github.com/pablocpas/OpenCMR2/issues).
The game code follows CMR2Decomp, and its progress is brought into the port
as it is made.

| Platform | Architecture | Status |
| --- | --- | --- |
| Linux | x86-64 | Plays stages and matches the original physics |
| Linux | i686 | Plays stages, reference build with the original memory layout |
| Linux | ARM64 | Builds and passes the tests, gameplay not yet tested |
| Windows | x86-64 | Plays stages and matches the original physics (tested under Wine) |
| macOS | ARM64 | Builds and passes the tests, gameplay not yet tested |
| Android | ARM64, x86-64 | Plays stages with on-screen controls (Android 10 or later, Vulkan) |

### Features

* Native SDL3 platform layer for windowing, input, audio and files, with saves
  and settings in the user directory
* SDL GPU renderer (Vulkan, Direct3D 12, Metal) reproducing the Direct3D 7
  fixed-function pipeline in shaders, and an OpenGL 3.3 renderer with the same
  shaders for GPUs without Vulkan or Direct3D 12 (Linux and Windows)
* Fullscreen on the desktop resolution or a resizable window, switched with
  Alt+Enter
* Keyboard, mouse and gamepads through SDL3, and on-screen controls on Android

### Keyboard remapping

Use the existing **Options > Controls** screen: choose a keyboard, select the
control, release the confirm key, then press its new key. Escape cancels capture
without changing the binding. Assigning a key clears its previous action in the
same configuration. Accept the controls edits through the normal menu save flow;
these remain in the existing `Configuration/Controller.rcf`, with no format or
profile migration. Menu arrows, Enter, Escape and text entry are unchanged.

For temporary bindings, the binary also accepts repeated `--bind-key` options:

```sh
./build/macos/src/opencmr2 --data "/path/to/owned/game-data" \
  --bind-key left=A --bind-key right=D --bind-key accelerate=W --bind-key brake=S
./build/macos/src/opencmr2 --help-controls
```

This is an example, not a new default. Actions are `left`, `right`, `accelerate`,
`brake`, `handbrake`, `gear-up`, `gear-down`, `camera`, and `look-back`. Keys use
SDL physical scancode names such as `W`, `Space`, `Up`, or `"Left Shift"`.
`none` unbinds an action. Prefix an action with `2:` to select the second local
keyboard configuration, for example `--bind-key "2:brake=Keypad 2"`. Escape is
reserved. Explicit duplicate keys on one keyboard are rejected; swaps work.
Unspecified actions keep saved keys, except a key reassigned to another action
is cleared there for this launch. Temporary overrides take priority over in-game
edits until restart, but are never serialized to the controls file. Omit them on
the next launch to return to your saved bindings. In-game edits still save normally.

* Bink intro and country videos with a built-in decoder
* Multiplayer over LAN and the internet using UDP instead of DirectPlay
* Folder dialog on first launch to locate the game installation
* In-race HUD scaled with the screen and kept at its edges on wide screens,
  with a configurable size
* Fixes for some original bugs, such as German in the language menu loading
  Spanish

The improvements of [SilentPatch](https://cookieplmonster.github.io/mods/colin-mcrae-rally-2/)
are built in as well:

* Widescreen resolutions up to the desktop resolution, with the correct
  perspective instead of a stretched picture
* Configurable field of view
* High and variable frame rates without changing the game speed or the
  physics, verified against the original game

## Project principle

> [!IMPORTANT]
> We do not change the **physics** or the **gameplay**. We add to the **audio-visuals**
> and the **feel**. Original modes, cameras and controls stay available next to
> anything new. The only named exceptions are curved roads (visual upgrade, the drive
> line must stay the same) and, at low priority, breakable trees.
> Details: [docs/PRINCIPLES.md](docs/PRINCIPLES.md).

## Approach

The game code in `game/` comes from CMR2Decomp and is kept as close to the
original as possible. Every change the port needs is marked with a `PORT:`
comment. The rest of the port (`src/`) implements the platform APIs the game
expects on top of SDL3.

Accuracy is tested against the original executable rather than assumed:

* The fixed-point helpers are compared with the original x87 assembly
* The startup math tables (16,896 entries) are compared with dumps from the
  original game
* 500 vehicle updates of a Finland stage, driven by recorded input, are
  compared field by field with the original game + SilentPatch at 30, 60,
  120, 144 and 240 FPS and at a variable frame rate. All of them match bit
  for bit ([details](docs/PHYSICS_PARITY.md)).

Graphical enhancements will be added one at a time and kept optional, so the
original look stays available.

## macOS rebuild and launch shortcut

`./rebuild.sh` still only does a clean macOS build. Updating and launching are opt-in:

```sh
export CMR2_DATA="/path/to/your/owned/CMR2"
export CMR2_MODS_DIR="/path/to/your/owned/car-zips"  # default: mods in this checkout
./rebuild.sh --update --all-mods --run
```

Use ASCII `--`, not typographic dashes. Run from an existing checkout with a local
`main` branch. The first time, use `git switch main && git pull --ff-only` to get
these new switches after their PR is merged. `--update` fetches `origin/main`,
switches to `main` and fast-forwards only. Tracked edits, untracked files, or an
ahead/diverged local `main` stop it without discarding, stashing or resetting work.
It then re-enters the updated script so the build and preset come from one version.

`--all-mods` is a named experimental audiovisual preset, not “turn every effect
on”. It preserves the current launch settings: shell trees with wind, animated
spectators, real driver names, the 205/Subaru/Fabia/Lancer liveries, Finland1 roads,
portable uprez textures, spatial AA/MetalFX, dust/body dirt, shadows/god rays,
vapor trails and the existing speed-FOV/tunnel/camera settings. Temporal AA,
volumetric clouds, bloom, particles, color grading, motion blur, DoF and speed
lines stay off. Source filtering does not restore missing detail; atlas mixing,
native appearance/performance and cockpit geometry clipping remain unverified.
No physics or gameplay changes. Original assets stay private and unchanged.

`--all-mods` also turns on the EXPERIMENTAL Greece1 roadside brush-wall extrusion
(`--brush-volume 1`): the walls get visual depth away from the road, collision is
unchanged, and it draws nothing new unless the Greece asset pack is in use. Only walls
within 60 units of the route that face the road are extruded; the rest stay flat (an offline
count on one recovered gre01hi archive: 19 of 38 walls, not a native result). The outward
direction on the real stage and the native look are not verified yet. Pass
`--brush-volume 0` after `--all-mods` to keep the original flat quads (also the default
without `--all-mods`).

`./rebuild.sh --data DIR --export-brush-placement [--brush-placement-output FILE]` is a
separate, read-only step: no build, no launch, no extrusion change. It writes a JSON of
numbers only (wall positions vs the stage route) to a new file (default in `$TMPDIR` or
`/tmp`, never inside the game data, never overwriting) and stops with a reason if its
checks fail. That JSON is what verifies the extrusion direction on real data.

Paths can instead be passed as `--data DIR --mods-dir DIR`; `OPENCMR2_DATA` is also
accepted when `CMR2_DATA` is unset. No personal path is stored in the repository.
This shortcut does not download missing car ZIPs or enhancement dependencies.
Missing/invalid sources fail in the existing hash-checking preparation helper.
`--all-mods` cannot be mixed with individual asset options. `--run` alone launches
with normal saved settings after building, without applying the preset.
`--assets-only --all-mods --run` uses an existing binary without rebuilding.
`--dry-run` prints the local plan, including an optional update, without fetching,
building, preparing or launching; it cannot validate incoming remote changes or
source hashes. See `./rebuild.sh --help`.

## Running

Download the package for your platform from
[Releases](https://github.com/pablocpas/OpenCMR2/releases):

| Platform | Package |
| --- | --- |
| Windows x64 | `OpenCMR2-Windows-x64.zip` |
| Linux x86-64 | `OpenCMR2-Linux-x86_64.tar.gz` |
| Linux ARM64 | `OpenCMR2-Linux-arm64.tar.gz` |
| Linux 32-bit | `OpenCMR2-Linux-i686.tar.gz` |
| macOS (Apple silicon) | `OpenCMR2-macOS-arm64.zip` |
| Android | `OpenCMR2-Android.apk` |

Extract it and run `opencmr2`. On first launch, choose the folder of your PC
installation. The installation itself, a folder inside it or a parent folder
all work. The location is checked and remembered, and the game files stay
where they are.

If you have the CD or an ISO, install the game or extract it first: OpenCMR2
reads the installed folders `Game`, `FrontEnd`, `Sounds`, `CountrySpecific`
and `SetupRep`, and does not start when one of them is missing.

On Android, install the APK and copy the installation folder to the
device first. On first launch, choose it in the system's folder picker: the
game files are copied into the app once (about 700 MB), as Android gives
apps no direct access to other folders. The on-screen controls press the
default keyboard keys, so they work in the menus and on the stages, and
hide while a gamepad is used, external or built into a handheld. Android selects
an SDL gamepad automatically; Start pauses the game. The launcher has a
**Controls** screen, usable by touch before starting the game, to test a pad and
correct individual buttons, sticks or triggers. These corrections are saved by
device and preserve the rest of SDL’s automatic layout and analog input.
**Restore automatic mapping** removes the corrections. You can also keep the
touch controls visible or choose to play with touch controls only. Android Back
pauses the game; a controller’s Back/Select follows its SDL mapping. Connect or
disconnect controllers in the menus (or pause first during a stage) to rescan.
Controller firmware must expose its input to Android; buttons intercepted by
system shortcuts cannot be mapped by the app.

The Linux packages are built on Ubuntu 24.04 and need a distribution of
similar age or newer. The macOS package is not signed: after extracting it,
run `xattr -dr com.apple.quarantine OpenCMR2-macOS-arm64` once.

Command-line options:

| Option | Description |
| --- | --- |
| `--data <dir>` | Use this game installation and skip the folder dialog |
| `--user-dir <dir>` | Keep saves, settings and logs in this directory |

The installation can also be set with the `OPENCMR2_DATA` environment variable
or `[paths] data` in `opencmr2.ini`, or copied into a `Data` folder next to the
executable, which is found without asking.

### Optional real-driver names

The real-driver names are a separate local patch, not an edit to the original
game files. Generate `driver-names.tsv` in a private user directory and launch
with that `--user-dir`. Without the sidecar, the original names remain.
See [enable, mapping and undo instructions](tools/driver-name-patch/README.md).

## Configuration

Settings are stored in `opencmr2.ini` in the user directory
(`~/Library/Application Support/OpenCMR2/` on macOS,
`~/.local/share/OpenCMR2/` on Linux,
`%APPDATA%\OpenCMR2\` on Windows; files from the older
`OpenCMR2/OpenCMR2` folder are moved there).
It is created with comments on first launch. The Screen page of the
graphics options sets `hud_scale`, `menu_scale`, `vsync` and `fullscreen` in
the game.

| Setting | Default | Description |
| --- | --- | --- |
| `[video] fullscreen` | `1` | Fullscreen (`1`) or window (`0`), changed and saved by Alt+Enter |
| `[video] fov` | `70` | Field of view in degrees, 30 to 150 (70 is the original) |
| `[video] run_in_background` | `0` | Keep running when the window loses focus |
| `[video] car_detail` | `1` | Every car with the model and textures of the player's car (`1`), or the original's lower detail for the other cars (`0`); applies with Car Detail set to High |
| `[video] hud_scale` | `100` | Size of the in-race HUD in percent of the original's at the screen's height; `0` keeps the original's layout in pixels |
| `[video] hud_aspect` | `16:9` | Widest shape the HUD spreads over, centred on wider screens: an aspect ratio such as `16:9` or `21:9`, or `full` for the whole screen |
| `[video] menu_scale` | `100` | Size of the menus in percent of the original's at the screen's height; `0` keeps the original's layout in pixels |
| `[video] outline_fonts` | `1` | Draw the text from outlines, sharp at any size (`1`), or from the original's bitmaps (`0`) |
| `[video] vector_flags` | `1` | Keep the original dotted menu flags, drawing each dot's circular edge at screen resolution when enlarged (`1`), or magnify the original dot bitmap (`0`); native-size dots and small statistics flags keep their original pixels |
| `[video] vector_icons` | `1` | Trace the original co-driver arrows and menu arrows/checkmarks/crosses/boxes/circles for sharp enlarged edges (`1`), or keep the bitmaps (`0`); native-size icons keep their original pixels |
| `[video] vector_maps` | `1` | Sharp enlarged arcade minimaps, car markers and circular points in circuit-selection maps (`1`), or keep the bitmaps (`0`); original routes, point grids, colours and native-size pixels are preserved |
| `[video] vsync` | `1` | Wait for the display's refresh (`1`) or draw frames as soon as they are ready (`0`) |
| `[video] graphics_api` | `auto` | Renderer: `sdl_gpu` (Vulkan, Direct3D 12, Metal), `opengl` (OpenGL 3.3, Linux and Windows), or `auto` for SDL GPU when the GPU has a Vulkan, Direct3D 12 or Metal driver (not a software one) and OpenGL otherwise |
| `[video] window_scale` | `1` | Size of the window in multiples of the game's resolution, applied when leaving fullscreen |
| `[video] smooth_scaling` | `1` | Smooth the picture when it is scaled to the window or screen (`1`), or keep sharp pixels (`0`) |
| `[video] anisotropy` | `1` | Anisotropic texture filtering, `2` to `16`; `1` is off, as in the original |
| `[video] motion_blur` | `1` | Blend each frame into the previous one for motion ghosting: `0` off, `1` on (SDL_GPU renderer only) |
| `[video] bloom` | `1` | Make bright pixels bleed into their neighbours with a soft glow: `0` off, `1` on (SDL_GPU renderer only) |
| `[video] particles` | `1` | Rain or snow falling through the scene: `0` off, `1` rain, `2` snow |
| `[video] shadows` | `1` | Directional light shadow map darkening occluded surfaces: `0` off, `1` on (SDL_GPU renderer only) |
| `[video] drone_speed_lens` | `100` | Accepted but unused: the drone camera lens no longer changes with speed (the drone shows no tunnel vision, speed lines or speed FOV either; vapor trails and dust stay). The steering lens is unchanged. Visual only |
| `[video] drone_shake` | `100` | No longer applies: the drone camera has no speed shake (the regular camera's `camera_shake` is unchanged). The key is still accepted so old INI files load. Visual only |
| `[video] speed_motes` | `0` | Prototype: sparse world-fixed specks beside the road ahead that stream past at speed. Off by default, `--effect speed_motes=on`. See docs/SPEED-MOTES.md. Visual only, native pixels unverified |
| `[video] speed_motes_stretch` | `0` | Needs `speed_motes`. 0 to 100: stretches the motes into radial speed streaks along the screen direction away from the view axis, longer at higher speed. `0` keeps round dots. Also `--speed-motes-stretch N`. Visual only, native look unverified. See docs/SPEED-MOTES.md |
| `[video] speed_motes_amount` | `100` | How many speed motes, 0 to 300 percent |
| `[video] fog` | `0` | Atmosphere fog: one full-screen scene+depth pass with distance fog, ground-mist gradient, sky cap and a sun-ward glow (screen-space, not volumetric shafts). `--effect fog=on`, F10 rows, Advanced>Effect amounts>Atmosphere. Visual only, native pixels/FPS unverified. See docs/ATMOSPHERE.md |
| `[video] fog_amount` | `65` | Fog density 0 to 200 percent. Default 65; 100 is the heavy overdo look; dial to taste. `--fog-amount N` |
| `[video] fog_glow` | `60` | Sun-ward glow in the fog, 0 to 100 percent. `--fog-glow N` |
| `[video] air_motes` | `0` | Slow-drifting floating dust motes (atmosphere, any speed - distinct from the speed motes above). `--effect air_motes=on`. Visual only, native appearance unverified |
| `[video] air_motes_amount` | `100` | Mote count/rate 0 to 300 percent; base cap 48 billboards at 100. `--air-motes-amount N` |
| `[video] speed_fov` | `0` | Regular cameras only (never the drone): widens the field of view with the player car's speed, by this many percent of the projection scale at full speed, 0 to 500 (101 to 500 is a testing range; 100 is about FOV preview 150; `0` off, about `10` to `15` is subtle). Eases over about half a second. Visual only: no change to camera position, car or physics. Also `--speed-fov N` (per launch, wins over the INI). Native look unverified |
| `[video] camera_move_chase`, `camera_move_cockpit`, `camera_move_car` | `0` | Per camera family (chase, cockpit, bonnet and bumper): moves the camera along its forward axis by this many 0.05 model unit steps at full speed (-200 to 200, + forward, - back). The move blends from 0 standing still to the full amount at the same speed ratio as the speed FOV. `0` off. F10 rows near the end and `--camera-move-chase`, `--camera-move-cockpit`, `--camera-move-car`; the rows work while paused. Visual only. |
| `[video] cockpit_eye_shift` | `0` | Cockpit camera only: moves the eye along the camera's forward axis as the view gets wider than the saved `video.fov` (speed FOV or preview), -100 to 100. `0` off. Which sign helps is for native tuning. Also `--cockpit-eye-shift N` and an F10 row. Visual only. Native look unverified |
| `[video] drone_distance` | `60` | (`--drone-distance`; a value in opencmr2.ini overrides the default, the flag overrides the ini) Drone camera only: camera distance in percent of the default, 50 to 150. The default already pulls back at low speed and closes in at speed; this scales that distance. Visual only |
| Replay keys | n/a | While a replay plays: `,` slower, `.` faster, `Space` pause or resume, `Tab` next camera (the automatic trackside camera is part of the cycle; the last stop is a free camera). Speeds 0.25x to 8x. A small bar at the bottom shows elapsed / total replay time (from the recorded control packets), speed and camera. The recorded run is re-run through the unchanged physics; only the real time that feeds the step count is scaled. Speed is capped by the game at 5 sim steps per frame, so 8x needs about 40 fps or better (the effective limit is min(8x, 5 x fps / 25)). While paused the scene stays still and a camera change is applied at once. There is no reverse. Free camera (replay only, also while paused): `W`/`S` forward and back along the view, `A`/`D` left and right, `Q`/`E` down and up, `Shift` four times faster, arrow keys turn and tilt, `R` back to the chase pose, `Tab` leaves it. It starts behind the car on entry, never carries over to another replay, and changes only the shown camera: no recorded control, physics or saved camera setting is touched. Replay keys are taken from the game while a replay plays, even if bound to a driving action (the recorded controls drive the car); Esc, Return and the other refused keys are never used. Air vortices (vapor trails) are off in every replay view; the saved toggle is unchanged. |
| `[video] speed_fov_cockpit` | `25` | Cockpit camera only: the share, in percent, of the Speed FOV amount the cockpit uses (0 to 100; 100 = the full Speed FOV as before). Other cameras are unchanged. Also a row in the F10 panel. Visual only |
| `[video] cockpit_blur_clear` | `16` | Cockpit camera only, SDL_GPU/Metal: tenths of a world unit (0 to 60; 16 = 1.6). Pixels nearer than this view distance (dashboard, wheel, hands, roof) are kept out of the tunnel-vision blur, judged from the depth buffer; 0 = off. Visual only; native pixels not yet verified |
| `[video] speed_effects` | `1` | Advanced graphics menu row "Speed Effects". 0 turns off tunnel vision, its lines, the radial streak and the speed FOV widening in every view; 1 is the previous behaviour. Saved from the menu; nothing else is changed. Air vortices have their own row ("Air Vortices", `[video] vapor_trails`). |
| `[video] tunnel_vision` | `130` | Speed-driven tunnel vision, 0 to 200 percent (above 100 is an experimental overdrive that closes the sharp zone further and blurs harder, with more blur taps; 100 and below are unchanged). `[video] tunnel_lines` (0/1, default 0, `--effect tunnel_lines=on`) draws radial speed lines in the tunnel zone, `0` off. Faster driving narrows the sharp zone and blurs the edges. Needs `video.depth_of_field=1` (SDL_GPU renderer only). Visual only |
| `[video] stretch_movies` | `0` | Stretch the movies over the whole window (`1`) instead of keeping their 4:3 proportions |
| `[input] key_repeat_delay` | `750` | Milliseconds before a key held in the menus repeats |
| `[input] key_repeat_interval` | `100` | Milliseconds between the repeats of a held key |
| `[install] language` | `English` | English, American, French, German, Italian, Spanish or Polish |
| `[install] sku` | `EUROPE` | Game edition: `EUROPE`, `AMERICA`, `POLAND` or `JAPAN` |
| `[install] menu_shortcut` | asked | Entry in the applications menu (Linux) or Start menu (Windows): `1` keeps it pointing at this copy, `0` removes it |
| `[net] ...` | | Network play, see [NETWORK.md](docs/NETWORK.md) |

### Startup diagnostics

`opencmr2.log` in the selected user directory prints a one-time options summary
at startup. `requested` means configuration, not proof of an effect running.
Follow-up lines report the initialized renderer, chosen shader format and
MetalFX extension availability. MetalFX's existing first-presentation line
reports `encoded` or `fallback`; availability alone is not an encoded frame.

Driver names report custom validated names or an original-name fallback reason
(missing/invalid/disabled sidecar). Scenery reports a manifest's parsed mapping
count, not loaded textures. Trees and spectators each log their first successful
stage override; failed asset preparation logs a fallback once per stage load.
No per-frame option dump, generated artwork or message contents are logged.
No performance or native visual-correctness claim follows from these messages.

## Building

OpenCMR2 builds with Clang, CMake 3.24+, Ninja and glslang, using the presets
in `CMakePresets.json`:

| Preset | Target |
| --- | --- |
| `linux` | Linux, 64-bit (x86-64 or ARM64) |
| `windows-x64` | Windows x64 (`clang++` with the MSVC runtime) |
| `macos` | macOS, Apple silicon |
| `linux-x86` | Linux, 32-bit x86 (reference build for the tests) |

OpenCMR2 needs SDL 3.4 or later. An installed SDL3 of that version is used;
otherwise CMake downloads and builds it and places it next to the executable
(to force this, pass `-DOPENCMR2_VENDORED_SDL=ON`). Start by cloning the repository:

```sh
git clone https://github.com/ollisulopuisto/OpenCMR2.git
cd OpenCMR2
```

### Linux

Install the dependencies.

Fedora (SDL3-devel is used when it is 3.4 or later):

```sh
sudo dnf install clang cmake ninja-build glslang SDL3-devel
```

Ubuntu 24.04 or Debian, where SDL3 is built from source with these
development libraries:

```sh
sudo apt install clang cmake ninja-build glslang-tools pkg-config \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxtst-dev \
    libxkbcommon-dev libwayland-dev wayland-protocols libegl1-mesa-dev \
    libasound2-dev libpulse-dev libudev-dev libdbus-1-dev
```

Then build:

```sh
cmake --preset linux
cmake --build --preset linux
```

The executable is `build/linux/src/opencmr2`.

### macOS (Apple silicon, including M1/M2)

Run these commands in Terminal. Install Apple's Command Line Tools first if
`clang` is not available (`xcode-select --install`), and install
[Homebrew](https://brew.sh/) if `brew` is not available. The tools and source
build do not include the original game data.

From a fresh clone, enter the `OpenCMR2` directory as shown above. For an
existing checkout, enter that checkout instead; its folder name may differ.
The commands below must run from the directory containing `CMakePresets.json`.

```sh
brew install cmake ninja glslang spirv-cross python
cmake --preset macos \
  -DOPENCMR2_VENDORED_SDL=ON \
  -DOPENCMR2_SHADERS_MSL=ON \
  -DOPENCMR2_RENDERER_GL=OFF \
  -DOPENCMR2_METALFX=ON
cmake --build --preset macos
ctest --preset macos
```

This builds `build/macos/src/opencmr2` for ARM64 with Clang. It uses the pinned
vendored SDL, including this fork's optional MetalFX spatial extension. CMake
needs network access to fetch SDL if its sources are not cached. SPIRV-Cross
from Homebrew translates the shaders locally instead of fetching that tool.
MetalFX SDK absence produces a warning and falls back to ordinary Metal;
compiling MetalFX support does not force it on at runtime. Passing unit tests
is not proof that native graphics or gameplay work on your Mac.

#### Launch with your original game installation

Replace `/path/to/owned/CMR2` with your installed/extracted PC game folder,
containing `Game`, `FrontEnd`, `Sounds`, `CountrySpecific` and `SetupRep`.
Keep the quotes when the path contains spaces. The original files are not
needed to compile; they are needed to play.

```sh
./build/macos/src/opencmr2 --data "/path/to/owned/CMR2"
```

Without `--data`, the first-launch folder picker asks for that installation.
Normal Mac settings, saves and logs live in
`~/Library/Application Support/OpenCMR2/`. You can use a separate profile
without changing the existing one:

```sh
./build/macos/src/opencmr2 --data "/path/to/owned/CMR2" --user-dir "$HOME/OpenCMR2-private"
```

This creates separate settings/saves/logs; it does not copy your existing
profile. The chosen user directory must be outside the original game folder.

#### Recover an earlier build with MSL disabled

If startup prints `SDL_CreateGPUDevice: No supported SDL_GPU backend found!`,
first check how the executable was built. SDL_GPU Metal requires MSL shaders;
a build with `OPENCMR2_SHADERS_MSL=OFF` only advertises SPIR-V and is not a
usable default Mac renderer. This error alone does not prove the GPU lacks
Metal. Changing the resolution cannot supply the missing shader format.

Install `spirv-cross` and repeat the configure/build commands above in the same
checkout. The explicit `OPENCMR2_SHADERS_MSL=ON` overrides an earlier cached
OFF value; running the bare preset does not reset cached options. There is no
need to delete the build directory, original data or player profile. OpenGL
OFF is the normal Mac setting. If fetching SDL fails, fix that network error
rather than disabling MSL to bypass it.

For ordinary Metal without this fork's MetalFX extension, an installed
SDL 3.4+ can be used with `OPENCMR2_METALFX=OFF` and
`OPENCMR2_VENDORED_SDL=OFF`; MSL must stay ON. This is a separate fallback,
not the MetalFX build above.

#### Optional enhancements from the command line

```sh
./build/macos/src/opencmr2 --help-enhancements
./build/macos/src/opencmr2 --data "/path/to/owned/game-data" --trees crossed --spectators animated --driver-names real
```

The binary verifies originals, prepares/reuses private versioned assets, and
starts the game. `--trees shell` selects the experimental tiered shell instead
of fixed crossed cutouts. Only3high-source tree mappings are generated; people
have four forearm poses on one verified atlas and limited sway on the other47,
not3Dmodels. Neither tree mode has native
visual acceptance. `original` disables that kind for this launch, and selections
are not written to settings. No `--user-dir` switch is needed: driver progress
and saves stay in the current/default profile.

Scenery requires supported owned archives and Python3.10+ with Pillow/numpy.
An existing converter `.venv` is reused. If dependencies are missing, opt in to
isolated uv setup with `--install-enhancement-deps` on the same command. No
Python/packages are downloaded without that explicit flag; uv itself must be
installed already. No proprietary asset downloads or public artwork are added.
`--prepare-only` validates/prepares and exits without launching.

`--cars LIST --mods-dir PATH` selects hash-verified local owned ZIP car
alternatives; help lists names and conflict groups. Slots are replacements,
not an expanded roster. Selected skins also supply car display labels from
verified archive metadata, for example Skoda Fabia WRC Red Bull in the Focus
Alternate 1 slot. Exact variants stay independent. `--car-names original`
restores the original labels while keeping the selected assets; `skin` is the
default with a selected overlay. Labels do not change slot IDs, unlocks,
physics or saves. Native compatibility, label fit and physics are unverified.
`--effect dust_volume=off` (or another listed effect) overrides a GPU option
for this launch; runtime logs still determine actual backend support.
`--driver-names real` selects the extracted15display names, not saved player
progress. Omit flags to preserve existing settings/mods.

See the [scenery and car preparation guide](tools/scenery-patch/README.md) for
requirements, cache/failure behavior, car caveats and the old manual converter
route. Existing `--scenery-dir PATH` remains a read-only pack selection; don't
combine it with automatic preparation flags. No original data or nonempty pack
is overwritten. Generated files stay in a separate private versioned cache.

Complete generated assets stay private and out of Git/public releases.
Generation success does not verify native placement, appearance or M2
performance. To undo automatic selections, omit those flags on the next launch. Existing
manual sidecars/settings remain as they were. See the
[pack format and limits](docs/SCENERY-PACK.md).

### Windows

Install Visual Studio 2022 with its "C++ Clang tools" component (or LLVM),
Ninja and [glslang](https://github.com/KhronosGroup/glslang/releases). From an
"x64 Native Tools" command prompt:

```bat
cmake --preset windows-x64
cmake --build --preset windows-x64
```

`clang-cl` is not supported, as the build uses GNU-style options. To
cross-build from Linux with LLVM-MinGW, see [WINDOWS.md](docs/WINDOWS.md).

### Android

The Gradle project in `android/` builds the same CMake project with the
Android NDK. It needs JDK 17 or later, the Android SDK (with `ANDROID_HOME`
set or `sdk.dir` in `android/local.properties`) and glslang on the `PATH`.
Gradle downloads SDL's source and fetches the NDK, CMake and SDK platform
when the SDK licences are accepted:

```sh
cd android
./gradlew assembleRelease
```

The APK is `android/app/build/outputs/apk/release/app-release.apk`. It is
signed with the release key given by `OPENCMR2_KEYSTORE`,
`OPENCMR2_KEYSTORE_PASSWORD`, `OPENCMR2_KEY_ALIAS` and
`OPENCMR2_KEY_PASSWORD` (or the `opencmr2.keystore` Gradle properties), and
with the debug key otherwise.

### Tests

```sh
ctest --preset linux
```

The 32-bit `linux-x86` build keeps the original data layout and is the
reference for the tests against the original game. Its dependencies and
the rest of the development workflow are described in
[DEVELOPMENT.md](docs/DEVELOPMENT.md).

## Documentation

| Document | Contents |
| --- | --- |
| [ARCHITECTURE.md](docs/ARCHITECTURE.md) | Source layout, platform APIs and renderer design |
| [DEVELOPMENT.md](docs/DEVELOPMENT.md) | Tests, syncing with CMR2Decomp, repository conventions |
| [PHYSICS_PARITY.md](docs/PHYSICS_PARITY.md) | Physics and timing comparison with the original game |
| [ORIGINAL_REFERENCE.md](docs/ORIGINAL_REFERENCE.md) | Recording the original game for the comparisons |
| [BASELINE.md](docs/BASELINE.md) | Stage capture, replay and CPU profiling |
| [NETWORK.md](docs/NETWORK.md) | Network play setup and protocol |
| [VIDEO.md](docs/VIDEO.md) | Bink decoder |
| [WINDOWS.md](docs/WINDOWS.md) | Release packages and cross-building for Windows |
| [64bit-raw-access.md](docs/64bit-raw-access.md) | 64-bit memory access audit |

The full index is in [docs/README.md](docs/README.md).

## Credits

* [CMR2Decomp](https://github.com/pablocpas/CMR2Decomp), the decompilation
  this port is built on
* Silent, for [SilentPatch](https://cookieplmonster.github.io/mods/colin-mcrae-rally-2/),
  whose improvements are built in and which is the reference for the
  comparisons with the original game
* The [SDL](https://libsdl.org) and [ENet](http://enet.bespin.org) projects
* Filippo Scognamiglio for [RadialGamePad](https://github.com/Swordfish90/RadialGamePad),
  the on-screen controls on Android
* Konstantin Shishkov, Peter Ross and Daniel Verkamp for FFmpeg's Bink
  decoder, which `src/video/` is adapted from

## Licence

OpenCMR2 is licensed under the [GPL-3.0](LICENSE). Bundled third-party code
keeps its own licence: SDL3 (zlib), ENet (MIT), Potrace (GPL-2.0-or-later),
the Bink decoder (LGPL-2.1) and RadialGamePad (GPL-3.0).

OpenCMR2 is an unofficial project, not affiliated with or endorsed by
Codemasters or Electronic Arts. Colin McRae Rally 2.0 is a trademark of its
respective owners.


The spectator option currently articulates the raised forearm on one verified
France atlas using the original demo recipe, not all crowd limbs. Other atlases
retain subtle cutout sway. `--tree-wind off` disables the new base-fixed2Dwind
(default on with a selected tree mode). Wind does not turn cards into3Dtrees.
See the preparation guide for exact coverage and the runtime draw counters.

`--tree-rustle 0..300` (default 100, 0 off) shifts the crown texture of the 3D replacement trees a little, in gusts, while the geometry is unchanged; trunk and texture edges stay still. It needs `--tree-wind on` and does not touch the 2D original trees. `--spectator-irregular 0|1` (default 1) varies crowd sway tempo and phase along each card and per placement; 0 restores the single-tempo sway. Both are visual only and not yet seen natively.

### Experimental curved Ballon roads

Pass `--roads curved` to the OpenCMR2 binary to prepare and select matching
curved render/contact geometry. `--roads original` or no road flag keeps the
original geometry. `--help-enhancements` lists the option; add `--prepare-only`
to prepare/verify without starting the game. Keep your usual data/profile flags;
no separate save profile is needed, and original game files are never modified.

This requires an existing Python 3.10+ environment with numpy. No dependency or
asset downloads are performed. Only the exact supported original France Ballon
high-detail archive is accepted. Other stages remain original. Missing or changed
source/preparation errors stop that launch with a diagnostic; runtime pair/source
mismatch falls back to both original render and contact geometry together.

This is experimental, not native acceptance: FIN barriers are unchanged and may
not align with moved boundaries. Native Mac driving, culling and performance have
not been verified. The source-only converter and cache require your owned data;
no game artwork or stage bytes are included in this repository.

### Volumetric cloud prototype

The race-world pass adds a moving world-space density layer in Metal/SDL_GPU
and OpenGL, using 48 view samples and six sunward extinction samples per occupied
step. Stage weather selects coverage. Density is anchored in world coordinates
and advects with a fixed wind. Opaque scene depth limits the view march. The
rendered cloud transmission is also the god-ray mask, separate from brightness.
Sun projection uses the race view/projection and no longer needs the old icon
texture. The original textured sky remains as the background/fallback.

Enabled by default; disable through **Effect amounts > Volumetric clouds**,
`--effect volumetric_clouds=off`, or `[video] volumetric_clouds=0`. God rays keep
their separate switch and amount. Menu/reflection/multiple-view and nonperspective
paths skip this full-screen cloud pass. This is bounded single-scattering-style
lighting, not weather simulation, ground cloud shadows or a full atmospheric model.

Source-inspected, untested prototype. Native Mac appearance, alignment, sun
projection and frame-time cost are not verified. Optimization is a later pass.

### Integrated experimental asset subset

After building, add `--asset-upgrades` to the game command. Put this preset
before any individual flags that should override it. It prepares a private
read-only cache from your owned data and enables broader supported trees/bushes,
animated 2D spectators, Finland1 visual-only road-edge curves and opaque
Finland1/France1 ground2x textures. Original contact/FIN/AI route and surface
files stay unchanged. Unsupported stages, atlases and strips stay original.
This is not the complete texture/hedge pack and not native-Metal acceptance.
Use `--prepare-only` to validate/generate without starting a race. Dependencies
are not downloaded without `--install-enhancement-deps`.


### Private fixed-strip / fence follow-up

`--asset-upgrades` also prepares exact-source2x forest-strip and wooden-fence
cutouts. Finland1forest and France1hedge/tree-wall material runs gain bounded visual relief and
optional wind without changing collision/driving; fences get texture filtering
only. See `tools/scenery-patch/README.md` for coverage and rollback flags.
Source interpolation adds no new artwork detail. Native appearance/FPS remain
unverified, and original assets are required and never modified.

### Replay chase-camera orbit

During replay, hold Left/Right to orbit horizontally and Up/Down to orbit
vertically in every chase camera (types 4, 5 and 6, including the drone variant).
R clears the active camera's rotation offsets and restores its normal current
position and angle. The car keeps moving underneath the normal chase camera;
this does not rewind the car or restore a saved world coordinate. Rotation works
while replay playback is paused and uses real frame time, not playback speed.
Switching camera/target starts with zero offsets; replay exit clears them.

Arrows/R are consumed only for replay chase/free cameras, never during driving
or in the race menu/F10 tuning panel. Free-camera WASD/QE/arrows/R, Tab camera
cycling, comma/period speed, Space pause and saved chase settings are unchanged.
Offsets are applied to a fresh interpolated render pose, not camera records,
recorded controls, physics or original game assets. Native Mac acceptance remains
unverified.
