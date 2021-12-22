#include "pch.h"
#include "LegionSettings.h"
#include "ExportManager.h"
#include "Process.h"

LegionSettings::LegionSettings()
	: Forms::Form()
{
	this->InitializeComponent();
}

void LegionSettings::InitializeComponent()
{
	const INT WindowX = 710;
	const INT WindowY = 315;

	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText("Legion | Application Settings");
	this->SetClientSize({ WindowX, WindowY });
	this->SetFormBorderStyle(Forms::FormBorderStyle::FixedSingle);
	this->SetStartPosition(Forms::FormStartPosition::CenterParent);
	this->SetMinimizeBox(false);
	this->SetMaximizeBox(false);

	//
	//	Custom Export Directory Box
	//
	this->groupBox1 = new UIX::UIXGroupBox();
	this->groupBox1->SetSize({ 458, 110 });
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
	//	About Box
	//
	this->groupBox2 = new UIX::UIXGroupBox();
	this->groupBox2->SetSize({ 218, 110 });
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
	this->label1->SetLocation({ 12, 12 });
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
	this->GithubButton->SetLocation({ 13, 73 });
	this->GithubButton->SetTabIndex(1);
	this->GithubButton->SetText("Github Repository");
	this->groupBox2->AddControl(this->GithubButton);
	//
	//	Discord Link Button
	//
	this->DiscordButton = new UIX::UIXButton();
	this->DiscordButton->SetSize({ 95, 25 });
	this->DiscordButton->SetLocation({ 112, 73 });
	this->DiscordButton->SetTabIndex(1);
	this->DiscordButton->SetText("Discord");
	this->groupBox2->AddControl(this->DiscordButton);

	// 
	//	Load Settings Box
	//
	this->groupBox3 = new UIX::UIXGroupBox();
	this->groupBox3->SetSize({ 341, 122 });
	this->groupBox3->SetLocation({ 12, 125 });
	this->groupBox3->SetTabIndex(3);
	this->groupBox3->SetText("Load Settings");
	this->groupBox3->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->groupBox3);

	// 
	//	Load Settings
	//
	this->LoadModels = new UIX::UIXCheckBox();
	this->LoadModels->SetSize({ 110, 18 });
	this->LoadModels->SetLocation({ 20, 30 });
	this->LoadModels->SetTabIndex(0);
	this->LoadModels->SetText("Load Models");
	this->LoadModels->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadModels);

	this->LoadImages = new UIX::UIXCheckBox();
	this->LoadImages->SetSize({ 110, 18 });
	this->LoadImages->SetLocation({ 175, 30 });
	this->LoadImages->SetTabIndex(2);
	this->LoadImages->SetText("Load Images");
	this->LoadImages->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadImages);

	this->LoadAnimations = new UIX::UIXCheckBox();
	this->LoadAnimations->SetSize({ 110, 18 });
	this->LoadAnimations->SetLocation({ 20, 53 });
	this->LoadAnimations->SetTabIndex(1);
	this->LoadAnimations->SetText("Load Animations");
	this->LoadAnimations->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadAnimations);

	this->LoadMaterials = new UIX::UIXCheckBox();
	this->LoadMaterials->SetSize({ 110, 18 });
	this->LoadMaterials->SetLocation({ 175, 53 });
	this->LoadMaterials->SetTabIndex(3);
	this->LoadMaterials->SetText("Load Materials");
	this->LoadMaterials->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadMaterials);

	this->LoadUIImages = new UIX::UIXCheckBox();
	this->LoadUIImages->SetSize({ 110, 18 });
	this->LoadUIImages->SetLocation({ 20, 76 });
	this->LoadUIImages->SetTabIndex(2);
	this->LoadUIImages->SetText("Load UI Images");
	this->LoadUIImages->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadUIImages);

	this->LoadDataTables = new UIX::UIXCheckBox();
	this->LoadDataTables->SetSize({ 110, 18 });
	this->LoadDataTables->SetLocation({ 175, 76 });
	this->LoadDataTables->SetTabIndex(2);
	this->LoadDataTables->SetText("Load DataTables");
	this->LoadDataTables->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox3->AddControl(this->LoadDataTables);

	// 
	//	Toggle Settings Box
	//
	this->groupBox4 = new UIX::UIXGroupBox();
	this->groupBox4->SetSize({ 341, 48 });
	this->groupBox4->SetLocation({ 12, 252 });
	this->groupBox4->SetTabIndex(3);
	this->groupBox4->SetText("Toggle Settings");
	this->groupBox4->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->groupBox4);

	// 
	//	Toggle Settings
	//
	this->ToggleOverwriting = new UIX::UIXCheckBox();
	this->ToggleOverwriting->SetSize({ 110, 18 });
	this->ToggleOverwriting->SetLocation({ 20, 20 });
	this->ToggleOverwriting->SetTabIndex(2);
	this->ToggleOverwriting->SetText("Overwrite Files");
	this->ToggleOverwriting->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox4->AddControl(this->ToggleOverwriting);

	//
	//	Assets Export Settings Box
	//
	this->groupBox5 = new UIX::UIXGroupBox();
	this->groupBox5->SetSize({ 341, 175 });
	this->groupBox5->SetLocation({ 357, 125 });
	this->groupBox5->SetTabIndex(0);
	this->groupBox5->SetText("Assets Export Settings");
	this->groupBox5->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->AddControl(this->groupBox5);

	//
	//	Model Export Format
	//
	this->label2 = new UIX::UIXLabel();
	this->label2->SetSize({ 120, 15 });
	this->label2->SetLocation({ 20, 20 });
	this->label2->SetTabIndex(8);
	this->label2->SetText("Model Format");
	this->label2->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label2->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox5->AddControl(this->label2);

	this->ModelExportFormat = new UIX::UIXComboBox();
	this->ModelExportFormat->SetSize({ 100, 21 });
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
	this->groupBox5->AddControl(this->ModelExportFormat);

	//
	//	Animation Export Format
	//
	this->label3 = new UIX::UIXLabel();
	this->label3->SetSize({ 120, 15 });
	this->label3->SetLocation({ 20, 65 });
	this->label3->SetTabIndex(11);
	this->label3->SetText("Animation Format");
	this->label3->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label3->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox5->AddControl(this->label3);

	this->AnimExportFormat = new UIX::UIXComboBox();
	this->AnimExportFormat->SetSize({ 100, 21 });
	this->AnimExportFormat->SetLocation({ 20, 80 });
	this->AnimExportFormat->SetTabIndex(0);
	this->AnimExportFormat->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AnimExportFormat->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->AnimExportFormat->Items.Add("Cast");
	this->AnimExportFormat->Items.Add("SEAnim");
	this->groupBox5->AddControl(this->AnimExportFormat);

	//
	//	Image Export Format 
	//
	this->label4 = new UIX::UIXLabel();
	this->label4->SetSize({ 120, 15 });
	this->label4->SetLocation({ 20, 110 });
	this->label4->SetTabIndex(9);
	this->label4->SetText("Image Format");
	this->label4->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label4->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox5->AddControl(this->label4);

	this->ImageExportFormat = new UIX::UIXComboBox();
	this->ImageExportFormat->SetSize({ 100, 21 });
	this->ImageExportFormat->SetLocation({ 20, 125 });
	this->ImageExportFormat->SetTabIndex(0);
	this->ImageExportFormat->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->ImageExportFormat->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->ImageExportFormat->Items.Add("DDS");
	this->ImageExportFormat->Items.Add("PNG");
	this->ImageExportFormat->Items.Add("TIFF");
	this->groupBox5->AddControl(this->ImageExportFormat);

	this->ResumeLayout(false);
	this->PerformLayout();
	// END DESIGNER CODE

	this->SetBackColor({ 30, 32, 55 });

	this->Load += &OnLoad;
	this->FormClosing += &OnClose;
	this->GithubButton->Click += &OnGithubClick;
	this->DiscordButton->Click += &OnDiscordClick;
	this->ExportBrowseButton->Click += &OnBrowseClick;
}

void LegionSettings::LoadSettings()
{
	auto ModelFormat = (RpakModelExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ModelFormat");
	auto AnimFormat = (RpakAnimExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("AnimFormat");
	auto ImageFormat = (RpakImageExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ImageFormat");

	switch (ModelFormat)
	{
	case RpakModelExportFormat::Cast:
		this->ModelExportFormat->SetSelectedIndex(0);
		break;
	case RpakModelExportFormat::FBX:
		this->ModelExportFormat->SetSelectedIndex(1);
		break;
	case RpakModelExportFormat::Maya:
		this->ModelExportFormat->SetSelectedIndex(2);
		break;
	case RpakModelExportFormat::OBJ:
		this->ModelExportFormat->SetSelectedIndex(3);
		break;
	case RpakModelExportFormat::SEModel:
		this->ModelExportFormat->SetSelectedIndex(4);
		break;
	case RpakModelExportFormat::SMD:
		this->ModelExportFormat->SetSelectedIndex(5);
		break;
	case RpakModelExportFormat::XModel:
		this->ModelExportFormat->SetSelectedIndex(6);
		break;
	case RpakModelExportFormat::XNALaraText:
		this->ModelExportFormat->SetSelectedIndex(7);
		break;
	case RpakModelExportFormat::XNALaraBinary:
		this->ModelExportFormat->SetSelectedIndex(8);
		break;
	}

	switch (AnimFormat)
	{
	case RpakAnimExportFormat::Cast:
		this->AnimExportFormat->SetSelectedIndex(0);
		break;
	case RpakAnimExportFormat::SEAnim:
		this->AnimExportFormat->SetSelectedIndex(1);
		break;
	}

	switch (ImageFormat)
	{
	case RpakImageExportFormat::Dds:
		this->ImageExportFormat->SetSelectedIndex(0);
		break;
	case RpakImageExportFormat::Png:
		this->ImageExportFormat->SetSelectedIndex(1);
		break;
	case RpakImageExportFormat::Tiff:
		this->ImageExportFormat->SetSelectedIndex(2);
		break;
	}

	this->LoadModels->SetChecked(ExportManager::Config.Get<System::SettingType::Boolean>("LoadModels"));
	this->LoadAnimations->SetChecked(ExportManager::Config.Get<System::SettingType::Boolean>("LoadAnimations"));
	this->LoadImages->SetChecked(ExportManager::Config.Get<System::SettingType::Boolean>("LoadImages"));
	this->LoadMaterials->SetChecked(ExportManager::Config.Get<System::SettingType::Boolean>("LoadMaterials"));
	this->LoadUIImages->SetChecked(ExportManager::Config.Get<System::SettingType::Boolean>("LoadUIImages"));
	this->LoadDataTables->SetChecked(ExportManager::Config.Get<System::SettingType::Boolean>("LoadDataTables"));
	this->ToggleOverwriting->SetChecked(ExportManager::Config.Get<System::SettingType::Boolean>("OverwriteExistingFiles"));


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
	auto ModelExportFormat = RpakModelExportFormat::Cast;
	auto AnimExportFormat = RpakAnimExportFormat::Cast;
	auto ImageExportFormat = RpakImageExportFormat::Dds;

	if (ThisPtr->ModelExportFormat->SelectedIndex() > -1)
	{
		switch (ThisPtr->ModelExportFormat->SelectedIndex())
		{
		case 1:
			ModelExportFormat = RpakModelExportFormat::FBX;
			break;
		case 2:
			ModelExportFormat = RpakModelExportFormat::Maya;
			break;
		case 3:
			ModelExportFormat = RpakModelExportFormat::OBJ;
			break;
		case 4:
			ModelExportFormat = RpakModelExportFormat::SEModel;
			break;
		case 5:
			ModelExportFormat = RpakModelExportFormat::SMD;
			break;
		case 6:
			ModelExportFormat = RpakModelExportFormat::XModel;
			break;
		case 7:
			ModelExportFormat = RpakModelExportFormat::XNALaraText;
			break;
		case 8:
			ModelExportFormat = RpakModelExportFormat::XNALaraBinary;
			break;
		}
	}

	if (ThisPtr->AnimExportFormat->SelectedIndex() > -1)
	{
		switch (ThisPtr->AnimExportFormat->SelectedIndex())
		{
		case 1:
			AnimExportFormat = RpakAnimExportFormat::SEAnim;
			break;
		}
	}

	if (ThisPtr->ImageExportFormat->SelectedIndex() > -1)
	{
		switch (ThisPtr->ImageExportFormat->SelectedIndex())
		{
		case 1:
			ImageExportFormat = RpakImageExportFormat::Png;
			break;
		case 2:
			ImageExportFormat = RpakImageExportFormat::Tiff;
			break;
		}
	}


	ExportManager::Config.Set<System::SettingType::Boolean>("LoadModels", ThisPtr->LoadModels->Checked());
	ExportManager::Config.Set<System::SettingType::Boolean>("LoadAnimations", ThisPtr->LoadAnimations->Checked());
	ExportManager::Config.Set<System::SettingType::Boolean>("LoadImages", ThisPtr->LoadImages->Checked());
	ExportManager::Config.Set<System::SettingType::Boolean>("LoadMaterials", ThisPtr->LoadMaterials->Checked());
	ExportManager::Config.Set<System::SettingType::Boolean>("LoadUIImages", ThisPtr->LoadUIImages->Checked());
	ExportManager::Config.Set<System::SettingType::Boolean>("LoadDataTables", ThisPtr->LoadDataTables->Checked());
	ExportManager::Config.Set<System::SettingType::Boolean>("OverwriteExistingFiles", ThisPtr->ToggleOverwriting->Checked());
	ExportManager::Config.Set<System::SettingType::Integer>("ModelFormat", (uint32_t)ModelExportFormat);
	ExportManager::Config.Set<System::SettingType::Integer>("AnimFormat", (uint32_t)AnimExportFormat);
	ExportManager::Config.Set<System::SettingType::Integer>("ImageFormat", (uint32_t)ImageExportFormat);

	auto ExportDirectory = ThisPtr->ExportBrowseFolder->Text();

	if (ExportDirectory == "Click on \"Browse\" to set a custom export directory")
	{
		ExportManager::Config.Remove<System::SettingType::String>("ExportDirectory");
	}
	else if (IO::Directory::Exists(ExportDirectory))
	{
		ExportManager::Config.Set<System::SettingType::String>("ExportDirectory", ExportDirectory.ToCString());
	}

	ExportManager::SaveConfigToDisk();
}

void LegionSettings::OnGithubClick(Forms::Control* Sender)
{
	Diagnostics::Process::Start("https://github.com/r-ex/Legion");
}

void LegionSettings::OnDiscordClick(Forms::Control* Sender)
{
	Diagnostics::Process::Start("https://youtu.be/-pHhb4biR9k");
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

