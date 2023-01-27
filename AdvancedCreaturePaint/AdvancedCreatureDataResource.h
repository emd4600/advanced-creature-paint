#pragma once

#include <Spore\Editors\cCreatureDataResource.h>
#include <Spore\Editors\EditorRigblock.h>

// We never instantiate this class, it is only to define some convenience methods
class AdvancedCreatureDataResource
	: public Editors::cCreatureDataResource
{
public:
	static const int FORMAT_VERSION = 1;

	// We inject this structure as if it was floats in the `mAnimationWeights` vector
	// An alternative was adding a new field, but this would have required modifying all functions that
	// create a cCreatureDataResource, which is unfeasible
	struct PaintInfo
	{
		int16_t rigblockIndex;
		int16_t paintRegion;
		uint32_t paintID;
		Math::Color color1;
		Math::Color color2;

		void FromRigblockPaint(const Editors::EditorRigblockPaint& paint);
	};

	void AddFromRigblock(Editors::EditorRigblock* rigblock, int rigblockIndex);

	int GetNumPaintInfos();

	// Returns the array of paint information. The array is ordered by rigblockIndex but there is no
	// fixed number of items per rigblock.
	const PaintInfo* GetPaintInfos();

	// Adds the given paint info in the correct position of the array;
	// if there already is an existing paint info with the same rigblockIndex and paintRegion, it replaces it.
	void SetPaintInfo(const PaintInfo& info);

	// Returns true if it was removed, false if it didn't exist
	bool RemovePaintInfo(int16_t rigblockIndex, int16_t paintRegion);

	void ClearPaintInfos();

	static void AttachDetours();
};

