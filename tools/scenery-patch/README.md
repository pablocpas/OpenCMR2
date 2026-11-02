# Local scenery asset patcher

Requires the source-fitted loader from PR27. This recipe creates only3
experimental high-source tree mappings, not a full64-tree replacement.

This is a procedural source patch, not an xdelta/bsdiff binary delta. It needs
original game files and cannot generate the artwork without them. GitHub gets
only this code and a filename/hash/dimension/classification index. Complete
modified assets stay in the player's separate private directory.

No textures, silhouette meshes, screenshots, BFLs, EXEs or binary extracts are
included. Local outputs are derived game artwork. This tool is not permission
to redistribute them and does not provide copyright or game-license clearance.
The new code follows this repository's GPLv3 license; that does not license the
game's artwork. No third-party executable is executed or distributed.

## One binary command (preferred)

After building a revision with enhancement CLI support, run:

```sh
./build/macos/src/opencmr2 --help-enhancements
./build/macos/src/opencmr2 --data "/path/to/owned/game-data" --trees crossed --spectators animated --driver-names real
```

The binary verifies originals, prepares or reuses a private versioned pack and
starts the game. Use `--trees shell` for the experimental tiered shell instead;
`--trees original` and `--spectators original` disable those kinds for this
launch. A newly prepared pack activates only the kinds explicitly requested,
so add both flags to enable trees and people together. `--prepare-only` prepares
and validates then exits without running the game. No selection is saved to INI.
No new launcher config or remembered path is created. Existing `--data`/folder
selection behavior stays as it was.

Newly generated packs also include source-identities.tsv with exact original DDS
byte hashes. The renderer captures identity when a texture loads, including loose
textures and stage-specific archives. Filenames alone never authorize a new-pack
replacement. Old packs without identity records keep their strict archive-based
matching; regenerate using this revision into a new cache for corrected routing.
An active draw marker proves the override was submitted, not native appearance or
frame-time acceptance. One spectator atlas has articulated forearm poses; the
other47 retain subtle sprite upper-body sway.

Generated cache is under `enhancements/` in the already-selected user profile;
this does NOT change the user/profile root. It never moves or overwrites an old
pack, original archive or save. Each reuse rechecks original hashes and every
generated file. Changed/incomplete cache fails rather than overwriting it. A
changed recipe creates a new version. Old caches are kept, not auto-deleted.
The old manual `--scenery-dir` route below still works, but don't combine it
with automatic tree/animated-spectator preparation flags.

Preparation uses the shipped source-only helper beside the executable. Source
builds also find the checkout helper and reuse its existing `.venv`. Python3.10+
with Pillow/numpy is required for scenery; no artwork is shipped or downloaded.
If dependencies are missing, the binary stops with an explanation. To explicitly
allow isolated Python/package setup using an already-installed `uv`, append
`--install-enhancement-deps` to the same binary command. This may download
Python3.12, Pillow and numpy from their package sources. It does not install uv,
change a global Python, replace an existing venv, or download game/mod assets.
Without that flag no Python/dependency download is allowed. Internet/package
availability and native Mac execution remain separate checks.

`--driver-names real` uses the15previously extracted display names directly,
without generating or changing a sidecar or player progress. `original` forces
the original table for this launch; `sidecar` selects the existing validated
`driver-names.tsv` route. No flags preserve the existing settings/sidecars.

### Skin-aware car labels

Selected owned overlays contain a hash-checked `car-names.tsv`, derived from
car/rally facts in their ZIP readmes, not inferred from alias names. The binary
uses those labels only for the exact selected archive slots in the frontend
and stage text tables, including selection lists, lobby and results labels.
It does not rename other variants in the same family (Fabia replaces FA1,
not the original Focus). Short labels are used where the game requests its
abbreviated car names. Limits are 39/23 printable ASCII characters; labels
remain consistent across languages rather than inventing translations.

`--car-names original` restores original labels for this launch without
removing the skins; `--car-names skin` is the default with a selected overlay.
No overlay means no label changes. Invalid/missing label metadata falls back
to original text. Other localized text tables reuse numeric IDs and are
explicitly excluded. Saves, unlocks and underlying slot IDs are unchanged.
New metadata creates a new versioned cache; old caches remain untouched.
Native Mac label fit/appearance remains unverified.

### Local car alternatives

Use `--cars ALIAS[,ALIAS] --mods-dir "/path/to/owned/downloaded-zips"` in the
same binary command. `--help-enhancements` lists the11known alternatives.
The helper reads only exact hash-matched local ZIPs and verifies each required
member, then creates/reuses a private read-only car-file overlay. It does not
modify original or mod files and does not download anything. Keep original ZIP
filenames. Only one205and oneSubaru alternative can be selected; conflicting
choices stop before writing a pack. Six other replacement slots are independent.
No extra roster slots are added. The models, information and textures from each
selected ZIP stay together; `.cin` information may change physics/performance.
Only the supplied file variants are replaced (some are high-resolution only).
This is import/hash evidence, NOT native model, damage, physics, replay, UI or
render acceptance. HQ Cars, CMR2.5 and V6 automatic import is unsupported.

Use `--cars original` or omit the car flag next launch to use the selected
base data without this overlay. Existing mods already inside the chosen
`--data` remain part of that base, so "original" here cannot undo files you had
previously merged into that game installation. No original car restoration is
claimed. Author licenses still apply; no derived assets/readmes/Drive URLs are
included publicly, only source filenames, byte counts and hashes.

## Manual macOS / Linux fallback

Use Python 3.10+ and `uv` in the checked-out source directory:

```sh
uv venv tools/scenery-patch/.venv --python 3.12
uv pip install --python tools/scenery-patch/.venv/bin/python -r tools/scenery-patch/requirements.txt
uv run --no-project --python tools/scenery-patch/.venv/bin/python tools/scenery-patch/convert.py --input-dir /path/to/owned/game-data --output-dir /path/to/private/user-dir/scenery-pack --check-only
uv run --no-project --python tools/scenery-patch/.venv/bin/python tools/scenery-patch/convert.py --input-dir /path/to/owned/game-data --output-dir /path/to/private/user-dir/scenery-pack
```

`uv pip` installs dependencies into this local environment. Dependency install
may use package repositories; the patcher itself has no network calls. These
commands are designed for Mac/Linux; actual Mac execution remains untested.

Input must contain exactly one each of `frahit.bfl`, `fralot.bfl`, `finhit.bfl`
and `finlot.bfl`, recursively. All194 supported texture hashes and dimensions
are checked before output. Other releases reject rather than guess. Symlink
archives/directories are not followed. Output must be outside the input tree
and new or empty. Allow about110MB plus temporary working space. Originals are
never written. Temporary output is committed only after successful generation.

The output has3 archive-qualified tree mappings: `frahit.bfl/tre_md1.dds`,
`frahit.bfl/tre_md4.dds`, `finhit.bfl/tre_fir1.dds`. All other61 individual
trees, including low variants and rejected fr1, remain original, along with
76forest strips and6shadow masks. Full-canvas geometry is fitted by the engine;
cropped/nonplanar/invalid cards fall back to originals. Each tree includes
versioned source-canvas header and recipe metadata. Atlas uses continuous
cross-projection rather than repeated stripes; trunk samples opaque source
stem pixels.4x premultiplied interpolation is NOT generated new detail. The
private restored-detail candidate is a separate pack and is not bundled here.

One verified France spectator atlas has four articulated forearm frames;
the other47 retain12frames each of upper-body sway and2xnearest upscale.
These are not new3Dmodels or generic arm
animation. Atlas layout/feet stay fixed. `ticks=5` is interpreted by the
engine loader, not render FPS. Missing/legacy tree assets never disable
working spectators.

## Use and undo

Build current `main`; the source-fit and separate read-only asset-root loaders
are merged. Generate into a NEW folder outside the original game installation.
Do not overwrite a previous pack. For example, after the setup commands above:

```sh
uv run --no-project --python tools/scenery-patch/.venv/bin/python tools/scenery-patch/convert.py --input-dir "/path/to/owned/game-data" --output-dir "/path/to/private-assets/scenery-pack-sourcefit-v1" --tree-geometry crossed --check-only
uv run --no-project --python tools/scenery-patch/.venv/bin/python tools/scenery-patch/convert.py --input-dir "/path/to/owned/game-data" --output-dir "/path/to/private-assets/scenery-pack-sourcefit-v1" --tree-geometry crossed
./build/macos/src/opencmr2 --data "/path/to/owned/game-data" --scenery-dir "/path/to/private-assets/scenery-pack-sourcefit-v1"
```

Replace these example paths with your own. Omit `--tree-geometry crossed` for
the default shell experiment. Neither option is native-accepted. The new folder
must contain `manifest.tsv` directly, not another nested `scenery-pack` folder.
Do NOT add `--user-dir` to load artwork: it selects a different driver/save and
settings profile. `--scenery-dir` only reads assets and keeps the default
profile, progress, settings and logs where they already are. It does not copy,
move, migrate or overwrite anything. A previously chosen explicit `--user-dir`
can stay only when you intentionally want that profile.

The earlier pack had112entries (64trees+48spectators). This recipe produces51
(3experimental trees+48spectators). Headerless old tree OBJ files are rejected
on purpose; adding a header by hand cannot repair their geometry. Regenerate
from owned originals into the new folder. The existing pack remains usable
for its spectators, but it is not compatible tree geometry.

A parsed manifest is not proof of an active tree. Check stage-run logs for
`tree sidecar active (source-fitted successful override draw)`. Spectators have
a separate success line. The loader names rejected archive/source/asset and
reason, such as missing source-canvas header, dimensions, invalid source card,
or texture preparation. Cropped/nonplanar cards deliberately stay original,
so even a valid3mapping pack will not replace every placement.

To undo, launch without `--scenery-dir` (or select the previous folder). Existing
relative-pack settings can still load another pack; `scenery_overrides=0`
disables all overrides if needed. Do not change save roots as an undo step.
Keep generated artwork out of Git and public releases.

Source hashes originate in four privately supplied original archives.
The3 deterministic tree candidates were offscreen-GPU checked from their
exact original high-source RGBA, original XYZ/UV fit and stage scale, at four
angles. Those pixels are not native gameplay acceptance. This source revision also tests missing, corrupt and duplicate inputs,
unsafe paths, hash/dimension mismatch, no-output checks and atomic generation
with synthetic artwork. Native game matching, terrain, fog, shadows and Mac
performance are separate acceptance gates, not proven by generation success.

Run the synthetic safety tests with the same environment:

```sh
uv run --no-project --python tools/scenery-patch/.venv/bin/python tools/scenery-patch/test_convert.py
```

## Optional fixed-cutout experiment

Add `--tree-geometry crossed` to generate the same3 verified source mappings
as four fixed intersecting planes instead of the default silhouette shell.
It is not camera-facing and is not a full3Dtree. Source proportions/placements
still use PR27's source fit. It uses64triangles for fir1 and80 for broadleaf
including one opaque stem, rather than roughly1400-1800shell triangles, but
transparent overdraw and native frame time have not been measured.

The shell has visible horizontal tiers; the crossed candidate avoids that
terraced silhouette but can show intersecting canopy seams and shape changes
as the view rotates. It is not an accepted visual upgrade. Broadleaf canopy
alpha excludes its lower central stem and bottom6% to avoid repeated trunks
and source grass specks; the separate stem samples original opaque pixels.
Fir1 keeps original alpha. These masks are explicit modifications, not exact
alpha preservation. Textures are still4x interpolation, not restored detail.
No low variants or rejected fr1 are added. Recipe metadata records the mode.


### Actual motion coverage and diagnostics

`--spectators animated` now brings the original manually articulated forearm
demo into the production frame generator for the exact hash-verified
`frahit.bfl/spc_mn9a.dds` atlas: four poses, fixed lower body, repaired clean
plate, stable instance phases. This is 2D articulated texture motion, not
3D people or a generic skeleton. The other47atlases retain the earlier
subtle upper-body warp; their arms are not separately animated. An offline
demo or generated frames do not establish native gameplay success.

The debug panel scales with resolution (minimum14px). Its Crowd line shows
actual drawn simulation tick, frame index and observed frame changes for one
representative instance. The early runtime log reports selected frame assets
and bound-frame-change count. A successful first draw alone is not animation
acceptance. Paused simulation holds the pose; extra views do not advance time.

`--trees shell|crossed` still replaces only three authored source textures.
Other trees remain original; cropped-atlas cards remain fail-closed for3D
replacement. `--tree-wind on|off` adds subtle wind to hash-verified individual
2D tree cards, default on with a selected tree mode. It preserves original
UVs and fixes the lowest vertices, moving only the top by at most4.5%of card
height. This is not a3Dupgrade. Forest strips, composite meshes, shadow masks,
unverified sources and non-billboard objects are excluded. No mesh/collision
records or saved settings change. Runtime logs name active wind sources and
uncovered original tree sources. The Tree line reports cumulative successful
replacement draws, wind draws and distinct uncovered original sources, not
an exact count of all visible trees or frame-rate claim.

New recipes create a new private cache automatically; old caches are untouched.
Native Mac motion, tree appearance, label fit and performance remain separate
acceptance checks. The small wind deformation currently copies at most192
vertices per selected card draw; no native cost estimate is claimed.


### Foreground limb coverage expansion

The authored limb route now covers France `spe_05`, `spe_10`, `spc_mn1a`,
`spc_mn15a`, and original-demo `spc_mn9a`, in both high and low source archives.
That is10atlas files, not10people or33%of a stage. Each contains one moving
person: one of four in `spe_05`/`spe_10`, and one of two in the three pairs.
Masks, pivots and clean-plate repairs are explicit source-coordinate recipes.
The bottom30%is identical across poses. Instance phase stays staggered by
placement; changing near/far source variants does not remove limb motion.

These foreground groups match the user's supplied screenshots, but native
motion and clean-plate appearance still need checking in game. The low-source
poses retain the original low-detail pixels; enlarging them invents no detail.
After the expanded France recipes below,24atlas files retain the limited upper-body sway route.

The requested25%-33%visible-person target remains open. A provisional decoded
France01whole-stage audit counts290person-placements; these recipes cover22
moving placements (7.6%), not33%. Finland10currently has no authored limbs.
Whole-stage placement counts do not prove the current camera's visible ratio.
More group segmentation and per-view people-weighted diagnostics are needed.
No coverage percentage is inferred from the count of atlas files.

### Expanded France articulated coverage

Explicit source-space arm masks now also cover `spe_06`, `spe_07`, `spe_08`,
`spe_09`, `spc_mn7a`, `spc_mn11a`, and `spc_mn13a`, with their matching low-detail
sources. Each moves one person, not the whole atlas. Original placements,
collision, feet and lower-body pixels stay fixed. Stable placement phases keep
nearby groups from moving in lockstep.

In the held decoded France stage1 (Ballon), these and the earlier recipes cover
73 articulated person-placements out of 290 people, about25.2% across the whole
stage. This is not a per-camera visible/occlusion measurement or native visual
acceptance. No broader stage claim follows from regional texture availability.
Finland still has no articulated limb recipes. The roughly-one-in-three-visible
people target remains open. The previous22/290 count describes PR40 alone.

### Export only observed missing source evidence

For the debug/font and eight-tree coverage gap, this read-only tool scans an
owned installation and creates a new private ZIP. It exports only stage font
slot1 (`general/hel_20pt`, resolved from `Surface_LoadStageFonts`) and the exact
eight tree names reported by the runtime, with archive/member/hash provenance.
The archive scan is not proof of which duplicate the runtime selected.
No game files are changed and nothing is uploaded. Python3 standard library
only. Output must be new and outside the game-data directory.

```sh
python3 tools/scenery-patch/export-evidence.py \
  --data "/Users/dst/Documents/games/CMR2" \
  --output "$HOME/Desktop/cmr2-observed-scenery-evidence.zip"
```

Review `manifest.json` for missing names and archive parse failures. Do not
publish the resulting ZIP or add it to Git. It contains original game artwork.

### Finland articulated pairs

Six Finland pair atlases (`spc_ma02/05/07/08/11/12`) now have explicit forearm
masks and four poses, with both high/low source variants. Feet and lower30%
remain fixed. Most poses are modest sleeve/hand gestures, not cheering or
whole-body stretching. Original source bytes stay hash-gated.

The current full-motion request removes that65%placement gate: every verified
supported card animates. A fixed-feet, bounded2.5%height sway also moves all
people on multi-person cards; existing authored forearm poses remain. This is
2Dcard motion, not a skeleton or invented hidden anatomy. Replacement trees
receive the same fixed-base4.5%height wind as verified original tree cards.
Unsupported source identities remain original. Native appearance, culling,
reflection alignment and performance remain unverified.

## Integrated experimental asset preset

`opencmr2 --data /path/to/owned/game --asset-upgrades` prepares and selects the
supported broader trees/bushes, animated2D spectators, opaque Finland1/France1
2x ground textures, and Finland1 visual-only road-edge geometry. This combines
existing individual options; put overrides after the preset. It does not alter
originals, saved driver data, controls, or profiles. Use `--prepare-only` first.
The private cache is regenerated/reverified from the player's owned sources.
No generated art is in this repository or release package.

Finland hi/lo have separate render gates. Original contact HPC, FIN barriers,
AI route and surfaces are retained; no exact-edge physics mode is enabled.
New visual vertices are inside and projected onto original terrain. Interior
triangulation remains visually different by up to3.65cm at sampled centroids;
no claim of native driving/appearance/performance acceptance.

`--textures uprez` currently loads86exact-source opaque outputs:41Finland1
materials in hi/lo and4France1 road variants, with full mip chains. Finland
uses explicit clamp filtering matching the ground-parts sampler, while the
existing France road policy stays periodic. The source UV dimensions remain
unchanged. Unknown sources, foliage/people/font atlases remain original.
This is2x interpolation, not AI restoration or the complete644-asset pack.

Broader vegetation produces132mappings:84trees/bushes and48spectator variants.
Two narrow birches use fixed cutouts after shell previews failed visual review.
The76multi-tree strips and6shadow masks are not converted. Unsupported barriers,
hedges and Greece1 are still open. Native Metal must still be tested.


### Fixed forest-strip follow-up

`--asset-upgrades` / `--textures=uprez` now includes 76 original full-canvas
forest-strip variants and six Finland wooden-fence variants, alongside the
86 ground variants. Cutouts use premultiplied interpolation and per-mip alpha
coverage matching; no AI detail, crop, atlas repaint, or original modification.

Finland1 `temp.sht` forest parts additionally use an exact position/UV hash
allowlist and planar full-canvas quad check. Successful parts are tessellated
ninefold with small source-anchored relief and optional feet/perimeter-fixed
wind; unrelated materials and invalid parts keep the original draw. This is a
shallow multi-tree strip, NOT separate3Dtrees. Fence upgrades are texture-only.
Original material binding, stage lighting, world transform, collision records,
and physics vertices remain untouched. Generated positions and wind stay within each original quad axis-aligned
bounds, leaving existing culling bounds unchanged; native culling, appearance and performance
remain unverified. Use `--trees original` to retain original strip geometry,
`--tree-wind off` for static relief, or `--textures original` to retain originals.


### France1hedges/tree walls

The France1follow-up adds exact original `temp.sht` material runs for
`bsh_st1a-f`, `tre_w2a-d` and `tre_wla`. Most source quads are nonplanar, so their two ORIGINAL triangle domains and diagonal are retained.
No plane fitting or original-edge flattening: only barycentric subdivision,
small bounded relief and fixed-perimeter wind. Warped-quad support is allowed
only after the exact France source-position/UV hash gate; unknown/cropped or
malformed quads still refuse. Finland retains its strict planar rule.

Twelve France hedge texture variants and six barbed-wire fence variants bring
the exact-source texture total to186. Fences remain texture-only. All186have
actual runtime upload/all-mip comparisons against owned hi/lo originals with
an executor stub; that does not establish native GPU appearance or FPS.
France hi and lo each accept749original material runs,50,868submitted
triangles,748moving parts. The lone degenerate-axis part remains still.
This is shallow strip relief, not a solid fully3Dhedge or individual trees.


### France1opaque terrain coverage

`--textures=uprez` adds51France1opaque terrain materials, hi and lo:102new
outputs,288indexed outputs total. Selected from original temp.c3dmaterial
usage, not every texture in the archive. Source terrain UVs stay in0..1and
material clamp routing is retained. All new filtering uses explicit edge clamp.
Bridge/cropped/alpha materials are excluded rather than assumed safe atlases.
This is2xinterpolation, not AI-restored detail. No geometry, physics or collision
change. All288outputs pass actual runtime upload/all-mip/stage/identity/reset
checks with owned originals and stubbed executor. Native appearance/FPS open.
Runtime manifest limit raised to128KiB because the expanded metadata exceeds
the former32KiB limit; original bounded read and exact source/output checks stay.


### Fixed trackside texture coverage

Adds50hi/lo full-canvas fixed temp.shtmaterial variants: Finland banners,
France banners, stone walls, guardrails and static ribbon materials.338indexed
outputs total, all actual runtime upload/all-mip/stage/identity/reset validated
with owned originals and stub executor. Texture filtering only, no geometry or
physics change. Original UVs and clamp policy retained. Original/derived artwork
stays private. No AI detail or whole644pack claim. Alpha mip histogram counting
replaces repeated per-pixel scans; all338outputs are byte-identical to the old
algorithm. Native Metal appearance/FPS remain open.

### Portable exact-output texture recipe repair

The native main build can pass while texture preparation stops with "Recipe
output differs from reviewed image". Source validation has already passed at
that point. Native Lanczos output was not proven identical on the user's ARM
runtime. All338old output digests reproduce on local Pillow10.4,11.3and12.3;
this does not establish the exact cause of the native failure.

The stage-uprez path now uses integer-only premultiplied bilinear2xand area mips,
with rational alpha-coverage comparisons and explicit repeat/clamp addressing.
It no longer uses native floating resize. This is a deliberately more portable
filter, slightly softer than Lanczos. New reviewed output digests remain strict;
source hashes/stage gates are unchanged. Recipe changes select a new private
cache rather than deleting/reusing old outputs. Failure diagnostics now identify
archive/member and expected/actual digest plus Pillow version. No integrity
bypass, no source changes, no native acceptance claim.

### Greece tre* sources (opt-in)

`source-index.json` now also pins the 45 `tre_*` DDS cards of Greece `grehit.bfl`
and `grelot.bfl` by archive, name, SHA-256 and dimensions, taken from a
user-run `export-greece-trees.py` ZIP (both archives identity-verified). They
convert only with the experimental `--all-trees` flag (binary: `--tree-coverage all`) (adaptive crown/stem from
each card's own alpha); the reviewed default and crossed mappings are unchanged.
The five `tre_fal*` cards (cliff/rock textures) and the 14 box-like hedge/strip
cards (`tre_fra/frb/frc/frz`, `tre_s1_b/c/z`) are pinned but never converted.
The converted Greece cards need `--tree-coverage all` (or `--asset-upgrades`). Prepare now needs `grehit.bfl` and `grelot.bfl` in the game folder like
the Finland/France archives. Reviewed on the software contact sheet only
(`preview_trees.py`); native appearance is unverified.

### Reviewed fixed-cutout extension

`--trees crossed` now adds six exact-source mappings: Finland hi/lo
`tre_fir2` and `tre_fr3`, and France hi/lo `tre_md2`. Crossed mode therefore
prepares9tree mappings plus48spectator atlases (57entries), rather than
3plus48. Shell mode retains its3tree mappings unchanged. No `--all-trees`
flag is required for these six additions; other sources keep the existing
opt-in/fallback behavior. No artwork is bundled or downloaded.

All416matching original Finland1/France1 hi/lo object meshes passed the
actual runtime full-canvas source-fit routine, and the actual loader submitted
a replacement draw for each in a private owned-source test. GPU execution was
stubbed in that routing test. Separately, all six generated fixed-cutout models
were inspected with offscreen GPU rendering at0/30/60/90degrees. These are
still intersecting textured planes, not fully modelled trees or recovered
detail. France broadleaf lower-stem/grass masking follows the existing crossed
recipe; low-source cutouts remain coarse and can show repeated branches.

An audit of all8520individual tree/bush object meshes found8516full-canvas
fits and4rejections (France `tre_co5a`, two per quality). Those rejected cards
are not added. Passing this fit audit alone is not enough to add an unreviewed
source. Runtime cropped/nonplanar/source-identity rejection is unchanged. Native
Metal driving, distance transitions, wind, culling and performance remain
unverified. Recipe hashing creates a new private cache and leaves old caches,
original files, geometry, collisions and physics unchanged.

### Experimental original-card foliage fallback (611 variants)

Adds66exact-source foliage variants actually referenced by Finland1/France1
`temp.obj`:20Finland and13France materials in each hi/lo archive. Trees, shrubs
and bushes left on original cards can now use the same portable source2x
filter/mips as the other uprez textures. This does not create new detail or new
3D geometry. Existing shell/crossed-tree and animated-spectator replacements
keep their own generated textures; they are not replaced by this fallback.

All66identities also match the existing scenery source index. Original UVs,
material addressing, culling, alpha policy, geometry and physics are unchanged.
Spectator atlases are deliberately excluded. The previous545payloads remain
byte-identical. Runtime upload/every-mip byte checks, stage/source refusal and
reset tests pass for all66new variants. Same-display-size source/top-level/mip2
comparisons were inspected privately. Source2x alpha coverage differs by up to
1.55percentage points at mip0; tiny mips cannot exactly preserve coverage. This
is experimental source filtering, not recovered artwork. Native Metal driving,
distance transitions, culling and performance remain unverified; no claim that
all stage assets or all trees are now fully upgraded. `--textures original`
disables this fallback without changing originals.

### Generated tree sampling

Generated tree materials now upload a complete mip chain, using premultiplied
BGRA box filtering with nearest-attainable >=128 alpha coverage. Tree draws use
linear minification/magnification and trilinear mips; existing wrap addressing
is retained for the continuous circumference atlas. Original top-level TGA
pixels, generated geometry and UVs are unchanged. The three current shell
mappings add about3.33MB of mip data to10.01MB base pixel data (CPU/GPU
allocations and backend overhead are separate). No pack regeneration is needed.

This affects shell and crossed tree materials only. Spectator atlases retain
one level and point/no-mip sampling, so filtering cannot mix adjacent people
or frames. Original-card wind/fixed-strip paths retain their existing sampler.
Tree/atlas cache keys are distinct even if a manual pack reuses one path.

Private offscreen GPU comparisons of all three current shells at32/16/8px
and a240px view show smoother, softer distance sampling. Tiny branches still
disappear and shell tiers remain visible. Static comparisons do not establish
motion stability or native Metal performance. Actual loader tests verify every
uploaded mip byte and recorded sampler state, while identity/reset and legacy
fallback tests remain intact. This is a sampling fix, not new source detail,
new tree mappings, or native gameplay acceptance.
