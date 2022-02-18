#include "pch.h"
#include "LegionMain.h"

#include "UIXButton.h"
#include "UIXTextBox.h"
#include "UIXListView.h"
#include "LegionSettings.h"
#include "LegionTitanfallConverter.h"
#include "LegionTablePreview.h"

LegionMain::LegionMain()
	: Forms::Form(), IsInExportMode(false)
{
	g_pLegionMain = this;
	this->InitializeComponent();
}

void LegionMain::InitializeComponent()
{
	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText("Legion+");
	this->SetClientSize({ 775, 481 });
	this->SetMinimumSize({ 791, 520 });
	this->SetStartPosition(Forms::FormStartPosition::CenterScreen);

	this->TitanfallConverterButton = new UIX::UIXButton();
	this->TitanfallConverterButton->SetSize({ 78, 27 });
	this->TitanfallConverterButton->SetLocation({ 290, 446 });
	this->TitanfallConverterButton->SetTabIndex(9);
	this->TitanfallConverterButton->SetText("Titanfall 2");
	this->TitanfallConverterButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->TitanfallConverterButton->Click += &OnTitanfallClick;
	this->AddControl(this->TitanfallConverterButton);

	this->RefreshAssetsButton = new UIX::UIXButton();
	this->RefreshAssetsButton->SetSize({ 78, 27 });
	this->RefreshAssetsButton->SetLocation({ 374, 446 });
	this->RefreshAssetsButton->SetTabIndex(9);
	this->RefreshAssetsButton->SetText("Refresh");
	this->RefreshAssetsButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->RefreshAssetsButton->Click += &OnRefreshClick;
	this->AddControl(this->RefreshAssetsButton);

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
	this->ExportAllButton->SetLocation({ 206, 446});
	this->ExportAllButton->SetTabIndex(4);
	this->ExportAllButton->SetText("Export All");
	this->ExportAllButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->ExportAllButton->Click += &OnExpAllClick;
	this->AddControl(this->ExportAllButton);

	this->ExportSelectedButton = new UIX::UIXButton();
	this->ExportSelectedButton->SetSize({ 97, 27 });
	this->ExportSelectedButton->SetLocation({ 103, 446});
	this->ExportSelectedButton->SetTabIndex(3);
	this->ExportSelectedButton->SetText("Export Selected");
	this->ExportSelectedButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->ExportSelectedButton->Click += &OnExpClick;
	this->AddControl(this->ExportSelectedButton);

	this->LoadRPakButton = new UIX::UIXButton();
	this->LoadRPakButton->SetSize({ 85, 27 });
	this->LoadRPakButton->SetLocation({ 12, 446});
	this->LoadRPakButton->SetTabIndex(2);
	this->LoadRPakButton->SetText("Load File");
	this->LoadRPakButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->LoadRPakButton->Click += &OnLoadClick;
	this->AddControl(this->LoadRPakButton);

	this->SettingsButton = new UIX::UIXButton();
	this->SettingsButton->SetSize({ 80, 27 });
	this->SettingsButton->SetLocation({ 683, 446});
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
	this->AssetsListView->MouseClick += &OnListRightClick;
	this->AssetsListView->KeyUp += &OnListKeyUp;
	this->AssetsListView->KeyPress += &OnListKeyPressed;
	this->AddControl(this->AssetsListView);

	this->ResumeLayout(false);
	this->PerformLayout();


	this->AssetsListView->RetrieveVirtualItem += &GetVirtualItem;
	//this->SetBackColor({ 30, 32, 55 });
	this->SetBackColor({ 33, 33, 33 });
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
					Forms::MessageBox::Show("An error occured while loading the RPak.", "Legion+", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Warning);
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
				if (ThisPtr->RpakFileSystem != nullptr)
				{
					ThisPtr->RpakFileSystem->InitializeImageExporter((RpakImageExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ImageFormat"));
					ThisPtr->RpakFileSystem->InitializeModelExporter((RpakModelExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ModelFormat"));
				}
				BspLibrary->InitializeModelExporter((RpakModelExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ModelFormat"));
				BspLibrary->ExportBsp(ThisPtr->RpakFileSystem, ThisPtr->LoadPath[0], ExportManager::GetMapExportPath());

				ThisPtr->Invoke([]()
				{
					Forms::MessageBox::Show("Successfully exported the bsp map file.", "Legion+", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Information);
				});
				ThisPtr->StatusLabel->SetText("Idle");
			}
			catch (...)
			{
				ThisPtr->Invoke([]()
				{
					Forms::MessageBox::Show("An error occurred while exporting the bsp file.", "Legion+", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Warning);
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
					Forms::MessageBox::Show("An error occured while loading the MBNK.", "Legion+", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Warning);
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
		Forms::MessageBox::Show("Please select at least one asset to export.", "Legion+", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Information);
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
		Forms::MessageBox::Show("Please find some assets to export.", "Legion+", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Information);
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

void LegionMain::SetAssetError(int32_t AssetIndex)
{
	(*this->LoadedAssets)[this->DisplayIndices[AssetIndex]].Status = ApexAssetStatus::Error;
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
		auto Texture = this->RpakFileSystem->BuildPreviewTexture(Asset.Hash);
		if (Texture == nullptr)
			return;
		this->PreviewWindow->AssignPreviewImage(*Texture.get(), Asset.Name);
	}
	break;
	case ApexAssetType::Material:
	{
		auto Material = this->RpakFileSystem->BuildPreviewMaterial(Asset.Hash);
		if (Material == nullptr)
			return;
		this->PreviewWindow->AssignPreviewImage(*Material.get(), Asset.Name);
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
	auto SearchText = this->SearchBox->Text();

	if (this->RpakFileSystem != nullptr)
	{
		this->AssetsListView->SetVirtualListSize(0);

		const bool ShowModels = ExportManager::Config.Get<System::SettingType::Boolean>("LoadModels");
		const bool ShowAnimations = ExportManager::Config.Get<System::SettingType::Boolean>("LoadAnimations");
		const bool ShowImages = ExportManager::Config.Get<System::SettingType::Boolean>("LoadImages");
		const bool ShowMaterials = ExportManager::Config.Get<System::SettingType::Boolean>("LoadMaterials");
		const bool ShowUIImages = ExportManager::Config.Get<System::SettingType::Boolean>("LoadUIImages");
		const bool ShowDataTables = ExportManager::Config.Get<System::SettingType::Boolean>("LoadDataTables");

		this->LoadedAssets = this->RpakFileSystem->BuildAssetList(ShowModels, ShowAnimations, ShowImages, ShowMaterials, ShowUIImages, ShowDataTables);
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
	
	// restore the search box's text after refreshing
	// yes this makes the window flash when refreshing as it performs the search
	if (SearchText.Length() > 0)
	{
		this->SearchBox->SetText(SearchText);

		this->SearchForAssets();
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

	auto PathParts = LoadPath[0].Split("\\");

	if (this->LoadPath.Count() == 1)
		this->StatusLabel->SetText(string::Format("%s Loaded %d assets", (PathParts[PathParts.Count()-1] + ": ").ToCString(), this->DisplayIndices.Count()));
	else
		this->StatusLabel->SetText(string::Format("Loaded %d assets", this->DisplayIndices.Count()));
}

void LegionMain::OnLoadClick(Forms::Control* Sender)
{
	auto ThisPtr = (LegionMain*)Sender->FindForm();

	List<string> OpenFileD = OpenFileDialog::ShowMultiFileDialog("Legion+: Select file(s) to load", "", "Apex Legends Files (MBnk, RPak, Bsp)|*.mbnk;*.rpak;*.bsp", Sender->FindForm());

	g_Logger.Info("OpenFileD.Count() == %i\n", OpenFileD.Count());

	for (int i = 0; i < OpenFileD.Count(); i++)
	{
		g_Logger.Info("Load rpak: %s\n", OpenFileD[i].ToCString());
	}

	if (OpenFileD.Count() == 0)
		return;
	else if (OpenFileD.Count() > MAX_LOADED_FILES)
	{
		string msg = string::Format("Please select %i or fewer files.", MAX_LOADED_FILES);
		MessageBox::Show(msg, "Legion+", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Warning);
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

void LegionMain::OnRefreshClick(Forms::Control* Sender)
{
	auto ThisPtr = (LegionMain*)Sender->FindForm();

	ThisPtr->RefreshView();
}

void CopyStringToClipboard(std::string s, HWND hWnd)
{
	HANDLE hData = GlobalAlloc(GMEM_FIXED, s.length()+1);
	memcpy_s(hData, s.length() + 1, s.c_str(), s.length() + 1);
	GlobalLock(hData);

	OpenClipboard(hWnd);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hData);
	CloseClipboard();

	GlobalUnlock(hData);
	GlobalFree(hData);
}

void LegionMain::OnListRightClick(const std::unique_ptr<MouseEventArgs>& EventArgs, Forms::Control* Sender)
{
	if (EventArgs->Button != Forms::MouseButtons::Right)
		return;

	// CopyStringToClipboard causes a heap corruption

	auto ThisPtr = ((LegionMain*)Sender->FindForm());
	auto AssetsListView = ThisPtr->AssetsListView;

	auto SelectedIndices = AssetsListView->SelectedIndices();
	std::string yes = "";

	g_Logger.Info("selected asset names:\n");
	for (uint32_t i = 0; i < SelectedIndices.Count(); i++)
	{
		auto& DisplayIndex = ThisPtr->DisplayIndices[SelectedIndices[i]];
		auto& Asset = (*ThisPtr->LoadedAssets.get())[DisplayIndex];

		g_Logger.Info(Asset.Name + "\n");

		//if (i != SelectedIndices.Count() - 1)
		//	yes += Asset.Name + "\n";
		//else
		//	yes += Asset.Name;
	}


	return;

	CopyStringToClipboard(yes, Sender->GetHandle());

	g_Logger.Info("copying %i asset name%s to clipboard\n", SelectedIndices.Count(), SelectedIndices.Count() == 1 ? "" : "s");
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
		LegionMain* Form = (LegionMain*)Sender->FindForm();

		if (!Form->LoadRPakButton->Enabled())
			return;


		if (!Form->RpakFileSystem)
			return;


		auto Selected = Form->AssetsListView->SelectedIndices();

		if (Selected.Count() <= 0)
			return;

		auto& Asset = (*Form->LoadedAssets.get())[Form->DisplayIndices[Selected[0]]];



		switch (Asset.Type) {
		case ApexAssetType::DataTable: {
			if (Form->TablePreviewWindow == nullptr || Form->TablePreviewWindow->GetState(Forms::ControlStates::StateDisposed))
			{
				Form->TablePreviewWindow = std::make_unique<LegionTablePreview>();
				Form->TablePreviewWindow->Show();
				auto& RpakAsset = Form->RpakFileSystem->Assets[Asset.Hash];
				Form->TablePreviewWindow->SetDataTable(Form->RpakFileSystem->ExtractDataTable(RpakAsset));
			}


			Form->TablePreviewWindow->BringToFront();
			break;
		}
		default: {
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
			break;
		}
		}


		
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

	static const char* AssetTypes[] = { "Model", "AnimationSet", "Image", "Material", "DataTable", "Sound", "Subtitles", "ShaderSet"};
	static const Drawing::Color AssetTypesColors[] = 
	{
		Drawing::Color(0, 157, 220),  // Model
		Drawing::Color(219, 80, 74),  // AnimationSet
		Drawing::Color(202, 97, 195), // Image
		Drawing::Color(27, 153, 139), // Material
		Drawing::Color(211, 7, 247),  // DataTable
		Drawing::Color(216, 30, 91),  // Sound,
		Drawing::Color(239, 130, 13), // Subtitles
		Drawing::Color(255, 246, 138),// ShaderSet
	};

	static const char* AssetStatus[] = { "Loaded", "Exporting", "Exported", "Error" };
	static const Drawing::Color AssetStatusColors[] = 
	{
		Drawing::Color(35,  206, 107),
		Drawing::Color(144, 122, 214),
		Drawing::Color(33,  184, 235),
		Drawing::Color(255, 102, 102),
	};

	auto AssetList = ThisPtr->LoadedAssets.get();
	auto& Asset = (*AssetList)[RemappedDisplayIndex];

	EventArgs->Style.ForeColor = Drawing::Color::White;
	EventArgs->Style.BackColor = Sender->BackColor();

	switch (EventArgs->SubItemIndex)
	{
	case 0:
		EventArgs->Text = Asset.Name;
		break;
	case 1:
		EventArgs->Text = AssetTypes[(uint32_t)Asset.Type];
		EventArgs->Style.ForeColor = AssetTypesColors[(uint32_t)Asset.Type];
		break;
	case 2:
		EventArgs->Text = AssetStatus[(uint32_t)Asset.Status];
		EventArgs->Style.ForeColor = AssetStatusColors[(uint32_t)Asset.Status];
		break;
	case 3:
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

LegionMain* g_pLegionMain;
