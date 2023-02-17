#include "stdafx.h"
#include "PaintSystemDetours.h"
#include "AdvancedCreatureDataResource.h"
#include "Constants.h"


const int NUM_EXTENDED_RASTERS = 8;
const int NUM_EXTENDED_PARAMS = 11;
bool sExtendedRenderEnabled = false;
RenderWare::Raster* sExtendedRasters[NUM_EXTENDED_RASTERS] = {};
Math::ColorRGBA sExtendedCustomParams[NUM_EXTENDED_PARAMS] = {};


void ResetExtendedRendering()
{
	memset(sExtendedRasters, 0, sizeof(sExtendedRasters));
	memset(sExtendedCustomParams, 0, sizeof(sExtendedCustomParams));
	sExtendedRenderEnabled = false;

	for (int i = 0; i < NUM_EXTENDED_RASTERS; i++)
	{
		Graphics::ActiveState::SetTexture(i, nullptr);
		Graphics::ActiveState::SetRasterDelta(Graphics::ActiveState::GetRasterDelta() | (1 << i));
	}
}


bool PaintPartsJob_Execute__detour::detoured()
{
	auto texture0 = PaintSystem.GetPainter()->mpTexture0;
	auto texture1 = PaintSystem.GetPainter()->mpTexture1;
	auto skinMesh = PaintSystem.GetSkinMesh();

	auto creatureData = (AdvancedCreatureDataResource*)skinMesh->mpCreatureData;
	int count = creatureData->mRigblocks.size();
	auto numPaintInfos = creatureData->GetNumPaintInfos();

	if (mStage == 1)
	{
		SporeDebugPrint("[PaintPartsJob detour]: Executed stage 1, %d paint infos available", creatureData->GetNumPaintInfos());
		for (int i = 0; i < creatureData->GetNumPaintInfos(); i++)
		{
			const auto& paintInfo = creatureData->GetPaintInfos()[i];
			SporeDebugPrint("   PaintInfo [rigblockIndex=%d, %x!%x]", paintInfo.rigblockIndex,
				creatureData->mRigblocks[paintInfo.rigblockIndex].mGroupID, creatureData->mRigblocks[paintInfo.rigblockIndex].mInstanceID);
		}
	}

	if (mStage == 1) mStage = 2;

	if (mStage >= 4)
	{
		return mStage == 4;
	}

	if (mStage >= 2) {
		texture1->StartRender();
	}
	else {
		texture0->StartRender();
	}

	float skinpaintPartBumpScale;
	App::Property::GetFloat(App::GetAppProperties(), 0x1C76D9B5, skinpaintPartBumpScale);
	if (skinpaintPartBumpScale == 0.0f) skinpaintPartBumpScale = 1.0f;

	int lastPaintInfoIndex = 0;
	int rigblockLimitCount = 0;
	while (mRigblockIndex < count && rigblockLimitCount < 10)
	{
		if (creatureData->mRigblocks[mRigblockIndex].mFlags & Editors::cCreatureDataResource::kFlagNotUseSkin)
		{
			mRigblockIndex++;
			continue;
		}

		rigblockLimitCount++;
		auto& uvs = skinMesh->mUVs[mRigblockIndex];
		Vector2 uv1{ uvs.x1, uvs.y1 };
		Vector2 uv2{ uvs.x2, uvs.y2 };
		auto propList = skinMesh->mRigblockPropLists[mRigblockIndex].get();

		Graphics::Texture* skinpaintDiffuseTexture = nullptr;
		Graphics::Texture* skinpaintTintMaskTexture = nullptr;
		Graphics::Texture* skinpaintSpecBumpTexture = nullptr;

		if (mStage == 0)
		{
			// skinpaintDiffuseTexture
			if (propList->HasProperty(0x2424655))
			{
				ResourceKey key{};
				App::Property::GetKey(propList, 0x2424655, key);
				if (TextureManager.HasTexture(key))
				{
					skinpaintDiffuseTexture = TextureManager.GetTexture(key,
						Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);
				}
			}
		}
		if (mStage == 0 || mStage == 1)
		{
			// skinpaintTintMaskTexture
			if (propList->HasProperty(0x2424657))
			{
				ResourceKey key{};
				App::Property::GetKey(propList, 0x2424657, key);
				if (TextureManager.HasTexture(key))
				{
					skinpaintTintMaskTexture = TextureManager.GetTexture(key,
						Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);
				}
			}
		}
		if (mStage == 2 || mStage == 3)
		{
			// skinpaintSpecBumpTexture
			if (propList->HasProperty(0x2424656))
			{
				ResourceKey key{};
				App::Property::GetKey(propList, 0x2424656, key);
				if (TextureManager.HasTexture(key))
				{
					skinpaintSpecBumpTexture = TextureManager.GetTexture(key,
						Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);
				}
			}
		}

		if (mStage == 0)
		{
			bool hasAdvancedPaint = false;
			const AdvancedCreatureDataResource::PaintInfo* paintInfos[AdvancedCreaturePaint::NUM_REGIONS] = {};

			// Find if there is any advanced paint on this rigblock
			for (; lastPaintInfoIndex < creatureData->GetNumPaintInfos(); lastPaintInfoIndex++)
			{
				const auto& paintInfo = creatureData->GetPaintInfos()[lastPaintInfoIndex];
				if (paintInfo.rigblockIndex == mRigblockIndex)
				{
					paintInfos[paintInfo.paintRegion] = &paintInfo;
					hasAdvancedPaint = true;
				}
				else if (paintInfo.rigblockIndex > mRigblockIndex)
				{
					// They are ordered by rigblock index, so if we reach this it means this rigblock had no paint info
					break;
				}
			}

			// Default Spore case: when no textures, just ignore
			if (!hasAdvancedPaint && (!skinpaintDiffuseTexture || !skinpaintTintMaskTexture))
			{
				continue;
			}
			// If we do have advanced paint, then replace empty textures
			// Some mods (Locked n Loaded) have some missing textures, compensate that
			if (!skinpaintDiffuseTexture)
			{
				skinpaintDiffuseTexture = TextureManager.GetTexture(ResourceKey(id("ACP_transparent"), TypeIDs::raster, GroupIDs::Global),
					Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);
			}
			if (!skinpaintTintMaskTexture)
			{
				skinpaintTintMaskTexture = TextureManager.GetTexture(ResourceKey(id("ACP_black"), TypeIDs::raster, GroupIDs::Global),
					Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);
			}

			Math::ColorRGBA baseColor{ Skinner::GetCurrentColors()[0], 0.0f };
			Math::ColorRGBA coatColor{ Skinner::GetCurrentColors()[1], 0.0f };
			Math::ColorRGBA detailColor{ Skinner::GetCurrentColors()[2], 0.0f };

			if (hasAdvancedPaint)
			{
				TexturePtr whiteTexture = TextureManager.GetTexture(ResourceKey(id("ACP_white"), TypeIDs::raster, GroupIDs::Global),
					Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);

				sExtendedRenderEnabled = true;
				sExtendedRasters[0] = skinpaintDiffuseTexture->GetLoadedRaster();
				sExtendedRasters[1] = skinpaintTintMaskTexture->GetLoadedRaster();
				for (int i = 0; i < AdvancedCreaturePaint::NUM_REGIONS; i++)
				{
					PropertyListPtr paintPropList;
					if (paintInfos[i] && PropManager.GetPropertyList(paintInfos[i]->paintID, GroupIDs::Paints, paintPropList))
					{
						// paintMaterialCol1BlendFactor and paintMaterialCol2BlendFactor
						float blendFactor1 = 0.5f, blendFactor2 = 0.5f;
						App::Property::GetFloat(paintPropList.get(), 0x025E49BB, blendFactor1);
						App::Property::GetFloat(paintPropList.get(), 0x0265FB07, blendFactor2);

						Math::ColorRGBA color1 = paintInfos[i]->color1;
						Math::ColorRGBA color2 = paintInfos[i]->color2;
						color1.a = blendFactor1;
						color2.a = blendFactor2;
						sExtendedCustomParams[i * 2 + 0] = color1;
						sExtendedCustomParams[i * 2 + 1] = color2;

						// paintMaterialDiffuse
						ResourceKey paintMaterialDiffuseID;
						if (App::Property::GetKey(paintPropList.get(), 0xB0E066A4, paintMaterialDiffuseID))
						{
							auto paintTexture = TextureManager.GetTexture(paintMaterialDiffuseID,
								Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);

							sExtendedRasters[i + 2] = paintTexture->GetLoadedRaster();
						}
						else
						{
							sExtendedRasters[i + 2] = whiteTexture->GetLoadedRaster();
						}
					}
					else
					{
						sExtendedRasters[i + 2] = whiteTexture->GetLoadedRaster();
						if (i < 3)
							sExtendedCustomParams[i * 2 + 0] = { Skinner::GetCurrentColors()[i], 1.0f };
						else
							sExtendedCustomParams[i * 2 + 0] = { 1.0f, 1.0f, 1.0f, 1.0f };
						sExtendedCustomParams[i * 2 + 1] = { 0.0f, 0.0f, 0.0f, 0.0f };
					}
					sExtendedCustomParams[10].r = paintInfos[AdvancedCreaturePaint::kBase] ? 1.0f : 0.0f;
					sExtendedCustomParams[10].g = paintInfos[AdvancedCreaturePaint::kIdentity] ? 1.0f : 0.0f;
					sExtendedCustomParams[10].b = paintInfos[AdvancedCreaturePaint::kTextured] ? 1.0f : 0.0f;
				}

				texture0->SetColorWriteEnable(true, true, true, false);
				texture0->mMaterialID = id("ACP_skpAdvancedSplatTint");
				texture0->PaintRegion(uv1, uv2);
				ResetExtendedRendering();
			}
			else
			{
				texture0->SetColorWriteEnable(true, true, true, false);
				// Uses 0x700000ad(skpSplatTintShader).shader
				texture0->mMaterialID = 0xD9EC39BC;  // skpSplatTint
				texture0->AddCustomParams(0, baseColor);
				texture0->AddCustomParams(1, coatColor);
				texture0->AddCustomParams(2, detailColor);
				texture0->AddRaster(0, skinpaintDiffuseTexture->GetLoadedRaster());
				texture0->AddRaster(1, skinpaintTintMaskTexture->GetLoadedRaster());
				texture0->PaintRegion(uv1, uv2);
			}
		}
		if (mStage == 1 && skinpaintTintMaskTexture)
		{
			texture0->SetColorWriteEnable(false, false, false, true);
			// Uses skpSplatCopyTexShader
			texture0->mMaterialID = 0xA91B6551;
			texture0->AddCustomParams(0, { 1.0f, 1.0f , 1.0f, 1.0f });
			texture0->AddRaster(0, skinpaintTintMaskTexture->GetLoadedRaster());
			texture0->mSrcRasters[1] = nullptr;
			texture0->PaintRegion(uv1, uv2);
		}
		if (mStage == 2 && skinpaintSpecBumpTexture)
		{
			float scale = PaintSystem.GetPainter()->mPartSpecScale;  // ?
			texture1->SetColorWriteEnable(true, false, false, false);
			// Uses skpBumpBrushShader
			texture1->mMaterialID = 0x3815422;
			texture1->AddCustomParams(0, { scale, scale, scale, 1.0f });
			texture1->AddRaster(0, skinpaintSpecBumpTexture->GetLoadedRaster());
			texture1->PaintRegion(uv1, uv2);
		}
		if (mStage == 3 && skinpaintSpecBumpTexture)
		{
			float scale = PaintSystem.GetPainter()->mPartBumpScale;  // ?
			texture1->SetColorWriteEnable(false, false, true, false);
			// Uses skpSplatCopyTexShader
			texture1->mMaterialID = 0x968E3FFF;
			texture1->AddCustomParams(0, { scale, scale, scale, 1.0f });
			texture1->AddRaster(0, skinpaintSpecBumpTexture->GetLoadedRaster());
			texture1->PaintRegion(uv1, uv2);
		}

		mRigblockIndex++;
	}

	if (mStage >= 2) {
		texture1->EndRender();
	}
	else {
		texture0->EndRender();
	}

	if (mRigblockIndex >= count) {
		mRigblockIndex = 0;
		mStage++;
	}
	return mStage == 4;
}


// BumpToNormal job does not have any fields. We want to do this
// to avoid the game from hanging up in creatures with lots of parts
// Note: I don't know if there can ever be two jobs at the same time,
// but let's use a map just in case
eastl::map<Skinner::cSkinPainterJobBumpToNormal*, int> BumpToNormalJob_mRigblockIndex;
bool BumpToNormalJob_mRigblockIndex_Initialized = false;
// I don't know if this is ever relevant

bool BumpToNormalJob_Execute__detour::detoured()
{
	original_function(this);
	//return ret;

	if (!BumpToNormalJob_mRigblockIndex_Initialized) 
	{
		BumpToNormalJob_mRigblockIndex = {};
		BumpToNormalJob_mRigblockIndex_Initialized = true;
	}

	int* mRigblockIndex;
	if (BumpToNormalJob_mRigblockIndex.find(this) == BumpToNormalJob_mRigblockIndex.end())
	{
		BumpToNormalJob_mRigblockIndex[this] = 0;
	}
	mRigblockIndex = &BumpToNormalJob_mRigblockIndex[this];

	// The paints from building/vehicle editor do not use bump mapping,
	// and use normal map instead. So we apply it after conversion.

	auto dstTexture = PaintSystem.GetPainter()->mpTexture2;
	auto skinMesh = PaintSystem.GetSkinMesh();

	dstTexture->StartRender();

	auto creatureData = (AdvancedCreatureDataResource*)skinMesh->mpCreatureData;
	int count = creatureData->mRigblocks.size();
	auto numPaintInfos = creatureData->GetNumPaintInfos();

	int lastPaintInfoIndex = 0;
	int rigblockLimitCount = 0;
	while (*mRigblockIndex < count && rigblockLimitCount < 10)
	{
		if (creatureData->mRigblocks[*mRigblockIndex].mFlags & Editors::cCreatureDataResource::kFlagNotUseSkin)
		{
			(*mRigblockIndex)++;
			continue;
		}

		auto& uvs = skinMesh->mUVs[*mRigblockIndex];
		Vector2 uv1{ uvs.x1, uvs.y1 };
		Vector2 uv2{ uvs.x2, uvs.y2 };
		auto propList = skinMesh->mRigblockPropLists[*mRigblockIndex].get();

		bool hasAdvancedPaint = false;
		const AdvancedCreatureDataResource::PaintInfo* paintInfos[AdvancedCreaturePaint::NUM_REGIONS] = {};

		// Find if there is any advanced paint on this rigblock
		for (; lastPaintInfoIndex < creatureData->GetNumPaintInfos(); lastPaintInfoIndex++)
		{
			const auto& paintInfo = creatureData->GetPaintInfos()[lastPaintInfoIndex];
			if (paintInfo.rigblockIndex == *mRigblockIndex)
			{
				paintInfos[paintInfo.paintRegion] = &paintInfo;
				hasAdvancedPaint = true;
			}
			else if (paintInfo.rigblockIndex > *mRigblockIndex)
			{
				// They are ordered by rigblock index, so if we reach this it means this rigblock had no paint info
				break;
			}
		}

		if (hasAdvancedPaint)
		{
			// There is no point in increasing this if there is no rendering to be done,
			// it would just make painting slower for no reason.
			rigblockLimitCount++;

			TexturePtr whiteTexture = TextureManager.GetTexture(ResourceKey(id("ACP_white"), TypeIDs::raster, GroupIDs::Global),
				Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);

			Graphics::Texture* skinpaintDiffuseTexture = nullptr;
			Graphics::Texture* skinpaintTintMaskTexture = nullptr;
			// skinpaintDiffuseTexture
			if (propList->HasProperty(0x2424655))
			{
				ResourceKey key{};
				App::Property::GetKey(propList, 0x2424655, key);
				if (TextureManager.HasTexture(key))
				{
					skinpaintDiffuseTexture = TextureManager.GetTexture(key,
						Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);
				}
			}
			// skinpaintTintMaskTexture
			if (propList->HasProperty(0x2424657))
			{
				ResourceKey key{};
				App::Property::GetKey(propList, 0x2424657, key);
				if (TextureManager.HasTexture(key))
				{
					skinpaintTintMaskTexture = TextureManager.GetTexture(key,
						Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);
				}
			}

			if (!skinpaintDiffuseTexture)
			{
				skinpaintDiffuseTexture = TextureManager.GetTexture(ResourceKey(id("ACP_transparent"), TypeIDs::raster, GroupIDs::Global),
					Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);
			}
			if (!skinpaintTintMaskTexture)
			{
				skinpaintTintMaskTexture = TextureManager.GetTexture(ResourceKey(id("ACP_black"), TypeIDs::raster, GroupIDs::Global),
					Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);
			}

			sExtendedRenderEnabled = true;

			sExtendedRasters[0] = PaintSystem.GetPainter()->mpTexture2->GetRaster();
			sExtendedRasters[1] = skinpaintDiffuseTexture->GetLoadedRaster();
			sExtendedRasters[2] = skinpaintTintMaskTexture->GetLoadedRaster();

			for (int i = 0; i < AdvancedCreaturePaint::NUM_REGIONS; i++)
			{
				PropertyListPtr paintPropList;
				if (paintInfos[i] && PropManager.GetPropertyList(paintInfos[i]->paintID, GroupIDs::Paints, paintPropList))
				{
					// paintMaterialNMapSpec
					ResourceKey paintMaterialNMapSpecID;
					if (App::Property::GetKey(paintPropList.get(), 0xB0E066A5, paintMaterialNMapSpecID))
					{
						auto paintTexture = TextureManager.GetTexture(paintMaterialNMapSpecID,
							Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);

						sExtendedRasters[i + 3] = paintTexture->GetLoadedRaster();
					}
					else
					{
						sExtendedRasters[i + 3] = whiteTexture->GetLoadedRaster();
					}
				}
			}

			sExtendedCustomParams[0].r = paintInfos[AdvancedCreaturePaint::kBase] ? 1.0f : 0.0f;
			sExtendedCustomParams[0].g = paintInfos[AdvancedCreaturePaint::kCoat] ? 1.0f : 0.0f;
			sExtendedCustomParams[0].b = paintInfos[AdvancedCreaturePaint::kDetail] ? 1.0f : 0.0f;
			sExtendedCustomParams[0].a = paintInfos[AdvancedCreaturePaint::kIdentity] ? 1.0f : 0.0f;
			sExtendedCustomParams[1].r = paintInfos[AdvancedCreaturePaint::kTextured] ? 1.0f : 0.0f;

			dstTexture->SetColorWriteEnable(true, true, true, true);
			dstTexture->mMaterialID = id("ACP_skpAdvancedSplatNormalSpec");
			dstTexture->PaintRegion(uv1, uv2);
			ResetExtendedRendering();
		}

		(*mRigblockIndex)++;
	}

	dstTexture->EndRender();

	return true;
}


RenderWare::CompiledState* cSkinnerTexturePainter_LoadMaterial__detour::detoured()
{
	auto compiledState = original_function(this);

	if (sExtendedRenderEnabled)
	{
		for (int i = 0; i < NUM_EXTENDED_RASTERS; i++)
		{
			if (sExtendedRasters[i])
			{
				compiledState->SetRaster(i, sExtendedRasters[i]);
			}
		}

		Graphics::ActiveState::SetShaderData(0x206, sExtendedCustomParams, true);

		sExtendedRenderEnabled = false;
	}

	return compiledState;
}
