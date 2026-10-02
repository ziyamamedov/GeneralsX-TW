#pragma once

#include "Lib/BaseType.h"

class OptionPreferences;

// GeneralsX @feature Codex 02/10/2026 Optional Zero Hour input mode and its options-menu integration.
// This is a local input preference, never a condition for executing synchronized orders.
namespace TotalWarControls
{
	Bool isEnabled();
	void loadPreferences();
	void initOptions(const OptionPreferences &preferences);
	void updateOptions();
	void resetOptions();
	void saveOptions(OptionPreferences &preferences);
}
