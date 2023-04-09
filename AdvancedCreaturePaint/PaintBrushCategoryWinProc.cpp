#include "stdafx.h"
#include "PaintBrushCategoryWinProc.h"

PaintBrushCategoryWinProc::PaintBrushCategoryWinProc()
	: mLayout()
	, mButtonRemovePaint()
	, mButtonBase()
	, mButtonCoat()
	, mButtonDetail()
	, mButtonTextured()
	, mButtonIdentity()
	, mSelectedRegions(0)
	, mIsSelectingMultipleRegions(false)
	, mDoubleClickTimer(Clock::Mode::Milliseconds)
	, mRegionButtons()
	, mLastClickedButton()
{
}


PaintBrushCategoryWinProc::~PaintBrushCategoryWinProc()
{
}

// For internal use, do not modify.
int PaintBrushCategoryWinProc::AddRef()
{
	return DefaultRefCounted::AddRef();
}

// For internal use, do not modify.
int PaintBrushCategoryWinProc::Release()
{
	return DefaultRefCounted::Release();
}

// You can extend this function to return any other types your class implements.
void* PaintBrushCategoryWinProc::Cast(uint32_t type) const
{
	CLASS_CAST(Object);
	CLASS_CAST(IWinProc);
	CLASS_CAST(PaintBrushCategoryWinProc);
	return nullptr;
}

// This method returns a combinations of values in UTFWin::EventFlags.
// The combination determines what types of events (messages) this window procedure receives.
// By default, it receives mouse/keyboard input and advanced messages.
int PaintBrushCategoryWinProc::GetEventFlags() const
{
	return UTFWin::kEventFlagBasicInput | UTFWin::kEventFlagAdvanced;
}

// The method that receives the message. The first thing you should do is probably
// checking what kind of message was sent...
bool PaintBrushCategoryWinProc::HandleUIMessage(UTFWin::IWindow* window, const UTFWin::Message& message)
{
	if (message.IsType(UTFWin::kMsgMouseDown))
	{
		mIsSelectingMultipleRegions = false;
		if (mRegionButtons.find(window) != mRegionButtons.end())
		{
			mIsSelectingMultipleRegions = (message.Mouse.mouseState & MouseStateFlags::kMouseCtrlDown) != 0;
		}
	}
	else if (message.IsType(UTFWin::kMsgComponentActivated))
	{
		if (mRegionButtons.find(window) != mRegionButtons.end())
		{
			// Check if it was double click
			bool doubleClicked = false;
			if (mDoubleClickTimer.IsRunning())
			{
				float elapsedMS = mDoubleClickTimer.GetElapsed();
				if (elapsedMS < 500.0f && window == mLastClickedButton)
				{
					doubleClicked = true;
				}
			}
			// Start the clock, this could be the first click of a double click
			mDoubleClickTimer.Reset();
			mDoubleClickTimer.Start();
			mLastClickedButton = window;

			if (doubleClicked)
			{
				for (auto btn : mRegionButtons)
				{
					object_cast<UTFWin::IButton>(btn)->SetButtonStateFlag(UTFWin::kBtnStateSelected, true);
				}
				mSelectedRegions = kRegionAll;
			}
			else
			{
				auto button = object_cast<UTFWin::IButton>(window);
				bool isSelected = (button->GetButtonStateFlags() & UTFWin::kBtnStateSelected) != 0;
				bool wasSelected = !isSelected;

				int region = (int)GetRegionForButton(window);

				if (mIsSelectingMultipleRegions)
				{
					if (wasSelected)
					{
						int newRegionFlags = mSelectedRegions & ~region;
						if (newRegionFlags == 0)
						{
							// We can only deselect if there are others selected
							button->SetButtonStateFlag(UTFWin::kBtnStateSelected, true);
						}
						else
						{
							button->SetButtonStateFlag(UTFWin::kBtnStateSelected, false);
							mSelectedRegions = newRegionFlags;
						}
					}
					else
					{
						mSelectedRegions |= region;
					}
				}
				else
				{
					int otherRegionFlags = mSelectedRegions & ~region;
					// Deselect everything except this button if it wasn't selected, or if it was part of a multiple selection
					if (!wasSelected || otherRegionFlags != 0)
					{
						// Deselect everything else, we do them all as we might come from deselect
						for (auto btn : mRegionButtons)
						{
							object_cast<UTFWin::IButton>(btn)->SetButtonStateFlag(UTFWin::kBtnStateSelected, false);
						}

						button->SetButtonStateFlag(UTFWin::kBtnStateSelected, true);
						mSelectedRegions = region;
					}
					else
					{
						// We always must have at least one selected
						button->SetButtonStateFlag(UTFWin::kBtnStateSelected, true);
					}
				}
			}
		}
	}

	// Return true if the message was handled, and therefore no other window procedure should receive it.
	return false;
}

void PaintBrushCategoryWinProc::AddToCategoryUI(Palettes::PaletteCategoryUI* categoryUI)
{
	Math::Rectangle originalColorPickersArea = categoryUI->mpColorPickersPanel->GetArea();
	Math::Rectangle area = categoryUI->mpColorPickersPanel->GetRealArea();

	const char* layoutName;
	if (area.GetWidth() >= 340.0f) {
		layoutName = "ACP_editorPaintBrushPaletteCategory";
	}
	else {
		layoutName = "ACP_editorPaintBrushPaletteCategory_small";
	}
	mLayout = new UTFWin::UILayout();
	mLayout->LoadByID(id(layoutName));

	auto acpContainer = mLayout->FindWindowByID(id("ACPButtonsContainer"));
	float containerHeight = acpContainer->GetArea().GetHeight();

	SporeDebugPrint("WIDTH: %f", area.GetWidth());
	// Let's move the color picker down
	area.y1 += containerHeight;
	area.y2 += containerHeight;
	categoryUI->mpColorPickersPanel->SetLayoutArea(area);

	// Let's shrink the page panel
	area = categoryUI->mpPagePanel->GetRealArea();
	area.y1 += containerHeight;
	categoryUI->mpPagePanel->SetLayoutArea(area);
	mLayout->SetParentWindow(categoryUI->mpPageFrame.get());

	// Place it in same spot as color picker, but use our custom height
	originalColorPickersArea.y2 = acpContainer->GetArea().y2;
	acpContainer->SetArea(originalColorPickersArea);

	mButtonBase = acpContainer->FindWindowByID(id("ButtonBase"));
	mButtonCoat = acpContainer->FindWindowByID(id("ButtonCoat"));
	mButtonDetail = acpContainer->FindWindowByID(id("ButtonDetail"));
	mButtonIdentity = acpContainer->FindWindowByID(id("ButtonIdentity"));
	mButtonTextured = acpContainer->FindWindowByID(id("ButtonTextured"));
	mButtonRemovePaint = acpContainer->FindWindowByID(id("ButtonRemovePaint"));

	mSelectedRegions = kRegionBase;

	mButtonBase->AddWinProc(this);
	mButtonCoat->AddWinProc(this);
	mButtonDetail->AddWinProc(this);
	mButtonIdentity->AddWinProc(this);
	mButtonTextured->AddWinProc(this);
	mButtonRemovePaint->AddWinProc(this);

	mRegionButtons.insert(mButtonBase.get());
	mRegionButtons.insert(mButtonCoat.get());
	mRegionButtons.insert(mButtonDetail.get());
	mRegionButtons.insert(mButtonIdentity.get());
	mRegionButtons.insert(mButtonTextured.get());
}

bool PaintBrushCategoryWinProc::RegionIsSelected(Regions region) const
{
	return (mSelectedRegions & region) != 0;
}

PaintBrushCategoryWinProc::Mode PaintBrushCategoryWinProc::GetActiveMode() const
{
	if (object_cast<UTFWin::IButton>(mButtonRemovePaint)->GetButtonStateFlags() & UTFWin::kBtnStateSelected)
	{
		return Mode::RemovePaint;
	}
	else
	{
		return Mode::Paint;
	}
}

PaintBrushCategoryWinProc::Regions PaintBrushCategoryWinProc::GetRegionForButton(UTFWin::IWindow* window)
{
	if (window == mButtonBase.get()) return kRegionBase;
	if (window == mButtonCoat.get()) return kRegionCoat;
	if (window == mButtonDetail.get()) return kRegionDetail;
	if (window == mButtonIdentity.get()) return kRegionIdentity;
	if (window == mButtonTextured.get()) return kRegionTextured;
	return kRegionNone;
}
