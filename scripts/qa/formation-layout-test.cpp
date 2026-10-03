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
#include "GameClient/FormationDrag.h"
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

// GeneralsX @tweak Codex 03/10/2026 Short and long drags always produce one row starting at mouse-down.
static void testThreeUnits()
{
	const Coord3D start = { 100.0f, 200.0f, 0.0f };
	const Coord3D north = { 0.0f, 1.0f, 0.0f };
	const float spacing = 40.0f;
	for (float width : { 0.0f, 10.0f, 39.99f, 40.0f, 60.0f, 79.99f, 80.0f })
	{
		for (int i = 0; i < 3; ++i)
			assertPosition(FormationLayout::slot(start, north, width, spacing, i, 3), 100.0f + i * spacing, 200.0f);
	}
	// Above the minimum width the row stretches to the cursor, still anchored at its first unit.
	for (int i = 0; i < 3; ++i)
		assertPosition(FormationLayout::slot(start, north, 200.0f, spacing, i, 3), 100.0f + i * 100.0f, 200.0f);
}

static void testTenUnits()
{
	const Coord3D start = { 100.0f, 200.0f, 0.0f };
	const Coord3D north = { 0.0f, 1.0f, 0.0f };
	// Growing and shrinking never creates rear ranks, including widths that used to produce 2x5 or 5x2.
	for (float width : { 0.0f, 40.0f, 80.0f, 160.0f, 200.0f, 360.0f, 720.0f, 360.0f, 160.0f, 40.0f, 0.0f })
	{
		const float spacing = width > 360.0f ? 80.0f : 40.0f;
		for (int i = 0; i < 10; ++i)
		{
			Coord3D pos = FormationLayout::slot(start, north, width, 40.0f, i, 10);
			assertPosition(pos, 100.0f + i * spacing, 200.0f);
		}
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
				float previousAlong = -1.0f;
				for (int i = 0; i < count; ++i)
				{
					Coord3D pos = FormationLayout::slot(start, direction, width, 40.0f, i, count);
					assert(std::isfinite(pos.x) && std::isfinite(pos.y));
					const float x = pos.x - start.x, y = pos.y - start.y;
					const float along = x * drag.x + y * drag.y;
					const float forward = x * direction.x + y * direction.y;
					assert(along >= -0.001f && along > previousAlong);
					assert(near(forward, 0.0f)); // All units stay on the same line for every drag direction.
					previousAlong = along;
					if (i == 0)
						assertPosition(pos, start.x, start.y);
					if (i == count - 1)
						assert(near(along, max(width, 40.0f * (count - 1))));
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

// GeneralsX @feature Codex 03/10/2026 Plain clicks preserve arbitrary shapes and center them on the clicked point.
static void testCenteredClicks()
{
	const Coord3D destination = { 1000.0f, 1500.0f, 0.0f };
	const Coord3D positions[] = {
		{ 100.0f, 200.0f, 0.0f }, { 160.0f, 230.0f, 0.0f },
		{ 70.0f, 140.0f, 0.0f }, { 270.0f, 110.0f, 0.0f }
	};
	const Coord3D center = { 150.0f, 170.0f, 0.0f };
	const Coord3D expected[] = {
		{ 950.0f, 1530.0f, 0.0f }, { 1010.0f, 1560.0f, 0.0f },
		{ 920.0f, 1470.0f, 0.0f }, { 1120.0f, 1440.0f, 0.0f }
	};
	Coord3D sum = { 0.0f, 0.0f, 0.0f };
	for (int i = 0; i < 4; ++i)
	{
		const Coord3D pos = FormationLayout::translatedSlot(destination, positions[i], center);
		assertPosition(pos, expected[i].x, expected[i].y);
		sum.x += pos.x;
		sum.y += pos.y;
		for (int j = 0; j < i; ++j)
		{
			const Coord3D other = FormationLayout::translatedSlot(destination, positions[j], center);
			assert(near(pos.x - other.x, positions[i].x - positions[j].x));
			assert(near(pos.y - other.y, positions[i].y - positions[j].y));
		}
		assertPosition(FormationLayout::translatedSlot(center, pos, destination), positions[i].x, positions[i].y);
	}
	assertPosition(sum, destination.x * 4, destination.y * 4);
	assertPosition(FormationLayout::translatedSlot(destination, center, center), destination.x, destination.y);

	// An existing row stays centered on a click; a newly dragged row starts at that same point instead.
	const Coord3D north = { 0.0f, 1.0f, 0.0f };
	const Coord3D first = { 110.0f, 170.0f, 0.0f };
	const Coord3D last = { 190.0f, 170.0f, 0.0f };
	assertPosition(FormationLayout::translatedSlot(destination, first, center), 960.0f, 1500.0f);
	assertPosition(FormationLayout::translatedSlot(destination, last, center), 1040.0f, 1500.0f);
	assertPosition(FormationLayout::slot(destination, north, 80.0f, 40.0f, 0, 3), 1000.0f, 1500.0f);
	assertPosition(FormationLayout::slot(destination, north, 80.0f, 40.0f, 2, 3), 1080.0f, 1500.0f);
}

// GeneralsX @feature Codex 03/10/2026 Keep three ranks and mixed headings rigid through translation/rotation.
static void testPreservedFormation()
{
	const Coord3D center = { 120.0f, 150.0f, 0.0f };
	const Coord3D destination = { 500.0f, 900.0f, 0.0f };
	const Coord3D positions[] = {
		{ 100.0f, 200.0f, 0.0f }, { 140.0f, 200.0f, 0.0f },
		{ 100.0f, 150.0f, 0.0f }, { 140.0f, 150.0f, 0.0f },
		{ 100.0f, 100.0f, 0.0f }, { 140.0f, 100.0f, 0.0f }
	};
	const Coord3D headings[] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.6f, 0.8f, 0.0f } };
	for (float angle : { 0.0f, 0.5f, -0.5f, 1.5707963f, -1.5707963f, 3.1415926f, 6.2831853f })
	{
		const float cosine = std::cos(angle), sine = std::sin(angle);
		Coord3D sum = { 0.0f, 0.0f, 0.0f };
		for (int i = 0; i < 6; ++i)
		{
			const Coord3D pos = FormationLayout::transformedSlot(destination, positions[i], center, cosine, sine);
			sum.x += pos.x;
			sum.y += pos.y;
			const Coord3D restored = FormationLayout::transformedSlot(center, pos, destination, cosine, -sine);
			assertPosition(restored, positions[i].x, positions[i].y);
			for (int j = 0; j < i; ++j)
			{
				const Coord3D other = FormationLayout::transformedSlot(destination, positions[j], center, cosine, sine);
				const float dx = pos.x - other.x, dy = pos.y - other.y;
				const float oldDx = positions[i].x - positions[j].x, oldDy = positions[i].y - positions[j].y;
				assert(std::fabs(dx * dx + dy * dy - oldDx * oldDx - oldDy * oldDy) < 0.05f);
			}
		}
		assertPosition(sum, destination.x * 6, destination.y * 6);
		for (const Coord3D &heading : headings)
		{
			const Coord3D rotated = FormationLayout::rotatedDirection(heading, cosine, sine);
			assert(near(rotated.x * rotated.x + rotated.y * rotated.y, 1.0f));
			const Coord3D restored = FormationLayout::rotatedDirection(rotated, cosine, -sine);
			assertPosition(restored, heading.x, heading.y);
		}
	}
	// Explicit quarter-turn oracle: the first tank ends left/behind center and an east heading becomes north.
	assertPosition(FormationLayout::transformedSlot(destination, positions[0], center, 0.0f, 1.0f), 450.0f, 880.0f);
	assertPosition(FormationLayout::rotatedDirection(headings[0], 0.0f, 1.0f), 0.0f, 1.0f);
	// A single unit remains on the destination while its facing rotates.
	assertPosition(FormationLayout::transformedSlot(destination, center, center, 0.0f, -1.0f), 500.0f, 900.0f);
}

static void testPreservedDragTransitions()
{
	FormationDrag drag;
	const Coord3D start = { 100.0f, 200.0f, 0.0f };
	const Coord3D destination = { 500.0f, 900.0f, 0.0f };
	const Coord3D beyond = { 700.0f, 1000.0f, 0.0f };
	drag.begin(start, { 100, 100 }, false);
	drag.update(destination, { 300, 200 }, false);
	assertPosition(drag.center(), 500.0f, 900.0f);
	assert(near(drag.angle(), 0.0f));
	drag.update(destination, { 300, 200 }, true); // Press Ctrl without moving.
	drag.update(beyond, { 400, 400 }, true); // Horizontal motion rotates; vertical motion does not move the pivot.
	assertPosition(drag.center(), 500.0f, 900.0f);
	assert(near(drag.angle(), 1.0f));
	drag.update(beyond, { 400, 400 }, false); // Ctrl-up.
	for (int i = 0; i < 4; ++i)
		drag.update(beyond, { 400, 400 }, false); // Position ticks and LMB-up must keep exactly the previewed target.
	assertPosition(drag.center(), 500.0f, 900.0f);
	assert(near(drag.angle(), 1.0f));
	drag.update(beyond, { 410, 400 }, false); // Moving again resumes cursor-centered translation with the new heading.
	assertPosition(drag.center(), 700.0f, 1000.0f);
	assert(near(drag.angle(), 1.0f));
	drag.update(beyond, { 410, 400 }, true);
	drag.update(start, { 210, 400 }, true);
	assertPosition(drag.center(), 700.0f, 1000.0f);
	assert(near(drag.angle(), -1.0f)); // Re-entering rotation accumulates, rather than resetting the heading.
	drag.begin(start, { 0, 0 }, true); // Starting with Alt+Ctrl rotates around the original center.
	drag.update(destination, { 0, 100 }, true);
	assertPosition(drag.center(), 100.0f, 200.0f);
	assert(near(drag.angle(), 0.0f));
	drag.update(destination, { -100, 100 }, true);
	assert(near(drag.angle(), -1.0f));
	drag.begin(destination, { 0, 0 }, false); // The next gesture starts fresh.
	assertPosition(drag.center(), 500.0f, 900.0f);
	assert(near(drag.angle(), 0.0f));
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
	testCenteredClicks();
	testPreservedFormation();
	testPreservedDragTransitions();
	std::puts("Formation layout: anchored rows, centered clicks, rigid multi-rank rotation, individual headings and drag modifier transitions passed.");
}
