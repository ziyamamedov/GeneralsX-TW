# Optional Total War controls (Zero Hour)

The Options menu has a **Controls** dropdown: **Classic** (default) and **Total War**.
Accept applies and saves the selection; Cancel discards it; Defaults selects Classic.
`ControlScheme = TotalWar` in the user's `Options.ini` enables the feature. A missing,
unknown or `Classic` value uses the original input path.

Total War temporarily uses right-click orders and disables right-button camera scrolling.
The existing `UseAlternateMouse` preference is retained for Classic; its checkbox is
disabled while Total War is selected. Switching back restores that preference and the
original camera-scroll policy. Selecting a mode does not require restarting the game.

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
single-unit facing and group line previews in Total War, then switch back during a match.
