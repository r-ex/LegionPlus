#include "Kore.h"
#include "LegionMain.h"
#include "LegionSplash.h"
#include "ExportManager.h"
#include "UIXTheme.h"
#include "RpakLib.h"
#include "KoreTheme.h"
#include "RBspLib.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace System;

#if _DEBUG
int main(int argc, char* argv[])

#else
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
	Forms::Application::EnableVisualStyles();

	UIX::UIXTheme::InitializeRenderer(new Themes::KoreTheme());
	ExportManager::InitializeExporter();

	bool ShowGUI = true;

#ifndef _DEBUG
	Forms::Application::Run(new LegionSplash());

	LPWSTR* argv;
	int argc;

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc == 3)
	{
		auto Opcode = wstring(argv[1]).ToString();
		auto Parameters = wstring(argv[2]).ToString();

		if (Opcode == "--export")
		{
			auto Rpak = std::make_unique<RpakLib>();
			auto ExportAssets = List<ExportAsset>();

			Rpak->LoadRpak(Parameters);
			Rpak->PatchAssets();

			const bool ShowModels = ExportManager::Config.Get<System::SettingType::Boolean>("LoadModels");
			const bool ShowAnimations = ExportManager::Config.Get<System::SettingType::Boolean>("LoadAnimations");
			const bool ShowImages = ExportManager::Config.Get<System::SettingType::Boolean>("LoadImages");
			const bool ShowMaterials = ExportManager::Config.Get<System::SettingType::Boolean>("LoadMaterials");
			const bool ShowUIImages = ExportManager::Config.Get<System::SettingType::Boolean>("LoadUIImages");
			const bool ShowDataTables = ExportManager::Config.Get<System::SettingType::Boolean>("LoadDataTables");

			auto AssetList = Rpak->BuildAssetList(ShowModels, ShowAnimations, ShowImages, ShowMaterials, ShowUIImages, ShowDataTables);

			for (auto& Asset : *AssetList.get())
			{
				ExportAsset EAsset;
				EAsset.AssetHash = Asset.Hash;
				EAsset.AssetIndex = 0;
				ExportAssets.EmplaceBack(EAsset);
			}

			ExportManager::ExportRpakAssets(Rpak, ExportAssets, [](uint32_t i, Forms::Form*, bool) {}, [](int32_t i, Forms::Form*) -> bool { return false; }, nullptr);

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