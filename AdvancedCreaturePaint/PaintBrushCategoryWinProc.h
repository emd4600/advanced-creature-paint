#pragma once

#include <Spore\BasicIncludes.h>

#define PaintBrushCategoryWinProcPtr eastl::intrusive_ptr<PaintBrushCategoryWinProc>

class PaintBrushCategoryWinProc 
	: public UTFWin::IWinProc
	, public DefaultRefCounted
{
public:
	static const uint32_t TYPE = id("ACP_PaintBrushCategoryWinProc");

	enum Regions
	{
		kRegionNone = 0,
		kRegionBase = 1 << 0,
		kRegionCoat = 1 << 1,
		kRegionDetail = 1 << 2,
		kRegionIdentity = 1 << 3,
		kRegionTextured = 1 << 4,
		kRegionAll = kRegionBase | kRegionCoat | kRegionDetail | kRegionIdentity | kRegionTextured
	};
	enum class Mode
	{
		Paint,
		RemovePaint
	};
	
	PaintBrushCategoryWinProc();
	~PaintBrushCategoryWinProc();

	int AddRef() override;
	int Release() override;
	void* Cast(uint32_t type) const override;
	
	int GetEventFlags() const override;
	// This is the function you have to implement, called when a window you added this winproc to received an event
	bool HandleUIMessage(UTFWin::IWindow* pWindow, const UTFWin::Message& message) override;
	
	void AddToCategoryUI(Palettes::PaletteCategoryUI* categoryUI);

	bool RegionIsSelected(Regions region) const;

	Mode GetActiveMode() const;

private:
	UILayoutPtr mLayout;
	IWindowPtr mButtonRemovePaint;
	IWindowPtr mButtonBase;
	IWindowPtr mButtonCoat;
	IWindowPtr mButtonDetail;
	IWindowPtr mButtonTextured;
	IWindowPtr mButtonIdentity;
	eastl::set<UTFWin::IWindow*> mRegionButtons;
	int mSelectedRegions;
	bool mIsSelectingMultipleRegions;
	Clock mDoubleClickTimer;
	IWindowPtr mLastClickedButton;

	Regions GetRegionForButton(UTFWin::IWindow*);
};
