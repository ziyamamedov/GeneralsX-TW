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

static void assertPosition(const Coord3D &position, float x, float y)
{
	assert(near(position.x, x) && near(position.y, y));
}

// GeneralsX @feature Codex 02/10/2026 Cover anchored ranks, exact capacity boundaries and incomplete rear ranks.
static void testThreeUnits()
{
	const Coord3D start = { 100.0f, 200.0f, 0.0f };
	const Coord3D north = { 0.0f, 1.0f, 0.0f };
	const float spacing = 40.0f;
	// A short drag gives one file extending behind the front unit, never sideways past the anchor.
	for (int i = 0; i < 3; ++i)
		assertPosition(FormationLayout::slot(start, north, 10.0f, spacing, i, 3), 100.0f, 200.0f - i * spacing);
	assertPosition(FormationLayout::slot(start, north, 39.99f, spacing, 1, 3), 100.0f, 160.0f);
	// At the spacing boundary the second unit joins the first rank; the third stays behind.
	assertPosition(FormationLayout::slot(start, north, 40.0f, spacing, 1, 3), 140.0f, 200.0f);
	assertPosition(FormationLayout::slot(start, north, 60.0f, spacing, 1, 3), 160.0f, 200.0f);
	assertPosition(FormationLayout::slot(start, north, 60.0f, spacing, 2, 3), 100.0f, 160.0f);
	assertPosition(FormationLayout::slot(start, north, 79.99f, spacing, 2, 3), 100.0f, 160.0f);
	// Once all fit, the front rank spans exactly from mouse-down to the cursor.
	for (int i = 0; i < 3; ++i)
	{
		assertPosition(FormationLayout::slot(start, north, 80.0f, spacing, i, 3), 100.0f + i * spacing, 200.0f);
		assertPosition(FormationLayout::slot(start, north, 200.0f, spacing, i, 3), 100.0f + i * 100.0f, 200.0f);
	}
}

static void testTenUnits()
{
	const Coord3D start = { 100.0f, 200.0f, 0.0f };
	const Coord3D north = { 0.0f, 1.0f, 0.0f };
	// Growing and then shrinking through 1..10 places covers 2x5, 3+3+3+1, 4+4+2, 5+5, 6+4, 7+3, etc.
	for (int step = 1; step <= 19; ++step)
	{
		const int columns = step <= 10 ? step : 20 - step;
		const float width = (columns - 1) * 40.0f;
		int frontCount = 0;
		for (int i = 0; i < 10; ++i)
		{
			Coord3D pos = FormationLayout::slot(start, north, width, 40.0f, i, 10);
			assertPosition(pos, 100.0f + (i % columns) * 40.0f, 200.0f - (i / columns) * 40.0f);
			if (near(pos.y, start.y))
				++frontCount;
		}
		assert(frontCount == columns);
	}
}

static void testDirectionsAndSpacing()
{
	const Coord3D start = { 100.0f, 200.0f, 0.0f };
	const Coord3D fallback = { 0.0f, 1.0f, 0.0f };
	const Coord3D dragDirections[] = {
		{ 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f },
		{ 0.6f, 0.8f, 0.0f }, { -0.6f, -0.8f, 0.0f }
	};
	for (const Coord3D &drag : dragDirections)
	{
		const Coord3D end = { start.x + drag.x * 100.0f, start.y + drag.y * 100.0f, 0.0f };
		Coord3D direction = FormationLayout::facing(start, end, 100.0f, false, fallback);
		assert(near(direction.x * drag.x + direction.y * drag.y, 0.0f));
		for (int count = 2; count <= 32; ++count)
		{
			for (float width : { 0.0f, 10.0f, 39.99f, 40.0f, 79.99f, 80.0f, 100.0f, 200.0f, 1240.0f })
			{
				for (int i = 0; i < count; ++i)
				{
					Coord3D pos = FormationLayout::slot(start, direction, width, 40.0f, i, count);
					assert(std::isfinite(pos.x) && std::isfinite(pos.y));
					const float x = pos.x - start.x, y = pos.y - start.y;
					const float along = x * drag.x + y * drag.y;
					const float forward = x * direction.x + y * direction.y;
					assert(along >= -0.001f && along <= width + 0.001f);
					assert(forward <= 0.001f); // Overflow is behind, regardless of the drag's direction.
					if (i == 0)
						assertPosition(pos, start.x, start.y);
					for (int j = 0; j < i; ++j)
					{
						Coord3D other = FormationLayout::slot(start, direction, width, 40.0f, j, count);
						const float dx = pos.x - other.x, dy = pos.y - other.y;
						assert(dx * dx + dy * dy >= 40.0f * 40.0f - 0.1f);
					}
				}
			}
		}
	}
}

int main()
{
	const Coord3D start = { 100.0f, 200.0f, 0.0f };
	const Coord3D east = { 200.0f, 200.0f, 0.0f };
	const Coord3D fallback = { 0.0f, 1.0f, 0.0f };
	Coord3D direction = FormationLayout::facing(start, east, 100.0f, true, fallback);
	assert(near(direction.x, 1.0f) && near(direction.y, 0.0f));
	assertPosition(FormationLayout::slot(start, direction, 100.0f, 40.0f, 0, 1), start.x, start.y);
	direction = FormationLayout::facing(start, start, 0.0f, false, fallback);
	assert(near(direction.x, fallback.x) && near(direction.y, fallback.y));
	testThreeUnits();
	testTenUnits();
	testDirectionsAndSpacing();
	std::puts("Formation layout: fixed anchor, expanding/shrinking ranks, reverse/diagonal drags, spacing and single-unit facing passed.");
}
