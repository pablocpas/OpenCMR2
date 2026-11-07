# OpenCMR2 cockpit add-on mesh helper for Blender (3.6+ / 4.x).
#
# Companion to docs/COCKPIT-MESH.md and the game's video.dump_interior /
# video.cockpit_mesh options. Two buttons in the 3D view sidebar (OpenCMR2
# tab):
#
#   Import interior scaffold  - imports interior_dump.obj with the axis
#                               settings the dump header asks for (Forward X,
#                               Up Y) into a locked "OCMR2 scaffold" collection
#                               so it cannot be edited or exported by accident.
#   Validate + export add-ons - exports every mesh object OUTSIDE the scaffold
#                               collection as cockpit_mesh.obj (same axis
#                               settings, selection only, UVs, triangulated),
#                               after checking that every face has a material
#                               named tex<N> (Blender's .001 suffix is fine,
#                               the game reads the leading number). The
#                               exported file is then re-validated with the
#                               same rules the game's parser
#                               (game/CockpitMesh.h, CockpitMesh_ParseObj)
#                               applies, so index errors and missing tex<N>
#                               materials are caught before the game sees
#                               them.
#
# Install: Blender > Edit > Preferences > Add-ons > Install... > pick this
# file, enable "OpenCMR2 cockpit mesh helper". Or paste it into the Scripting
# workspace and Run Script (the panel registers the same way).
#
# The pure validator below runs without Blender too; tests/blender_helper.py
# exercises it with plain python3.

import os
import re

try:
    import bpy
    from bpy_extras.io_utils import ImportHelper, ExportHelper
    BLENDER = True
except ImportError:
    BLENDER = False

SCAFFOLD_COLLECTION = "OCMR2 scaffold (do not edit)"
MAX_TRIS = 100000   # kCmMaxTris in game/CockpitMesh.h
# Same acceptance as CockpitMesh_ParseObj: "tex" + decimal (sscanf %d stops at
# any non-digit, so Blender's ".001" suffixes are fine).
TEX_RE = re.compile(r"^tex(\d+)")


def valid_material_name(name):
    return bool(name) and TEX_RE.match(name) is not None


def validate_obj_text(text, max_tris=MAX_TRIS):
    """Mirror of CockpitMesh_ParseObj (game/CockpitMesh.h): v, vt, f (v,
    v/vt, v//vn, v/vt/vn, negative indices, n-gons as fans) and usemtl tex<N>.
    Returns a dict: triangles, skipped_no_texture, skipped_bad, error.
    error is empty unless the game would reject the whole file."""
    stats = {"triangles": 0, "skipped_no_texture": 0, "skipped_bad": 0, "error": ""}
    pos, uv = [], []
    tex = -1
    for raw in text.splitlines():
        line = raw.rstrip("\r")
        if line.startswith("v "):
            parts = line[2:].split()
            if len(parts) >= 3:
                try:
                    pos.append((float(parts[0]), float(parts[1]), float(parts[2])))
                except ValueError:
                    pass
        elif line.startswith("vt "):
            parts = line[3:].split()
            if len(parts) >= 2:
                try:
                    uv.append((float(parts[0]), float(parts[1])))
                except ValueError:
                    pass
        elif line.startswith("usemtl"):
            q = line[6:].strip()
            m = TEX_RE.match(q)
            tex = int(m.group(1)) if m else -1
        elif line.startswith("f "):
            tokens = line[2:].split()
            pi = []
            for tok in tokens:
                fields = tok.split("/")
                try:
                    a = int(fields[0])
                except ValueError:
                    stats["error"] = "bad face token"
                    return stats
                ai = a - 1 if a > 0 else len(pos) + a
                if ai < 0 or ai >= len(pos):
                    stats["error"] = "face index out of range"
                    return stats
                if len(fields) > 1 and fields[1] != "":
                    try:
                        t = int(fields[1])
                    except ValueError:
                        stats["error"] = "bad face token"
                        return stats
                    ti = t - 1 if t > 0 else len(uv) + t
                    if ti < 0 or ti >= len(uv):
                        stats["error"] = "face index out of range"
                        return stats
                pi.append(ai)
            if len(pi) < 3:
                stats["skipped_bad"] += 1
                continue
            if tex < 0:
                stats["skipped_no_texture"] += len(pi) - 2
                continue
            for _ in range(1, len(pi) - 1):
                if stats["triangles"] >= max_tris:
                    stats["error"] = "too many triangles"
                    return stats
                stats["triangles"] += 1
    return stats


if BLENDER:

    class OCMR2_OT_import_scaffold(bpy.types.Operator, ImportHelper):
        bl_idname = "ocmr2.import_scaffold"
        bl_label = "Import interior scaffold"
        bl_description = ("Import interior_dump.obj (Forward X, Up Y) into a "
                          "locked reference collection")
        filename_ext = ".obj"
        filter_glob: bpy.props.StringProperty(default="*.obj", options={"HIDDEN"})

        def execute(self, context):
            before = set(bpy.data.objects)
            try:
                bpy.ops.wm.obj_import(filepath=self.filepath, forward_axis="X", up_axis="Y")
            except TypeError:   # Blender 3.x operator and argument names
                bpy.ops.import_scene.obj(filepath=self.filepath, axis_forward="X", axis_up="Y")
            coll = bpy.data.collections.get(SCAFFOLD_COLLECTION)
            if coll is None:
                coll = bpy.data.collections.new(SCAFFOLD_COLLECTION)
                context.scene.collection.children.link(coll)
            moved = 0
            for obj in [o for o in bpy.data.objects if o not in before]:
                for c in list(obj.users_collection):
                    c.objects.unlink(obj)
                coll.objects.link(obj)
                obj.hide_select = True   # reference only: no accidental edits
                moved += 1
            self.report({"INFO"}, "scaffold: %d object(s) locked in '%s'" % (moved, coll.name))
            return {"FINISHED"}

    def scene_addon_objects(context):
        scaffold = bpy.data.collections.get(SCAFFOLD_COLLECTION)
        skip = set(scaffold.objects) if scaffold else set()
        return [o for o in context.scene.objects if o.type == "MESH" and o not in skip]

    class OCMR2_OT_export_addon(bpy.types.Operator, ExportHelper):
        bl_idname = "ocmr2.export_addon"
        bl_label = "Validate + export add-ons"
        bl_description = ("Export every mesh outside the scaffold collection as "
                          "cockpit_mesh.obj, checking tex<N> materials first")
        filename_ext = ".obj"
        filter_glob: bpy.props.StringProperty(default="*.obj", options={"HIDDEN"})

        def invoke(self, context, event):
            if not self.filepath:
                self.filepath = "cockpit_mesh.obj"
            return super().invoke(context, event)

        def execute(self, context):
            objs = scene_addon_objects(context)
            if not objs:
                self.report({"ERROR"}, "no add-on meshes: model new pieces outside the scaffold collection first")
                return {"CANCELLED"}
            problems = []
            for o in objs:
                if not o.data.materials:
                    problems.append("%s: no material (faces would be skipped by the game)" % o.name)
                for m in o.data.materials:
                    if m and not valid_material_name(m.name):
                        problems.append("%s: material '%s' is not tex<N>" % (o.name, m.name))
            if problems:
                for p in problems[:10]:
                    self.report({"ERROR"}, p)
                return {"CANCELLED"}
            bpy.ops.object.select_all(action="DESELECT")
            for o in objs:
                o.select_set(True)
            try:
                bpy.ops.wm.obj_export(filepath=self.filepath, export_selected_objects=True,
                                      forward_axis="X", up_axis="Y", export_uv=True,
                                      export_materials=True, export_triangulated_mesh=True,
                                      apply_modifiers=True)
            except TypeError:   # Blender 3.x operator and argument names
                bpy.ops.export_scene.obj(filepath=self.filepath, use_selection=True,
                                         axis_forward="X", axis_up="Y", use_uvs=True,
                                         use_materials=True, use_triangles=True,
                                         use_mesh_modifiers=True)
            try:
                with open(self.filepath, "r", encoding="utf-8", errors="replace") as f:
                    stats = validate_obj_text(f.read())
            except OSError as e:
                self.report({"ERROR"}, "export written but re-validation failed: %s" % e)
                return {"CANCELLED"}
            if stats["error"]:
                self.report({"ERROR"}, "the game would reject this file: %s" % stats["error"])
                return {"CANCELLED"}
            self.report({"INFO"},
                        "cockpit mesh: %d triangles (%d faces without tex<N> skipped, %d bad faces) -> %s"
                        % (stats["triangles"], stats["skipped_no_texture"], stats["skipped_bad"],
                           os.path.basename(self.filepath)))
            return {"FINISHED"}

    class OCMR2_PT_panel(bpy.types.Panel):
        bl_label = "OpenCMR2 cockpit mesh"
        bl_space_type = "VIEW_3D"
        bl_region_type = "UI"
        bl_category = "OpenCMR2"

        def draw(self, context):
            col = self.layout.column(align=True)
            col.operator("ocmr2.import_scaffold", icon="IMPORT")
            col.operator("ocmr2.export_addon", icon="EXPORT")
            scaffold = bpy.data.collections.get(SCAFFOLD_COLLECTION)
            n = len(scene_addon_objects(context))
            col.label(text="add-on meshes: %d" % n)
            if scaffold:
                col.label(text="scaffold objects: %d (locked)" % len(scaffold.objects))

    def register():
        bpy.utils.register_class(OCMR2_OT_import_scaffold)
        bpy.utils.register_class(OCMR2_OT_export_addon)
        bpy.utils.register_class(OCMR2_PT_panel)

    def unregister():
        bpy.utils.unregister_class(OCMR2_PT_panel)
        bpy.utils.unregister_class(OCMR2_OT_export_addon)
        bpy.utils.unregister_class(OCMR2_OT_import_scaffold)

    if __name__ == "__main__":
        register()

bl_info = {
    "name": "OpenCMR2 cockpit mesh helper",
    "blender": (3, 6, 0),
    "category": "Import-Export",
    "description": "Import the OpenCMR2 interior scaffold and export add-on cockpit meshes (docs/COCKPIT-MESH.md)",
}
