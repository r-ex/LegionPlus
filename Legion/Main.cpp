#include "pch.h"
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
int main()
#else
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
	Forms::Application::EnableVisualStyles();

	UIX::UIXTheme::InitializeRenderer(new Themes::KoreTheme());
	ExportManager::InitializeExporter();

	bool ShowGUI = true;
	wstring sRpakFileToLoad;

	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	CommandLine cmdline(argc, argv);

#if _DEBUG
	SetConsoleTitleA("Legion+ | Console");
#endif

	if (argv) // This can fail yes, would be good to also check that.	
	{
		if (!cmdline.HasParam(L"--export") && !cmdline.HasParam(L"--list"))
		{

			if (cmdline.ArgC() >= 1) {
				sRpakFileToLoad = wstring(cmdline.GetParamAtIdx(0));
			}
			else {
#ifndef _DEBUG
				Forms::Application::Run(new LegionSplash());
#endif
			}
		}
		else
		{
			string rpakPath;

			bool bExportRpak = cmdline.HasParam(L"--export");
			bool bListRpak = cmdline.HasParam(L"--list");

			if (bExportRpak)
			{
				rpakPath = wstring(cmdline.GetParamValue(L"--export")).ToString();
			}
			else if (bListRpak)
			{
				rpakPath = wstring(cmdline.GetParamValue(L"--list")).ToString();
			}

			// handle cli stuff
			if (!string::IsNullOrEmpty(rpakPath))
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
				
				if (cmdline.HasParam(L"--overwrite"))
					ExportManager::Config.Set<System::SettingType::Boolean>("OverwriteExistingFiles", true);
				else
					ExportManager::Config.Set<System::SettingType::Boolean>("OverwriteExistingFiles", false);

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
				else
				{
					AssetList = Rpak->BuildAssetList(bLoadModels, bLoadAnims, bLoadImages, bLoadMaterials, bLoadUIImages, bLoadDataTables);
				}

				if (bExportRpak)
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
				else if (bListRpak)
				{
					string filename = IO::Path::GetFileNameWithoutExtension(rpakPath);
					ExportManager::ExportRpakAssetList(AssetList, filename);
				}

				ShowGUI = false;
			}
		}
	}
	if (ShowGUI)
	{
		LegionMain* main = new LegionMain();
		if (!wstring::IsNullOrEmpty(sRpakFileToLoad))
		{
			List<string> paks;
			paks.EmplaceBack(sRpakFileToLoad.ToString());
			main->LoadApexFile(paks);
			main->RefreshView();
		}
		Forms::Application::Run(main);

	}

	UIX::UIXTheme::ShutdownRenderer();

	return 0;
}