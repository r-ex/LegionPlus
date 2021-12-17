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
	const INT WindowX = 752;
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
	//	General Settings Box
	//
	this->groupBox4 = new UIX::UIXGroupBox();
	this->groupBox4->SetSize({ 728, 110 });
	this->groupBox4->SetLocation({ 12, 12 });
	this->groupBox4->SetTabIndex(3);
	this->groupBox4->SetText("General Settings");
	this->groupBox4->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->groupBox4);

	this->ExportBrowseButton = new UIX::UIXButton();
	this->ExportBrowseButton->SetSize({ 80, 25 });
	this->ExportBrowseButton->SetLocation({ 642, 26 });
	this->ExportBrowseButton->SetTabIndex(5);
	this->ExportBrowseButton->SetText("Browse");
	this->ExportBrowseButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Right);
	this->groupBox4->AddControl(this->ExportBrowseButton);

	this->ExportBrowseFolder = new UIX::UIXTextBox();
	this->ExportBrowseFolder->SetSize({ 342, 25 });
	this->ExportBrowseFolder->SetLocation({ 296, 26 });
	this->ExportBrowseFolder->SetTabIndex(5);
	this->ExportBrowseFolder->SetReadOnly(true);
	this->ExportBrowseFolder->SetText("Click on \"Browse\" to set a custom export directory");
	this->ExportBrowseFolder->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Right);
	this->groupBox4->AddControl(this->ExportBrowseFolder);
	//
	//	Load Flags
	//
	this->LoadModels = new UIX::UIXCheckBox();
	this->LoadModels->SetSize({ 88, 18 });
	this->LoadModels->SetLocation({ 20, 30 });
	this->LoadModels->SetTabIndex(0);
	this->LoadModels->SetText("Load Models");
	this->LoadModels->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox4->AddControl(this->LoadModels);

	this->LoadAnimations = new UIX::UIXCheckBox();
	this->LoadAnimations->SetSize({ 108, 18 });
	this->LoadAnimations->SetLocation({ 20, 53 });
	this->LoadAnimations->SetTabIndex(1);
	this->LoadAnimations->SetText("Load Animations");
	this->LoadAnimations->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox4->AddControl(this->LoadAnimations);

	this->LoadUIImages = new UIX::UIXCheckBox();
	this->LoadUIImages->SetSize({ 108, 18 });
	this->LoadUIImages->SetLocation({ 20, 76 });
	this->LoadUIImages->SetTabIndex(2);
	this->LoadUIImages->SetText("Load UI Images");
	this->LoadUIImages->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox4->AddControl(this->LoadUIImages);

	this->LoadImages = new UIX::UIXCheckBox();
	this->LoadImages->SetSize({ 89, 18 });
	this->LoadImages->SetLocation({ 175, 30 });
	this->LoadImages->SetTabIndex(2);
	this->LoadImages->SetText("Load Images");
	this->LoadImages->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox4->AddControl(this->LoadImages);

	this->LoadMaterials = new UIX::UIXCheckBox();
	this->LoadMaterials->SetSize({ 97, 18 });
	this->LoadMaterials->SetLocation({ 175, 53 });
	this->LoadMaterials->SetTabIndex(3);
	this->LoadMaterials->SetText("Load Materials");
	this->LoadMaterials->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox4->AddControl(this->LoadMaterials);

	this->LoadDataTables = new UIX::UIXCheckBox();
	this->LoadDataTables->SetSize({ 108, 18 });
	this->LoadDataTables->SetLocation({ 175, 76 });
	this->LoadDataTables->SetTabIndex(2);
	this->LoadDataTables->SetText("Load DataTables");
	this->LoadDataTables->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox4->AddControl(this->LoadDataTables);

	//
	//	Animation Settings Box
	//
	this->groupBox5 = new UIX::UIXGroupBox();
	this->groupBox5->SetSize({ 402, 85 });
	this->groupBox5->SetLocation({ 338, 130 });
	this->groupBox5->SetTabIndex(4);
	this->groupBox5->SetText("Animation Settings");
	this->groupBox5->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->AddControl(this->groupBox5);
	//
	//	animationExportFormatLabel
	//
	this->label4 = new UIX::UIXLabel();
	this->label4->SetSize({ 72, 17 });
	this->label4->SetLocation({ 14, 23 });
	this->label4->SetTabIndex(11);
	this->label4->SetText("Export format");
	this->label4->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label4->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox5->AddControl(this->label4);
	//
	//	Animation Export Formats
	//
	this->ExportCastAnim = new UIX::UIXRadioButton();
	this->ExportCastAnim->SetSize({ 79, 18 });
	this->ExportCastAnim->SetLocation({ 20, 45 });
	this->ExportCastAnim->SetTabIndex(12);
	this->ExportCastAnim->SetText("Cast (Beta)");
	this->ExportCastAnim->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox5->AddControl(this->ExportCastAnim);

	this->ExportSEAnim = new UIX::UIXRadioButton();
	this->ExportSEAnim->SetSize({ 63, 18 });
	this->ExportSEAnim->SetLocation({ 189, 45 });
	this->ExportSEAnim->SetTabIndex(10);
	this->ExportSEAnim->SetText("SEAnim");
	this->ExportSEAnim->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox5->AddControl(this->ExportSEAnim);

	//
	//	Image Settings Box
	//
	this->groupBox2 = new UIX::UIXGroupBox();
	this->groupBox2->SetSize({ 402, 80 });
	this->groupBox2->SetLocation({ 338, 220 });
	this->groupBox2->SetTabIndex(1);
	this->groupBox2->SetText("Image Settings");
	this->groupBox2->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->AddControl(this->groupBox2);
	//
	//	imageExportFormatLabel
	//
	this->label2 = new UIX::UIXLabel();
	this->label2->SetSize({ 72, 17 });
	this->label2->SetLocation({ 14, 23 });
	this->label2->SetTabIndex(9);
	this->label2->SetText("Export format");
	this->label2->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label2->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox2->AddControl(this->label2);
	//
	//	Image Export Formats
	//
	this->ImageExportFormat = new UIX::UIXComboBox();
	this->ImageExportFormat->SetSize({ 205, 21 });
	this->ImageExportFormat->SetLocation({ 20, 45 });
	this->ImageExportFormat->SetTabIndex(0);
	this->ImageExportFormat->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->ImageExportFormat->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->ImageExportFormat->Items.Add("DDS");
	this->ImageExportFormat->Items.Add("PNG");
	this->ImageExportFormat->Items.Add("TIFF");
	this->groupBox2->AddControl(this->ImageExportFormat);

	//
	//	Model Settings Box
	//
	this->groupBox1 = new UIX::UIXGroupBox();
	this->groupBox1->SetSize({ 320, 170 });
	this->groupBox1->SetLocation({ 12, 130 });
	this->groupBox1->SetTabIndex(0);
	this->groupBox1->SetText("Model Settings");
	this->groupBox1->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->AddControl(this->groupBox1);
	//
	//	modelExportFormatLabel
	//
	this->label1 = new UIX::UIXLabel();
	this->label1->SetSize({ 72, 17 });
	this->label1->SetLocation({ 14, 23 });
	this->label1->SetTabIndex(8);
	this->label1->SetText("Export format");
	this->label1->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->label1->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->groupBox1->AddControl(this->label1);
	//
	//	Model Export Formats
	//
	this->ExportCastModel = new UIX::UIXRadioButton();
	this->ExportCastModel->SetSize({ 79, 18 });
	this->ExportCastModel->SetLocation({ 20, 45 });
	this->ExportCastModel->SetTabIndex(9);
	this->ExportCastModel->SetText("Cast (Beta)");
	this->ExportCastModel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox1->AddControl(this->ExportCastModel);

	this->ExportFBX = new UIX::UIXRadioButton();
	this->ExportFBX->SetSize({ 89, 18 });
	this->ExportFBX->SetLocation({ 175, 114 });
	this->ExportFBX->SetTabIndex(7);
	this->ExportFBX->SetText("Kaydara FBX");
	this->ExportFBX->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox1->AddControl(this->ExportFBX);

	this->ExportMA = new UIX::UIXRadioButton();
	this->ExportMA->SetSize({ 100, 18 });
	this->ExportMA->SetLocation({ 175, 91 });
	this->ExportMA->SetTabIndex(6);
	this->ExportMA->SetText("Autodesk Maya");
	this->ExportMA->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox1->AddControl(this->ExportMA);

	this->ExportXModel = new UIX::UIXRadioButton();
	this->ExportXModel->SetSize({ 84, 18 });
	this->ExportXModel->SetLocation({ 175, 68 });
	this->ExportXModel->SetTabIndex(5);
	this->ExportXModel->SetText("Cod XModel");
	this->ExportXModel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox1->AddControl(this->ExportXModel);

	this->ExportSMD = new UIX::UIXRadioButton();
	this->ExportSMD->SetSize({ 79, 18 });
	this->ExportSMD->SetLocation({ 175, 45 });
	this->ExportSMD->SetTabIndex(4);
	this->ExportSMD->SetText("Valve SMD");
	this->ExportSMD->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox1->AddControl(this->ExportSMD);

	this->ExportXNABinary = new UIX::UIXRadioButton();
	this->ExportXNABinary->SetSize({ 103, 18 });
	this->ExportXNABinary->SetLocation({ 20, 114 });
	this->ExportXNABinary->SetTabIndex(3);
	this->ExportXNABinary->SetText("XNALara Binary");
	this->ExportXNABinary->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox1->AddControl(this->ExportXNABinary);

	this->ExportXNAAscii = new UIX::UIXRadioButton();
	this->ExportXNAAscii->SetSize({ 101, 18 });
	this->ExportXNAAscii->SetLocation({ 20, 91 });
	this->ExportXNAAscii->SetTabIndex(2);
	this->ExportXNAAscii->SetText("XNALara ASCII");
	this->ExportXNAAscii->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox1->AddControl(this->ExportXNAAscii);

	this->ExportOBJ = new UIX::UIXRadioButton();
	this->ExportOBJ->SetSize({ 99, 18 });
	this->ExportOBJ->SetLocation({ 20, 137 });
	this->ExportOBJ->SetTabIndex(1);
	this->ExportOBJ->SetText("Wavefront OBJ");
	this->ExportOBJ->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox1->AddControl(this->ExportOBJ);

	this->ExportSEModel = new UIX::UIXRadioButton();
	this->ExportSEModel->SetSize({ 68, 18 });
	this->ExportSEModel->SetLocation({ 20, 68 });
	this->ExportSEModel->SetTabIndex(0);
	this->ExportSEModel->SetText("SEModel");
	this->ExportSEModel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->groupBox1->AddControl(this->ExportSEModel);

	this->ResumeLayout(false);
	this->PerformLayout();
	// END DESIGNER CODE

	this->SetBackColor({ 30, 32, 55 });

	this->Load += &OnLoad;
	this->FormClosing += &OnClose;
	//this->DonateButton->Click += &OnDonateClick;
	//this->TwitterButton->Click += &OnTwitterClick;
	this->ExportBrowseButton->Click += &OnBrowseClick;
}

void LegionSettings::LoadSettings()
{
	auto ModelFormat = (RpakModelExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ModelFormat");
	auto AnimFormat = (RpakAnimExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("AnimFormat");
	auto ImageFormat = (RpakImageExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ImageFormat");

	switch (ModelFormat)
	{
	case RpakModelExportFormat::SEModel:
		this->ExportSEModel->SetChecked(true);
		break;
	case RpakModelExportFormat::FBX:
		this->ExportFBX->SetChecked(true);
		break;
	case RpakModelExportFormat::Maya:
		this->ExportMA->SetChecked(true);
		break;
	case RpakModelExportFormat::OBJ:
		this->ExportOBJ->SetChecked(true);
		break;
	case RpakModelExportFormat::SMD:
		this->ExportSMD->SetChecked(true);
		break;
	case RpakModelExportFormat::XModel:
		this->ExportXModel->SetChecked(true);
		break;
	case RpakModelExportFormat::XNALaraBinary:
		this->ExportXNABinary->SetChecked(true);
		break;
	case RpakModelExportFormat::XNALaraText:
		this->ExportXNAAscii->SetChecked(true);
		break;
	case RpakModelExportFormat::Cast:
		this->ExportCastModel->SetChecked(true);
		break;
	}

	switch (AnimFormat)
	{
	case RpakAnimExportFormat::Cast:
		this->ExportCastAnim->SetChecked(true);
		break;
	case RpakAnimExportFormat::SEAnim:
		this->ExportSEAnim->SetChecked(true);
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
	auto ModelExportFormat = RpakModelExportFormat::SEModel;
	auto AnimExportFormat = RpakAnimExportFormat::SEAnim;
	auto ImageExportFormat = RpakImageExportFormat::Dds;

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

	if (ThisPtr->ExportFBX->Checked())
		ModelExportFormat = RpakModelExportFormat::FBX;
	else if (ThisPtr->ExportMA->Checked())
		ModelExportFormat = RpakModelExportFormat::Maya;
	else if (ThisPtr->ExportOBJ->Checked())
		ModelExportFormat = RpakModelExportFormat::OBJ;
	else if (ThisPtr->ExportSMD->Checked())
		ModelExportFormat = RpakModelExportFormat::SMD;
	else if (ThisPtr->ExportXModel->Checked())
		ModelExportFormat = RpakModelExportFormat::XModel;
	else if (ThisPtr->ExportXNAAscii->Checked())
		ModelExportFormat = RpakModelExportFormat::XNALaraText;
	else if (ThisPtr->ExportXNABinary->Checked())
		ModelExportFormat = RpakModelExportFormat::XNALaraBinary;
	else if (ThisPtr->ExportSEModel->Checked())
		ModelExportFormat = RpakModelExportFormat::SEModel;
	else if (ThisPtr->ExportCastModel->Checked())
		ModelExportFormat = RpakModelExportFormat::Cast;

	if (ThisPtr->ExportCastAnim->Checked())
		AnimExportFormat = RpakAnimExportFormat::Cast;
	else if (ThisPtr->ExportSEAnim->Checked())
		AnimExportFormat = RpakAnimExportFormat::SEAnim;

	ExportManager::Config.Set<System::SettingType::Boolean>("LoadModels", ThisPtr->LoadModels->Checked());
	ExportManager::Config.Set<System::SettingType::Boolean>("LoadAnimations", ThisPtr->LoadAnimations->Checked());
	ExportManager::Config.Set<System::SettingType::Boolean>("LoadImages", ThisPtr->LoadImages->Checked());
	ExportManager::Config.Set<System::SettingType::Boolean>("LoadMaterials", ThisPtr->LoadMaterials->Checked());
	ExportManager::Config.Set<System::SettingType::Boolean>("LoadUIImages", ThisPtr->LoadUIImages->Checked());
	ExportManager::Config.Set<System::SettingType::Boolean>("LoadDataTables", ThisPtr->LoadDataTables->Checked());
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

void LegionSettings::OnDonateClick(Forms::Control* Sender)
{
	Diagnostics::Process::Start("https://forum.modme.co/donate/");
}

void LegionSettings::OnTwitterClick(Forms::Control* Sender)
{
	Diagnostics::Process::Start("https://twitter.com/dtzxporter");
}

void LegionSettings::OnBrowseClick(Forms::Control* Sender)
{
	auto ThisPtr = (LegionSettings*)Sender->FindForm();
	auto ExportDirectory = ThisPtr->ExportBrowseFolder->Text();

	printf("TODO(rx): LegionSettings::OnBrowseClick is stubbed.\n");
	return;
	//auto Result = OpenFileDialog::ShowFolderDialog("Select a folder to export assets to or press \"Cancel\" to reset back to default.", ExportDirectory != "Click on \"Browse\" to set a custom export directory" ? ExportDirectory : "", ThisPtr);

	//if (Result == "" || Result.Length() == 0)
	//{
	//	ThisPtr->ExportBrowseFolder->SetText("Click on \"Browse\" to set a custom export directory");
	//}
	//else if (IO::Directory::Exists(Result))
	//{
	//	ThisPtr->ExportBrowseFolder->SetText(Result);
	//}
}
