#include "stdafx.h"
#include "AdvancedCreatureDataResource.h"


unsigned int PAINT_INFO_SIZE = sizeof(AdvancedCreatureDataResource::PaintInfo);
unsigned int PAINT_INFO_NUM_WORDS = PAINT_INFO_SIZE / 4;


void AdvancedCreatureDataResource::PaintInfo::FromRigblockPaint(const Editors::EditorRigblockPaint& paint)
{
	paintID = paint.mPaintID;
	color1 = paint.mPaintColor1.ToIntColor();
	color2 = paint.mPaintColor2.ToIntColor();
}

void AdvancedCreatureDataResource::ClearPaintInfos()
{
	unsigned int numAnimations = mAnimationValues.size();
	mAnimationWeights.resize(numAnimations);
}

void AdvancedCreatureDataResource::AddFromRigblock(Editors::EditorRigblock* rigblock, int rigblockIndex)
{
	for (auto& paint : rigblock->mPaints)
	{
		PaintInfo info;
		info.rigblockIndex = rigblockIndex;
		info.paint.reserved = 0;
		info.paint.region = paint.first;
		info.FromRigblockPaint(paint.second);

		// Unfortunately, we cannot assume this function will be called in order of 'rigblockIndex',
		// as FromRigblocks() reorders the parts
		/*int previousSize = mAnimationWeights.size();
		mAnimationWeights.resize(previousSize + PAINT_INFO_NUM_WORDS);
		PaintInfo* dstData = (PaintInfo*)(mAnimationWeights.begin() + previousSize);
		*dstData = info;*/

		SetPaintInfo(info);
	}
}

int AdvancedCreatureDataResource::GetNumPaintInfos()
{
	unsigned int numAnimations = mAnimationValues.size();
	return (mAnimationWeights.size() - numAnimations) / PAINT_INFO_NUM_WORDS;
}

const AdvancedCreatureDataResource::PaintInfo* AdvancedCreatureDataResource::GetPaintInfos()
{
	unsigned int numAnimations = mAnimationValues.size();
	return (const PaintInfo*)(mAnimationWeights.data() + numAnimations);
}

void AdvancedCreatureDataResource::SetPaintInfo(const PaintInfo& info)
{
	auto infos = (PaintInfo*)(mAnimationWeights.data() + mAnimationValues.size());
	int count = GetNumPaintInfos();

#ifdef _DEBUG
	eastl::vector<PaintInfo> debugView;
	for (int i = 0; i < count; ++i) debugView.push_back(infos[i]);
#endif

	for (int i = 0; i < count; i++)
	{
		if (infos[i].rigblockIndex > info.rigblockIndex ||
			(infos[i].rigblockIndex == info.rigblockIndex && infos[i].paint.region > info.paint.region))
		{
			// The new info must be inserted right here.
			int previousSize = mAnimationWeights.size();
			mAnimationWeights.resize(previousSize + PAINT_INFO_NUM_WORDS);
			infos = (PaintInfo*)(mAnimationWeights.data() + mAnimationValues.size());
			memcpy(infos + i  + 1, infos + i, (count - i) * PAINT_INFO_SIZE);
			infos[i] = info;
			return;
		}
		else if (infos[i].rigblockIndex == info.rigblockIndex && infos[i].paint.region == info.paint.region)
		{
			infos[i] = info;
			return;
		}
	}
	mAnimationWeights.resize(mAnimationWeights.size() + PAINT_INFO_NUM_WORDS);
	infos = (PaintInfo*)(mAnimationWeights.data() + mAnimationValues.size());
	infos[count] = info;
}

bool AdvancedCreatureDataResource::RemovePaintInfo(int rigblockIndex, int paintRegion)
{
	auto numAnimations = mAnimationValues.size();
	auto infos = (PaintInfo*)(mAnimationWeights.data() + numAnimations);
	int count = GetNumPaintInfos();

	for (int i = 0; i < count; i++)
	{
		if (infos[i].rigblockIndex > rigblockIndex ||
			(infos[i].rigblockIndex == rigblockIndex && infos[i].paint.region > paintRegion))
		{
			// Since they are ordered, this means the paint info did not exist
			return false;
		}
		else if (infos[i].rigblockIndex == rigblockIndex && infos[i].paint.region == paintRegion)
		{
			auto eraseFirst = mAnimationWeights.begin() + numAnimations + i * PAINT_INFO_NUM_WORDS;
			auto eraseLast = mAnimationWeights.begin() + numAnimations + (i + 1) * PAINT_INFO_NUM_WORDS;
			mAnimationWeights.erase(eraseFirst, eraseLast);
			return true;
		}
	}
	return false;
}

static_detour(cCreatureDataResource_Read__detour, bool(IO::IStream* stream, Editors::cCreatureDataResource*))
{
	bool detoured(IO::IStream* stream, Editors::cCreatureDataResource* creatureData)
	{
		bool result = original_function(stream, creatureData);
		if (!result) return false;

		// We want modified files to still be readable without the mod, as otherwise it might give errors such as "Corrupted save files"
		// So we store the additional data after the original file data
		if (stream->GetAvailable() >= 8)
		{
			int version;
			if (!IO::ReadInt32(stream, &version, 1, IO::Endian::Little))
				return false;

			if (version > AdvancedCreatureDataResource::MAX_FORMAT_VERSION)
				return false;

			unsigned int numPaintInfos;
			if (!IO::ReadUInt32(stream, &numPaintInfos, 1, IO::Endian::Little)) 
				return false;

			if (numPaintInfos > 0)
			{
				unsigned int numAnimations = creatureData->mAnimationValues.size();
				creatureData->mAnimationWeights.resize(numAnimations + numPaintInfos * PAINT_INFO_NUM_WORDS);
				if (version == 1)
				{
					auto dstData = (AdvancedCreatureDataResource::PaintInfo*)(creatureData->mAnimationWeights.data() + numAnimations);
					for (unsigned int i = 0; i < numAnimations; i++)
					{
						dstData[i].rigblockIndex = 0;
						uint16_t region;
						IO::ReadUInt16(stream, (uint16_t*)&dstData[i].rigblockIndex, 1, IO::Endian::Little);
						IO::ReadUInt16(stream, &region, 1, IO::Endian::Little);

						dstData[i].paint.reserved = 0;
						dstData[i].paint.region = region;

						IO::ReadUInt32(stream, &dstData[i].paintID, 1, IO::Endian::Little);

						stream->Read(&dstData[i].color1, 4);
						stream->Read(&dstData[i].color2, 4);
					}
				}
				else if (version == 2)
				{
					float* dstData = creatureData->mAnimationWeights.data() + numAnimations;
					int numBytes = numPaintInfos * PAINT_INFO_SIZE;
					if (stream->Read(dstData, numBytes) != numBytes)
						return false;
				}
			}
		}
		return true;
	}
};

static_detour(cCreatureDataResource_Write__detour, bool(IO::IStream* stream, Editors::cCreatureDataResource*))
{
	bool detoured(IO::IStream* stream, Editors::cCreatureDataResource* creatureData)
	{
		bool result = original_function(stream, creatureData);
		if (!result) return false;

		// The Write function uses mAnimationValues to calculate how many animations it writes
		// We write the injected data (which is in mAnimationWeights) at the end of the file
		unsigned int numAnimations = creatureData->mAnimationValues.size();
		unsigned int numPaintInfos = (creatureData->mAnimationWeights.size() - numAnimations) / PAINT_INFO_NUM_WORDS;

		// If the mod is not used, just keep the original file intact
		if (numPaintInfos > 0)
		{
			if (!stream->Write(&AdvancedCreatureDataResource::MAX_FORMAT_VERSION, sizeof(int)))
				return false;

			if (!stream->Write(&numPaintInfos, sizeof(unsigned int)))
				return false;

			if (!stream->Write(creatureData->mAnimationWeights.data() + numAnimations, numPaintInfos * PAINT_INFO_SIZE))
				return false;
		}

		return true;
	}
};

member_detour(
	cEditorSkinMeshBase_FromRigblocks__detour,
	Editors::cEditorSkinMeshBase,
	bool(Editors::EditorRigblock**, int, eastl::vector<Editors::EditorRigblock*>*))
{
	bool detoured(Editors::EditorRigblock** rigblocks, int count, eastl::vector<Editors::EditorRigblock*>*dst)
	{
		bool result = original_function(this, rigblocks, count, dst);
		if (!result) return false;

		int rigblockPaintCount = 0;
		for (int i = 0; i < count; i++) {
			if (rigblocks[i]->mPaints.size() > 0) rigblockPaintCount++;
		}

		auto creatureData = (AdvancedCreatureDataResource*)this->mpCreatureData;
		for (int i = 0; i < count; ++i)
		{
			// The index is not 'i', since they are stored in a different order in the creature data
			// (that same order is reflected on dst)
			creatureData->AddFromRigblock(rigblocks[i], rigblocks[i]->field_DD0.mIndex);
			//creatureData->AddFromRigblock(rigblocks[i], i);
		}

		return true;
	}
};

void AdvancedCreatureDataResource::AttachDetours()
{
	cCreatureDataResource_Read__detour::attach(GetAddress(Editors::cCreatureDataResource, Read));
	cCreatureDataResource_Write__detour::attach(GetAddress(Editors::cCreatureDataResource, Write));
	cEditorSkinMeshBase_FromRigblocks__detour::attach(GetAddress(Editors::cEditorSkinMeshBase, FromRigblocks));
}
