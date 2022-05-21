#include "pch.h"
#include "LegionSettings.h"
#include "ExportManager.h"
#include "Process.h"
#include "LegionMain.h"
#include "MilesLib.h"

LegionSettings::LegionSettings()
	: Forms::Form()
{
	this->InitializeComponent();
}

void LegionSettings::InitializeComponent()
{
	const INT WindowX = 710;
	const INT WindowY = 335;

	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText("Legion+ | Application Settings");
	this->SetClientSize({ WindowX, WindowY });
	this->SetFormBorderStyle(Forms::FormBorderStyle::FixedSingle);
	this->SetStartPosition(Forms::FormStartPosition::CenterParent);
	this->SetMinimizeBox(false);
	this->SetMaximizeBox(false);

	//
	//	Custom Export Directory Box
	//
	this->groupBox1 = new UIX::UIXGroupBox();
	this->groupBox1->SetSize({ 458, 65 });
	this->groupBox1->SetLocation({ 12, 10 });
	this->groupBox1->SetTabIndex(3);
	this->groupBox1->SetText("General Directory Settings");
	this->groupBox1->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->groupBox1);

	//
	//	Custom Export Directory
	//
	this->ExportBrowseFolder = new UIX::UIXTextBox();
	this->ExportBrowseFolder->SetSize({ 342, 25 });
	this->ExportBrowseFolder->SetLocation({ 15, 25 });
	this->ExportBrowseFolder->SetTabIndex(5);
	this->ExportBrowseFolder->SetReadOnly(true);
	this->ExportBrowseFolder->SetText("Click on \"Browse\" to set a custom export directory");
	this->ExportBrowseFolder->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Right);
	this->groupBox1->AddControl(this->ExportBrowseFolder);

	this->ExportBrowseButton = new UIX::UIXButton();
	this->ExportBrowseButton->SetSize({ 80, 25 });
	this->ExportBrowseButton->SetLocation({ 365, 25 });
	this->ExportBrowseButton->SetTabIndex(5);
	this->ExportBrowseButton->SetText("Browse");
	this->ExportBrowseButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Right);
	this->groupBox1->AddControl(this->ExportBrowseButton);

	// 
	//	Toggle Settings Box
	//
	this->groupBox4 = new UIX::UIXGroupBox();
	this->groupBox4->SetSize({ 458, 68 });
	this->groupBox4->SetLocation({ 12, 80 });
	this->groupBox4->SetTabIndex(3);
	this->groupBox4->SetText("Toggle Settings");
	this->groupBox4->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->groupBox4);

	// 
	//	Toggle Settings
	//
	this->ToggleOverwriting = new UIX::UIXCheckBox();
	this->ToggleOverwriting->SetSize({ 110, 18 });
	this->ToggleOverwriting->SetLocation({ 15, 20 });
	this->ToggleOverwriting->SetTabIndex(2);
	this->ToggleOverwriting->SetText("Overwrite Files");
	this->ToggleOverwriting->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox4->AddControl(this->ToggleOverwriting);

	this->ToggleAudioLanguageFolders = new UIX::UIXCheckBox();
	this->ToggleAudioLanguageFolders->SetSize({ 150, 18 });
	this->ToggleAudioLanguageFolders->SetLocation({ 130, 20 });
	this->ToggleAudioLanguageFolders->SetTabIndex(2);
	this->ToggleAudioLanguageFolders->SetText("Audio Language Folders");
	this->ToggleAudioLanguageFolders->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox4->AddControl(this->ToggleAudioLanguageFolders);

	this->ToggleUseFullPaths = new UIX::UIXCheckBox();
	this->ToggleUseFullPaths->SetSize({ 150, 18 });
	this->ToggleUseFullPaths->SetLocation({ 290, 20 });
	this->ToggleUseFullPaths->SetTabIndex(2);
	this->ToggleUseFullPaths->SetText("Show Full Asset Paths");
	this->ToggleUseFullPaths->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox4->AddControl(this->ToggleUseFullPaths);

	this->ToggleUseTxtrGuids = new UIX::UIXCheckBox();
	this->ToggleUseTxtrGuids->SetSize({ 105, 18 });
	this->ToggleUseTxtrGuids->SetLocation({ 15, 43 });
	this->ToggleUseTxtrGuids->SetTabIndex(2);
	this->ToggleUseTxtrGuids->SetText("Use Image Guids");
	this->ToggleUseTxtrGuids->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox4->AddControl(this->ToggleUseTxtrGuids);


	//
	//	About Box
	//
	this->groupBox2 = new UIX::UIXGroupBox();
	this->groupBox2->SetSize({ 218, 138 });
	this->groupBox2->SetLocation({ 480, 10 });
	this->groupBox2->SetTabIndex(3);
	this->groupBox2->SetText("About");
	this->groupBox2->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->groupBox2);

	//
	//	About Text Label
	//
	this->label1 = new UIX::UIXLabel();
	this->label1->SetSize({ 200, 60 });
	this->label1->SetLocation({ 12, 27 });
	this->label1->SetTabIndex(0);
	this->label1->SetText("Legion is the Apex Legends asset extraction tool. Originally created by DTZxPorter in 2019. Currently maintained by various contributors on GitHub.");
	this->label1->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label1->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox2->AddControl(this->label1);
	//
	//	Github Link Button
	//
	this->GithubButton = new UIX::UIXButton();
	this->GithubButton->SetSize({ 95, 25 });
	this->GithubButton->SetLocation({ 13, 93 });
	this->GithubButton->SetTabIndex(1);
	this->GithubButton->SetText("Github Repository");
	this->groupBox2->AddControl(this->GithubButton);
	//
	//	Discord Link Button
	//
	this->DiscordButton = new UIX::UIXButton();
	this->DiscordButton->SetSize({ 95, 25 });
	this->DiscordButton->SetLocation({ 112, 93 });
	this->DiscordButton->SetTabIndex(1);
	this->DiscordButton->SetText("Discord");
	this->groupBox2->AddControl(this->DiscordButton);

	// 
	//	Load Settings Box
	//
	this->groupBox3 = new UIX::UIXGroupBox();
	this->groupBox3->SetSize({ 340, 175 });
	this->groupBox3->SetLocation({ 12, 155 });
	this->groupBox3->SetTabIndex(3);
	this->groupBox3->SetText("Load Settings");
	this->groupBox3->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->groupBox3);

	// 
	//	Load Settings
	//
	this->LoadModels = new UIX::UIXCheckBox();
	this->LoadModels->SetSize({ 105, 18 });
	this->LoadModels->SetLocation({ 15, 22 });
	this->LoadModels->SetTabIndex(0);
	this->LoadModels->SetText("Load Models");
	this->LoadModels->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadModels);

	this->LoadImages = new UIX::UIXCheckBox();
	this->LoadImages->SetSize({ 105, 18 });
	this->LoadImages->SetLocation({ 130, 22 });
	this->LoadImages->SetTabIndex(2);
	this->LoadImages->SetText("Load Images");
	this->LoadImages->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadImages);

	this->LoadAnimations = new UIX::UIXCheckBox();
	this->LoadAnimations->SetSize({ 105, 18 });
	this->LoadAnimations->SetLocation({ 15, 45 });
	this->LoadAnimations->SetTabIndex(1);
	this->LoadAnimations->SetText("Load Animations");
	this->LoadAnimations->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadAnimations);

	this->LoadMaterials = new UIX::UIXCheckBox();
	this->LoadMaterials->SetSize({ 105, 18 });
	this->LoadMaterials->SetLocation({ 130, 45 });
	this->LoadMaterials->SetTabIndex(3);
	this->LoadMaterials->SetText("Load Materials");
	this->LoadMaterials->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadMaterials);

	this->LoadUIImages = new UIX::UIXCheckBox();
	this->LoadUIImages->SetSize({ 105, 18 });
	this->LoadUIImages->SetLocation({ 15, 68 });
	this->LoadUIImages->SetTabIndex(2);
	this->LoadUIImages->SetText("Load UI Images");
	this->LoadUIImages->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadUIImages);

	this->LoadDataTables = new UIX::UIXCheckBox();
	this->LoadDataTables->SetSize({ 105, 18 });
	this->LoadDataTables->SetLocation({ 130, 68 });
	this->LoadDataTables->SetTabIndex(2);
	this->LoadDataTables->SetText("Load DataTables");
	this->LoadDataTables->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadDataTables);

	this->LoadShaderSets = new UIX::UIXCheckBox();
	this->LoadShaderSets->SetSize({ 105, 18 });
	this->LoadShaderSets->SetLocation({ 15, 91 });
	this->LoadShaderSets->SetTabIndex(2);
	this->LoadShaderSets->SetText("Load ShaderSets");
	this->LoadShaderSets->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadShaderSets);

	//
	//	Assets Export Settings Box
	//
	this->groupBox5 = new UIX::UIXGroupBox();
	this->groupBox5->SetSize({ 340, 175 });
	this->groupBox5->SetLocation({ 359, 155 });
	this->groupBox5->SetTabIndex(0);
	this->groupBox5->SetText("Assets Export Settings");
	this->groupBox5->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->AddControl(this->groupBox5);

	//
	//	Model Export Format
	//
	this->label2 = new UIX::UIXLabel();
	this->label2->SetSize({ 90, 15 });
	this->label2->SetLocation({ 20, 20 });
	this->label2->SetTabIndex(8);
	this->label2->SetText("Model Format");
	this->label2->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label2->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox5->AddControl(this->label2);

	this->ModelExportFormat = new UIX::UIXComboBox();
	this->ModelExportFormat->SetSize({ 90, 21 });
	this->ModelExportFormat->SetLocation({ 20, 35 });
	this->ModelExportFormat->SetTabIndex(9);
	this->ModelExportFormat->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->ModelExportFormat->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->ModelExportFormat->Items.Add("Cast");
	this->ModelExportFormat->Items.Add("FBX");
	this->ModelExportFormat->Items.Add("Maya");
	this->ModelExportFormat->Items.Add("OBJ");
	this->ModelExportFormat->Items.Add("SEModel");
	this->ModelExportFormat->Items.Add("SMD");
	this->ModelExportFormat->Items.Add("XModel");
	this->ModelExportFormat->Items.Add("XNALara ASCII");
	this->ModelExportFormat->Items.Add("XNALara Binary");
	this->ModelExportFormat->Items.Add("RMDL");
	this->groupBox5->AddControl(this->ModelExportFormat);

	//
	//	Animation Export Format
	//
	this->label3 = new UIX::UIXLabel();
	this->label3->SetSize({ 90, 15 });
	this->label3->SetLocation({ 20, 65 });
	this->label3->SetTabIndex(11);
	this->label3->SetText("Animation Format");
	this->label3->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label3->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox5->AddControl(this->label3);

	this->AnimExportFormat = new UIX::UIXComboBox();
	this->AnimExportFormat->SetSize({ 90, 21 });
	this->AnimExportFormat->SetLocation({ 20, 80 });
	this->AnimExportFormat->SetTabIndex(0);
	this->AnimExportFormat->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AnimExportFormat->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->AnimExportFormat->Items.Add("Cast");
	this->AnimExportFormat->Items.Add("SEAnim");
	this->AnimExportFormat->Items.Add("RAnim");
	this->groupBox5->AddControl(this->AnimExportFormat);

	//
	//	Image Export Format
	//
	this->label4 = new UIX::UIXLabel();
	this->label4->SetSize({ 90, 15 });
	this->label4->SetLocation({ 20, 110 });
	this->label4->SetTabIndex(9);
	this->label4->SetText("Image Format");
	this->label4->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label4->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox5->AddControl(this->label4);

	this->ImageExportFormat = new UIX::UIXComboBox();
	this->ImageExportFormat->SetSize({ 90, 21 });
	this->ImageExportFormat->SetLocation({ 20, 125 });
	this->ImageExportFormat->SetTabIndex(0);
	this->ImageExportFormat->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->ImageExportFormat->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->ImageExportFormat->Items.Add("DDS");
	this->ImageExportFormat->Items.Add("PNG");
	this->ImageExportFormat->Items.Add("TIFF");
	this->ImageExportFormat->Items.Add("TGA");
	this->groupBox5->AddControl(this->ImageExportFormat);

	//
	//	Text/Subtitles/Datatables Export Format
	//
	this->label5 = new UIX::UIXLabel();
	this->label5->SetSize({ 90, 15 });
	this->label5->SetLocation({ 125, 20 });
	this->label5->SetTabIndex(9);
	this->label5->SetText("Text Format");
	this->label5->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label5->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox5->AddControl(this->label5);

	this->TextExportFormat = new UIX::UIXComboBox();
	this->TextExportFormat->SetSize({ 90, 20 });
	this->TextExportFormat->SetLocation({ 125, 35 });
	this->TextExportFormat->SetTabIndex(0);
	this->TextExportFormat->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->TextExportFormat->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->TextExportFormat->Items.Add("CSV");
	this->TextExportFormat->Items.Add("TXT");
	this->groupBox5->AddControl(this->TextExportFormat);

	//
	//	NormalRecalcType
	//
	this->label6 = new UIX::UIXLabel();
	this->label6->SetSize({ 120, 15 });
	this->label6->SetLocation({ 125, 65 });
	this->label6->SetTabIndex(9);
	this->label6->SetText("Normal Recalculation");
	this->label6->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label6->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox5->AddControl(this->label6);

	this->NormalRecalcType = new UIX::UIXComboBox();
	this->NormalRecalcType->SetSize({ 90, 20 });
	this->NormalRecalcType->SetLocation({ 125, 80 });
	this->NormalRecalcType->SetTabIndex(0);
	this->NormalRecalcType->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->NormalRecalcType->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->NormalRecalcType->Items.Add("None");
	this->NormalRecalcType->Items.Add("DirectX");
	this->NormalRecalcType->Items.Add("OpenGL");
	this->groupBox5->AddControl(this->NormalRecalcType);

	//
	//	Audio Language
	//
	this->label7 = new UIX::UIXLabel();
	this->label7->SetSize({ 120, 15 });
	this->label7->SetLocation({ 125, 110 });
	this->label7->SetTabIndex(9);
	this->label7->SetText("Audio Language");
	this->label7->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label7->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox5->AddControl(this->label7);

	this->AudioLanguage = new UIX::UIXComboBox();
	this->AudioLanguage->SetSize({ 90, 20 });
	this->AudioLanguage->SetLocation({ 125, 125 });
	this->AudioLanguage->SetTabIndex(0);
	this->AudioLanguage->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AudioLanguage->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	for (int i = (int)MilesLanguageID::English; i <= (int)MilesLanguageID::Korean; i++) {
		this->AudioLanguage->Items.Add(imstring(LanguageName((MilesLanguageID)i)));
	}
	this->groupBox5->AddControl(this->AudioLanguage);

	this->ResumeLayout(false);
	this->PerformLayout();
	// END DESIGNER CODE

	//this->SetBackColor({ 30, 32, 55 });
	this->SetBackColor({ 33, 33, 33 });

	this->Load += &OnLoad;
	this->FormClosing += &OnClose;
	this->GithubButton->Click += &OnGithubClick;
	this->DiscordButton->Click += &OnDiscordClick;
	this->ExportBrowseButton->Click += &OnBrowseClick;
}

void LegionSettings::LoadSettings()
{
	ModelExportFormat_t ModelFormat = (ModelExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("ModelFormat");
	AnimExportFormat_t AnimFormat = (AnimExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("AnimFormat");
	ImageExportFormat_t ImageFormat = (ImageExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("ImageFormat");
	TextExportFormat_t TextFormat = (TextExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("TextFormat");
	NormalRecalcType_t NormalRecalcType = (NormalRecalcType_t)ExportManager::Config.Get<System::SettingType::Integer>("NormalRecalcType");
	MilesLanguageID AudioLanguage = (MilesLanguageID)ExportManager::Config.Get<System::SettingType::Integer>("AudioLanguage");

	if (!ExportManager::Config.Has("NormalRecalcType"))
		NormalRecalcType = NormalRecalcType_t::OpenGl;

	switch (ModelFormat)
	{
	case ModelExportFormat_t::Cast:
		this->ModelExportFormat->SetSelectedIndex(0);
		break;
	case ModelExportFormat_t::FBX:
		this->ModelExportFormat->SetSelectedIndex(1);
		break;
	case ModelExportFormat_t::Maya:
		this->ModelExportFormat->SetSelectedIndex(2);
		break;
	case ModelExportFormat_t::OBJ:
		this->ModelExportFormat->SetSelectedIndex(3);
		break;
	case ModelExportFormat_t::SEModel:
		this->ModelExportFormat->SetSelectedIndex(4);
		break;
	case ModelExportFormat_t::SMD:
		this->ModelExportFormat->SetSelectedIndex(5);
		break;
	case ModelExportFormat_t::XModel:
		this->ModelExportFormat->SetSelectedIndex(6);
		break;
	case ModelExportFormat_t::XNALaraText:
		this->ModelExportFormat->SetSelectedIndex(7);
		break;
	case ModelExportFormat_t::XNALaraBinary:
		this->ModelExportFormat->SetSelectedIndex(8);
		break;
	case ModelExportFormat_t::RMDL:
		this->ModelExportFormat->SetSelectedIndex(9);
		break;
	}

	switch (AnimFormat)
	{
	case AnimExportFormat_t::Cast:
		this->AnimExportFormat->SetSelectedIndex(0);
		break;
	case AnimExportFormat_t::SEAnim:
		this->AnimExportFormat->SetSelectedIndex(1);
		break;
	case AnimExportFormat_t::RAnim:
		this->AnimExportFormat->SetSelectedIndex(2);
		break;
	}

	switch (ImageFormat)
	{
	case ImageExportFormat_t::Dds:
		this->ImageExportFormat->SetSelectedIndex(0);
		break;
	case ImageExportFormat_t::Png:
		this->ImageExportFormat->SetSelectedIndex(1);
		break;
	case ImageExportFormat_t::Tiff:
		this->ImageExportFormat->SetSelectedIndex(2);
		break;
	case ImageExportFormat_t::Tga:
		this->ImageExportFormat->SetSelectedIndex(3);
		break;
	}

	switch (TextFormat)
	{
	case TextExportFormat_t::CSV:
		this->TextExportFormat->SetSelectedIndex(0);
		break;
	case TextExportFormat_t::TXT:
		this->TextExportFormat->SetSelectedIndex(1);
		break;
	}

	switch (NormalRecalcType)
	{
	case NormalRecalcType_t::None:
		this->NormalRecalcType->SetSelectedIndex(0);
		break;
	case NormalRecalcType_t::DirectX:
		this->NormalRecalcType->SetSelectedIndex(1);
		break;
	case NormalRecalcType_t::OpenGl:
		this->NormalRecalcType->SetSelectedIndex(2);
		break;
	}

	this->AudioLanguage->SetSelectedIndex(static_cast<int32_t>(AudioLanguage));

	this->LoadModels->SetChecked(ExportManager::Config.GetBool("LoadModels"));
	this->LoadAnimations->SetChecked(ExportManager::Config.GetBool("LoadAnimations"));
	this->LoadImages->SetChecked(ExportManager::Config.GetBool("LoadImages"));
	this->LoadMaterials->SetChecked(ExportManager::Config.GetBool("LoadMaterials"));
	this->LoadUIImages->SetChecked(ExportManager::Config.GetBool("LoadUIImages"));
	this->LoadDataTables->SetChecked(ExportManager::Config.GetBool("LoadDataTables"));
	this->LoadShaderSets->SetChecked(ExportManager::Config.GetBool("LoadShaderSets"));
	this->ToggleOverwriting->SetChecked(ExportManager::Config.GetBool("OverwriteExistingFiles"));
	this->ToggleAudioLanguageFolders->SetChecked(ExportManager::Config.GetBool("AudioLanguageFolders"));
	this->ToggleUseFullPaths->SetChecked(ExportManager::Config.GetBool("UseFullPaths"));
	this->ToggleUseTxtrGuids->SetChecked(ExportManager::Config.GetBool("UseTxtrGuids"));


	if (ExportManager::Config.Has<System::SettingType::String>("ExportDirectory"))
	{
		this->ExportBrowseFolder->SetText(ExportManager::Config.Get<System::SettingType::String>("ExportDirectory"));
	}
}

void LegionSettings::OnLoad(Forms::Control* Sender)
{
	((LegionSettings*)Sender->FindForm())->LoadSettings();
}

void LegionSettings::OnClose(const std::unique_ptr<FormClosingEventArgs>& EventArgs, Forms::Control* Sender)
{
	auto ThisPtr = (LegionSettings*)Sender->FindForm();

	// Fetch settings from controls
	auto ModelExportFormat = ModelExportFormat_t::Cast;
	auto AnimExportFormat = AnimExportFormat_t::Cast;
	auto ImageExportFormat = ImageExportFormat_t::Dds;
	auto TextExportFormat = TextExportFormat_t::CSV;
	auto NormalRecalcType = NormalRecalcType_t::OpenGl;
	auto AudioLanguage = MilesLanguageID::English;

	if (ThisPtr->ModelExportFormat->SelectedIndex() > -1)
	{
		switch (ThisPtr->ModelExportFormat->SelectedIndex())
		{
		case 1:
			ModelExportFormat = ModelExportFormat_t::FBX;
			break;
		case 2:
			ModelExportFormat = ModelExportFormat_t::Maya;
			break;
		case 3:
			ModelExportFormat = ModelExportFormat_t::OBJ;
			break;
		case 4:
			ModelExportFormat = ModelExportFormat_t::SEModel;
			break;
		case 5:
			ModelExportFormat = ModelExportFormat_t::SMD;
			break;
		case 6:
			ModelExportFormat = ModelExportFormat_t::XModel;
			break;
		case 7:
			ModelExportFormat = ModelExportFormat_t::XNALaraText;
			break;
		case 8:
			ModelExportFormat = ModelExportFormat_t::XNALaraBinary;
			break;
		case 9:
			ModelExportFormat = ModelExportFormat_t::RMDL;
			break;
		}
	}

	if (ThisPtr->AnimExportFormat->SelectedIndex() > -1)
	{
		switch (ThisPtr->AnimExportFormat->SelectedIndex())
		{
		case 1:
			AnimExportFormat = AnimExportFormat_t::SEAnim;
			break;
		case 2:
			AnimExportFormat = AnimExportFormat_t::RAnim;
			break;
		}
	}

	if (ThisPtr->ImageExportFormat->SelectedIndex() > -1)
	{
		switch (ThisPtr->ImageExportFormat->SelectedIndex())
		{
		case 1:
			ImageExportFormat = ImageExportFormat_t::Png;
			break;
		case 2:
			ImageExportFormat = ImageExportFormat_t::Tiff;
			break;
		case 3:
			ImageExportFormat = ImageExportFormat_t::Tga;
			break;
		}
	}

	if (ThisPtr->TextExportFormat->SelectedIndex() > -1)
	{
		switch (ThisPtr->TextExportFormat->SelectedIndex())
		{
		case 1:
			TextExportFormat = TextExportFormat_t::TXT;
			break;
		}
	}

	if (ThisPtr->NormalRecalcType->SelectedIndex() > -1)
	{
		switch (ThisPtr->NormalRecalcType->SelectedIndex())
		{
		case 0:
			NormalRecalcType = NormalRecalcType_t::None;
			break;
		case 1:
			NormalRecalcType = NormalRecalcType_t::DirectX;
			break;
		}
	}

	if (ThisPtr->AudioLanguage->SelectedIndex() > -1) {
		AudioLanguage = (MilesLanguageID)ThisPtr->AudioLanguage->SelectedIndex();
	}

	// have the settings actually changed?
	bool bRefreshView = false;

	if (ThisPtr->LoadModels->Checked() != ExportManager::Config.GetBool("LoadModels"))
		bRefreshView = true;
	if (ThisPtr->LoadAnimations->Checked() != ExportManager::Config.GetBool("LoadAnimations"))
		bRefreshView = true;
	if (ThisPtr->LoadImages->Checked() != ExportManager::Config.GetBool("LoadImages"))
		bRefreshView = true;
	if (ThisPtr->LoadMaterials->Checked() != ExportManager::Config.GetBool("LoadMaterials"))
		bRefreshView = true;
	if (ThisPtr->LoadUIImages->Checked() != ExportManager::Config.GetBool("LoadUIImages"))
		bRefreshView = true;
	if (ThisPtr->LoadDataTables->Checked() != ExportManager::Config.GetBool("LoadDataTables"))
		bRefreshView = true;
	if (ThisPtr->LoadShaderSets->Checked() != ExportManager::Config.GetBool("LoadShaderSets"))
		bRefreshView = true;
	if (ThisPtr->ToggleUseFullPaths->Checked() != ExportManager::Config.GetBool("UseFullPaths"))
		bRefreshView = true;

	ExportManager::Config.SetBool("LoadModels", ThisPtr->LoadModels->Checked());
	ExportManager::Config.SetBool("LoadAnimations", ThisPtr->LoadAnimations->Checked());
	ExportManager::Config.SetBool("LoadImages", ThisPtr->LoadImages->Checked());
	ExportManager::Config.SetBool("LoadMaterials", ThisPtr->LoadMaterials->Checked());
	ExportManager::Config.SetBool("LoadUIImages", ThisPtr->LoadUIImages->Checked());
	ExportManager::Config.SetBool("LoadDataTables", ThisPtr->LoadDataTables->Checked());
	ExportManager::Config.SetBool("LoadShaderSets", ThisPtr->LoadShaderSets->Checked());
	ExportManager::Config.SetBool("OverwriteExistingFiles", ThisPtr->ToggleOverwriting->Checked());
	ExportManager::Config.SetBool("AudioLanguageFolders", ThisPtr->ToggleAudioLanguageFolders->Checked());
	ExportManager::Config.SetBool("UseFullPaths", ThisPtr->ToggleUseFullPaths->Checked());
	ExportManager::Config.SetBool("UseTxtrGuids", ThisPtr->ToggleUseTxtrGuids->Checked());
	ExportManager::Config.SetInt("ModelFormat", (uint32_t)ModelExportFormat);
	ExportManager::Config.SetInt("AnimFormat", (uint32_t)AnimExportFormat);
	ExportManager::Config.SetInt("ImageFormat", (uint32_t)ImageExportFormat);
	ExportManager::Config.SetInt("TextFormat", (uint32_t)TextExportFormat);
	ExportManager::Config.SetInt("NormalRecalcType", (uint32_t)NormalRecalcType);
	ExportManager::Config.SetInt("AudioLanguage", (uint32_t)AudioLanguage);

	auto ExportDirectory = ThisPtr->ExportBrowseFolder->Text();

	if (ExportDirectory == "Click on \"Browse\" to set a custom export directory")
	{
		ExportManager::Config.Remove<System::SettingType::String>("ExportDirectory");
	}
	else if (IO::Directory::Exists(ExportDirectory))
	{
		ExportManager::Config.Set<System::SettingType::String>("ExportDirectory", ExportDirectory);
	}

	ExportManager::SaveConfigToDisk();

	if(bRefreshView)
		g_pLegionMain->RefreshView();
}

void LegionSettings::OnGithubClick(Forms::Control* Sender)
{
	Diagnostics::Process::Start("https://github.com/r-ex/LegionPlus");
}

void LegionSettings::OnDiscordClick(Forms::Control* Sender)
{
	Diagnostics::Process::Start("https://discord.gg/ADek6fxVGe");
}

void LegionSettings::OnBrowseClick(Forms::Control* Sender)
{
	auto ThisPtr = (LegionSettings*)Sender->FindForm();
	auto ExportDirectory = ThisPtr->ExportBrowseFolder->Text();

	auto Result = OpenFileDialog::ShowFolderDialog("Select a folder to export assets to or press \"Cancel\" to reset back to default.", ExportDirectory != "Click on \"Browse\" to set a custom export directory" ? ExportDirectory : string(""), ThisPtr);

	if (Result == "" || Result.Length() == 0)
	{
		ThisPtr->ExportBrowseFolder->SetText("Click on \"Browse\" to set a custom export directory");
	}
	else if (IO::Directory::Exists(Result))
	{
		ThisPtr->ExportBrowseFolder->SetText(Result);
	}
}

