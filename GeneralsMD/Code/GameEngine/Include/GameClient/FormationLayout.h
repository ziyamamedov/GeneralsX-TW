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

	inline Coord3D slot(const Coord3D &start, const Coord3D &end, const Coord3D &direction,
		Real distance, Real minimumSpacing, Int index, Int count, Bool dragged)
	{
		Coord3D result = start;
		if (count <= 1)
			return result;
		Real spacing = minimumSpacing;
		if (dragged)
		{
			const Real requestedSpacing = distance / (count - 1);
			if (requestedSpacing > spacing)
				spacing = requestedSpacing;
			result.x = (start.x + end.x) * 0.5f;
			result.y = (start.y + end.y) * 0.5f;
		}
		const Real offset = (index - (count - 1) * 0.5f) * spacing;
		result.x += direction.y * offset;
		result.y -= direction.x * offset;
		return result;
	}
}
