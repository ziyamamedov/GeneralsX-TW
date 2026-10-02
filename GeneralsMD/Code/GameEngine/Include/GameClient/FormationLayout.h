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

	// GeneralsX @feature Codex 02/10/2026 Anchor the front rank at mouse-down and wrap overflow behind it.
	inline Coord3D slot(const Coord3D &start, const Coord3D &direction,
		Real width, Real minimumSpacing, Int index, Int count)
	{
		Coord3D result = start;
		if (count <= 1)
			return result;

		Int columns = 1;
		if (minimumSpacing > 0.0f && width >= minimumSpacing)
		{
			// Bound the quotient before converting to an integer, including very long drags.
			columns = width >= minimumSpacing * (count - 1)
				? count : 1 + (Int)(width / minimumSpacing);
		}
		const Real columnSpacing = columns > 1 ? width / (columns - 1) : minimumSpacing;
		const Real along = (index % columns) * columnSpacing;
		const Real behind = (index / columns) * minimumSpacing;
		result.x += direction.y * along - direction.x * behind;
		result.y -= direction.x * along + direction.y * behind;
		return result;
	}
}
