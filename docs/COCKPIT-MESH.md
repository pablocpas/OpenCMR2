# Cockpit add-on mesh (video.cockpit_mesh) and interior dump (video.dump_interior)

Model extra cockpit pieces (roof or headliner, pillars, upper side, gap fillers) in Blender
and see them in the game. The original `a5.c3d` is never rewritten. Both files live in the
user directory (the folder that holds `opencmr2.ini` and `opencmr2.log`) and are never committed.

## 1. Dump the scaffold

    OpenCMR2 --dump-interior 1

Start a stage in the cockpit view. One time per stage load the game writes
`<user dir>/interior_dump.obj` and logs `interior dump: car N wrote T triangles ...`.
The dump holds every mesh part of the interior (dash, door cards, seats, wheel, hands...) in
the interior root node's space, game axes written unchanged (x forward, y up, z lateral), scale 1.
Groups are named `n<node>_p<part>`; each has material `tex<N>` (N = interior texture id).
Moving parts (wheel, wipers) are written at their pose at dump time.
Texture pixels are not exported (assets are private); the UVs are in the file.

## 2. Blender

Import: File > Import > Wavefront (.obj), Forward Axis = X, Up Axis = Y, scale 1.0.
Keep the scaffold as reference (do not edit or export it). Add new pieces as separate objects.

UV rules:
- Unwrap onto the existing interior texture. Copy UVs from a nearby scaffold face or place the
  UV island over the part of the atlas you want to reuse (the scaffold's UVs show where).
- Give every new face a material named exactly `tex<N>` (for example `tex1`). Blender's `.001`
  suffix is fine. Faces with any other material are skipped (the log counts them).

Export only the new objects: File > Export > Wavefront (.obj), Selection Only on,
Forward Axis = X, Up Axis = Y (the same as import), scale 1.0, Include UVs on, Write Materials
on or off (ignored), Triangulate Faces optional (quads and n-gons are split by the game),
Normals optional. Save as `<user dir>/cockpit_mesh.obj`.

## 3. See it in the game

    OpenCMR2 --cockpit-mesh 1

The file is re-read when its timestamp changes (polled about twice a second), so re-export
in Blender and look at the cockpit again. The log shows `cockpit mesh: loaded T triangles ...`
or the reason a file was rejected (face index out of range, more than 100000 triangles).
Pieces draw depth tested with the interior, unlit, with no culling.

## Limits

- Add-on pieces only: original interior vertices cannot be edited this way.
- Subaru cockpit, one file per user directory (all cars share it).
- Draws only in the cockpit view's 3D pass.
- Rendering was CI-compiled only; native Metal acceptance is the user's.
