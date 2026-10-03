// GeneralsX @feature Codex 04/10/2026 Regression tests for held modes, cancellation and free J replacements.
// c++ -std=c++20 -I GeneralsMD/Code/GameEngine/Include scripts/qa/total-war-modifiers-test.cpp \
//   -o /tmp/total-war-modifiers-test && /tmp/total-war-modifiers-test
#include "GameClient/TotalWarModifierInput.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main()
{
	TotalWarModifierInput input;
	// Alt deliberately has no input here: it cannot arm queueing or force attack.
	for (bool shift : { false, true })
		for (bool control : { false, true })
		{
			auto modes = input.modes(shift, control, true);
			assert(modes.queue == shift && modes.selection == control && !modes.forceAttack);
		}
	assert(input.keyEvent(true, false, true));
	auto modes = input.modes(false, false, true);
	assert(modes.forceAttack && !modes.selection && !modes.queue);
	modes = input.modes(true, true, true);
	assert(modes.queue && modes.forceAttack && !modes.selection); // Force attack wins over additive selection.
	assert(input.keyEvent(false, false, true));
	modes = input.modes(false, true, true);
	assert(modes.selection && !modes.forceAttack); // J-up restores held Ctrl selection.
	assert(input.keyEvent(true, false, true));
	input.cancel(); // Menu, text focus, input disable or control-scheme change.
	assert(input.keyEvent(true, true, true));
	assert(!input.modes(false, false, true).forceAttack); // Repeat cannot restart a cancelled hold.
	assert(input.keyEvent(false, false, false)); // Still consume our release outside gameplay.
	assert(!input.keyEvent(true, false, false)); // Typing J in chat does not become an attack modifier.
	assert(!input.keyEvent(false, false, true));
	assert(input.keyEvent(true, false, true));
	input.checkReleased(false); // Release swallowed by a GUI window.
	assert(!input.modes(false, false, true).forceAttack);
	assert(input.keyEvent(false, false, true));
	modes = input.modes(true, true, false);
	assert(!modes.queue && !modes.selection && !modes.forceAttack);

	unsigned int occupied = 0;
	assert(TotalWarModifierInput::replacementLetter(occupied) == 'k');
	// Every fallback must be genuinely unused and avoid camera/global hotkeys.
	const char *reserved = "cjwasdehqxf";
	for (const char *p = reserved; *p; ++p)
		occupied |= 1u << (*p - 'a');
	for (int i = 0; i < 13; ++i)
	{
		const char key = TotalWarModifierInput::replacementLetter(occupied);
		assert(key && !(occupied & (1u << (key - 'a'))));
		occupied |= 1u << (key - 'a');
	}
	assert(TotalWarModifierInput::replacementLetter(~0u) == '\0');
	std::puts("Total War modifiers: queue/additive/force modes, combinations, repeat/release ownership, focus cancellation and collision-free replacement keys passed.");
}
