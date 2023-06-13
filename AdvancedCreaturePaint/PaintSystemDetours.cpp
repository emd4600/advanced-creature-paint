#include "stdafx.h"
#include "PaintSystemDetours.h"
#include "AdvancedCreatureDataResource.h"
#include "Constants.h"
//#include <d3d9.h>
//#include <d3dx9tex.h>
//
//void SaveTexture(RenderWare::Raster* raster, char16_t* path, bool isSurface)
//{
//	auto device = Graphics::RenderUtils::GetDevice();
//	HRESULT hr;
//	IDirect3DSurface9* surface;
//	if (isSurface) {
//		/*surface = raster->pSurface;
//		hr = D3D_OK;*/
//		hr = device->GetRenderTarget(0, &surface);
//	}
//	else {
//		//hr = device->GetRenderTarget(0, &surface);
//		hr = raster->pTexture->GetSurfaceLevel(0, &surface);
//	}
//
//	if (hr == D3D_OK) {
//		RECT rect;
//		rect.left = 0;
//		rect.top = 0;
//		rect.right = raster->width;
//		rect.bottom = raster->height;
//
//		hr = D3DXSaveSurfaceToFile((LPCWSTR)path, D3DXIMAGE_FILEFORMAT::D3DXIFF_PNG,
//			surface, NULL, &rect);
//
//		if (hr != D3D_OK) {
//			App::ConsolePrintF("Could not save surface to file: %d", hr);
//		}
//		else {
//			App::ConsolePrintF("Texture successfully saved to: %ls", path);
//		}
//	}
//	else {
//		App::ConsolePrintF("Could not get render surface: %d", hr);
//	}
//
//	if (isSurface) {
//		device->SetRenderTarget(0, surface);
//	}
//
//	//device->SetRenderTarget(0, surface);
//}

bool PaintPartsJob_Execute__detour::detoured()
{
	auto texture0 = PaintSystem.GetPainter()->mpTexture0;
	auto texture1 = PaintSystem.GetPainter()->mpTexture1;
	auto texture2 = PaintSystem.GetPainter()->mpTexture2;
	auto skinMesh = PaintSystem.GetSkinMesh();

	TexturePtr whiteTexture = nullptr;
	TexturePtr transparentTexture = nullptr;

	auto creatureData = (AdvancedCreatureDataResource*)skinMesh->mpCreatureData;
	int count = creatureData->mRigblocks.size();
	auto numPaintInfos = creatureData->GetNumPaintInfos();

	if (mStage == 1) mStage = 2;

	// Spore usually has 4 stages. We use an extra one to write the temporary advanced paints into the diffuse texture
	// This is necessary because apparently, we can only use StartRender()/EndRender() on one texture per call.
	// Doing it more than once causes the texture to become black
	// Stage 4: Generate temporary advanced paints to texture2
	// Stage 5: Blend temporary advanced paints from texture2 into texture0
	if (mStage >= 6)
	{
		return mStage == 6;
	}

	if (mStage == 4) {
		texture2->StartRender();
	}
	else if (mStage == 2 || mStage == 3) {
		texture1->StartRender();
	}
	else if (mStage == 0 || mStage == 1 || mStage == 5) {
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

		// Find if there is any advanced paint on this rigblock
		bool hasAdvancedPaint = false;
		const AdvancedCreatureDataResource::PaintInfo* paintInfos[AdvancedCreaturePaint::NUM_REGIONS] = {};
		for (; lastPaintInfoIndex < creatureData->GetNumPaintInfos(); lastPaintInfoIndex++)
		{
			const auto& paintInfo = creatureData->GetPaintInfos()[lastPaintInfoIndex];
			if (paintInfo.rigblockIndex == mRigblockIndex)
			{
				paintInfos[paintInfo.paint.region] = &paintInfo;
				hasAdvancedPaint = true;
			}
			else if (paintInfo.rigblockIndex > mRigblockIndex)
			{
				// They are ordered by rigblock index, so if we reach this it means this rigblock had no paint info
				break;
			}
		}

		// Nothing to do on the extra stages for standard painting
		if ((mStage == 4 || mStage == 5) && !hasAdvancedPaint) {
			mRigblockIndex++;
			continue;
		}
		
		// Nothing to do on the initial stage for advanced painting
		if (mStage == 0 && hasAdvancedPaint) {
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

		if (mStage == 0 || mStage == 4 || mStage == 5)
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
		if (mStage == 0 || mStage == 1 || mStage == 4)
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

		Math::ColorRGBA baseColor{ Skinner::GetCurrentColors()[0], 0.0f };
		Math::ColorRGBA coatColor{ Skinner::GetCurrentColors()[1], 0.0f };
		Math::ColorRGBA detailColor{ Skinner::GetCurrentColors()[2], 0.0f };

		if (mStage == 0)
		{
			// Default Spore case: when no textures, just ignore
			if (!hasAdvancedPaint && (!skinpaintDiffuseTexture || !skinpaintTintMaskTexture))
			{
				mRigblockIndex++;
				continue;
			}

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

		Math::ColorRGBA paintColor1[AdvancedCreaturePaint::NUM_REGIONS];
		Math::ColorRGBA paintColor2[AdvancedCreaturePaint::NUM_REGIONS];
		RenderWare::Raster* paintRasters[AdvancedCreaturePaint::NUM_REGIONS];

		if (mStage == 4 || mStage == 5) 
		{
			if (!whiteTexture)
				whiteTexture = TextureManager.GetTexture(ResourceKey(id("ACP_white"), TypeIDs::raster, GroupIDs::Global),
					Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);

			// Some modded parts don't have these textures, let's replace the to avoid crashes
			if (!skinpaintDiffuseTexture || !skinpaintTintMaskTexture) {
				if (!transparentTexture)
					transparentTexture = TextureManager.GetTexture(ResourceKey(id("ACP_black_transparent"), TypeIDs::raster, GroupIDs::Global),
						Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);
			}
			if (!skinpaintDiffuseTexture) {
				skinpaintDiffuseTexture = transparentTexture.get();
			}
			if (!skinpaintTintMaskTexture) {
				skinpaintTintMaskTexture = transparentTexture.get();
			}

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
					paintColor1[i] = color1;
					paintColor2[i] = color2;

					// paintMaterialDiffuse
					ResourceKey paintMaterialDiffuseID;
					if (App::Property::GetKey(paintPropList.get(), 0xB0E066A4, paintMaterialDiffuseID))
					{
						auto paintTexture = TextureManager.GetTexture(paintMaterialDiffuseID,
							Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);

						paintRasters[i] = paintTexture->GetLoadedRaster();
					}
					else
					{
						paintRasters[i] = whiteTexture->GetLoadedRaster();
					}
				}
				else
				{
					paintRasters[i] = whiteTexture->GetLoadedRaster();
					if (i < 3)
						paintColor1[i] = { Skinner::GetCurrentColors()[i], 1.0f };
					else
						paintColor1[i] = { 1.0f, 1.0f, 1.0f, 1.0f };
					paintColor2[i] = { 0.0f, 0.0f, 0.0f, 0.0f };
				}
			}
		}

		// Stage 4: Generate temporary advanced paints to texture2
		if (mStage == 4) 
		{
			auto diffuseRaster = skinpaintDiffuseTexture->GetLoadedRaster();
			auto tintMaskRaster = skinpaintTintMaskTexture->GetLoadedRaster();
			// Blend all colors in a separate texture, then blend the final result with the existing texture
			texture2->SetColorWriteEnable(true, true, true, false);

			// texture2 is not used yet, but SPSkinPaintSettingsEffect does not clear it, so it might have contents
			// It does not matter, as we just replace it here (we use no blend)
			if (paintInfos[AdvancedCreaturePaint::kTextured])
			{
				// Use the part's diffuse texture alpha as mask
				texture2->mMaterialID = id("ACP_skpAdvancedSplat_paintNoMaskNoBlend");
				texture2->AddRaster(0, paintRasters[AdvancedCreaturePaint::kTextured]);
				texture2->AddRaster(1, nullptr);
				texture2->AddCustomParams(0, paintColor1[AdvancedCreaturePaint::kTextured]);
				texture2->AddCustomParams(1, paintColor2[AdvancedCreaturePaint::kTextured]);
			}
			else
			{
				// Just paint the part diffuse texture as usual
				texture2->mMaterialID = id("ACP_skpAdvancedSplat_copyTexture");
				texture2->AddRaster(0, diffuseRaster);
				texture2->AddRaster(1, nullptr);
			}
			texture2->PaintRegion(uv1, uv2);

			texture2->mMaterialID = id("ACP_skpAdvancedSplat_paintByTintMask");
			texture2->AddRaster(0, paintRasters[AdvancedCreaturePaint::kBase]);
			texture2->AddRaster(1, tintMaskRaster);
			texture2->AddCustomParams(0, paintColor1[AdvancedCreaturePaint::kBase]);
			texture2->AddCustomParams(1, paintColor2[AdvancedCreaturePaint::kBase]);
			texture2->AddCustomParams(2, { 1.0f, 0.0f, 0.0f, 0.0f });
			texture2->PaintRegion(uv1, uv2);

			texture2->mMaterialID = id("ACP_skpAdvancedSplat_paintByTintMask");
			texture2->AddRaster(0, paintRasters[AdvancedCreaturePaint::kCoat]);
			texture2->AddRaster(1, tintMaskRaster);
			texture2->AddCustomParams(0, paintColor1[AdvancedCreaturePaint::kCoat]);
			texture2->AddCustomParams(1, paintColor2[AdvancedCreaturePaint::kCoat]);
			texture2->AddCustomParams(2, { 0.0f, 1.0f, 0.0f, 0.0f });
			texture2->PaintRegion(uv1, uv2);
			
			texture2->mMaterialID = id("ACP_skpAdvancedSplat_paintByTintMask");
			texture2->AddRaster(0, paintRasters[AdvancedCreaturePaint::kDetail]);
			texture2->AddRaster(1, tintMaskRaster);
			texture2->AddCustomParams(0, paintColor1[AdvancedCreaturePaint::kDetail]);
			texture2->AddCustomParams(1, paintColor2[AdvancedCreaturePaint::kDetail]);
			texture2->AddCustomParams(2, { 0.0f, 0.0f, 1.0f, 0.0f });
			texture2->PaintRegion(uv1, uv2);
			
			if (paintInfos[AdvancedCreaturePaint::kIdentity])
			{
				texture2->mMaterialID = id("ACP_skpAdvancedSplat_paintByTintMask");
				texture2->AddRaster(0, paintRasters[AdvancedCreaturePaint::kIdentity]);
				texture2->AddRaster(1, tintMaskRaster);
				texture2->AddCustomParams(0, paintColor1[AdvancedCreaturePaint::kIdentity]);
				texture2->AddCustomParams(1, paintColor2[AdvancedCreaturePaint::kIdentity]);
				texture2->AddCustomParams(2, { 0.0f, 0.0f, 0.0f, 1.0f });
				texture2->PaintRegion(uv1, uv2);
			}
		}

		// Stage 5: Blend temporary advanced paints from texture2 into texture0
		if (mStage == 5)
		{
			texture0->SetColorWriteEnable(true, true, true, false);

			// First we have to blend all the colors (stage 4), and then the final result with the existing texture
			// But if we use base, we replace the existing texture with the base texture
			if (paintInfos[AdvancedCreaturePaint::kBase])
			{
				texture0->mMaterialID = id("ACP_skpAdvancedSplat_paintNoMaskNoBlend");
				texture0->AddRaster(0, paintRasters[AdvancedCreaturePaint::kBase]);
				texture0->AddCustomParams(0, paintColor1[AdvancedCreaturePaint::kBase]);
				texture0->AddCustomParams(1, paintColor2[AdvancedCreaturePaint::kBase]);
				texture0->PaintRegion(uv1, uv2);
			}

			//SaveTexture(texture2->GetRaster(), u"C:\\Users\\Eric\\Desktop\\test_textures\\texture2_5.png", false);

			// Now that we have the total tinted part texture, blend it into the creature's texture
			texture0->mMaterialID = id("ACP_skpAdvancedSplat_blendTexture_specificPart");
			texture0->AddRaster(0, texture2->GetRaster());
			texture0->AddRaster(1, skinpaintDiffuseTexture->GetLoadedRaster());
			texture0->AddCustomParams(0, { 0.0f, 0.0f, 0.0f, 1.0f });
			texture0->PaintRegion(uv1, uv2);
		}

		mRigblockIndex++;
	}

	if (mStage == 4) {
		texture2->EndRender();

		// There is a bug in Spore's code. The next thing that uses texture2 is the ColorDilateRepeatJob,
			// but it does not call SetColorWriteEnable! So we restore it ourselves.
		texture2->SetColorWriteEnable(true, true, true, true);
	}
	else if (mStage == 2 || mStage == 3) {
		texture1->EndRender();
	}
	else if (mStage == 0 || mStage == 1 || mStage == 5) {
		texture0->EndRender();
	}

	if (mRigblockIndex >= count) {
		mRigblockIndex = 0;
		mStage++;
	}
	return mStage == 6;
}


// BumpToNormal job does not have any fields. We want to do this
// to avoid the game from hanging up in creatures with lots of parts
// Note: I don't know if there can ever be two jobs at the same time,
// but let's use a map just in case
eastl::map<Skinner::cSkinPainterJobBumpToNormal*, int> BumpToNormalJob_mRigblockIndex;
eastl::map<Skinner::cSkinPainterJobBumpToNormal*, int> BumpToNormalJob_mStage;
bool BumpToNormalJob_mRigblockIndex_Initialized = false;
// I don't know if this is ever relevant

bool BumpToNormalJob_Execute__detour::detoured()
{
	if (!BumpToNormalJob_mRigblockIndex_Initialized) 
	{
		BumpToNormalJob_mRigblockIndex = {};
		BumpToNormalJob_mStage = {};
		BumpToNormalJob_mRigblockIndex_Initialized = true;
	}

	int* mStage;
	int* mRigblockIndex;
	if (BumpToNormalJob_mRigblockIndex.find(this) == BumpToNormalJob_mRigblockIndex.end())
	{
		BumpToNormalJob_mRigblockIndex[this] = 0;
		BumpToNormalJob_mStage[this] = 0;
		// Only execute it once
		original_function(this);
	}
	mRigblockIndex = &BumpToNormalJob_mRigblockIndex[this];
	mStage = &BumpToNormalJob_mStage[this];

	// The paints from building/vehicle editor do not use bump mapping,
	// and use normal map instead. So we apply it after conversion.

	// Since we are restricted to two textures per shader, this is limited.
	// We decided to not support normalSpec maps for the 'textured' region,
	// as it is the only one that is not determined in the tintMask

	auto tempTexture = PaintSystem.GetPainter()->mpTexture1;
	auto dstTexture = PaintSystem.GetPainter()->mpTexture2;
	auto skinMesh = PaintSystem.GetSkinMesh();

	if (*mStage == 0) {
		tempTexture->StartRender();
	}
	else {
		dstTexture->StartRender();
	}

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
				paintInfos[paintInfo.paint.region] = &paintInfo;
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

			if (*mStage == 0)
			{
				TexturePtr whiteTexture = TextureManager.GetTexture(ResourceKey(id("ACP_white"), TypeIDs::raster, GroupIDs::Global),
					Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);

				Graphics::Texture* skinpaintTintMaskTexture = nullptr;
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

				if (!skinpaintTintMaskTexture)
				{
					skinpaintTintMaskTexture = TextureManager.GetTexture(ResourceKey(id("ACP_black_transparent"), TypeIDs::raster, GroupIDs::Global),
						Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);
				}

				RenderWare::Raster* paintRasters[AdvancedCreaturePaint::NUM_REGIONS];
				float normalStrengths[AdvancedCreaturePaint::NUM_REGIONS];
				float specStrengths[AdvancedCreaturePaint::NUM_REGIONS];
				float strongestNormal = 0.0f;
				float strongestSpec = 0.0f;

				for (int i = 0; i < AdvancedCreaturePaint::NUM_REGIONS; i++)
				{
					paintRasters[i] = nullptr;
					normalStrengths[i] = 0.0f;
					specStrengths[i] = 0.0f;

					PropertyListPtr paintPropList;
					if (paintInfos[i] && PropManager.GetPropertyList(paintInfos[i]->paintID, GroupIDs::Paints, paintPropList))
					{
						// paintMaterialBumpiness
						if (!App::Property::GetFloat(paintPropList.get(), 0x0562a106, normalStrengths[i]))
						{
							// paintMaterialInvBumpiness
							if (App::Property::GetFloat(paintPropList.get(), 0x02327a87, normalStrengths[i]))
							{
								normalStrengths[i] = 1.0f / normalStrengths[i];
							}
						}

						// paintMaterialSpecStrength
						App::Property::GetFloat(paintPropList.get(), 0x0562a3b7, specStrengths[i]);

						// paintMaterialNMapSpec
						ResourceKey paintMaterialNMapSpecID;
						if (App::Property::GetKey(paintPropList.get(), 0xB0E066A5, paintMaterialNMapSpecID))
						{
							auto paintTexture = TextureManager.GetTexture(paintMaterialNMapSpecID,
								Graphics::kTextureFlagForceLoad | Graphics::kTextureFlagSetLOD);

							paintRasters[i] = paintTexture->GetLoadedRaster();
						}
						else
						{
							normalStrengths[i] = 0.0f;
							specStrengths[i] = 0.0f;
							paintRasters[i] = whiteTexture->GetLoadedRaster();
						}

						if (normalStrengths[i] > strongestNormal) {
							strongestNormal = normalStrengths[i];
						}
						if (specStrengths[i] > strongestSpec) {
							strongestSpec = specStrengths[i];
						}
					}
				}

				// Normalize the strengths so the strongest one is 1.0
				// If there is no bumpiness at all, distribute it evenly
				for (int i = 0; i < AdvancedCreaturePaint::NUM_REGIONS; i++)
				{
					if (strongestNormal > 0.0f) normalStrengths[i] /= strongestNormal;
					if (strongestSpec > 0.0f) specStrengths[i] /= strongestSpec;
				}

				Math::ColorRGBA customParamsNormal = {
					normalStrengths[AdvancedCreaturePaint::kBase],
					normalStrengths[AdvancedCreaturePaint::kCoat],
					normalStrengths[AdvancedCreaturePaint::kDetail],
					normalStrengths[AdvancedCreaturePaint::kIdentity]
				};
				Math::ColorRGBA customParamsSpec = {
					specStrengths[AdvancedCreaturePaint::kBase],
					specStrengths[AdvancedCreaturePaint::kCoat],
					specStrengths[AdvancedCreaturePaint::kDetail],
					specStrengths[AdvancedCreaturePaint::kIdentity]
				};
				Math::ColorRGBA customParamsApply = {
					paintInfos[AdvancedCreaturePaint::kIdentity] ? 1.0f : 0.0f,
					0.0f,
					0.0f,
					0.0f
				};

				tempTexture->SetColorWriteEnable(true, true, true, true);

				tempTexture->mMaterialID = id("ACP_skpAdvancedSplatNormalSpec_preBlend");
				if (paintInfos[AdvancedCreaturePaint::kBase]) {
					tempTexture->AddRaster(0, paintRasters[AdvancedCreaturePaint::kBase]);
				}
				else {
					tempTexture->AddRaster(0, dstTexture->GetRaster());
				}
				tempTexture->AddRaster(1, skinpaintTintMaskTexture->GetRaster());
				tempTexture->AddCustomParams(0, customParamsNormal);
				tempTexture->AddCustomParams(1, customParamsSpec);
				tempTexture->AddCustomParams(2, customParamsApply);
				tempTexture->PaintRegion(uv1, uv2);

				if (paintInfos[AdvancedCreaturePaint::kBase]) 
				{
					tempTexture->mMaterialID = id("ACP_skpAdvancedSplatNormalSpec_blendR");
					tempTexture->AddRaster(0, paintRasters[AdvancedCreaturePaint::kBase]);
					tempTexture->AddRaster(1, skinpaintTintMaskTexture->GetRaster());
					tempTexture->AddCustomParams(0, customParamsNormal);
					tempTexture->AddCustomParams(1, customParamsSpec);
					tempTexture->AddCustomParams(2, customParamsApply);
					tempTexture->PaintRegion(uv1, uv2);
				}
				if (paintInfos[AdvancedCreaturePaint::kCoat])
				{
					tempTexture->mMaterialID = id("ACP_skpAdvancedSplatNormalSpec_blendG");
					tempTexture->AddRaster(0, paintRasters[AdvancedCreaturePaint::kCoat]);
					tempTexture->AddRaster(1, skinpaintTintMaskTexture->GetRaster());
					tempTexture->AddCustomParams(0, customParamsNormal);
					tempTexture->AddCustomParams(1, customParamsSpec);
					tempTexture->AddCustomParams(2, customParamsApply);
					tempTexture->PaintRegion(uv1, uv2);
				}
				if (paintInfos[AdvancedCreaturePaint::kDetail])
				{
					tempTexture->mMaterialID = id("ACP_skpAdvancedSplatNormalSpec_blendB");
					tempTexture->AddRaster(0, paintRasters[AdvancedCreaturePaint::kDetail]);
					tempTexture->AddRaster(1, skinpaintTintMaskTexture->GetRaster());
					tempTexture->AddCustomParams(0, customParamsNormal);
					tempTexture->AddCustomParams(1, customParamsSpec);
					tempTexture->AddCustomParams(2, customParamsApply);
					tempTexture->PaintRegion(uv1, uv2);
				}
				if (paintInfos[AdvancedCreaturePaint::kIdentity])
				{
					tempTexture->mMaterialID = id("ACP_skpAdvancedSplatNormalSpec_blendA");
					tempTexture->AddRaster(0, paintRasters[AdvancedCreaturePaint::kIdentity]);
					tempTexture->AddRaster(1, skinpaintTintMaskTexture->GetRaster());
					tempTexture->AddCustomParams(0, customParamsNormal);
					tempTexture->AddCustomParams(1, customParamsSpec);
					tempTexture->AddCustomParams(2, customParamsApply);
					tempTexture->PaintRegion(uv1, uv2);
				}
			}
			else if (*mStage == 1)
			{
				// Copy from the temporary texture to the final texture
				dstTexture->mMaterialID = id("ACP_skpAdvancedSplat_copyTexture_specificPart");
				dstTexture->AddRaster(0, tempTexture->GetRaster());
				dstTexture->AddRaster(1, nullptr);
				dstTexture->PaintRegion(uv1, uv2);
			}
		}

		(*mRigblockIndex)++;
	}

	if (*mStage == 0) {
		tempTexture->EndRender();
	}
	else {
		dstTexture->EndRender();
	}

	if (*mRigblockIndex >= count) {
		*mRigblockIndex = 0;
		(*mStage)++;
	}
	if (*mStage == 2) {
		BumpToNormalJob_mRigblockIndex.erase(this);
		BumpToNormalJob_mStage.erase(this);
	}
	return *mStage == 2;
}
