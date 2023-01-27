#pragma once
#include <Spore\BasicIncludes.h>
#include "AdvancedCreatureDataResource.h"

// We use this file for the user-side of the mod: the paint categories, clicking on parts, etc

member_detour(PaletteUI_SetActiveCategory__detour, Palettes::PaletteUI, void(int))
{
	void detoured(int index);
};

member_detour(PaletteCategoryUI_LayoutPagePanel__detour, Palettes::PaletteCategoryUI, void())
{
	void detoured();
};

virtual_detour(Editor_HandleMessage__detour, Editors::cEditor, App::IMessageListener, bool(uint32_t messageID, void* msg))
{
	bool detoured(uint32_t messageID, void* msg);
};

virtual_detour(Editor_OnMouseDown__detour, Editors::cEditor, App::IGameMode, bool(MouseButton, float, float, MouseState))
{
	bool detoured(MouseButton mouseButton, float mouseX, float mouseY, MouseState mouseState);
};

virtual_detour(Editor_OnMouseUp__detour, Editors::cEditor, App::IGameMode, bool(MouseButton, float, float, MouseState))
{
	bool detoured(MouseButton mouseButton, float mouseX, float mouseY, MouseState mouseState);
};


virtual_detour(Editor_Update__detour, Editors::cEditor, App::IGameMode, void(float, float))
{
	void detoured(float delta1, float delta2);
};
