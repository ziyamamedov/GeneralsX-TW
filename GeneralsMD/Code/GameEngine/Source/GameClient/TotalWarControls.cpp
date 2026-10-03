#include "PreRTS.h"

#include "GameClient/TotalWarControls.h"
#include "GameClient/TotalWarCamera.h"
#include "Common/GlobalData.h"
#include "Common/OptionPreferences.h"
#include "GameClient/FormationTranslator.h"
#include "GameClient/GadgetCheckBox.h"
#include "GameClient/GadgetComboBox.h"
#include "GameClient/GadgetStaticText.h"
#include "GameClient/GameText.h"
#include "GameClient/GameWindowManager.h"

// GeneralsX @feature Codex 02/10/2026 Keep optional controls, persistence and UI out of retail option classes/assets.
namespace
{
	Bool enabled = false;
	Bool classicAlternateMouse = false;
	const char *const preferenceKey = "ControlScheme";
	const char *const comboName = "OptionsMenu.wnd:ComboBoxControlScheme";

	GameWindow *optionWindow(const char *name)
	{
		return TheWindowManager->winGetWindowFromId(nullptr, TheNameKeyGenerator->nameToKey(name));
	}

	Bool totalWarSelected()
	{
		Int index = 0;
		GameWindow *combo = optionWindow(comboName);
		if (combo)
			GadgetComboBoxGetSelectedPos(combo, &index);
		return index == 1;
	}

	void apply(const OptionPreferences &preferences)
	{
		// Missing or unknown settings always retain the original controls.
		enabled = preferences.getAsciiString(preferenceKey, "Classic") == "TotalWar";
		classicAlternateMouse = preferences.getBool("UseAlternateMouse", classicAlternateMouse);
		TheWritableGlobalData->m_useAlternateMouse = enabled || classicAlternateMouse;
		TotalWarCamera::reset();
		if (TheFormationTranslator)
			TheFormationTranslator->reset();
	}

	void copyAppearance(GameWindow *target, GameWindow *source)
	{
		WinInstanceData *to = target->winGetInstanceData();
		WinInstanceData *from = source->winGetInstanceData();
		for (Int i = 0; i < MAX_DRAW_DATA; ++i)
		{
			to->m_enabledDrawData[i] = from->m_enabledDrawData[i];
			to->m_disabledDrawData[i] = from->m_disabledDrawData[i];
			to->m_hiliteDrawData[i] = from->m_hiliteDrawData[i];
		}
		target->winSetEnabledTextColors(source->winGetEnabledTextColor(), source->winGetEnabledTextBorderColor());
		target->winSetDisabledTextColors(source->winGetDisabledTextColor(), source->winGetDisabledTextBorderColor());
		target->winSetHiliteTextColors(source->winGetHiliteTextColor(), source->winGetHiliteTextBorderColor());
	}
}

Bool TotalWarControls::isEnabled()
{
	return enabled;
}

void TotalWarControls::loadPreferences()
{
	classicAlternateMouse = TheGlobalData->m_useAlternateMouse;
	OptionPreferences preferences;
	apply(preferences);
}

void TotalWarControls::initOptions(const OptionPreferences &preferences)
{
	GameWindow *alternate = optionWindow("OptionsMenu.wnd:CheckAlternateMouse");
	GameWindow *rightColumn = optionWindow("OptionsMenu.wnd:CheckDoubleClickAttackMove");
	GameWindow *secondRow = optionWindow("OptionsMenu.wnd:Retaliation");
	GameWindow *reference = optionWindow("OptionsMenu.wnd:ComboBoxResolution");
	if (!alternate || !rightColumn || !secondRow || !reference)
		return;

	GameWindow *combo = optionWindow(comboName);
	if (!combo)
	{
		// Use the unused second row in the mouse panel; follow the loaded layout's scaling.
		Int x, y, unused, width, height;
		rightColumn->winGetPosition(&x, &unused);
		secondRow->winGetPosition(&unused, &y);
		rightColumn->winGetSize(&width, &height);
		const Int labelWidth = width / 3;
		WinInstanceData labelData;
		labelData.m_style = GWS_STATIC_TEXT;
		TextData textData = {};
		textData.centeredVertically = true;
		GameWindow *label = TheWindowManager->gogoGadgetStaticText(rightColumn->winGetParent(),
			WIN_STATUS_ENABLED, x, y, labelWidth, height, &labelData, &textData, alternate->winGetFont(), false);
		GadgetStaticTextSetText(label, TheGameText->FETCH_OR_SUBSTITUTE("GUI:ControlScheme", L"Controls"));
		label->winSetEnabledTextColors(alternate->winGetEnabledTextColor(), alternate->winGetEnabledTextBorderColor());

		WinInstanceData instance;
		instance.m_style = GWS_COMBO_BOX | GWS_MOUSE_TRACK;
		instance.m_id = TheNameKeyGenerator->nameToKey(comboName);
		instance.m_font = alternate->winGetFont();
		ComboBoxData data = {};
		data.maxDisplay = 2;
		data.maxChars = 32;
		data.entryData = NEW EntryData{};
		data.entryData->maxTextLen = data.maxChars;
		data.listboxData = NEW ListboxData{};
		data.listboxData->listLength = 2;
		data.listboxData->columns = 1;
		data.listboxData->forceSelect = true;
		combo = TheWindowManager->gogoGadgetComboBox(rightColumn->winGetParent(),
			WIN_STATUS_ENABLED | (reference->winGetStatus() & WIN_STATUS_IMAGE),
			x + labelWidth, y, width - labelWidth, height, &instance, &data, alternate->winGetFont(), false);
		copyAppearance(combo, reference);
		copyAppearance(GadgetComboBoxGetDropDownButton(combo), GadgetComboBoxGetDropDownButton(reference));
		copyAppearance(GadgetComboBoxGetEditBox(combo), GadgetComboBoxGetEditBox(reference));
		copyAppearance(GadgetComboBoxGetListBox(combo), GadgetComboBoxGetListBox(reference));
		GadgetComboBoxSetFont(combo, alternate->winGetFont());
	}

	GadgetComboBoxReset(combo);
	GadgetComboBoxAddEntry(combo, TheGameText->FETCH_OR_SUBSTITUTE("GUI:ClassicControls", L"Classic"),
		reference->winGetEnabledTextColor());
	GadgetComboBoxAddEntry(combo, TheGameText->FETCH_OR_SUBSTITUTE("GUI:TotalWarControls", L"Total War"),
		reference->winGetEnabledTextColor());
	GadgetComboBoxSetSelectedPos(combo, enabled ? 1 : 0);
	// Show the stored classic preference, even while Total War temporarily forces RMB orders.
	GadgetCheckBoxSetChecked(alternate, preferences.getBool("UseAlternateMouse", classicAlternateMouse));
	updateOptions();
}

void TotalWarControls::updateOptions()
{
	GameWindow *alternate = optionWindow("OptionsMenu.wnd:CheckAlternateMouse");
	if (alternate)
		alternate->winEnable(!totalWarSelected());
}

void TotalWarControls::resetOptions()
{
	GameWindow *combo = optionWindow(comboName);
	if (combo)
		GadgetComboBoxSetSelectedPos(combo, 0);
	updateOptions();
}

void TotalWarControls::saveOptions(OptionPreferences &preferences)
{
	preferences.setAsciiString(preferenceKey, totalWarSelected() ? "TotalWar" : "Classic");
	// OptionsMenu has already restored/saved the classic mouse-button preference.
	apply(preferences);
}
