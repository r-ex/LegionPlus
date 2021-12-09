#pragma once

#include <Kore.h>
#include "UIXButton.h"
#include "UIXLabel.h"
#include "UIXCheckBox.h"
#include "UIXComboBox.h"
#include "UIXTextBox.h"
#include "UIXGroupBox.h"
#include "UIXRadioButton.h"

class LegionSettings : public Forms::Form
{
public:
	LegionSettings();
	virtual ~LegionSettings() = default;

private:
	// Internal routine to setup the component
	void InitializeComponent();

	// Loads settings from the application config
	void LoadSettings();

	static void OnLoad(Forms::Control* Sender);
	static void OnClose(const std::unique_ptr<FormClosingEventArgs>& EventArgs, Forms::Control* Sender);
	static void OnDonateClick(Forms::Control* Sender);
	static void OnTwitterClick(Forms::Control* Sender);
	static void OnBrowseClick(Forms::Control* Sender);

	// Internal controls reference
	UIX::UIXGroupBox* groupBox5;
	UIX::UIXRadioButton* ExportCastAnim;
	UIX::UIXLabel* label4;
	UIX::UIXRadioButton* ExportSEAnim;
	UIX::UIXGroupBox* groupBox4;
	UIX::UIXTextBox* ExportBrowseFolder;
	UIX::UIXButton* ExportBrowseButton;
	UIX::UIXCheckBox* LoadMaterials;
	UIX::UIXCheckBox* LoadImages;
	UIX::UIXCheckBox* LoadUIImages;
	UIX::UIXCheckBox* LoadAnimations;
	UIX::UIXCheckBox* LoadModels;
	UIX::UIXGroupBox* groupBox3;
	UIX::UIXButton* DonateButton;
	UIX::UIXButton* TwitterButton;
	UIX::UIXLabel* label3;
	UIX::UIXGroupBox* groupBox2;
	UIX::UIXLabel* label2;
	UIX::UIXComboBox* ImageExportFormat;
	UIX::UIXGroupBox* groupBox1;
	UIX::UIXRadioButton* ExportCastModel;
	UIX::UIXLabel* label1;
	UIX::UIXRadioButton* ExportFBX;
	UIX::UIXRadioButton* ExportMA;
	UIX::UIXRadioButton* ExportXModel;
	UIX::UIXRadioButton* ExportSMD;
	UIX::UIXRadioButton* ExportXNABinary;
	UIX::UIXRadioButton* ExportXNAAscii;
	UIX::UIXRadioButton* ExportOBJ;
	UIX::UIXRadioButton* ExportSEModel;
};
