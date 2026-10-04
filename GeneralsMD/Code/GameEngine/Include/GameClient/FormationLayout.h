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

	// GeneralsX @tweak Codex 04/10/2026 Fit the front rank to the drag width and place overflow behind it.
	inline Coord3D slot(const Coord3D &start, const Coord3D &direction,
		Real width, Real minimumSpacing, Int index, Int count)
	{
		Coord3D result = start;
		if (count <= 1)
			return result;

		width = max(0.0f, width);
		minimumSpacing = max(1.0f, minimumSpacing);
		// Bound before converting to an integer; even very long drags cannot exceed the selection size.
		const Int columns = 1 + (Int)min(width / minimumSpacing, (Real)(count - 1));
		const Real spacing = columns > 1 ? width / (columns - 1) : 0.0f;
		const Real along = (index % columns) * spacing;
		const Real behind = (index / columns) * minimumSpacing;
		result.x += direction.y * along - direction.x * behind;
		result.y -= direction.x * along + direction.y * behind;
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

	// GeneralsX @feature Codex 03/10/2026 Rotate offsets and individual headings by the same rigid transform.
	inline Coord3D rotatedDirection(const Coord3D &direction, Real cosine, Real sine)
	{
		Coord3D result = { direction.x * cosine - direction.y * sine,
			direction.x * sine + direction.y * cosine, 0.0f };
		return result;
	}

	inline Coord3D transformedSlot(const Coord3D &destination, const Coord3D &position,
		const Coord3D &center, Real cosine, Real sine)
	{
		Coord3D offset = { position.x - center.x, position.y - center.y, 0.0f };
		offset = rotatedDirection(offset, cosine, sine);
		Coord3D result = { destination.x + offset.x, destination.y + offset.y, destination.z };
		return result;
	}
}
