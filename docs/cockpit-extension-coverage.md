# Both-side cockpit extension (2026-11-05)

The old driver-side rear strip did not cover the reported right-hand gap below
the dashboard. The original Subaru passenger door/floor stops much farther
forward. Register both actual rear edges and connect both side walls and floor
strips to the requested rear plane with the existing cockpit-extend-rear flag.
Keep current FOV, eye shift, and camera move settings unchanged.

The strips now span and repeat the source door-card UV patch per bounded cell,
not one UV point. Each cell stays within the source atlas patch, the final cell
crops it. No atlas wrap or invented mirrored-gradient pattern. The source patch
is a smooth blue gradient: this does not invent a patterned door fabric. The
original door-extend deformation remains separate and unchanged.

Tests use the production geometry helper for connected cells, partial repeats,
source-UV bounds, finite positions and both side/floor coverage. A private
asset-backed offline preview drives that exact helper and shows the below-dash
passenger opening filled at unchanged eye/FOV. It is NOT native Metal proof.
Assets/previews are not committed. Native view still needs confirmation.

Limits: wall/floor continuation only, not a new passenger window, pillar or roof.
The old one-shot corner probe is labelled driver-edge only and its 16.16 pixel
conversion is fixed; corner counts do not prove triangle coverage.
