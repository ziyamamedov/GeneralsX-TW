#include "PreRTS.h"

#include "GameClient/TotalWarCamera.h"
#include "GameClient/TotalWarCameraInput.h"
#include "GameClient/TotalWarControls.h"
#include "Common/FramePacer.h"
#include "Common/GameEngine.h"
#include "Common/GlobalData.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/InGameUI.h"
#include "GameClient/Keyboard.h"
#include "GameClient/Mouse.h"
#include "GameClient/Shell.h"
#include "GameClient/View.h"
#include "GameLogic/GameLogic.h"

// GeneralsX @feature Codex 03/10/2026 WASD pan and MMB yaw for the opt-in Total War control scheme.
namespace
{
	TotalWarCameraInput input;
	const KeyDefType cameraKeys[] = { KEY_W, KEY_S, KEY_A, KEY_D };
	Bool rotating = false;
	Int rotationAnchorX = 0;
	Real rotationAnchorAngle = 0.0f;

	Bool canControlCamera()
	{
		return TotalWarControls::isEnabled() && TheGameEngine && TheGameEngine->isActive()
			&& TheTacticalView && TheInGameUI && TheInGameUI->getInputEnabled()
			&& TheGameLogic && TheGameLogic->isInGame() && !TheGameLogic->isGamePaused()
			&& TheShell && !TheShell->isShellActive()
			&& TheWindowManager && !TheWindowManager->winGetFocus();
	}

	void updateRotation(const ICoord2D &position)
	{
		// Match the native camera sensitivity; dragging vertically does not change pitch or zoom.
		TheTacticalView->userSetAngle(rotationAnchorAngle + 0.01f * (position.x - rotationAnchorX));
	}
}

void TotalWarCamera::reset()
{
	input.reset();
	rotating = false;
}

GameMessageDisposition TotalWarCamera::translateGameMessage(const GameMessage *msg)
{
	if (!TotalWarControls::isEnabled() || msg->getType() == GameMessage::MSG_CLEAR_GAME_DATA)
	{
		reset();
		return KEEP_MESSAGE;
	}
	const Bool allowed = canControlCamera();
	if (!allowed)
	{
		input.cancelMovement();
		rotating = false;
	}

	switch (msg->getType())
	{
		case GameMessage::MSG_RAW_KEY_DOWN:
		case GameMessage::MSG_RAW_KEY_UP:
		{
			const Int key = msg->getArgument(0)->integer;
			const Int state = msg->getArgument(1)->integer;
			for (Int i = 0; i < TotalWarCameraInput::COUNT; ++i)
			{
				if (key != cameraKeys[i])
					continue;
				const Bool consumed = input.keyEvent((TotalWarCameraInput::Direction)i,
					(state & KEY_STATE_UP) == 0, (state & KEY_STATE_AUTOREPEAT) != 0,
					allowed && !(state & (KEY_STATE_CONTROL | KEY_STATE_ALT)));
				return consumed ? DESTROY_MESSAGE : KEEP_MESSAGE;
			}
			break;
		}
		case GameMessage::MSG_RAW_MOUSE_MIDDLE_BUTTON_DOWN:
		case GameMessage::MSG_RAW_MOUSE_MIDDLE_DOUBLE_CLICK:
			if (allowed && !TheInGameUI->isSelecting() && TheMouse->getMouseStatus()->rightState == MBS_Up)
			{
				rotating = true;
				rotationAnchorX = msg->getArgument(0)->pixel.x;
				rotationAnchorAngle = TheTacticalView->getAngle();
			}
			return DESTROY_MESSAGE;
		case GameMessage::MSG_RAW_MOUSE_MIDDLE_BUTTON_UP:
			if (rotating)
				updateRotation(msg->getArgument(0)->pixel);
			rotating = false;
			return DESTROY_MESSAGE;
		case GameMessage::MSG_RAW_MOUSE_MIDDLE_DRAG:
			if (rotating)
				updateRotation(msg->getArgument(0)->pixel);
			return DESTROY_MESSAGE;
		case GameMessage::MSG_RAW_MOUSE_POSITION:
			if (rotating)
			{
				// Release may have been consumed by the HUD or lost when leaving the window.
				if (TheMouse->getMouseStatus()->middleState == MBS_Up)
					rotating = false;
				else
					updateRotation(msg->getArgument(0)->pixel);
				return DESTROY_MESSAGE;
			}
			break;
		default:
			break;
	}
	return KEEP_MESSAGE;
}

Bool TotalWarCamera::getScrollOffset(Coord2D &offset)
{
	if (!canControlCamera() || !TheKeyboard)
	{
		input.cancelMovement();
		rotating = false;
		return false;
	}
	for (Int i = 0; i < TotalWarCameraInput::COUNT; ++i)
		input.checkReleased((TotalWarCameraInput::Direction)i, TheKeyboard->isKeyDown(cameraKeys[i]));
	if (rotating && TheMouse->getMouseStatus()->middleState == MBS_Up)
		rotating = false;
	if (TheInGameUI->isSelecting())
		return false;
	const Int x = input.horizontal(), y = input.vertical();
	if (!x && !y && !rotating)
		return false;
	// Use the same FPS-independent speed and options-menu factor as native arrow scrolling.
	const Real speed = 200.0f * TheFramePacer->getBaseOverUpdateFpsRatio() * TheGlobalData->m_keyboardScrollFactor;
	offset.x = x * speed * TheGlobalData->m_horizontalScrollSpeedFactor;
	offset.y = y * speed * TheGlobalData->m_verticalScrollSpeedFactor;
	return true;
}
