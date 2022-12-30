#include "pch.h"
#include "LegionSplash.h"
#include "resource.h"

LegionSplash::LegionSplash()
	: Forms::Form()
{
	this->InitializeComponent();
}

void LegionSplash::OnPaint(const std::unique_ptr<Forms::PaintEventArgs>& EventArgs)
{
	Form::OnPaint(EventArgs);
	if (this->SplashImage != nullptr)
		EventArgs->Graphics->DrawImage(this->SplashImage.get(), this->ClientRectangle());
}

void LegionSplash::OnLoad()
{
	Form::OnLoad();

	SetLayeredWindowAttributes(this->GetHandle(), Drawing::ColorToWin32(Drawing::Color(7, 255, 0)), 0, LWA_COLORKEY);

	auto hRegion = CreateRoundRectRgn(0, 0, this->_ClientWidth, this->_ClientHeight, 9, 9);
	SetWindowRgn(this->GetHandle(), hRegion, TRUE);
	DeleteObject(hRegion);

	Threading::Thread(SleepTimer).Start(this);
}

Forms::CreateParams LegionSplash::GetCreateParams()
{
	auto Cp = Form::GetCreateParams();
	Cp.ExStyle = WS_EX_LAYERED;

	return Cp;
}

void LegionSplash::InitializeComponent()
{
	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText("Legion+ Loading...");
	this->SetClientSize({ 690, 388 });
	this->SetMinimumSize({ 690, 388 });
	this->SetFormBorderStyle(Forms::FormBorderStyle::None);
	this->SetTopMost(true);
	this->SetShowInTaskbar(false);
	this->SetStartPosition(Forms::FormStartPosition::CenterScreen);

	this->ResumeLayout(false);
	this->PerformLayout();
	// END DESIGNER CODE

	SplashImage.reset(Drawing::ImageFromResource(IDB_JPEG1));

	this->Refresh();
}

void LegionSplash::SleepTimer(void* Sender)
{
	Sleep(2500);
	// Close the splash
	((LegionSplash*)Sender)->Close();
}
