# Optional Total War controls (Zero Hour)

The Options menu has a **Controls** dropdown: **Classic** (default) and **Total War**.
Accept applies and saves the selection; Cancel discards it; Defaults selects Classic.
`ControlScheme = TotalWar` in the user's `Options.ini` enables the feature. A missing,
unknown or `Classic` value uses the original input path.

Total War temporarily uses right-click orders and disables right-button camera scrolling.
The existing `UseAlternateMouse` preference is retained for Classic; its checkbox is
disabled while Total War is selected. Switching back restores that preference and the
original camera-scroll policy. Selecting a mode does not require restarting the game.

Total War camera controls:

- **W/S/A/D** pan forward/back/left/right relative to the view, replacing arrow-key panning.
  Diagonals work; movement uses the existing keyboard scroll-speed setting.
- **Hold the middle mouse button and drag horizontally** to rotate. Releasing stops
  rotation; a short click does not reset the camera. The wheel still controls zoom.
- WASD are reserved for the camera, including Shift+WASD. Their unmodified retail
  commands (including S for Stop and panel hotkeys) must be used through the command
  panel instead. Ctrl/Alt combinations retain their existing bindings.
- Menus, chat/text focus, disabled input and app focus loss cancel held camera input.
  After cancellation, press the key/button again to resume. Classic retains its bindings.

Total War mouse-order modifiers:

- **Alt** shows existing unit/destination markers without queueing orders. Ordinary
  right-clicks and formation drags remain available while the markers are visible.
- **Shift + right-click** appends native movement waypoints, previously assigned to Alt.
  Shift no longer adds units to a mouse selection. Queued movement uses the original
  waypoint behavior rather than creating a new formation preview.
- **Command on macOS / Ctrl on Windows or Linux + LMB click or selection box** adds
  units to the selection. Clicking a selected unit toggles it out, as Shift did before.
- **Hold J while issuing an order** forces attack, including on neutral targets.
  Release J to return to normal orders. J takes priority over additive selection when
  both J and Command/Ctrl are held. Menus, text focus and app focus loss cancel the hold.
- **C** and its combinations retain their original commands, including Ctrl+C cheer.
- Former **J** command-panel actions move to **K**, or another unused letter if K is
  already used in that panel. Their tooltips show the replacement. The choice checks
  both panel hotkeys and loaded global shortcuts, without changing retail assets.
  Any custom global J binding moves to K with its original modifiers.

Alt+LMB formation dragging and Command/Ctrl rotation remain available as described
below. Other keyboard shortcuts, including numbered control-group bindings, retain
their existing combinations. These changes apply only to the Total War scheme.

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

To reposition an existing formation, hold **Alt** and **drag LMB from an already selected
ground unit**. The markers follow the cursor with the captured arrangement's center
under it; all relative offsets and individual headings are retained, including multiple
ranks. Starting on empty ground or an unselected unit keeps normal selection behavior.
Only eligible mobile ground units participate; aircraft and buildings are not moved.

While holding LMB, add **Command on macOS / Ctrl on Windows or Linux**, then drag
horizontally to rotate the arrangement and all its headings around the fixed preview
center. Release the modifier and move the mouse to resume cursor-centered translation.
Releasing the modifier without moving keeps the last preview in place, so releasing
both keys/buttons together does not accidentally shift the destination. Starting the
gesture with Alt+Command/Ctrl already held rotates around the formation's original center.

Release **LMB** to issue the previewed orders; **Escape** cancels. Alt may be released
after starting the gesture without losing it. A selection change, menu, text focus,
disabled input or app focus loss cancels the preview. A release consumed by the HUD is
cancelled instead of issuing a hidden order. These gestures are local to Total War mode
and use the existing formation-order protocol. Normal pathfinding still controls travel.

Hold **Alt** without dragging to show pale blue ground triangles for all your selectable
mobile ground units, including unselected ones. An active formation order shows its
destination and final heading; after arrival, its marker sits beneath the unit. Idle,
stopped or attacking units show their current position and heading. Ordinary move orders
and waypoint queues show their destination (the last queued point) with the current heading.
Markers disappear on Alt release and are hidden in menus, text input and Classic mode.

When placing a new formation while holding Alt, the active preview stays green and the
other units' markers remain visible, so a new rank can be placed behind an existing one.
Units in the active preview do not also display their old order markers. The overlay
reads live AI state rather than retaining a history of clicks: replacement orders, Stop,
arrival, death and loading a save cannot leave stale destination markers. Terrain and
pathfinding can adjust actual arrival positions; markers then follow those positions.

## Feature implementation

- `GeneralsMD/Code/GameEngine/Source/GameClient/TotalWarControls.cpp`: preference loading,
  application and the dropdown, created with existing game widgets. No replacement of
  the retail `OptionsMenu.wnd` or changes to shared preference/global-data classes.
- `GeneralsMD/Code/GameEngine/Source/GameClient/FormationTranslator.cpp`: optional mouse
  gestures, previews and sending orders. Classic passes every message through unchanged.
- `GeneralsMD/Code/GameEngine/Include/GameClient/FormationLayout.h`: preview geometry.
- `GeneralsMD/Code/GameEngine/Include/GameClient/FormationDrag.h`: preserved formation
  translation/rotation state and modifier transitions, independent of rendering/input devices.
- `GeneralsMD/Code/GameEngine/Source/GameClient/TotalWarCamera.cpp`: input reservation,
  WASD scrolling and middle-button rotation. `TotalWarCameraInput.h` holds the testable
  key ownership state. Input is routed after windows but before gameplay hotkeys; the
  shared camera applies the resulting offset and keeps its normal replay recording.
- `GeneralsMD/Code/GameEngine/Source/GameClient/TotalWarInput.cpp`: local modifier
  modes and collision-aware replacements for J. `TotalWarModifierInput.h` holds the
  independently testable state. Small Zero Hour-guarded hooks in the shared meta-event,
  hotkey, selection and command-tooltip paths preserve Classic behavior.
- `GeneralsMD/Code/GameEngine/Source/GameLogic/AI/FormationAI.cpp`: synchronized order
  validation, move-then-face command setup, both formation AI states and a read-only
  marker query exposed by the Zero Hour `AIUpdateInterface`. The query stores no data
  and changes neither simulation state nor the save/replay format.

## Upstream integration points

Small hooks remain in the Zero Hour options menu, client initialization/reset, preview
drawing, AI command dispatcher and state registration. The shared camera and logic
dispatcher hooks are guarded by `RTS_ZEROHOUR`. Base Generals is unchanged.
The shared keyboard exposes a read-only `isKeyDown` query to catch key releases
consumed by a GUI window; it does not change how either game's keyboard is processed.
The shared hotkey manager exposes a read-only membership query for choosing a free key.

Keep the appended message, AI command and state IDs stable when merging upstream.
Keep the two formation state registrations in `GameMemoryInitPools_GeneralsMD.inl`.
Their names must match the classes in `FormationAI.cpp`.

The control scheme is a **local input preference**. Never check it in the synchronized
formation-order handler or AI states: replays and orders from other players must execute
the same way regardless of the local player's controls. Existing formation orders continue
normally after switching to Classic.

## Validation

Run the standalone geometry test using the command in `scripts/qa/formation-layout-test.cpp`.
Run `scripts/qa/total-war-camera-input-test.cpp` using its header command for camera
direction, diagonal/opposing keys, key-up ownership, modifiers and cancellation checks.
Run `scripts/qa/total-war-modifiers-test.cpp` using its header command for queue/selection/
force-attack combinations, J release ownership, cancellation and replacement-key conflicts.
Build `z_generals`, play an original replay and a replay containing formation commands,
and check the dropdown's default, Accept, Cancel, Defaults and restart persistence.
For interaction checks, compare ordinary right-click/drag behavior in Classic with the
single-unit facing, anchored group rows and centered group clicks in Total War, then
switch back during a match. Check short/long drags in both directions, followed by a
click after arrival: the row should retain its spacing and heading around the new center.
Check WASD with and without selected units, release over the HUD, chat typing, menu/app
focus transitions, middle-button rotation/click/zoom and switching back to Classic.
For Alt+LMB, arrange tanks, anti-air and infantry in separate ranks, select them together,
and drag from a selected unit. Check the centered markers and the final arrangement.
Add/remove Command/Ctrl during the drag, rotate in both directions and release it together
with LMB: the issued positions/headings should match the last preview. Also check rotation
in place, a single unit, Alt release before LMB, Escape, release over the HUD, selection
changes and starting on empty ground/an unselected unit. The geometry test covers rigid
transforms, mixed headings, a fixed rotation pivot and modifier transition behavior;
it does not replace interactive checks of the actual mouse/key gesture.
For the Alt overlay, issue a formation order, deselect that group and hold Alt while it
travels and after it arrives. Select another group and place it behind those markers.
Check replacement moves, Stop, attack orders, destroyed/contained units, Alt release,
menu/text focus, and switching to Classic; old destinations must not linger.
For remapped modifiers, hold Alt while sending normal moves and drawing new rows: they
must replace the previous order while blue markers remain visible. Shift+RMB several
locations must create the native waypoint queue. Command/Ctrl+LMB and selection boxes
must add units; Shift+selection must not. Hold J to attack neutral buildings, then
release it and check normal orders. Check J release over the HUD, chat typing, pause
and focus loss. Confirm C still activates its original panel action. On a command panel
with a J action, verify its replacement tooltip hotkey and activation, including a panel
where K is already occupied. Switching to Classic must restore native Alt/Shift/Ctrl/J
behavior. These interaction checks need a real keyboard
and mouse; standalone tests and replays do not validate the input gesture itself.
