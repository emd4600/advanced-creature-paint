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

#define TestModelGroup(name) if (model->HasGroup((uint32_t)Graphics::ModelGroups::name)) SporeDebugPrint("     " #name);

class SkinCheat
	: public ArgScript::ICommand
{
public:
	virtual void ParseLine(const ArgScript::Line& line) override
	{
		//for (int i = 0; i < Editor.GetEditorModel()->GetRigblocksCount(); i++)
		//{
		//	auto rigblock = Editor.GetEditorModel()->GetRigblock(i);
		//	//rigblock->mpModelWorld->SetInWorld(rigblock->mpModel.get(), false);
		//	SporeDebugPrint("   %d", rigblock->mpModel->IsVisible());
		//}
		//auto rigblock = Editor.GetEditorModel()->GetRigblock(0);
		//auto modelWorld = rigblock->mpModelWorld;
		//eastl::vector<Graphics::Model*> dst;
		//modelWorld->FindModels(dst);

		//SporeDebugPrint("%x   %d", modelWorld.get(), modelWorld->GetActive());
		//SporeDebugPrint("Contains %d models", dst.size());

		//for (auto model : dst)
		//{
		//	auto resKey = model->GetPropList()->GetResourceKey();
		//	SporeDebugPrint("  %x!%x    OwnerIsRigblock? %d", resKey.groupID, resKey.instanceID, object_cast<Editors::EditorRigblock>(model->GetOwner()) != nullptr);

		//	//SporeDebugPrint("      %d", model->HasGroup((uint32_t)Graphics::ModelGroups::AnimatedCreature));
		//	TestModelGroup(DeformHandle);
		//	TestModelGroup(DeformHandleOverdraw);
		//	TestModelGroup(Background);
		//	TestModelGroup(Overdraw);
		//	TestModelGroup(EffectsMask);
		//	TestModelGroup(TestEnv);
		//	TestModelGroup(PartsPaintEnv);
		//	TestModelGroup(Skin);
		//	TestModelGroup(RotationRing);
		//	TestModelGroup(RotationBall);
		//	TestModelGroup(SocketConnector);
		//	TestModelGroup(AnimatedCreature);
		//	TestModelGroup(TestMode);
		//	TestModelGroup(Vertebra);
		//	TestModelGroup(PaletteSkin);
		//	TestModelGroup(ExcludeFromPinning);
		//	TestModelGroup(Palette);
		//	TestModelGroup(BallConnector);
		//	TestModelGroup(Rigblock);
		//	TestModelGroup(RigblockEffect);
		//	TestModelGroup(GameBackground);
		//}

		//modelWorld->SetActive(!modelWorld->GetActive());


		/*eastl::vector<uint32_t> worldIDs;
		EffectsManager.GetWorldIDs(worldIDs);

		for (auto worldID : worldIDs)
		{
			auto world = EffectsManager.World(worldID);

			SporeDebugPrint("EffectsWorld ID: 0x%x, State: %d", worldID, world->GetState());
		}

		size_t numArgs = 0;
		auto args = line.GetArgumentsRange(&numArgs, 0, 1);
		if (numArgs > 0)
		{
			auto worldID = mpFormatParser->ParseUInt(args[0]);
			auto world = EffectsManager.World(worldID);
			if (world)
			{
				world->SetState(Swarm::SwarmState::Hidden);
			}
			else
			{
				SporeDebugPrint("No effects world with ID 0x%x", worldID);
			}
		}*/

		auto world = EffectsManager.World(0xe4c6e4);

		eastl::vector<Swarm::cEffectInfo> effectInfos;
		world->GetActiveEffectInfo(effectInfos, "*", 0);
		
		for (auto& effectInfo : effectInfos)
		{
			SporeDebugPrint("Effect %s,   IsRunning: %d", effectInfo.mIDString.c_str(), effectInfo.mpEffect->IsRunning());
		}
	}
};

class ClearPaintsCheat
	: public ArgScript::ICommand
{
public:
	virtual void ParseLine(const ArgScript::Line& line) override
	{
		if (Editor.GetSkin() && Editor.GetSkin()->GetMesh())
		{
			((AdvancedCreatureDataResource*)Editor.GetSkin()->GetMesh()->mpCreatureData)->ClearPaintInfos();
		}
		for (auto& rigblock : Editor.GetEditorModel()->mRigblocks)
		{
			rigblock->mPaints.clear();
		}
	}
};

void Initialize()
{
	CheatManager.AddCheat("skin", new SkinCheat());
	CheatManager.AddCheat("acpClear", new ClearPaintsCheat());
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
	//PaintPartsJob_Execute__detour::attach(Address(0x519780));
	EditorModel_Load__detour::attach(GetAddress(Editors::EditorModel, Load));
	EditorModel_Save__detour::attach(GetAddress(Editors::EditorModel, Save));
	Editor_HandleMessage__detour::attach(GetAddress(Editors::cEditor, HandleMessage));
	Editor_OnMouseDown__detour::attach(GetAddress(Editors::cEditor, OnMouseDown));
	Editor_OnMouseUp__detour::attach(GetAddress(Editors::cEditor, OnMouseUp));
	Editor_Update__detour::attach(GetAddress(Editors::cEditor, Update));
	PaletteUI_SetActiveCategory__detour::attach(GetAddress(Palettes::PaletteUI, SetActiveCategory));
	PaletteCategoryUI_LayoutPagePanel__detour::attach(GetAddress(Palettes::PaletteCategoryUI, LayoutPagePanel));
	//Test__detour::attach(Address(0x43A730));
	//Test__detour::attach(Address(0x4BECA0));

	PaintPartsJob_Execute__detour::attach(GetAddress(Skinner::cSkinPainterJobPaintParts, Execute));
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

