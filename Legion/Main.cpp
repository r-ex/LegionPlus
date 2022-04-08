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
		if (!cmdline.HasParam(L"--nologfile"))
			g_Logger.InitializeLogFile();

		if (cmdline.HasParam(L"--prioritylvl"))
		{

			wstring sFmt = cmdline.GetParamValue(L"--prioritylvl");
			sFmt = sFmt.ToLower();

			if (sFmt == L"realtime")
				SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
			if (sFmt == L"high")
				SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
			if (sFmt == L"above_normal")
				SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
			if (sFmt == L"normal")
				SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
			if (sFmt == L"below_normal")
				SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
			if (sFmt == L"idle")
				SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
		}

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
				bool bLoadShaderSets = cmdline.HasParam(L"--loadshadersets");
				
				if (cmdline.HasParam(L"--overwrite"))
					ExportManager::Config.Set<System::SettingType::Boolean>("OverwriteExistingFiles", true);
				else
					ExportManager::Config.Set<System::SettingType::Boolean>("OverwriteExistingFiles", false);

				if (cmdline.HasParam(L"--fullpath"))
					ExportManager::Config.Set<System::SettingType::Boolean>("UseFullPaths", true);
				else
					ExportManager::Config.Set<System::SettingType::Boolean>("UseFullPaths", false);

				// asset formats
				if (cmdline.HasParam(L"--mdlfmt"))
				{
					RpakModelExportFormat MdlFmt = (RpakModelExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ModelFormat");

					wstring sFmt = cmdline.GetParamValue(L"--mdlfmt");
					sFmt = sFmt.ToLower();

					if (sFmt == L"semodel")
						MdlFmt = RpakModelExportFormat::SEModel;
					if (sFmt == L"obj" || sFmt == L"wavefront")
						MdlFmt = RpakModelExportFormat::OBJ;
					if (sFmt == L"xnalara_ascii")
						MdlFmt = RpakModelExportFormat::XNALaraText;
					if (sFmt == L"xnalara_binary")
						MdlFmt = RpakModelExportFormat::XNALaraBinary;
					if (sFmt == L"smd" || sFmt == L"source")
						MdlFmt = RpakModelExportFormat::SMD;
					if (sFmt == L"xmodel")
						MdlFmt = RpakModelExportFormat::XModel;
					if (sFmt == L"maya" || sFmt == L"ma")
						MdlFmt = RpakModelExportFormat::Maya;
					if (sFmt == L"fbx")
						MdlFmt = RpakModelExportFormat::FBX;
					if (sFmt == L"cast")
						MdlFmt = RpakModelExportFormat::Cast;
					if (sFmt == L"rmdl")
						MdlFmt = RpakModelExportFormat::RMDL;
					
					if (MdlFmt != (RpakModelExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ModelFormat"))
						ExportManager::Config.Set<System::SettingType::Integer>("ModelFormat", (uint32_t)MdlFmt);
				}

				if (cmdline.HasParam(L"--animfmt"))
				{
					RpakAnimExportFormat AnimFmt = (RpakAnimExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("AnimFormat");

					wstring sFmt = cmdline.GetParamValue(L"--animfmt");
					sFmt = sFmt.ToLower();

					if (sFmt == L"seanim")
						AnimFmt = RpakAnimExportFormat::SEAnim;
					if (sFmt == L"cast")
						AnimFmt = RpakAnimExportFormat::Cast;
					if (sFmt == L"ranim")
						AnimFmt = RpakAnimExportFormat::RAnim;

					if (AnimFmt != (RpakAnimExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("AnimFormat"))
						ExportManager::Config.Set<System::SettingType::Integer>("AnimFormat", (uint32_t)AnimFmt);
				}

				if (cmdline.HasParam(L"--imgfmt"))
				{
					RpakImageExportFormat ImgFmt = (RpakImageExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ImageFormat");

					wstring sFmt = cmdline.GetParamValue(L"--imgfmt");
					sFmt = sFmt.ToLower();

					if (sFmt == L"dds")
						ImgFmt = RpakImageExportFormat::Dds;
					if (sFmt == L"png")
						ImgFmt = RpakImageExportFormat::Png;
					if (sFmt == L"tiff")
						ImgFmt = RpakImageExportFormat::Tiff;

					if (ImgFmt != (RpakImageExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("ImageFormat"))
						ExportManager::Config.Set<System::SettingType::Integer>("ImageFormat", (uint32_t)ImgFmt);
				}

				if (cmdline.HasParam(L"--subtitlefmt"))
				{
					RpakSubtitlesExportFormat SubtFmt = (RpakSubtitlesExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("SubtitlesFormat");

					wstring sFmt = cmdline.GetParamValue(L"--subtitlefmt");
					sFmt = sFmt.ToLower();

					if (sFmt == L"csv")
						SubtFmt = RpakSubtitlesExportFormat::CSV;
					if (sFmt == L"txt")
						SubtFmt = RpakSubtitlesExportFormat::TXT;

					if (SubtFmt != (RpakSubtitlesExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("SubtitlesFormat"))
						ExportManager::Config.Set<System::SettingType::Integer>("SubtitlesFormat", (uint32_t)SubtFmt);
				}

				if (cmdline.HasParam(L"--nmlrecalc"))
				{
					eNormalRecalcType NmlRecalcType = (eNormalRecalcType)ExportManager::Config.Get<System::SettingType::Integer>("NormalRecalcType");

					wstring sFmt = cmdline.GetParamValue(L"--nmlrecalc");
					sFmt = sFmt.ToLower();

					if (sFmt == L"none")
						NmlRecalcType = eNormalRecalcType::None;
					if (sFmt == L"directx" || sFmt == L"dx")
						NmlRecalcType = eNormalRecalcType::DirectX;
					if (sFmt == L"opengl" || sFmt == L"ogl")
						NmlRecalcType = eNormalRecalcType::OpenGl;

					if (NmlRecalcType != (eNormalRecalcType)ExportManager::Config.Get<System::SettingType::Integer>("NormalRecalcType"))
						ExportManager::Config.Set<System::SettingType::Integer>("NormalRecalcType", (uint32_t)NmlRecalcType);
				}

				std::unique_ptr<List<ApexAsset>> AssetList;

				bool bNoFlagsSpecified = !bLoadModels && !bLoadAnims && !bLoadImages && !bLoadMaterials && !bLoadUIImages && !bLoadDataTables && !bLoadShaderSets;

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
					ExportManager::Config.Set<System::SettingType::Boolean>("LoadShaderSets", bLoadShaderSets);
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
	else {
		g_Logger.InitializeLogFile();
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