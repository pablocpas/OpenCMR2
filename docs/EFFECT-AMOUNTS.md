# Effect amounts

Open Graphics > Detail > Effect amounts. Up/Down moves across three pages;
Left/Right changes a value. OK saves and applies; Back discards this page's edits.
The parent Detail page has its own OK. All settings are in [video], not a save
profile migration. No setting is written merely by opening the page.

- Motion blur percent is previous-frame mix, capped at35 in this menu to limit
  trails. Existing INI values above35 are not rewritten until this page's OK.
- Bloom percent is glow blend; precipitation density affects added rain/snow,
  not dust, original stage weather or grip.
- Shadow quality is512/1024/2048 map size. Softness is filter radius, not opacity.
  Shadows remain fixed-origin/direction and exclude alpha foliage.
- Defocus blur percent changes maximum blur, not focus distance in metres.
  Focus/range remain existing normalized-depth settings, not automatic focus.
- Sun rays percent changes the projected-sun effect. Hidden sun may skip it.
- Grade blend scales the restrained analytic grade;0 is neutral, not a LUT.
- Camera Shake on Detail is separate from lens dirt and flare. Their amounts are
  independent. New keys inherit the old camera_effects toggle until saved.
  The operator camera continues suppressing random chase shake.
- Windscreen Dirt percent on Detail now scales accumulation, including fractional
  increments, rather than treating every nonzero number equally.
- Dust amount scales world-volume density and fallback cloud opacity. It does not
  change water/snow/sparks. Dust plume and world dust each retain their own toggle.
  World-volume availability/fallback still depends on the renderer.
- Cube reflection size is Stage/64/128/256. It applies when cube textures are
  created on stage reload. This is quality, not reflection strength.
- Atmosphere... lives directly on the Graphics options page (moved out of
  Effect amounts); it is a separate submenu (Menu::items is
  layout-locked). Fog, fog amount/glow and air motes amount live there and in
  the F10 panel; all default off. Fog amount defaults to 65 (100 is the heavy look); see
  docs/ATMOSPHERE.md.

Both GL and Metal consume the same bounded effect reader. These controls do not
prove native pixels. Full-frame depth effects include HUD and use nonlinear
normalized depth. Lens overlay drawing retains its existing projected-challenge
path; gameplay reach needs native verification, not an assertion from menu state.

This checkpoint does not implement automatic stage-weather selection, autofocus,
reflection blend or the later distance/shadow grouping. Those remain separate.
