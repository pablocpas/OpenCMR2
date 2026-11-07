# Unit tests for the pure OBJ validator in tools/blender/cockpit_mesh_helper.py
# (mirrors game/CockpitMesh.h CockpitMesh_ParseObj) plus wiring checks.
import importlib.util
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("cockpit_mesh_helper", root / "tools/blender/cockpit_mesh_helper.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
validate = mod.validate_obj_text

# Material names: same acceptance as CockpitMesh_ParseObj (tex + leading digits,
# Blender's .001 suffix fine), anything else rejected.
assert mod.valid_material_name("tex1")
assert mod.valid_material_name("tex12.003")
assert mod.valid_material_name("tex1x")
assert not mod.valid_material_name("tex")
assert not mod.valid_material_name("Tex1")
assert not mod.valid_material_name("material")
assert not mod.valid_material_name("")

QUAD = """\
v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
vt 0 0
vt 1 0
vt 1 1
vt 0 1
usemtl tex1
f 1/1 2/2 3/3 4/4
"""
s = validate(QUAD)
assert s == {"triangles": 2, "skipped_no_texture": 0, "skipped_bad": 0, "error": ""}, s

# Negative indices and v//vn forms parse.
NEG = """\
v 0 0 0
v 1 0 0
v 0 1 0
usemtl tex0
f -3//1 -2//1 -1//1
"""
s = validate(NEG)
assert s["triangles"] == 1 and not s["error"], s

# Faces before any usemtl, or after a non-tex material, are skipped per n-gon triangle.
SKIP = """\
v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
f 1 2 3 4
usemtl wood
f 1 2 3
usemtl tex2
f 1 3 4
f 1 2
"""
s = validate(SKIP)
assert s["triangles"] == 1 and s["skipped_no_texture"] == 3 and s["skipped_bad"] == 1 and not s["error"], s

# Out-of-range position and uv indices are hard errors (the game rejects the file).
BAD_V = "v 0 0 0\nusemtl tex1\nf 1 2 3\n"
assert validate(BAD_V)["error"] == "face index out of range"
BAD_VT = "v 0 0 0\nv 1 0 0\nv 0 1 0\nusemtl tex1\nf 1/9 2/9 3/9\n"
assert validate(BAD_VT)["error"] == "face index out of range"
BAD_TOK = "v 0 0 0\nusemtl tex1\nf 1 x 3\n"
assert validate(BAD_TOK)["error"] == "bad face token"

# Triangle cap (kCmMaxTris): the file is rejected once it would exceed it.
tri = "v 0 0 0\nv 1 0 0\nv 0 1 0\nusemtl tex1\n" + "f 1 2 3\n" * 12
s = validate(tri, max_tris=10)
assert s["error"] == "too many triangles" and s["triangles"] == 10, s
s = validate(tri)
assert s["triangles"] == 12 and not s["error"], s

# The validator's cap must stay in sync with the game parser.
assert "kCmMaxTris = %d" % mod.MAX_TRIS in (root / "game/CockpitMesh.h").read_text()

# Docs point at the helper; the add-on registers the operators it promises.
doc = (root / "docs/COCKPIT-MESH.md").read_text()
assert "cockpit_mesh_helper.py" in doc
src = (root / "tools/blender/cockpit_mesh_helper.py").read_text()
for op in ("ocmr2.import_scaffold", "ocmr2.export_addon"):
    assert op in src, op
assert "Import interior scaffold" in doc and "Validate + export add-ons" in doc
# The game's file and axis conventions are what the add-on writes.
assert "interior_dump.obj" in doc and "cockpit_mesh.obj" in doc
print("ok blender helper")
