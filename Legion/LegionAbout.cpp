#include "LegionAbout.h"

LegionAbout::LegionAbout() : Forms::Form()
{
	this->InitializeComponent();
}

void LegionAbout::InitializeComponent()
{
	const INT WindowX = 400;
	const INT WindowY = 150;

	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetText("Legion | About");
	this->SetClientSize({ WindowX, WindowY });
	this->SetFormBorderStyle(Forms::FormBorderStyle::FixedSingle);
	this->SetStartPosition(Forms::FormStartPosition::CenterParent);
	this->SetMinimizeBox(false);
	this->SetMaximizeBox(false);

	// add ui elements here

	//
	//	aboutGroupBox
	//
	this->aboutGroupBox = new UIX::UIXGroupBox();
	this->aboutGroupBox->SetSize({ WindowX-20, WindowY-20 });
	this->aboutGroupBox->SetLocation({ 12, 12 });
	this->aboutGroupBox->SetText("About");
	this->aboutGroupBox->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->AddControl(this->aboutGroupBox);

	//
	//	aboutTextLabel
	//
	this->aboutTextLabel = new UIX::UIXLabel();
	this->aboutTextLabel->SetSize({ WindowX-45, 60 });
	this->aboutTextLabel->SetLocation({ 15, 20 });
	this->aboutTextLabel->SetTabIndex(9);
	this->aboutTextLabel->SetText("Legion is the Apex Legends asset extraction tool. Originally created by DTZxPorter in 2019. Currently maintained by various contributors on GitHub.");
	this->aboutTextLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->aboutTextLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->aboutGroupBox->AddControl(this->aboutTextLabel);


	this->ResumeLayout(false);
	this->PerformLayout();
	// END DESIGNER CODE

	this->SetBackColor({ 30, 32, 55 });
}