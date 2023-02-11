#pragma once
#include "stdafx.h"
#include <Spore\Skinner\cSkinPainterJobPaintParts.h>
#include <Spore\Skinner\cSkinPainterJobBumpToNormal.h>
#include "AdvancedCreatureDataResource.h"

member_detour(PaintPartsJob_Execute__detour, Skinner::cSkinPainterJobPaintParts, bool())
{
	bool detoured();
};

member_detour(BumpToNormalJob_Execute__detour, Skinner::cSkinPainterJobBumpToNormal, bool())
{
	bool detoured();
};

member_detour(cSkinnerTexturePainter_LoadMaterial__detour, Skinner::cSkinnerTexturePainter, RenderWare::CompiledState* ())
{
	RenderWare::CompiledState* detoured();
};