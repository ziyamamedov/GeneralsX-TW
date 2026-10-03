#pragma once

#include "Common/MessageStream.h"

// GeneralsX @feature Codex 04/10/2026 Optional local bindings; retail command execution stays unchanged.
namespace TotalWarInput
{
	void reset();
	GameMessageDisposition translateGameMessage(const GameMessage *msg);
	Bool ownsModifierBinding(GameMessage::Type type);
	Int commandKey(Int originalKey);
	AsciiString panelKey(const AsciiString &pressedKey);
	UnicodeString panelLabel(const UnicodeString &label);
}
