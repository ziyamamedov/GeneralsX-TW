# Optional Total War controls (Zero Hour)

The Options menu has a **Controls** dropdown: **Classic** (default) and **Total War**.
Accept applies and saves the selection; Cancel discards it; Defaults selects Classic.
`ControlScheme = TotalWar` in the user's `Options.ini` enables the feature. A missing,
unknown or `Classic` value uses the original input path.

Total War temporarily uses right-click orders and disables right-button camera scrolling.
The existing `UseAlternateMouse` preference is retained for Classic; its checkbox is
disabled while Total War is selected. Switching back restores that preference and the
original camera-scroll policy. Selecting a mode does not require restarting the game.

For multiple ground units, dragging creates a single row starting at mouse-down and
extending toward the cursor. Units face perpendicular to the drag. A short drag keeps
the minimum spacing (the largest selected unit's footprint plus a gap), so the row may
extend beyond the cursor; longer drags spread it out. No rear ranks are created.

An ordinary group click translates the current arrangement so its center lands at the
clicked point. Positions and headings are captured at mouse-down; the center is the
average of those positions. Each unit keeps its relative offset and final heading.
This also preserves existing multi-row or irregular arrangements instead of creating
a new row. Preview markers and orders use the same destinations. These are arrival
positions: the normal pathfinder still handles travel, obstacles and map boundaries,
so units do not maintain a rigid formation while moving.

Only the individual triangle markers are drawn. A single unit stays at the clicked
destination while dragging selects its final facing. Ordinary single-unit clicks and
clicks with mixed selections containing unsupported units (such as aircraft) keep normal
movement. Object context orders such as attacking and garrisoning also remain unchanged.

## Feature implementation

- `GeneralsMD/Code/GameEngine/Source/GameClient/TotalWarControls.cpp`: preference loading,
  application and the dropdown, created with existing game widgets. No replacement of
  the retail `OptionsMenu.wnd` or changes to shared preference/global-data classes.
- `GeneralsMD/Code/GameEngine/Source/GameClient/FormationTranslator.cpp`: optional mouse
  gestures, previews and sending orders. Classic passes every message through unchanged.
- `GeneralsMD/Code/GameEngine/Include/GameClient/FormationLayout.h`: preview geometry.
- `GeneralsMD/Code/GameEngine/Source/GameLogic/AI/FormationAI.cpp`: synchronized order
  validation, move-then-face command setup, and both formation AI states.

## Upstream integration points

Small hooks remain in the Zero Hour options menu, client initialization/reset, preview
drawing, AI command dispatcher and state registration. The shared camera and logic
dispatcher hooks are guarded by `RTS_ZEROHOUR`. Base Generals is unchanged.

Keep the appended message, AI command and state IDs stable when merging upstream.
Keep the two formation state registrations in `GameMemoryInitPools_GeneralsMD.inl`.
Their names must match the classes in `FormationAI.cpp`.

The control scheme is a **local input preference**. Never check it in the synchronized
formation-order handler or AI states: replays and orders from other players must execute
the same way regardless of the local player's controls. Existing formation orders continue
normally after switching to Classic.

## Validation

Run the standalone geometry test using the command in `scripts/qa/formation-layout-test.cpp`.
Build `z_generals`, play an original replay and a replay containing formation commands,
and check the dropdown's default, Accept, Cancel, Defaults and restart persistence.
For interaction checks, compare ordinary right-click/drag behavior in Classic with the
single-unit facing, anchored group rows and centered group clicks in Total War, then
switch back during a match. Check short/long drags in both directions, followed by a
click after arrival: the row should retain its spacing and heading around the new center.
