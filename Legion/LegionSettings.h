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
	static void OnBrowseClick(Forms::Control* Sender);
	static void OnGithubClick(Forms::Control* Sender);
	static void OnDiscordClick(Forms::Control* Sender);

	// Internal controls reference
	UIX::UIXTextBox* ExportBrowseFolder;
	UIX::UIXButton* ExportBrowseButton;
	UIX::UIXLabel* label1;
	UIX::UIXLabel* label2;
	UIX::UIXLabel* label3;
	UIX::UIXLabel* label4;
	UIX::UIXLabel* label5;
	UIX::UIXGroupBox* groupBox1;
	UIX::UIXGroupBox* groupBox2;
	UIX::UIXGroupBox* groupBox3;
	UIX::UIXGroupBox* groupBox4;
	UIX::UIXCheckBox* LoadMaterials;
	UIX::UIXCheckBox* LoadImages;
	UIX::UIXCheckBox* LoadUIImages;
	UIX::UIXCheckBox* LoadDataTables;
	UIX::UIXCheckBox* LoadAnimations;
	UIX::UIXCheckBox* LoadModels;
	UIX::UIXComboBox* ImageExportFormat;
	UIX::UIXComboBox* AnimExportFormat;
	UIX::UIXComboBox* ModelExportFormat;
	UIX::UIXRadioButton* ExportCastAnim;
	UIX::UIXRadioButton* ExportSEAnim;
	UIX::UIXRadioButton* ExportCastModel;
	UIX::UIXRadioButton* ExportFBX;
	UIX::UIXRadioButton* ExportMA;
	UIX::UIXRadioButton* ExportXModel;
	UIX::UIXRadioButton* ExportSMD;
	UIX::UIXRadioButton* ExportXNABinary;
	UIX::UIXRadioButton* ExportXNAAscii;
	UIX::UIXRadioButton* ExportOBJ;
	UIX::UIXRadioButton* ExportSEModel;
	UIX::UIXButton* GithubButton;
	UIX::UIXButton* DiscordButton;
};
