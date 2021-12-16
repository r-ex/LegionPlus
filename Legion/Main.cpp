#include "Kore.h"
#include "LegionMain.h"
#include "LegionSplash.h"
#include "ExportManager.h"
#include "UIXTheme.h"
#include "RpakLib.h"
#include "KoreTheme.h"
#include "RBspLib.h"
#include "CommandLine.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace System;

#if _DEBUG
int main(int argc, char** argv)
#else
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
	Forms::Application::EnableVisualStyles();

	UIX::UIXTheme::InitializeRenderer(new Themes::KoreTheme());
	ExportManager::InitializeExporter();

	bool ShowGUI = true;

#ifndef _DEBUG
	LPWSTR* argv;
	int argc;

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	CommandLine cmdline(argc, argv);

	if (!cmdline.HasParam(L"--export") && !cmdline.HasParam(L"--list"))
	{
		Forms::Application::Run(new LegionSplash());
	}
	else {
		string rpakPath;
		if (cmdline.HasParam(L"--export"))
			rpakPath = wstring(cmdline.GetParamValue(L"--export")).ToString();
		else
			rpakPath = wstring(cmdline.GetParamValue(L"--list")).ToString();
		if (rpakPath != "")
		{
			auto Rpak = std::make_unique<RpakLib>();
			auto ExportAssets = List<ExportAsset>();

			Rpak->LoadRpak(rpakPath);
			Rpak->PatchAssets();

			bool bLoadModels = cmdline.HasParam(L"--loadmodels");
			bool bLoadAnims = cmdline.HasParam(L"--loadanimations");
			bool bLoadImages = cmdline.HasParam(L"--loadimages");
			bool bLoadMaterials = cmdline.HasParam(L"--loadmaterials");
			bool bLoadUIImages = cmdline.HasParam(L"--loaduiimages");
			bool bLoadDataTables = cmdline.HasParam(L"--loaddatatables");

			std::unique_ptr<List<ApexAsset>> AssetList;

			bool bNoFlagsSpecified = !bLoadModels && !bLoadAnims && !bLoadImages && !bLoadMaterials && !bLoadUIImages && !bLoadDataTables;

			if (bNoFlagsSpecified)
			{
				const bool ShowModels = ExportManager::Config.Get<System::SettingType::Boolean>("LoadModels");
				const bool ShowAnimations = ExportManager::Config.Get<System::SettingType::Boolean>("LoadAnimations");
				const bool ShowImages = ExportManager::Config.Get<System::SettingType::Boolean>("LoadImages");
				const bool ShowMaterials = ExportManager::Config.Get<System::SettingType::Boolean>("LoadMaterials");
				const bool ShowUIImages = ExportManager::Config.Get<System::SettingType::Boolean>("LoadUIImages");
				const bool ShowDataTables = ExportManager::Config.Get<System::SettingType::Boolean>("LoadDataTables");

				AssetList = Rpak->BuildAssetList(ShowModels, ShowAnimations, ShowImages, ShowMaterials, ShowUIImages, ShowDataTables);
			}
			else {
				AssetList = Rpak->BuildAssetList(bLoadModels, bLoadAnims, bLoadImages, bLoadMaterials, bLoadUIImages, bLoadDataTables);
			}

			if (cmdline.HasParam(L"--export"))
			{
				for (auto& Asset : *AssetList.get())
				{
					ExportAsset EAsset;
					EAsset.AssetHash = Asset.Hash;
					EAsset.AssetIndex = 0;
					ExportAssets.EmplaceBack(EAsset);
				}
				ExportManager::ExportRpakAssets(Rpak, ExportAssets, [](uint32_t i, Forms::Form*, bool) {}, [](int32_t i, Forms::Form*) -> bool { return false; }, nullptr);
			}
			else
			{
				string filename = IO::Path::GetFileNameWithoutExtension(rpakPath);
				ExportManager::ExportRpakAssetList(AssetList, filename);
			}


			ShowGUI = false;
		}
	}
#endif
	
	if (ShowGUI)
	{
		Forms::Application::Run(new LegionMain());
	}

	UIX::UIXTheme::ShutdownRenderer();

	return 0;
}