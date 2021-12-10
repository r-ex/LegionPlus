#include "LegionMain.h"

#include "UIXButton.h"
#include "UIXTextBox.h"
#include "UIXListView.h"
#include "LegionSettings.h"
#include "LegionTitanfallConverter.h"

LegionMain::LegionMain()
	: Forms::Form(), IsInExportMode(false)
{
	this->InitializeComponent();
}

void LegionMain::InitializeComponent()
{
	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText("Legion (Apex Legends Extraction Tool) DTZxPorter");
	this->SetClientSize({ 775, 481 });
	this->SetMinimumSize({ 791, 520 });
	this->SetStartPosition(Forms::FormStartPosition::CenterScreen);

	this->TitanfallConverterButton = new UIX::UIXButton();
	this->TitanfallConverterButton->SetSize({ 78, 27 });
	this->TitanfallConverterButton->SetLocation({ 290, 442 });
	this->TitanfallConverterButton->SetTabIndex(9);
	this->TitanfallConverterButton->SetText("Titanfall 2");
	this->TitanfallConverterButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->TitanfallConverterButton->Click += &OnTitanfallClick;
	this->AddControl(this->TitanfallConverterButton);

	this->ClearSearchButton = new UIX::UIXButton();
	this->ClearSearchButton->SetSize({ 85, 24 });
	this->ClearSearchButton->SetLocation({ 381, 8 });
	this->ClearSearchButton->SetTabIndex(8);
	this->ClearSearchButton->SetText("Clear");
	this->ClearSearchButton->SetVisible(false);
	this->ClearSearchButton->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->ClearSearchButton->Click += &OnClearClick;
	this->AddControl(this->ClearSearchButton);

	this->StatusLabel = new UIX::UIXLabel();
	this->StatusLabel->SetSize({ 255, 24 });
	this->StatusLabel->SetLocation({ 508, 8 });
	this->StatusLabel->SetTabIndex(7);
	this->StatusLabel->SetText("Idle");
	this->StatusLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Right);
	this->StatusLabel->SetTextAlign(Drawing::ContentAlignment::MiddleRight);
	this->AddControl(this->StatusLabel);

	this->SearchButton = new UIX::UIXButton();
	this->SearchButton->SetSize({ 85, 24 });
	this->SearchButton->SetLocation({ 290, 8 });
	this->SearchButton->SetTabIndex(6);
	this->SearchButton->SetText("Search");
	this->SearchButton->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->SearchButton->Click += &OnSearchClick;
	this->AddControl(this->SearchButton);

	this->SearchBox = new UIX::UIXTextBox();
	this->SearchBox->SetSize({ 272, 24 });
	this->SearchBox->SetLocation({ 12, 8 });
	this->SearchBox->SetTabIndex(5);
	this->SearchBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->SearchBox->KeyPress += &OnSearchKeyPressed;
	this->AddControl(this->SearchBox);

	this->ExportAllButton = new UIX::UIXButton();
	this->ExportAllButton->SetSize({ 78, 27 });
	this->ExportAllButton->SetLocation({ 206, 442 });
	this->ExportAllButton->SetTabIndex(4);
	this->ExportAllButton->SetText("Export All");
	this->ExportAllButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->ExportAllButton->Click += &OnExpAllClick;
	this->AddControl(this->ExportAllButton);

	this->ExportSelectedButton = new UIX::UIXButton();
	this->ExportSelectedButton->SetSize({ 97, 27 });
	this->ExportSelectedButton->SetLocation({ 103, 442 });
	this->ExportSelectedButton->SetTabIndex(3);
	this->ExportSelectedButton->SetText("Export Selected");
	this->ExportSelectedButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->ExportSelectedButton->Click += &OnExpClick;
	this->AddControl(this->ExportSelectedButton);

	this->LoadRPakButton = new UIX::UIXButton();
	this->LoadRPakButton->SetSize({ 85, 27 });
	this->LoadRPakButton->SetLocation({ 12, 442 });
	this->LoadRPakButton->SetTabIndex(2);
	this->LoadRPakButton->SetText("Load File");
	this->LoadRPakButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->LoadRPakButton->Click += &OnLoadClick;
	this->AddControl(this->LoadRPakButton);

	this->SettingsButton = new UIX::UIXButton();
	this->SettingsButton->SetSize({ 80, 27 });
	this->SettingsButton->SetLocation({ 683, 442 });
	this->SettingsButton->SetTabIndex(1);
	this->SettingsButton->SetText("Settings");
	this->SettingsButton->Click += &OnSettingsClick;
	this->SettingsButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Right);

	this->AddControl(this->SettingsButton);

	this->AssetsListView = new UIX::UIXListView();
	this->AssetsListView->SetSize({ 751, 398 });
	this->AssetsListView->SetLocation({ 12, 38 });
	this->AssetsListView->SetTabIndex(0);
	this->AssetsListView->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->AssetsListView->SetView(Forms::View::Details);
	this->AssetsListView->Columns.Add({ "Name", 259 });
	this->AssetsListView->Columns.Add({ "Type", 100 });
	this->AssetsListView->Columns.Add({ "Status", 100 });
	this->AssetsListView->Columns.Add({ "Info", 270 });
	this->AssetsListView->SetVirtualMode(true);
	this->AssetsListView->SetFullRowSelect(true);
	this->AssetsListView->VirtualItemsSelectionRangeChanged += &OnSelectedIndicesChanged;
	this->AssetsListView->DoubleClick += &OnListDoubleClick;
	this->AssetsListView->KeyUp += &OnListKeyUp;
	this->AssetsListView->KeyPress += &OnListKeyPressed;
	this->AddControl(this->AssetsListView);

	this->ResumeLayout(false);
	this->PerformLayout();


	this->AssetsListView->RetrieveVirtualItem += &GetVirtualItem;
	this->SetBackColor({ 30, 32, 55 });
}

void LegionMain::LoadApexFile(const List<string>& File)
{
	this->LoadPath = File;

	this->LoadRPakButton->SetEnabled(false);
	this->StatusLabel->SetText("Loading package...");

	Threading::Thread Th([](void* Data)
	{
		auto ThisPtr = (LegionMain*)Data;

		if (ThisPtr->LoadPath[0].EndsWith(".rpak"))
		{
			ThisPtr->AssetsListView->SetVirtualListSize(0);
			ThisPtr->RpakFileSystem = nullptr;
			ThisPtr->MilesFileSystem = nullptr;

			try
			{
				ThisPtr->RpakFileSystem = std::make_unique<RpakLib>();
				ThisPtr->RpakFileSystem->LoadRpaks(ThisPtr->LoadPath);
				ThisPtr->RpakFileSystem->PatchAssets();

				ThisPtr->RefreshView();
			}
			catch (...)
			{
				ThisPtr->Invoke([]()
				{
					Forms::MessageBox::Show("An error occured while loading the RPak.", "Legion", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Warning);
				});
				ThisPtr->StatusLabel->SetText("Idle");
			}
		}
		else if (ThisPtr->LoadPath[0].EndsWith(".bsp"))
		{
			ThisPtr->StatusLabel->SetText("Loading bsp...");

			auto BspLibrary = std::make_unique<RBspLib>();

			try
			{
				ThisPtr->RpakFileSystem->InitializeImageExporter((RpakImageExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ImageFormat"));
				BspLibrary->InitializeModelExporter((RpakModelExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ModelFormat"));
				BspLibrary->ExportBsp(ThisPtr->RpakFileSystem, ThisPtr->LoadPath[0], ExportManager::GetMapExportPath());

				ThisPtr->Invoke([]()
				{
					Forms::MessageBox::Show("Successfully exported the bsp map file.", "Legion", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Information);
				});
			}
			catch (...)
			{
				ThisPtr->Invoke([]()
				{
					Forms::MessageBox::Show("An error occured while exporting the bsp file.", "Legion", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Warning);
				});
			}

			ThisPtr->RefreshView();
		}
		else
		{
			ThisPtr->AssetsListView->SetVirtualListSize(0);
			ThisPtr->RpakFileSystem = nullptr;
			ThisPtr->MilesFileSystem = nullptr;

			try
			{
				ThisPtr->MilesFileSystem = std::make_unique<MilesLib>();
				ThisPtr->MilesFileSystem->Initialize();
				ThisPtr->MilesFileSystem->MountBank(ThisPtr->LoadPath[0]);

				ThisPtr->RefreshView();
			}
			catch (...)
			{
				ThisPtr->Invoke([]()
				{
					Forms::MessageBox::Show("An error occured while loading the MBNK.", "Legion", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Warning);
				});
				ThisPtr->StatusLabel->SetText("Idle");
			}
		}

		ThisPtr->LoadRPakButton->SetEnabled(true);
	});

	Th.Start(this);
}

void LegionMain::ExportSelectedAssets()
{
	if (!this->LoadRPakButton->Enabled())
		return;

	auto SelectedIndices = this->AssetsListView->SelectedIndices();

	if (SelectedIndices.Count() == 0)
	{
		Forms::MessageBox::Show("Please select at least one asset to export.", "Legion", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Information);
		return;
	}

	List<ExportAsset> AssetsToExport(SelectedIndices.Count(), true);

	for (uint32_t i = 0; i < AssetsToExport.Count(); i++)
	{
		auto& DisplayIndex = this->DisplayIndices[SelectedIndices[i]];
		auto& Asset = (*this->LoadedAssets.get())[DisplayIndex];

		AssetsToExport[i].AssetHash = Asset.Hash;
		AssetsToExport[i].AssetIndex = SelectedIndices[i];
	}

	this->ProgressWindow = std::make_unique<LegionProgress>();

	Threading::Thread([this, &AssetsToExport] {
		if (this->MilesFileSystem != nullptr)
		{
			ExportManager::ExportMilesAssets(this->MilesFileSystem, AssetsToExport, &LegionMain::ExportProgressCallback, &LegionMain::CheckStatusCallback, this);
		}
		else
		{
			ExportManager::ExportRpakAssets(this->RpakFileSystem, AssetsToExport, &LegionMain::ExportProgressCallback, &LegionMain::CheckStatusCallback, this);
		}
	}).Start();

	this->ProgressWindow->ShowDialog();
	this->AssetsListView->Refresh();
}

void LegionMain::ExportAllAssets()
{
	if (!this->LoadRPakButton->Enabled())
		return;

	if (this->DisplayIndices.Count() == 0)
	{
		Forms::MessageBox::Show("Please find some assets to export.", "Legion", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Information);
		return;
	}

	List<ExportAsset> AssetsToExport(this->DisplayIndices.Count(), true);

	for (uint32_t i = 0; i < AssetsToExport.Count(); i++)
	{
		auto& DisplayIndex = this->DisplayIndices[i];
		auto& Asset = (*this->LoadedAssets.get())[DisplayIndex];

		AssetsToExport[i].AssetHash = Asset.Hash;
		AssetsToExport[i].AssetIndex = i;
	}

	this->ProgressWindow = std::make_unique<LegionProgress>();

	Threading::Thread([this, &AssetsToExport] {
		if (this->MilesFileSystem != nullptr)
		{
			ExportManager::ExportMilesAssets(this->MilesFileSystem, AssetsToExport, &LegionMain::ExportProgressCallback, &LegionMain::CheckStatusCallback, this);
		}
		else
		{
			ExportManager::ExportRpakAssets(this->RpakFileSystem, AssetsToExport, &LegionMain::ExportProgressCallback, &LegionMain::CheckStatusCallback, this);
		}
	}).Start();

	this->ProgressWindow->ShowDialog();
	this->AssetsListView->Refresh();
}

void LegionMain::SearchForAssets()
{
	if (this->LoadedAssets == nullptr)
		return;

	auto SearchText = this->SearchBox->Text().ToLower();

	if (string::IsNullOrEmpty(SearchText))
	{
		this->ResetDisplayIndices();
		return;
	}

	bool isBlackList = SearchText.StartsWith("!");
	if (isBlackList)
		SearchText = SearchText.Substring(1);

	auto SearchMap = SearchText.Split(",");

	for (auto& Search : SearchMap)
		Search = Search.Trim();

	List<uint32_t> SearchResults;
	uint32_t CurrentIndex = 0;

	for (auto& Asset : *LoadedAssets)
	{
		auto AssetNameLowercase = Asset.Name.ToLower();
		bool IsMatch = isBlackList;

		for (auto& Search : SearchMap)
		{
			auto Result = AssetNameLowercase.Contains(Search);

			if (!isBlackList && Result)
			{
				IsMatch = true;
				break;
			}
			else if (isBlackList && Result)
			{
				IsMatch = false;
				break;
			}
		}

		if (IsMatch)
			SearchResults.Add(CurrentIndex);

		CurrentIndex++;
	}

	this->AssetsListView->SetVirtualListSize(0);

	this->DisplayIndices = std::move(SearchResults);

	this->AssetsListView->SetVirtualListSize(this->DisplayIndices.Count());
	this->AssetsListView->Refresh();

	this->StatusLabel->SetText(string::Format("Found %d assets", this->DisplayIndices.Count()));
	this->ClearSearchButton->SetVisible(true);
}

void LegionMain::ExportSingleAsset()
{
	if (!this->LoadRPakButton->Enabled())
		return;

	if (this->IsInExportMode)
		return;

	this->IsInExportMode = true;

	if (this->LoadedAssets == nullptr)
		return;

	auto SelectedIndices = this->AssetsListView->SelectedIndices();

	if (SelectedIndices.Count() == 0)
		return;

	List<ExportAsset> AssetsToExport(1, true);

	auto& DisplayIndex = this->DisplayIndices[SelectedIndices[0]];
	auto& Asset = (*this->LoadedAssets.get())[DisplayIndex];

	Asset.Status = ApexAssetStatus::Exporting;
	this->AssetsListView->Refresh();

	AssetsToExport[0].AssetHash = Asset.Hash;
	AssetsToExport[0].AssetIndex = SelectedIndices[0];

	this->ProgressWindow = nullptr;

	Threading::Thread([this, &AssetsToExport] {
		if (this->MilesFileSystem != nullptr)
		{
			ExportManager::ExportMilesAssets(this->MilesFileSystem, AssetsToExport, &LegionMain::ExportProgressCallback, &LegionMain::CheckStatusCallback, this);
		}
		else
		{
			ExportManager::ExportRpakAssets(this->RpakFileSystem, AssetsToExport, &LegionMain::ExportProgressCallback, &LegionMain::CheckStatusCallback, this);
		}
	}).Start();
}

void LegionMain::ExportProgressChanged(uint32_t Progress, bool Finished)
{
	if (Finished)
		this->IsInExportMode = false;

	if (this->ProgressWindow == nullptr)
	{
		if (Finished)
			this->AssetsListView->Refresh();

		return;
	}

	this->ProgressWindow->UpdateProgress(Progress, Finished);
}

bool LegionMain::CheckStatus(int32_t AssetIndex)
{
	if (AssetIndex < 0)
		return (this->ProgressWindow != nullptr) ? this->ProgressWindow->IsCanceled() : false;

	(*this->LoadedAssets)[this->DisplayIndices[AssetIndex]].Status = ApexAssetStatus::Exported;

	return (this->ProgressWindow != nullptr) ? this->ProgressWindow->IsCanceled() : false;
}

void LegionMain::DoPreviewSwap()
{
	if (!this->RpakFileSystem || !this->PreviewWindow || this->PreviewWindow->GetHandle() == nullptr)
		return;

	auto Selected = this->AssetsListView->SelectedIndices();

	if (Selected.Count() <= 0)
		return;

	auto& Asset = (*this->LoadedAssets.get())[this->DisplayIndices[Selected[0]]];

	switch (Asset.Type)
	{
	case ApexAssetType::Model:
	{
		auto Mdl = this->RpakFileSystem->BuildPreviewModel(Asset.Hash);
		if (Mdl == nullptr)
			return;
		this->PreviewWindow->AssignPreviewModel(*Mdl.get(), Asset.Name);
	}
	break;
	case ApexAssetType::Image:
	{
		auto Txt = this->RpakFileSystem->BuildPreviewTexture(Asset.Hash);
		if (Txt == nullptr)
			return;
		this->PreviewWindow->AssignPreviewImage(*Txt.get(), Asset.Name);
	}
	break;
	case ApexAssetType::Material:
	{
		auto Txt = this->RpakFileSystem->BuildPreviewMaterial(Asset.Hash);
		if (Txt == nullptr)
			return;
		this->PreviewWindow->AssignPreviewImage(*Txt.get(), Asset.Name);
	}
	break;
	}


}

std::unique_ptr<Assets::Texture> LegionMain::MaterialStreamCallback(string Source, uint64_t Hash)
{
	if (!this->RpakFileSystem)
		return nullptr;
	return this->RpakFileSystem->BuildPreviewTexture(Hash);
}

void LegionMain::RefreshView()
{
	if (this->RpakFileSystem != nullptr)
	{
		this->AssetsListView->SetVirtualListSize(0);

		const bool ShowModels = ExportManager::Config.Get<System::SettingType::Boolean>("LoadModels");
		const bool ShowAnimations = ExportManager::Config.Get<System::SettingType::Boolean>("LoadAnimations");
		const bool ShowImages = ExportManager::Config.Get<System::SettingType::Boolean>("LoadImages");
		const bool ShowMaterials = ExportManager::Config.Get<System::SettingType::Boolean>("LoadMaterials");
		const bool ShowUIImages = ExportManager::Config.Get<System::SettingType::Boolean>("LoadUIImages");

		this->LoadedAssets = this->RpakFileSystem->BuildAssetList(ShowModels, ShowAnimations, ShowImages, ShowMaterials, ShowUIImages);
		this->LoadedAssets->Sort([](const ApexAsset& lhs, const ApexAsset& rhs) { return lhs.Name.Compare(rhs.Name) < 0; });

		this->ResetDisplayIndices();
	}
	else if (this->MilesFileSystem != nullptr)
	{
		this->AssetsListView->SetVirtualListSize(0);

		this->LoadedAssets = this->MilesFileSystem->BuildAssetList();
		this->LoadedAssets->Sort([](const ApexAsset& lhs, const ApexAsset& rhs) { return lhs.Name.Compare(rhs.Name) < 0; });

		this->ResetDisplayIndices();
	}
}

void LegionMain::ResetDisplayIndices()
{
	this->AssetsListView->SetVirtualListSize(0);

	this->SearchBox->SetText("");
	this->ClearSearchButton->SetVisible(false);

	List<uint32_t> TempIndices(this->LoadedAssets->Count(), true);
	for (uint32_t i = 0; i < this->LoadedAssets->Count(); i++)
		TempIndices[i] = i;

	this->DisplayIndices = std::move(TempIndices);

	this->AssetsListView->SetVirtualListSize(this->DisplayIndices.Count());
	this->AssetsListView->Refresh();

	this->StatusLabel->SetText(string::Format("Loaded %d assets", this->DisplayIndices.Count()));
}

void LegionMain::OnLoadClick(Forms::Control* Sender)
{
	auto ThisPtr = (LegionMain*)Sender->FindForm();

	List<string> OpenFileD;
	if (ThisPtr->RpakFileSystem != nullptr)
	{
		OpenFileD =  OpenFileDialog::ShowMultiFileDialog("Legion: Select file(s) to load", "", "Apex Legends Files (MBnk, RPak, Bsp)|*.mbnk;*.rpak;*.bsp", Sender->FindForm());
	}
	else
	{
		OpenFileD = OpenFileDialog::ShowMultiFileDialog("Legion: Select file(s) to load", "", "Apex Legends Files (MBnk, RPak)|*.mbnk;*.rpak;", Sender->FindForm());
	}

	printf("OpenFileD.Count() == %i\n", OpenFileD.Count());

	if (OpenFileD.Count() == 0)
		return;
	else if (OpenFileD.Count() > 4)
	{
		MessageBox::Show("Please select 4 or less files, Legion will run out of memory processing all of the files at once.", "Legion", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Warning);
		return;
	}

	ThisPtr->LoadApexFile(OpenFileD);
}

void LegionMain::OnSettingsClick(Forms::Control* Sender)
{
	auto Settings = std::make_unique<LegionSettings>();
	Settings->ShowDialog((Forms::Form*)Sender->FindForm());
}

void LegionMain::OnExpClick(Forms::Control* Sender)
{
	((LegionMain*)Sender->FindForm())->ExportSelectedAssets();
}

void LegionMain::OnExpAllClick(Forms::Control* Sender)
{
	((LegionMain*)Sender->FindForm())->ExportAllAssets();
}

void LegionMain::OnSearchClick(Forms::Control* Sender)
{
	((LegionMain*)Sender->FindForm())->SearchForAssets();
}

void LegionMain::OnClearClick(Forms::Control* Sender)
{
	((LegionMain*)Sender->FindForm())->ResetDisplayIndices();
}

void LegionMain::OnTitanfallClick(Forms::Control* Sender)
{
	auto Titanfall = std::make_unique<LegionTitanfallConverter>();
	Titanfall->ShowDialog((Forms::Form*)Sender->FindForm());
}

void LegionMain::OnListDoubleClick(Forms::Control* Sender)
{
	((LegionMain*)Sender->FindForm())->ExportSingleAsset();
}

void LegionMain::OnListKeyUp(const std::unique_ptr<KeyEventArgs>& EventArgs, Forms::Control* Sender)
{
	if (EventArgs->KeyCode() == Keys::E)
	{
		((LegionMain*)Sender->FindForm())->ExportSingleAsset();
	}
	else if (EventArgs->KeyCode() == Keys::P)
	{
		auto Form = (LegionMain*)Sender->FindForm();

		if (!Form->LoadRPakButton->Enabled())
			return;

		if (Form->PreviewWindow == nullptr || Form->PreviewWindow->GetState(Forms::ControlStates::StateDisposed))
		{
			Form->PreviewWindow = std::make_unique<LegionPreview>();
			Form->PreviewWindow->SetMaterialStreamer([Form](string source, uint64_t hash)
			{
				return Form->MaterialStreamCallback(source, hash);
			});
			Form->PreviewWindow->Show();
		}

		Form->PreviewWindow->BringToFront();
		Form->DoPreviewSwap();
	}
}

void LegionMain::OnListKeyPressed(const std::unique_ptr<KeyPressEventArgs>& EventArgs, Forms::Control* Sender)
{
	EventArgs->SetHandled(true);
}

void LegionMain::OnSearchKeyPressed(const std::unique_ptr<KeyPressEventArgs>& EventArgs, Forms::Control* Sender)
{
	if (EventArgs->KeyChar == '\r')
	{
		EventArgs->SetHandled(true);
		((LegionMain*)Sender->FindForm())->SearchForAssets();
	}
}

void LegionMain::OnSelectedIndicesChanged(const std::unique_ptr<Forms::ListViewVirtualItemsSelectionRangeChangedEventArgs>& EventArgs, Forms::Control* Sender)
{
	((LegionMain*)Sender->FindForm())->DoPreviewSwap();
}

void LegionMain::GetVirtualItem(const std::unique_ptr<Forms::RetrieveVirtualItemEventArgs>& EventArgs, Forms::Control* Sender)
{
	auto ThisPtr = (LegionMain*)Sender->FindForm();

	if (ThisPtr->LoadedAssets == nullptr)
		return;
	if (EventArgs->ItemIndex > (int32_t)ThisPtr->DisplayIndices.Count())
		return;

	auto RemappedDisplayIndex = ThisPtr->DisplayIndices[EventArgs->ItemIndex];

	static const char* AssetTypes[] = { "Model", "AnimationSet", "Image", "Material", "DataTable", "Sound" };
	static const Drawing::Color AssetTypesColors[] = 
	{
		Drawing::Color(0, 157, 220),
		Drawing::Color(219, 80, 74),
		Drawing::Color(202, 97, 195),
		Drawing::Color(27, 153, 139),
		Drawing::Color(216, 30, 91),
	};

	static const char* AssetStatus[] = { "Loaded", "Exporting", "Exported" };
	static const Drawing::Color AssetStatusColors[] = 
	{
		Drawing::Color(35, 206, 107),
		Drawing::Color(144, 122, 214),
		Drawing::Color(33, 184, 235)
	};

	auto AssetList = ThisPtr->LoadedAssets.get();
	auto& Asset = (*AssetList)[RemappedDisplayIndex];

	EventArgs->Style.ForeColor = Drawing::Color::White;
	EventArgs->Style.BackColor = Sender->BackColor();

	switch (EventArgs->SubItemIndex)
	{
		EventArgs->Text = Asset.Name;
		break;
		EventArgs->Text = AssetTypes[(uint32_t)Asset.Type];
		EventArgs->Style.ForeColor = AssetTypesColors[(uint32_t)Asset.Type];
		break;
		EventArgs->Text = AssetStatus[(uint32_t)Asset.Status];
		EventArgs->Style.ForeColor = AssetStatusColors[(uint32_t)Asset.Status];
		break;
		EventArgs->Text = Asset.Info;
		break;
	}
}

void LegionMain::ExportProgressCallback(uint32_t Progress, Forms::Form* MainForm, bool Finished)
{
	((LegionMain*)MainForm)->ExportProgressChanged(Progress, Finished);
}

bool LegionMain::CheckStatusCallback(int32_t AssetIndex, Forms::Form* MainForm)
{
	return ((LegionMain*)MainForm)->CheckStatus(AssetIndex);
}
