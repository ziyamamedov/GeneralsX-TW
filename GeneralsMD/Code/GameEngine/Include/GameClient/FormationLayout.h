#pragma once

#include "Lib/BaseType.h"

// GeneralsX @feature Codex 26/09/2026 Share the preview's layout math with focused geometry tests.
namespace FormationLayout
{
	inline Coord3D facing(const Coord3D &start, const Coord3D &end, Real distance,
		Bool singleUnit, const Coord3D &fallback)
	{
		if (distance <= 0.1f)
			return fallback;
		const Real x = (end.x - start.x) / distance;
		const Real y = (end.y - start.y) / distance;
		Coord3D result = { singleUnit ? x : -y, singleUnit ? y : x, 0.0f };
		return result;
	}

	// GeneralsX @tweak Codex 03/10/2026 Keep one row extending from mouse-down, even when the drag is short.
	inline Coord3D slot(const Coord3D &start, const Coord3D &direction,
		Real width, Real minimumSpacing, Int index, Int count)
	{
		Coord3D result = start;
		if (count <= 1)
			return result;

		const Real spacing = max(minimumSpacing, width / (count - 1));
		const Real along = index * spacing;
		result.x += direction.y * along;
		result.y -= direction.x * along;
		return result;
	}

	// GeneralsX @feature Codex 03/10/2026 A click translates the existing shape with its center at the destination.
	inline Coord3D translatedSlot(const Coord3D &destination, const Coord3D &position, const Coord3D &center)
	{
		Coord3D result = destination;
		result.x += position.x - center.x;
		result.y += position.y - center.y;
		return result;
	}
}
