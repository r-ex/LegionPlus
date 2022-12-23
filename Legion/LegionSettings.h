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
	// Labels
	UIX::UIXLabel* label1;
	UIX::UIXLabel* label2;
	UIX::UIXLabel* label3;
	UIX::UIXLabel* label4;
	UIX::UIXLabel* label5;
	UIX::UIXLabel* label6;
	UIX::UIXLabel* label7;
	UIX::UIXLabel* label8;
	UIX::UIXLabel* labelVersion; // version
	UIX::UIXLabel* labelAbout; // about
	// Boxes
	UIX::UIXGroupBox* groupBox1;
	UIX::UIXGroupBox* groupBox2;
	UIX::UIXGroupBox* groupBox3;
	UIX::UIXGroupBox* groupBox4;
	UIX::UIXGroupBox* groupBox5;
	// Load Types
	UIX::UIXCheckBox* LoadModels;
	UIX::UIXCheckBox* LoadImages;
	UIX::UIXCheckBox* LoadMaterials;
	UIX::UIXCheckBox* LoadShaderSets;
	UIX::UIXCheckBox* LoadRSONs;
	UIX::UIXCheckBox* LoadEffects;
	UIX::UIXCheckBox* LoadAnimations;
	UIX::UIXCheckBox* LoadAnimationSeqs;
	UIX::UIXCheckBox* LoadUIImages;
	UIX::UIXCheckBox* LoadDataTables;
	UIX::UIXCheckBox* LoadSettingsSets;
	// Toggles
	UIX::UIXCheckBox* ToggleOverwriting;
	UIX::UIXCheckBox* ToggleAudioLanguageFolders;
	UIX::UIXCheckBox* ToggleUseFullPaths;
	UIX::UIXCheckBox* ToggleUseTxtrGuids;
	UIX::UIXCheckBox* ToggleSkinExport;
	// Export Types
	UIX::UIXComboBox* ModelExportFormat;
	UIX::UIXComboBox* AnimExportFormat;
	UIX::UIXComboBox* ImageExportFormat;
	UIX::UIXComboBox* TextExportFormat;
	UIX::UIXComboBox* AudioLanguage;
	UIX::UIXComboBox* NormalRecalcType;
	UIX::UIXComboBox* MatCPUExportFormat;
	UIX::UIXComboBox* AudioExportFormat;
	// Export Formats
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
	// Other Buttons
	UIX::UIXButton* GithubButton;
	UIX::UIXButton* DiscordButton;
};
