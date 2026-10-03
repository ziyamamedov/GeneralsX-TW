#pragma once

// GeneralsX @feature Codex 03/10/2026 Track camera key ownership through repeats, modifiers and swallowed releases.
class TotalWarCameraInput
{
public:
	enum Direction { FORWARD, BACKWARD, LEFT, RIGHT, COUNT };
	TotalWarCameraInput() { reset(); }
	void reset()
	{
		for (int i = 0; i < COUNT; ++i)
			m_down[i] = m_captured[i] = false;
	}
	void cancelMovement()
	{
		for (int i = 0; i < COUNT; ++i)
			m_down[i] = false;
	}
	bool keyEvent(Direction direction, bool pressed, bool repeat, bool allowed)
	{
		if (!pressed)
		{
			const bool consumed = m_captured[direction];
			m_down[direction] = m_captured[direction] = false;
			return consumed;
		}
		if (!repeat)
			m_down[direction] = m_captured[direction] = allowed;
		// Repeats cannot restart movement cancelled by a menu or focus change.
		return m_captured[direction];
	}
	void checkReleased(Direction direction, bool physicallyDown)
	{
		if (!physicallyDown)
			m_down[direction] = false;
		// Keep ownership until key-up is routed, so releasing S cannot also issue Stop.
	}
	int horizontal() const { return int(m_down[RIGHT]) - int(m_down[LEFT]); }
	int vertical() const { return int(m_down[BACKWARD]) - int(m_down[FORWARD]); }

private:
	bool m_down[COUNT];
	bool m_captured[COUNT];
};
