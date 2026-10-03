#pragma once

#include "Common/MessageStream.h"

// GeneralsX @feature Codex 03/10/2026 Local camera input, isolated from formation orders and retail bindings.
namespace TotalWarCamera
{
	void reset();
	GameMessageDisposition translateGameMessage(const GameMessage *msg);
	Bool getScrollOffset(Coord2D &offset);
}
