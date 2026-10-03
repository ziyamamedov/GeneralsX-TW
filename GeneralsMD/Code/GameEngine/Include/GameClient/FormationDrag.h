#pragma once

#include "Lib/BaseType.h"

// GeneralsX @feature Codex 03/10/2026 Client-only placement state for an Alt+LMB preserved formation.
class FormationDrag
{
public:
	void begin(const Coord3D &center, const ICoord2D &screen, Bool rotating)
	{
		m_center = center;
		m_screen = screen;
		m_angle = m_rotationAnchorAngle = 0.0f;
		m_rotationAnchorX = screen.x;
		m_rotating = rotating;
		m_waitForMovement = false;
	}

	void update(const Coord3D &cursor, const ICoord2D &screen, Bool rotating)
	{
		if (rotating != m_rotating)
		{
			m_rotating = rotating;
			m_rotationAnchorX = screen.x;
			m_rotationAnchorAngle = m_angle;
			// Releasing Ctrl and LMB together must commit the rotated preview without recentering it.
			m_waitForMovement = !rotating;
		}
		if (m_rotating)
			m_angle = m_rotationAnchorAngle + 0.01f * (screen.x - m_rotationAnchorX);
		else if (!m_waitForMovement || screen.x != m_screen.x || screen.y != m_screen.y)
		{
			m_center = cursor;
			m_waitForMovement = false;
		}
		m_screen = screen;
	}

	const Coord3D &center() const { return m_center; }
	Real angle() const { return m_angle; }

private:
	Coord3D m_center;
	ICoord2D m_screen;
	Real m_angle;
	Real m_rotationAnchorAngle;
	Int m_rotationAnchorX;
	Bool m_rotating;
	Bool m_waitForMovement;
};
