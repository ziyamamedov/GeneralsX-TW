#include "PreRTS.h"

#include "GameClient/TotalWarInput.h"
#include "GameClient/TotalWarModifierInput.h"
#include "GameClient/TotalWarControls.h"
#include "Common/GameEngine.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/HotKey.h"
#include "GameClient/InGameUI.h"
#include "GameClient/Keyboard.h"
#include "GameClient/MetaEvent.h"
#include "GameClient/Shell.h"
#include "GameLogic/GameLogic.h"

// GeneralsX @feature Codex 04/10/2026 Shift queues orders, Ctrl adds selection, J holds force attack.
namespace
{
	TotalWarModifierInput input;
	Bool ownsModes = false;

	Bool canUseBindings()
	{
		return TotalWarControls::isEnabled() && TheGameEngine && TheGameEngine->isActive()
			&& TheGameLogic && TheGameLogic->isInInteractiveGame() && !TheGameLogic->isGamePaused()
			&& TheInGameUI && TheInGameUI->getInputEnabled()
			&& TheShell && !TheShell->isShellActive()
			&& TheWindowManager && !TheWindowManager->winGetFocus();
	}

	char replacementLetter()
	{
		if (!TheHotKeyManager || !TheHotKeyManager->hasHotKey("j"))
			return '\0';
		UnsignedInt occupied = 0;
		for (char letter = 'a'; letter <= 'z'; ++letter)
		{
			char key[] = { letter, '\0' };
			if (TheHotKeyManager->hasHotKey(key))
				occupied |= 1u << (letter - 'a');
		}
		// Include loaded global bindings too, so mods/custom CommandMap.ini keys are not shadowed.
		if (TheMetaMap && TheKeyboard)
			for (const MetaMapRec *map = TheMetaMap->getFirstMetaMapRec(); map; map = map->m_next)
			{
				if (map->m_modState != NONE || !(map->m_usableIn & COMMANDUSABLE_GAME) || map->m_key == MK_NONE)
					continue;
				WideChar letter = TheKeyboard->getPrintableKey((KeyDefType)TotalWarInput::commandKey(map->m_key), 0);
				if (letter >= L'A' && letter <= L'Z')
					letter += L'a' - L'A';
				if (letter >= L'a' && letter <= L'z')
					occupied |= 1u << (letter - L'a');
			}
		return TotalWarModifierInput::replacementLetter(occupied);
	}
}

void TotalWarInput::reset()
{
	input.cancel(); // Retain ownership of J-up, including when switching back to Classic while held.
	if (ownsModes && TheInGameUI)
	{
		TheInGameUI->setWaypointMode(false);
		TheInGameUI->setPreferSelectionMode(false);
		TheInGameUI->setForceAttackMode(false);
	}
	ownsModes = false;
}

GameMessageDisposition TotalWarInput::translateGameMessage(const GameMessage *msg)
{
	const GameMessage::Type type = msg->getType();
	if (type == GameMessage::MSG_CLEAR_GAME_DATA)
		reset();
	const Bool allowed = canUseBindings();
	if (!allowed)
		reset();
	if (type == GameMessage::MSG_FRAME_TICK && TheKeyboard)
		input.checkReleased(TheKeyboard->isKeyDown(KEY_J));
	Bool consumed = false;
	if ((type == GameMessage::MSG_RAW_KEY_DOWN || type == GameMessage::MSG_RAW_KEY_UP)
		&& msg->getArgument(0)->integer == KEY_J)
	{
		const Int state = msg->getArgument(1)->integer;
		consumed = input.keyEvent((state & KEY_STATE_UP) == 0,
			(state & KEY_STATE_AUTOREPEAT) != 0, allowed);
	}
	if (allowed && TheKeyboard)
	{
		const TotalWarModifierInput::Modes modes = input.modes(TheKeyboard->isShift(), TheKeyboard->isCtrl(), true);
		TheInGameUI->setWaypointMode(modes.queue);
		TheInGameUI->setPreferSelectionMode(modes.selection);
		TheInGameUI->setForceAttackMode(modes.forceAttack);
		ownsModes = true;
	}
	return consumed ? DESTROY_MESSAGE : KEEP_MESSAGE;
}

Bool TotalWarInput::ownsModifierBinding(GameMessage::Type type)
{
	if (!TotalWarControls::isEnabled())
		return false;
	return type == GameMessage::MSG_META_BEGIN_WAYPOINTS || type == GameMessage::MSG_META_END_WAYPOINTS
		|| type == GameMessage::MSG_META_BEGIN_PREFER_SELECTION || type == GameMessage::MSG_META_END_PREFER_SELECTION
		|| type == GameMessage::MSG_META_BEGIN_FORCEATTACK || type == GameMessage::MSG_META_END_FORCEATTACK;
}

Int TotalWarInput::commandKey(Int originalKey)
{
	// GeneralsX @tweak Codex 04/10/2026 Restore C shortcuts; reserve J and move any mapped J command to K.
	return TotalWarControls::isEnabled() && originalKey == KEY_J ? KEY_K : originalKey;
}

AsciiString TotalWarInput::panelKey(const AsciiString &pressedKey)
{
	if (canUseBindings() && pressedKey.getLength() == 1)
	{
		const char replacement = replacementLetter();
		if (replacement && (pressedKey.getCharAt(0) == replacement || pressedKey.getCharAt(0) == replacement - 'a' + 'A'))
			return "j";
	}
	return pressedKey;
}

UnicodeString TotalWarInput::panelLabel(const UnicodeString &label)
{
	if (!TotalWarControls::isEnabled())
		return label;
	const WideChar *text = label.str();
	const WideChar *hotkey = nullptr;
	for (const WideChar *p = text; *p; ++p)
		if (*p == L'&') { hotkey = p; break; }
	if (!hotkey || (hotkey[1] != L'J' && hotkey[1] != L'j'))
		return label;
	const char replacement = replacementLetter();
	if (!replacement)
		return label;
	UnicodeString result;
	for (const WideChar *p = text; *p; ++p)
		if (p != hotkey)
			result.concat(*p);
	WideChar suffix[] = { L' ', L'(', L'&', WideChar(replacement - 'a' + 'A'), L')', 0 };
	result.concat(suffix);
	return result;
}
