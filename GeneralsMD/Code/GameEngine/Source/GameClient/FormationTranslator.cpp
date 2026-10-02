#include "PreRTS.h"

#include "GameClient/FormationTranslator.h"
#include "GameClient/FormationLayout.h"
#include "Common/GameEngine.h"
#include "Common/GlobalData.h"
#include "Common/Recorder.h"
#include "Common/StatsCollector.h"
#include "GameClient/CommandXlat.h"
#include "GameClient/Display.h"
#include "GameClient/Drawable.h"
#include "GameClient/GameClient.h"
#include "GameClient/InGameUI.h"
#include "GameClient/Keyboard.h"
#include "GameClient/Mouse.h"
#include "GameClient/SelectionInfo.h"
#include "GameClient/Shell.h"
#include "GameClient/View.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/TerrainLogic.h"
#include "WWMath/wwmath.h"

// GeneralsX @feature Codex 26/09/2026 Total War-style destination/facing gestures for Zero Hour.
FormationTranslator *TheFormationTranslator = nullptr;

namespace
{
	Bool canFormUp(const Object *obj)
	{
		return obj && obj->isLocallyControlled() && obj->isMobile()
			&& !obj->isEffectivelyDead() && !obj->isContained()
			&& !obj->isKindOf(KINDOF_AIRCRAFT) && obj->getAIUpdateInterface();
	}

	void drawGroundLine(View *view, Coord3D a, Coord3D b, UnsignedInt color, Real width)
	{
		a.z = TheTerrainLogic->getGroundHeight(a.x, a.y) + 1.0f;
		b.z = TheTerrainLogic->getGroundHeight(b.x, b.y) + 1.0f;
		ICoord2D start, end;
		if (view->worldToScreen(&a, &start) && view->worldToScreen(&b, &end))
			TheDisplay->drawLine(start.x, start.y, end.x, end.y, width, color);
	}
}

FormationTranslator::FormationTranslator()
{
	TheFormationTranslator = this;
	reset();
}

FormationTranslator::~FormationTranslator()
{
	TheFormationTranslator = nullptr;
}

void FormationTranslator::reset()
{
	m_active = false;
	m_cancelled = false;
	m_dragged = false;
	m_validPosition = false;
	m_releasePending = false;
	m_spacing = 20.0f;
	m_selectionCount = 0;
	m_slots.clear();
}

void FormationTranslator::cancel()
{
	// Keep consuming this gesture until release, so cancellation cannot become a normal click.
	m_cancelled = true;
	m_slots.clear();
}

Bool FormationTranslator::canStart() const
{
	return TheGlobalData->m_useAlternateMouse && TheTacticalView && TheInGameUI
		&& TheInGameUI->getInputEnabled() && TheInGameUI->areSelectedObjectsControllable()
		&& !TheInGameUI->getGUICommand() && !TheInGameUI->getPendingPlaceType()
		&& !TheInGameUI->isSelecting() && !TheInGameUI->isInWaypointMode()
		&& !TheInGameUI->isInForceAttackMode() && !TheInGameUI->isInForceMoveToMode()
		&& !TheInGameUI->isInAttackMoveToMode()
		&& TheGameEngine->isActive() && TheGameLogic->isInInteractiveGame()
		&& !TheGameLogic->isGamePaused() && !TheShell->isShellActive()
		&& TheRecorder->getMode() != RECORDERMODETYPE_PLAYBACK;
}

Bool FormationTranslator::selectionUnchanged() const
{
	if (TheInGameUI->getSelectCount() != m_selectionCount)
		return false;
	for (std::vector<Slot>::const_iterator it = m_slots.begin(); it != m_slots.end(); ++it)
	{
		Object *obj = TheGameLogic->findObjectByID(it->objectID);
		if (!canFormUp(obj) || !obj->getDrawable() || !obj->getDrawable()->isSelected())
			return false;
	}
	return true;
}

Bool FormationTranslator::begin(const ICoord2D &screen)
{
	if (!canStart() || !TheTacticalView->screenToTerrain(&screen, &m_anchor))
		return false;

	// Leave attacks, garrisoning, repair, salvage and other object context commands to the normal translator.
	Drawable *picked = TheTacticalView->pickDrawable(&screen, false,
		(PickType)getPickTypesForContext(false));
	if (picked && picked->getObject() && !picked->getObject()->isEffectivelyDead())
		return false;

	reset();
	const DrawableList *selected = TheInGameUI->getAllSelectedDrawables();
	for (DrawableListCIt it = selected->begin(); it != selected->end(); ++it)
	{
		Object *obj = (*it)->getObject();
		if (!canFormUp(obj))
			continue;
		Slot slot;
		slot.objectID = obj->getID();
		slot.position = m_anchor;
		m_slots.push_back(slot);
		m_spacing = max(m_spacing, 2.0f * obj->getGeometryInfo().getBoundingCircleRadius() + 8.0f);
	}
	if (m_slots.empty())
		return false;

	Object *first = TheGameLogic->findObjectByID(m_slots.front().objectID);
	m_direction.x = WWMath::Cos(first->getOrientation());
	m_direction.y = WWMath::Sin(first->getOrientation());
	m_direction.z = 0.0f;
	m_screenAnchor = screen;
	m_selectionCount = TheInGameUI->getSelectCount();
	m_active = true;
	updatePreview(screen);
	return true;
}

void FormationTranslator::updatePreview(const ICoord2D &screen)
{
	Coord3D end;
	m_validPosition = TheTacticalView->screenToTerrain(&screen, &end);
	if (!m_validPosition || m_slots.empty())
		return;

	const Int dx = screen.x - m_screenAnchor.x;
	const Int dy = screen.y - m_screenAnchor.y;
	const Real wx = end.x - m_anchor.x;
	const Real wy = end.y - m_anchor.y;
	const Real distance = WWMath::Sqrt(wx * wx + wy * wy);
	if (dx * dx + dy * dy >= 64 && distance > 0.1f)
	{
		m_dragged = true;
		m_direction = FormationLayout::facing(m_anchor, end, distance, m_slots.size() == 1, m_direction);
	}

	const Int count = (Int)m_slots.size();
	for (Int i = 0; i < count; ++i)
	{
		Coord3D &pos = m_slots[i].position;
		pos = FormationLayout::slot(m_anchor, end, m_direction, distance, m_spacing, i, count, m_dragged);
		pos.z = TheTerrainLogic->getGroundHeight(pos.x, pos.y);
	}
}

void FormationTranslator::finish(const ICoord2D &screen)
{
	if (!m_cancelled && canStart() && selectionUnchanged())
	{
		updatePreview(screen);
		if (m_validPosition)
		{
			if (!m_dragged)
			{
				// A simple click keeps the original movement behavior, including unit voice feedback.
				TheGameClient->evaluateContextCommand(nullptr, &m_anchor, CommandTranslator::DO_COMMAND);
			}
			else
			{
				for (std::vector<Slot>::const_iterator it = m_slots.begin(); it != m_slots.end(); ++it)
				{
					Coord3D facingPoint = it->position;
					facingPoint.x += m_direction.x * 100.0f;
					facingPoint.y += m_direction.y * 100.0f;
					GameMessage *order = TheMessageStream->appendMessage(GameMessage::MSG_DO_FORMATION_MOVE);
					order->appendObjectIDArgument(it->objectID);
					order->appendLocationArgument(it->position);
					order->appendLocationArgument(facingPoint);
				}
				PickAndPlayInfo info;
				pickAndPlayUnitVoiceResponse(TheInGameUI->getAllSelectedDrawables(), GameMessage::MSG_DO_MOVETO, &info);
				if (TheStatsCollector)
					TheStatsCollector->incrementMoveCount();
			}
		}
	}
	reset();
}

GameMessageDisposition FormationTranslator::translateGameMessage(const GameMessage *msg)
{
	const GameMessage::Type type = msg->getType();
	if (type == GameMessage::MSG_CLEAR_GAME_DATA)
		reset();
	if (m_active && !m_cancelled && (!canStart() || !selectionUnchanged()))
		cancel();

	if (type == GameMessage::MSG_RAW_MOUSE_RIGHT_BUTTON_DOWN || type == GameMessage::MSG_RAW_MOUSE_RIGHT_DOUBLE_CLICK)
	{
		if (begin(msg->getArgument(0)->pixel))
			return DESTROY_MESSAGE;
	}
	if (!m_active)
		return KEEP_MESSAGE;

	switch (type)
	{
		case GameMessage::MSG_RAW_MOUSE_RIGHT_BUTTON_UP:
			finish(msg->getArgument(0)->pixel);
			return DESTROY_MESSAGE;
		case GameMessage::MSG_RAW_MOUSE_POSITION:
			if (m_cancelled)
				return KEEP_MESSAGE;
			updatePreview(msg->getArgument(0)->pixel);
			return DESTROY_MESSAGE;
		case GameMessage::MSG_RAW_MOUSE_RIGHT_DRAG:
			if (!m_cancelled)
				updatePreview(msg->getArgument(0)->pixel);
			return DESTROY_MESSAGE;
		case GameMessage::MSG_RAW_KEY_DOWN:
		case GameMessage::MSG_RAW_KEY_UP:
			if (msg->getArgument(0)->integer == KEY_ESC)
			{
				cancel();
				return DESTROY_MESSAGE;
			}
			break;
		case GameMessage::MSG_RAW_MOUSE_LEFT_BUTTON_DOWN:
			cancel();
			// Let both halves of a new selection click reach the selection translator.
			return KEEP_MESSAGE;
		case GameMessage::MSG_FRAME_TICK:
			// FRAME_TICK precedes this frame's raw input. Allow its release through before
			// cancelling a release swallowed by the window system (for example above the HUD).
			if (!TheGameEngine->isActive()
				|| (m_releasePending && TheMouse->getMouseStatus()->rightState == MBS_Up))
				reset();
			else
				m_releasePending = TheMouse->getMouseStatus()->rightState == MBS_Up;
			break;
		default:
			break;
	}
	return KEEP_MESSAGE;
}

void FormationTranslator::draw(View *view)
{
	if (!m_active || m_cancelled || !canStart() || !selectionUnchanged())
		return;
	const UnsignedInt color = m_validPosition ? 0xD060FF80 : 0xD0FFB040;
	const Real sideX = m_direction.y;
	const Real sideY = -m_direction.x;
	for (std::vector<Slot>::const_iterator it = m_slots.begin(); it != m_slots.end(); ++it)
	{
		Coord3D tip = it->position, left = it->position, right = it->position;
		tip.x += m_direction.x * 12.0f;
		tip.y += m_direction.y * 12.0f;
		left.x += sideX * 7.0f - m_direction.x * 7.0f;
		left.y += sideY * 7.0f - m_direction.y * 7.0f;
		right.x -= sideX * 7.0f + m_direction.x * 7.0f;
		right.y -= sideY * 7.0f + m_direction.y * 7.0f;
		drawGroundLine(view, tip, left, color, 2.0f);
		drawGroundLine(view, left, right, color, 2.0f);
		drawGroundLine(view, right, tip, color, 2.0f);
	}
	if (m_slots.size() > 1)
		drawGroundLine(view, m_slots.front().position, m_slots.back().position, color, 1.0f);
}
