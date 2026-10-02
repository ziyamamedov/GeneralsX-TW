// GeneralsX @feature Codex 26/09/2026 Geometry regressions for the actual Zero Hour preview helpers.
// Run from the repository root:
// c++ -std=c++20 -ffp-contract=off -I Core/Libraries/Include -I Dependencies/Utility \
//   -I Core/Libraries/Include/Precompiled -I GeneralsMD/Code/GameEngine/Include scripts/qa/formation-layout-test.cpp \
//   -o /tmp/formation-layout-test && /tmp/formation-layout-test
// GeneralsX @build Codex 02/10/2026 Supply the basic definitions normally provided by the engine's PCH.
#include <cstdint>
#include <algorithm>
#include "Precompiled/BaseTypes.h"
#include "Precompiled/BaseMacros.h"
using std::min;
using std::max;
#include "GameClient/FormationLayout.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static bool near(float a, float b)
{
	return std::fabs(a - b) < 0.001f;
}

int main()
{
	const Coord3D origin = { 100.0f, 200.0f, 0.0f };
	const Coord3D east = { 200.0f, 200.0f, 0.0f };
	const Coord3D fallback = { 0.0f, 1.0f, 0.0f };
	Coord3D direction = FormationLayout::facing(origin, east, 100.0f, true, fallback);
	assert(near(direction.x, 1.0f) && near(direction.y, 0.0f));
	Coord3D pos = FormationLayout::slot(origin, east, direction, 100.0f, 20.0f, 0, 1, true);
	assert(near(pos.x, origin.x) && near(pos.y, origin.y)); // Single-unit destination never follows the cursor.

	direction = FormationLayout::facing(origin, east, 100.0f, false, fallback);
	assert(near(direction.x, 0.0f) && near(direction.y, 1.0f));
	for (int i = 0; i < 5; ++i)
	{
		pos = FormationLayout::slot(origin, east, direction, 100.0f, 20.0f, i, 5, true);
		assert(near(pos.x, 100.0f + i * 25.0f) && near(pos.y, 200.0f));
	}
	Coord3D reverse = FormationLayout::facing(east, origin, 100.0f, false, fallback);
	assert(near(reverse.x, -direction.x) && near(reverse.y, -direction.y));

	// Short drags keep a safe gap and expand equally around the requested center.
	const Coord3D shortEnd = { 110.0f, 200.0f, 0.0f };
	Coord3D first = FormationLayout::slot(origin, shortEnd, direction, 10.0f, 40.0f, 0, 4, true);
	Coord3D last = FormationLayout::slot(origin, shortEnd, direction, 10.0f, 40.0f, 3, 4, true);
	assert(near(last.x - first.x, 120.0f) && near((first.x + last.x) * 0.5f, 105.0f));

	// Returning the cursor to the anchor keeps the last heading and avoids division by zero.
	reverse = FormationLayout::facing(origin, origin, 0.0f, false, fallback);
	assert(near(reverse.x, fallback.x) && near(reverse.y, fallback.y));
	pos = FormationLayout::slot(origin, origin, reverse, 0.0f, 20.0f, 0, 1, false);
	assert(std::isfinite(pos.x) && std::isfinite(pos.y));

	const Coord3D diagonal = { 160.0f, 280.0f, 0.0f };
	direction = FormationLayout::facing(origin, diagonal, 100.0f, false, fallback);
	assert(near(direction.x, -0.8f) && near(direction.y, 0.6f));
	assert(near(direction.x * 60.0f + direction.y * 80.0f, 0.0f));
	first = FormationLayout::slot(origin, diagonal, direction, 100.0f, 20.0f, 0, 3, true);
	last = FormationLayout::slot(origin, diagonal, direction, 100.0f, 20.0f, 2, 3, true);
	assert(near(first.x, origin.x) && near(first.y, origin.y));
	assert(near(last.x, diagonal.x) && near(last.y, diagonal.y));
	std::puts("Formation layout: single unit, line, reverse, spacing, zero drag and diagonal passed.");
}
