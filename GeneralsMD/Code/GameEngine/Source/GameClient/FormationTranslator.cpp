#include "PreRTS.h"

#include "GameClient/FormationTranslator.h"
#include "GameClient/TotalWarControls.h"
#include "GameClient/TotalWarCamera.h"
#include "GameClient/TotalWarInput.h"
#include "GameClient/FormationLayout.h"
#include "Common/GameEngine.h"
#include "Common/GlobalData.h"
#include "Common/Recorder.h"
#include "Common/StatsCollector.h"
#include "GameClient/CommandXlat.h"
#include "GameClient/Display.h"
#include "GameClient/Drawable.h"
#include "GameClient/GameClient.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/InGameUI.h"
#include "GameClient/Keyboard.h"
#include "GameClient/Mouse.h"
#include "GameClient/SelectionInfo.h"
#include "GameClient/Shell.h"
#include "GameClient/View.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/AIUpdate.h"
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

	// GeneralsX @refactor Codex 04/10/2026 Use identical triangles for active previews and existing orders.
	void drawGroundMarker(View *view, const Coord3D &position, const Coord3D &direction, UnsignedInt color)
	{
		const Real sideX = direction.y;
		const Real sideY = -direction.x;
		Coord3D tip = position, left = position, right = position;
		tip.x += direction.x * 12.0f;
		tip.y += direction.y * 12.0f;
		left.x += sideX * 7.0f - direction.x * 7.0f;
		left.y += sideY * 7.0f - direction.y * 7.0f;
		right.x -= sideX * 7.0f + direction.x * 7.0f;
		right.y -= sideY * 7.0f + direction.y * 7.0f;
		drawGroundLine(view, tip, left, color, 2.0f);
		drawGroundLine(view, left, right, color, 2.0f);
		drawGroundLine(view, right, tip, color, 2.0f);
	}

	Bool showOrderMarkers()
	{
		return TotalWarControls::isEnabled() && TheKeyboard && TheKeyboard->isAlt()
			&& TheGameEngine && TheGameEngine->isActive()
			&& TheGameLogic && TheGameLogic->isInInteractiveGame() && !TheGameLogic->isGamePaused()
			&& TheInGameUI && TheInGameUI->getInputEnabled()
			&& TheShell && !TheShell->isShellActive()
			&& TheWindowManager && !TheWindowManager->winGetFocus()
			&& TheRecorder && TheRecorder->getMode() != RECORDERMODETYPE_PLAYBACK;
	}
}

FormationTranslator::FormationTranslator()
{
	TheFormationTranslator = this;
	reset();
	TotalWarControls::loadPreferences();
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
	m_preserving = false;
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
	return TotalWarControls::isEnabled() && TheGlobalData->m_useAlternateMouse && TheTacticalView && TheInGameUI
		&& TheInGameUI->getInputEnabled() && TheInGameUI->areSelectedObjectsControllable()
		&& !TheInGameUI->getGUICommand() && !TheInGameUI->getPendingPlaceType()
		&& !TheInGameUI->isSelecting()
		// GeneralsX @feature Codex 04/10/2026 Shift/J retain native queue/force orders; Alt no longer queues.
		&& !TheInGameUI->isInWaypointMode()
		&& !TheInGameUI->isInForceAttackMode() && !TheInGameUI->isInForceMoveToMode()
		&& !TheInGameUI->isInAttackMoveToMode()
		&& TheWindowManager && !TheWindowManager->winGetFocus()
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

Bool FormationTranslator::begin(const ICoord2D &screen, Bool preserving, Int modifiers)
{
	if (!canStart() || !TheTacticalView->screenToTerrain(&screen, &m_anchor))
		return false;

	// GeneralsX @feature Codex 03/10/2026 Alt+LMB must start on a selected ground unit, never empty ground.
	Drawable *picked = TheTacticalView->pickDrawable(&screen, false,
		preserving ? PICK_TYPE_SELECTABLE : (PickType)getPickTypesForContext(false));
	if (preserving)
	{
		if (!picked || !picked->isSelected() || !canFormUp(picked->getObject())
			|| TheMouse->getMouseStatus()->middleState != MBS_Up
			|| TheMouse->getMouseStatus()->rightState != MBS_Up)
			return false;
	}
	// Leave attacks, garrisoning, repair, salvage and other RMB object context commands unchanged.
	else if (picked && picked->getObject() && !picked->getObject()->isEffectivelyDead())
		return false;

	reset();
	m_preserving = preserving;
	m_center.zero();
	const DrawableList *selected = TheInGameUI->getAllSelectedDrawables();
	for (DrawableListCIt it = selected->begin(); it != selected->end(); ++it)
	{
		Object *obj = (*it)->getObject();
		if (!canFormUp(obj))
			continue;
		Slot slot;
		slot.objectID = obj->getID();
		slot.position = m_anchor;
		// GeneralsX @feature Codex 03/10/2026 Preserve each unit's offset and facing for ordinary group clicks.
		slot.originalPosition = *obj->getPosition();
		slot.originalDirection.x = WWMath::Cos(obj->getOrientation());
		slot.originalDirection.y = WWMath::Sin(obj->getOrientation());
		slot.originalDirection.z = 0.0f;
		slot.direction = slot.originalDirection;
		m_center.x += slot.originalPosition.x;
		m_center.y += slot.originalPosition.y;
		m_slots.push_back(slot);
		m_spacing = max(m_spacing, 2.0f * obj->getGeometryInfo().getBoundingCircleRadius() + 8.0f);
	}
	if (m_slots.empty())
		return false;

	m_center.x /= (Real)m_slots.size();
	m_center.y /= (Real)m_slots.size();
	m_direction = m_slots.front().originalDirection;
	m_screenAnchor = screen;
	m_previewScreen = screen;
	if (preserving)
	{
		const Bool rotating = (modifiers & KEY_STATE_CONTROL) != 0;
		// Starting with Ctrl already held rotates in place; otherwise the preview centers on the cursor.
		m_preservedDrag.begin(rotating ? m_center : m_anchor, screen, rotating);
	}
	m_selectionCount = TheInGameUI->getSelectCount();
	m_active = true;
	updatePreview(screen, modifiers);
	return true;
}

void FormationTranslator::updatePreview(const ICoord2D &screen, Int modifiers)
{
	m_previewScreen = screen;
	Coord3D end;
	m_validPosition = TheTacticalView->screenToTerrain(&screen, &end);
	if (!m_validPosition || m_slots.empty())
		return;

	if (m_preserving)
	{
		m_preservedDrag.update(end, screen, (modifiers & KEY_STATE_CONTROL) != 0);
		const Real cosine = WWMath::Cos(m_preservedDrag.angle());
		const Real sine = WWMath::Sin(m_preservedDrag.angle());
		for (std::vector<Slot>::iterator it = m_slots.begin(); it != m_slots.end(); ++it)
		{
			it->position = FormationLayout::transformedSlot(m_preservedDrag.center(),
				it->originalPosition, m_center, cosine, sine);
			it->position.z = TheTerrainLogic->getGroundHeight(it->position.x, it->position.y);
			it->direction = FormationLayout::rotatedDirection(it->originalDirection, cosine, sine);
		}
		return;
	}

	const Int dx = screen.x - m_screenAnchor.x;
	const Int dy = screen.y - m_screenAnchor.y;
	const Real wx = end.x - m_anchor.x;
	const Real wy = end.y - m_anchor.y;
	const Real distance = WWMath::Sqrt(wx * wx + wy * wy);
	// GeneralsX @bugfix Codex 02/10/2026 Keep following the drag direction when narrowing an existing formation.
	if ((m_dragged || dx * dx + dy * dy >= 64) && distance > 0.1f)
	{
		m_dragged = true;
		m_direction = FormationLayout::facing(m_anchor, end, distance, m_slots.size() == 1, m_direction);
	}

	const Int count = (Int)m_slots.size();
	for (Int i = 0; i < count; ++i)
	{
		Coord3D &pos = m_slots[i].position;
		pos = m_dragged
			? FormationLayout::slot(m_anchor, m_direction, distance, m_spacing, i, count)
			: FormationLayout::translatedSlot(m_anchor, m_slots[i].originalPosition, m_center);
		pos.z = TheTerrainLogic->getGroundHeight(pos.x, pos.y);
		m_slots[i].direction = m_dragged ? m_direction : m_slots[i].originalDirection;
	}
}

void FormationTranslator::finish(const ICoord2D &screen, Int modifiers)
{
	if (!m_cancelled && canStart() && selectionUnchanged())
	{
		updatePreview(screen, modifiers);
		if (m_validPosition)
		{
			if (!m_preserving && !m_dragged && (m_slots.size() == 1 || (Int)m_slots.size() != m_selectionCount))
			{
				// Keep normal single-unit movement and context handling for mixed selections (e.g. aircraft).
				TheGameClient->evaluateContextCommand(nullptr, &m_anchor, CommandTranslator::DO_COMMAND);
			}
			else
			{
				for (std::vector<Slot>::const_iterator it = m_slots.begin(); it != m_slots.end(); ++it)
				{
					const Coord3D &direction = it->direction;
					Coord3D facingPoint = it->position;
					facingPoint.x += direction.x * 100.0f;
					facingPoint.y += direction.y * 100.0f;
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
	// Update remapped modes before deciding whether the gesture belongs to formations or retail orders.
	if (TotalWarInput::translateGameMessage(msg) == DESTROY_MESSAGE)
		return DESTROY_MESSAGE;
	// GeneralsX @feature Codex 03/10/2026 Reserve camera gestures after GUI routing, before unit hotkeys.
	if (TotalWarCamera::translateGameMessage(msg) == DESTROY_MESSAGE)
		return DESTROY_MESSAGE;
	// GeneralsX @feature Codex 02/10/2026 Classic input passes through without consuming any events.
	if (!TotalWarControls::isEnabled())
	{
		reset();
		return KEEP_MESSAGE;
	}
	const GameMessage::Type type = msg->getType();
	if (type == GameMessage::MSG_CLEAR_GAME_DATA)
		reset();
	if (m_active && !m_cancelled && (!canStart() || !selectionUnchanged()))
		cancel();

	if (type == GameMessage::MSG_RAW_MOUSE_RIGHT_BUTTON_DOWN || type == GameMessage::MSG_RAW_MOUSE_RIGHT_DOUBLE_CLICK)
	{
		if (m_active && m_preserving)
		{
			cancel();
			// Let both halves of an overriding RMB command pass through.
			return KEEP_MESSAGE;
		}
		if (!m_active && begin(msg->getArgument(0)->pixel, false, msg->getArgument(1)->integer))
			return DESTROY_MESSAGE;
	}
	// GeneralsX @feature Codex 03/10/2026 Capture Alt+LMB before native selection and click synthesis.
	if (type == GameMessage::MSG_RAW_MOUSE_LEFT_BUTTON_DOWN || type == GameMessage::MSG_RAW_MOUSE_LEFT_DOUBLE_CLICK)
	{
		if (m_active)
		{
			if (m_preserving)
				return DESTROY_MESSAGE;
			cancel();
			return KEEP_MESSAGE;
		}
		const Int modifiers = msg->getArgument(1)->integer;
		if ((modifiers & KEY_STATE_ALT) && begin(msg->getArgument(0)->pixel, true, modifiers))
			return DESTROY_MESSAGE;
	}
	if (!m_active)
		return KEEP_MESSAGE;

	switch (type)
	{
		case GameMessage::MSG_RAW_MOUSE_RIGHT_BUTTON_UP:
			if (m_preserving)
				return KEEP_MESSAGE;
			finish(msg->getArgument(0)->pixel, msg->getArgument(1)->integer);
			return DESTROY_MESSAGE;
		case GameMessage::MSG_RAW_MOUSE_LEFT_BUTTON_UP:
			if (!m_preserving)
				return KEEP_MESSAGE;
			finish(msg->getArgument(0)->pixel, msg->getArgument(1)->integer);
			return DESTROY_MESSAGE;
		case GameMessage::MSG_RAW_MOUSE_POSITION:
			if (m_cancelled)
				return KEEP_MESSAGE;
			updatePreview(msg->getArgument(0)->pixel, msg->getArgument(1)->integer);
			return DESTROY_MESSAGE;
		case GameMessage::MSG_RAW_MOUSE_RIGHT_DRAG:
			if (m_preserving)
				return KEEP_MESSAGE;
			if (!m_cancelled)
				updatePreview(msg->getArgument(0)->pixel, msg->getArgument(2)->integer);
			return DESTROY_MESSAGE;
		case GameMessage::MSG_RAW_MOUSE_LEFT_DRAG:
			if (!m_preserving)
				return KEEP_MESSAGE;
			if (!m_cancelled)
				updatePreview(msg->getArgument(0)->pixel, msg->getArgument(2)->integer);
			return DESTROY_MESSAGE;
		case GameMessage::MSG_RAW_KEY_DOWN:
		case GameMessage::MSG_RAW_KEY_UP:
			if (msg->getArgument(0)->integer == KEY_ESC)
			{
				cancel();
				return DESTROY_MESSAGE;
			}
			if (m_preserving && !m_cancelled
				&& (msg->getArgument(0)->integer == KEY_LCTRL || msg->getArgument(0)->integer == KEY_RCTRL))
				updatePreview(m_previewScreen, msg->getArgument(1)->integer);
			// Keep physical Ctrl transitions for formation rotation and ordinary keyboard shortcuts.
			break;
		case GameMessage::MSG_FRAME_TICK:
		{
			// FRAME_TICK precedes this frame's raw input. Allow its release through before
			// cancelling a release swallowed by the window system (for example above the HUD).
			const Bool released = (m_preserving ? TheMouse->getMouseStatus()->leftState
				: TheMouse->getMouseStatus()->rightState) == MBS_Up;
			if (!TheGameEngine->isActive() || (m_releasePending && released))
				reset();
			else
				m_releasePending = released;
			break;
		}
		default:
			break;
	}
	return KEEP_MESSAGE;
}

void FormationTranslator::draw(View *view)
{
	const Bool preview = m_active && !m_cancelled && canStart() && selectionUnchanged();
	// GeneralsX @feature Codex 04/10/2026 Alt reveals all own ground-unit orders, including unselected ranks.
	if (showOrderMarkers())
	{
		for (Drawable *drawable = TheGameClient->getDrawableList(); drawable; drawable = drawable->getNextDrawable())
		{
			Object *obj = drawable->getObject();
			if (!canFormUp(obj) || !drawable->isSelectable() || drawable->isDrawableEffectivelyHidden()
				|| (preview && drawable->isSelected()))
				continue;
			Coord3D position, direction;
			obj->getAIUpdateInterface()->getFormationMarker(position, direction);
			drawGroundMarker(view, position, direction, 0xB060DFFF);
		}
	}
	if (!preview)
		return;
	const UnsignedInt color = m_validPosition ? 0xD060FF80 : 0xD0FFB040;
	// GeneralsX @tweak Codex 02/10/2026 Show only individual destination/facing markers, without a connecting line.
	for (std::vector<Slot>::const_iterator it = m_slots.begin(); it != m_slots.end(); ++it)
		drawGroundMarker(view, it->position, it->direction, color);
}
