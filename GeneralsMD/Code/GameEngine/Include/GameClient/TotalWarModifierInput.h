#pragma once

// GeneralsX @feature Codex 04/10/2026 Hold-to-force-attack ownership and independent modifier modes.
class TotalWarModifierInput
{
public:
	struct Modes { bool queue; bool selection; bool forceAttack; };
	TotalWarModifierInput() : m_forceDown(false), m_captured(false) { }
	void cancel() { m_forceDown = false; }
	void checkReleased(bool down) { if (!down) cancel(); }
	bool keyEvent(bool pressed, bool repeat, bool allowed)
	{
		if (!pressed)
		{
			const bool consumed = m_captured;
			m_forceDown = m_captured = false;
			return consumed;
		}
		if (!repeat)
			m_forceDown = m_captured = allowed;
		return m_captured;
	}
	Modes modes(bool shift, bool control, bool allowed) const
	{
		const bool force = allowed && m_forceDown;
		return { allowed && shift, allowed && control && !force, force };
	}

	// Choose a free panel letter; exclude camera keys and unmodified global shortcuts.
	static char replacementLetter(unsigned int occupied)
	{
		// GeneralsX @tweak Codex 04/10/2026 J is force attack; keep C available for its original commands.
		const char *candidates = "kvzolpiuynbrtg";
		for (const char *letter = candidates; *letter; ++letter)
			if (!(occupied & (1u << (*letter - 'a'))))
				return *letter;
		return '\0';
	}

private:
	bool m_forceDown;
	bool m_captured;
};
