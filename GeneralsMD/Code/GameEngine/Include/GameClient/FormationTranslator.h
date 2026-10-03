#pragma once

#include "Common/MessageStream.h"
#include <vector>

class Object;
class View;

// GeneralsX @feature Codex 26/09/2026 Client-only preview; orders enter the synchronized stream on release.
class FormationTranslator : public GameMessageTranslator
{
public:
	FormationTranslator();
	virtual ~FormationTranslator() override;
	virtual GameMessageDisposition translateGameMessage(const GameMessage *msg) override;
	void reset();
	void draw(View *view);

private:
	struct Slot
	{
		ObjectID objectID;
		Coord3D position;
		// GeneralsX @feature Codex 03/10/2026 Capture the arrangement and headings before previewing a new order.
		Coord3D originalPosition;
		Coord3D originalDirection;
	};
	Bool canStart() const;
	Bool selectionUnchanged() const;
	Bool begin(const ICoord2D &screen);
	void updatePreview(const ICoord2D &screen);
	void finish(const ICoord2D &screen);
	void cancel();

	Bool m_active;
	Bool m_cancelled;
	Bool m_dragged;
	Bool m_validPosition;
	Bool m_releasePending;
	ICoord2D m_screenAnchor;
	Coord3D m_anchor;
	Coord3D m_center;
	Coord3D m_direction;
	Real m_spacing;
	Int m_selectionCount;
	std::vector<Slot> m_slots;
};

extern FormationTranslator *TheFormationTranslator;
