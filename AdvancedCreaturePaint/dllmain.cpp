// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "AdvancedCreatureDataResource.h"
#include "UserInteractionDetours.h"
#include "PaintSystemDetours.h"
#include <Spore\GeneralAllocator.h>
#include <Spore\Skinner\cPaintSystem.h>

member_detour(EditorModel_Load__detour, Editors::EditorModel, void(Editors::cEditorResource*))
{
	void detoured(Editors::cEditorResource * resourceParam)
	{
		original_function(this, resourceParam);

		return;
	}
};

member_detour(EditorModel_Save__detour, Editors::EditorModel, void(Editors::cEditorResource*))
{
	void detoured(Editors::cEditorResource* resourceParam)
	{
		original_function(this, resourceParam);

		// This is what Spore does, maybe the parameter can be something else? no idea
		auto resource = object_cast<Editors::cEditorResource>(resourceParam);
		if (field_2C && resource)
		{
			for (unsigned int i = 0; i < mRigblocks.size(); i++)
			{
				auto& block = resource->mBlocks[i];
				auto& paints = mRigblocks[i]->mPaints;
				if (paints.size() > 0)
				{
					block.paintListCount = paints.size();
					for (int j = 0; j < eastl::min((int)paints.size(), 8); j++)
					{
						block.paintRegions[j] = paints[j].first;
						block.paintIDs[j] = paints[j].second.mPaintID;
						block.paintColors1[j] = paints[j].second.mPaintColor1;
						block.paintColors2[j] = paints[j].second.mPaintColor2;
					}
				}
			}
		}
	}
};


void Initialize()
{
}

void Dispose()
{
}

static_detour(Test__detour, bool(int, int))
{
	bool detoured(int arg1, int)
	{
		return false;
	}
};

void AttachDetours()
{
	EditorModel_Load__detour::attach(GetAddress(Editors::EditorModel, Load));
	EditorModel_Save__detour::attach(GetAddress(Editors::EditorModel, Save));
	Editor_HandleMessage__detour::attach(GetAddress(Editors::cEditor, HandleMessage));
	Editor_OnMouseDown__detour::attach(GetAddress(Editors::cEditor, OnMouseDown));
	Editor_OnMouseUp__detour::attach(GetAddress(Editors::cEditor, OnMouseUp));
	Editor_Update__detour::attach(GetAddress(Editors::cEditor, Update));
	PaletteCategoryUI_LayoutPagePanel__detour::attach(GetAddress(Palettes::PaletteCategoryUI, LayoutPagePanel));

	PaintPartsJob_Execute__detour::attach(GetAddress(Skinner::cSkinPainterJobPaintParts, Execute));
	BumpToNormalJob_Execute__detour::attach(GetAddress(Skinner::cSkinPainterJobBumpToNormal, Execute));
	cSkinnerTexturePainter_LoadMaterial__detour::attach(GetAddress(Skinner::cSkinnerTexturePainter, LoadMaterial));

	AdvancedCreatureDataResource::AttachDetours();
}


// Generally, you don't need to touch any code here
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		ModAPI::AddPostInitFunction(Initialize);
		ModAPI::AddDisposeFunction(Dispose);

		PrepareDetours(hModule);
		AttachDetours();
		CommitDetours();
		break;

	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

