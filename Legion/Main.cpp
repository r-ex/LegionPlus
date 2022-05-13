#include "pch.h"
#include "Kore.h"
#include "LegionMain.h"
#include "LegionSplash.h"
#include "ExportManager.h"
#include "UIXTheme.h"
#include "RpakLib.h"
#include "MilesLib.h"
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
	wstring sFileToLoad;

	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	CommandLine cmdline(argc, argv);

#if _DEBUG
	SetConsoleTitleA("Legion+ | Console");
#endif

	// should we create a log file for this session?
	if (!cmdline.HasParam(L"--nologfile"))
		g_Logger.InitializeLogFile();

	// set process priority level
	{
		wstring sFmt = ((wstring)cmdline.GetParamValue(L"--prioritylvl")).ToLower();

		// if the param isn't set, leave the priority alone
		if (sFmt != L"")
		{
			DWORD priorityClass = 0;
			if (sFmt == L"realtime")
				priorityClass = REALTIME_PRIORITY_CLASS;
			if (sFmt == L"high")
				priorityClass = HIGH_PRIORITY_CLASS;
			if (sFmt == L"above_normal")
				priorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
			if (sFmt == L"normal")
				priorityClass = NORMAL_PRIORITY_CLASS;
			if (sFmt == L"below_normal")
				priorityClass = BELOW_NORMAL_PRIORITY_CLASS;
			if (sFmt == L"idle")
				priorityClass = IDLE_PRIORITY_CLASS;

			SetPriorityClass(GetCurrentProcess(), priorityClass);
		}
	}

	if (cmdline.HasParam(L"--export") || cmdline.HasParam(L"--list") || cmdline.HasParam(L"--exportaudio"))
	{
		string rpakPath;

		bool bExportRpak = cmdline.HasParam(L"--export");
		bool bListRpak = cmdline.HasParam(L"--list");
		bool bExportAudio = cmdline.HasParam(L"--exportaudio");

		if (bExportRpak)
		{
			rpakPath = wstring(cmdline.GetParamValue(L"--export")).ToString();
		}
		else if (bListRpak)
		{
			rpakPath = wstring(cmdline.GetParamValue(L"--list")).ToString();
		}
		else if (bExportAudio)
		{
			rpakPath = wstring(cmdline.GetParamValue(L"--exportaudio")).ToString();
		}

		// handle cli stuff
		if (!string::IsNullOrEmpty(rpakPath))
		{
			auto Rpak = std::make_unique<RpakLib>();
			auto ExportAssets = List<ExportAsset>();

			Rpak->LoadRpak(rpakPath);
			Rpak->PatchAssets();

			// load rpak flags
			bool bLoadModels = cmdline.HasParam(L"--loadmodels");
			bool bLoadAnims = cmdline.HasParam(L"--loadanimations");
			bool bLoadImages = cmdline.HasParam(L"--loadimages");
			bool bLoadMaterials = cmdline.HasParam(L"--loadmaterials");
			bool bLoadUIImages = cmdline.HasParam(L"--loaduiimages");
			bool bLoadDataTables = cmdline.HasParam(L"--loaddatatables");
			bool bLoadShaderSets = cmdline.HasParam(L"--loadshadersets");

			// other rpak flags
			ExportManager::Config.SetBool("UseFullPaths", cmdline.HasParam(L"--fullpath"));
			ExportManager::Config.SetBool("AudioLanguageFolders", cmdline.HasParam(L"--audiolanguagefolder"));
			ExportManager::Config.SetBool("OverwriteExistingFiles", cmdline.HasParam(L"--overwrite"));
			ExportManager::Config.SetBool("UseTxtrGuids", cmdline.HasParam(L"--usetxtrguids"));

			// asset rpak formats flags
			if (cmdline.HasParam(L"--mdlfmt"))
			{
				ModelExportFormat_t MdlFmt = (ModelExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("ModelFormat");

				wstring sFmt = cmdline.GetParamValue(L"--mdlfmt");
				sFmt = sFmt.ToLower();

				if (sFmt == L"semodel")
					MdlFmt = ModelExportFormat_t::SEModel;
				if (sFmt == L"obj" || sFmt == L"wavefront")
					MdlFmt = ModelExportFormat_t::OBJ;
				if (sFmt == L"xnalara_ascii")
					MdlFmt = ModelExportFormat_t::XNALaraText;
				if (sFmt == L"xnalara_binary")
					MdlFmt = ModelExportFormat_t::XNALaraBinary;
				if (sFmt == L"smd" || sFmt == L"source")
					MdlFmt = ModelExportFormat_t::SMD;
				if (sFmt == L"xmodel")
					MdlFmt = ModelExportFormat_t::XModel;
				if (sFmt == L"maya" || sFmt == L"ma")
					MdlFmt = ModelExportFormat_t::Maya;
				if (sFmt == L"fbx")
					MdlFmt = ModelExportFormat_t::FBX;
				if (sFmt == L"cast")
					MdlFmt = ModelExportFormat_t::Cast;
				if (sFmt == L"rmdl")
					MdlFmt = ModelExportFormat_t::RMDL;

				if (MdlFmt != (ModelExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("ModelFormat"))
					ExportManager::Config.Set<System::SettingType::Integer>("ModelFormat", (uint32_t)MdlFmt);
			}

			if (cmdline.HasParam(L"--animfmt"))
			{
				AnimExportFormat_t AnimFmt = (AnimExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("AnimFormat");

				wstring sFmt = cmdline.GetParamValue(L"--animfmt");
				sFmt = sFmt.ToLower();

				if (sFmt == L"seanim")
					AnimFmt = AnimExportFormat_t::SEAnim;
				if (sFmt == L"cast")
					AnimFmt = AnimExportFormat_t::Cast;
				if (sFmt == L"ranim")
					AnimFmt = AnimExportFormat_t::RAnim;

				if (AnimFmt != (AnimExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("AnimFormat"))
					ExportManager::Config.Set<System::SettingType::Integer>("AnimFormat", (uint32_t)AnimFmt);
			}

			if (cmdline.HasParam(L"--imgfmt"))
			{
				ImageExportFormat_t ImgFmt = (ImageExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("ImageFormat");

				wstring sFmt = cmdline.GetParamValue(L"--imgfmt");
				sFmt = sFmt.ToLower();

				if (sFmt == L"dds")
					ImgFmt = ImageExportFormat_t::Dds;
				if (sFmt == L"png")
					ImgFmt = ImageExportFormat_t::Png;
				if (sFmt == L"tiff")
					ImgFmt = ImageExportFormat_t::Tiff;
				if (sFmt == L"tga")
					ImgFmt = ImageExportFormat_t::Tga;

				if (ImgFmt != (ImageExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("ImageFormat"))
					ExportManager::Config.Set<System::SettingType::Integer>("ImageFormat", (uint32_t)ImgFmt);
			}

			if (cmdline.HasParam(L"--textfmt"))
			{
				TextExportFormat_t SubtFmt = (TextExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("TextFormat");

				wstring sFmt = cmdline.GetParamValue(L"--textfmt");
				sFmt = sFmt.ToLower();

				if (sFmt == L"csv")
					SubtFmt = TextExportFormat_t::CSV;
				if (sFmt == L"txt")
					SubtFmt = TextExportFormat_t::TXT;

				if (SubtFmt != (TextExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("TextFormat"))
					ExportManager::Config.Set<System::SettingType::Integer>("TextFormat", (uint32_t)SubtFmt);
			}

			if (cmdline.HasParam(L"--nmlrecalc"))
			{
				NormalRecalcType_t NmlRecalcType = (NormalRecalcType_t)ExportManager::Config.Get<System::SettingType::Integer>("NormalRecalcType");

				wstring sFmt = cmdline.GetParamValue(L"--nmlrecalc");
				sFmt = sFmt.ToLower();

				if (sFmt == L"none")
					NmlRecalcType = NormalRecalcType_t::None;
				if (sFmt == L"directx" || sFmt == L"dx")
					NmlRecalcType = NormalRecalcType_t::DirectX;
				if (sFmt == L"opengl" || sFmt == L"ogl")
					NmlRecalcType = NormalRecalcType_t::OpenGl;

				if (NmlRecalcType != (NormalRecalcType_t)ExportManager::Config.Get<System::SettingType::Integer>("NormalRecalcType"))
					ExportManager::Config.Set<System::SettingType::Integer>("NormalRecalcType", (uint32_t)NmlRecalcType);
			}

			if (cmdline.HasParam(L"--audiolanguage"))
			{
				MilesLanguageID SubtFmt = (MilesLanguageID)ExportManager::Config.Get<System::SettingType::Integer>("AudioLanguage");

				wstring sFmt = cmdline.GetParamValue(L"--audiolanguage");
				sFmt = sFmt.ToLower();

				if (sFmt == L"english")
					SubtFmt = MilesLanguageID::English;
				if (sFmt == L"french")
					SubtFmt = MilesLanguageID::French;
				if (sFmt == L"german")
					SubtFmt = MilesLanguageID::German;
				if (sFmt == L"spanish")
					SubtFmt = MilesLanguageID::Spanish;
				if (sFmt == L"italian")
					SubtFmt = MilesLanguageID::Italian;
				if (sFmt == L"japanese")
					SubtFmt = MilesLanguageID::Japanese;
				if (sFmt == L"polish")
					SubtFmt = MilesLanguageID::Polish;
				if (sFmt == L"russian")
					SubtFmt = MilesLanguageID::Russian;
				if (sFmt == L"mandarin")
					SubtFmt = MilesLanguageID::Mandarin;
				if (sFmt == L"korean")
					SubtFmt = MilesLanguageID::Korean;

				if (SubtFmt != (MilesLanguageID)ExportManager::Config.Get<System::SettingType::Integer>("AudioLanguage"))
					ExportManager::Config.Set<System::SettingType::Integer>("AudioLanguage", (uint32_t)SubtFmt);
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
			else if (bExportAudio)
			{
				auto Audio = std::make_unique<MilesLib>();

				Audio->MountBank(rpakPath); // not actually an rpak path?
				Audio->Initialize();

				AssetList = Audio->BuildAssetList();
				for (auto& Asset : *AssetList.get())
				{
					ExportAsset EAsset;
					EAsset.AssetHash = Asset.Hash;
					EAsset.AssetIndex = 0;
					ExportAssets.EmplaceBack(EAsset);
				}
				ExportManager::ExportMilesAssets(Audio, ExportAssets, [](uint32_t i, Forms::Form*, bool) {}, [](int32_t i, Forms::Form*) -> bool { return false; }, nullptr);
			}
			else if (bListRpak)
			{
				string filename = IO::Path::GetFileNameWithoutExtension(rpakPath);
				ExportManager::ExportRpakAssetList(AssetList, filename);
			}

			ShowGUI = false;
		}
	} else {
		if (cmdline.ArgC() >= 1) {
			wstring firstParam = cmdline.GetParamAtIdx(0);
			
			// check that the first param is actually a file that exists
			// i'm sure this is bad in some way but once i push it, it's not my problem anymore
			if (IO::File::Exists(firstParam.ToString()))
				sFileToLoad = firstParam;
		}
		else {
#ifndef _DEBUG
			// splash screen only exists on release because yes
			Forms::Application::Run(new LegionSplash());
#endif
		}
	}
	g_Logger.InitializeLogFile();

	if (ShowGUI)
	{
		LegionMain* main = new LegionMain();
		if (!wstring::IsNullOrEmpty(sFileToLoad))
		{
			List<string> paks;
			paks.EmplaceBack(sFileToLoad.ToString());
			main->LoadApexFile(paks);
			main->RefreshView();
		}
		Forms::Application::Run(main);
	}

	UIX::UIXTheme::ShutdownRenderer();

	return 0;
}