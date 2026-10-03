// GeneralsX @feature Codex 03/10/2026 Regression coverage for camera keys without loading the game engine.
// c++ -std=c++20 -I GeneralsMD/Code/GameEngine/Include scripts/qa/total-war-camera-input-test.cpp \
//   -o /tmp/total-war-camera-input-test && /tmp/total-war-camera-input-test
#include "GameClient/TotalWarCameraInput.h"
#include <cassert>
#include <cstdio>

int main()
{
	using Input = TotalWarCameraInput;
	Input input;
	assert(input.keyEvent(Input::FORWARD, true, false, true));
	assert(input.vertical() == -1 && input.horizontal() == 0);
	assert(input.keyEvent(Input::RIGHT, true, false, true));
	assert(input.vertical() == -1 && input.horizontal() == 1);
	assert(input.keyEvent(Input::BACKWARD, true, false, true));
	assert(input.vertical() == 0);
	assert(input.keyEvent(Input::FORWARD, false, false, true));
	assert(input.vertical() == 1);
	assert(input.keyEvent(Input::LEFT, true, false, true));
	assert(input.horizontal() == 0);

	// A release polled before its message must stop scrolling but still consume the key-up hotkey.
	input.checkReleased(Input::BACKWARD, false);
	assert(input.vertical() == 0);
	assert(input.keyEvent(Input::BACKWARD, false, false, false));
	assert(!input.keyEvent(Input::BACKWARD, false, false, true));

	// Opening chat, menus or losing app focus cancels movement without reactivating on autorepeat.
	input.cancelMovement();
	assert(input.horizontal() == 0 && input.vertical() == 0);
	assert(input.keyEvent(Input::RIGHT, true, true, true));
	assert(input.horizontal() == 0);
	assert(input.keyEvent(Input::RIGHT, false, false, false));
	assert(!input.keyEvent(Input::FORWARD, true, true, true));
	assert(input.vertical() == 0);

	// Ctrl/Alt shortcuts or text input are not claimed, including after a GUI swallowed the previous release.
	assert(input.keyEvent(Input::BACKWARD, true, false, true));
	input.cancelMovement();
	assert(!input.keyEvent(Input::BACKWARD, true, false, false));
	assert(!input.keyEvent(Input::BACKWARD, false, false, true));
	assert(input.vertical() == 0);

	// Releasing a camera key with different modifiers must never invoke the old command.
	assert(input.keyEvent(Input::BACKWARD, true, false, true));
	assert(input.keyEvent(Input::BACKWARD, false, false, false));
	assert(input.vertical() == 0);
	input.reset();
	assert(input.horizontal() == 0 && input.vertical() == 0);
	assert(!input.keyEvent(Input::LEFT, false, false, true));
	std::puts("Total War camera keys: directions, diagonals, opposing keys, release ownership, modifiers, focus cancellation and reset passed.");
}
