#include "pch.h"
#include "ExportManager.h"
#include "ParallelTask.h"
#include "Path.h"
#include "Directory.h"
#include "File.h"
#include "Environment.h"
#include "LegionMain.h"

#define CONFIG_PATH "LegionPlus.cfg"

#define INIT_SETTING(SType, Name, Val) \
		if (!Config.Has(Name)) \
			Config.Set<System::SettingType::SType>(Name, Val);

System::Settings ExportManager::Config = System::Settings();
string ExportManager::ApplicationPath = "";
string ExportManager::ExportPath = "";

void ExportManager::InitializeExporter()
{
	ApplicationPath = System::Environment::GetApplicationPath();

	string ConfigPath = IO::Path::Combine(ApplicationPath, CONFIG_PATH);

	if (IO::File::Exists(ConfigPath))
	{
		Config.Load(ConfigPath);
	}

	if (!Config.Has("ExportDirectory"))
	{
		ExportPath = IO::Path::Combine(ApplicationPath, "exported_files");
	}
	else
	{
		string ExportDirectory = Config.Get<System::SettingType::String>("ExportDirectory");

		if (IO::Directory::Exists(ExportDirectory))
		{
			ExportPath = ExportDirectory;
		}
		else
		{
			ExportPath = IO::Path::Combine(ApplicationPath, "exported_files");
			Config.Remove<System::SettingType::String>("ExportDirectory");
		}
	}

	// init settings if they don't already exist
	INIT_SETTING(Integer, "ModelFormat", (uint32_t)ModelExportFormat_t::Cast);
	INIT_SETTING(Integer, "AnimFormat", (uint32_t)AnimExportFormat_t::Cast);
	INIT_SETTING(Integer, "ImageFormat", (uint32_t)ImageExportFormat_t::Dds);

	INIT_SETTING(Boolean, "LoadModels", true);
	INIT_SETTING(Boolean, "LoadAnimations", true);
	INIT_SETTING(Boolean, "LoadAnimationSeqs", false);
	INIT_SETTING(Boolean, "LoadImages", true);
	INIT_SETTING(Boolean, "LoadMaterials", true);
	INIT_SETTING(Boolean, "LoadUIImages", true);
	INIT_SETTING(Boolean, "LoadDataTables", true);
	INIT_SETTING(Boolean, "LoadShaderSets", true);
	INIT_SETTING(Boolean, "LoadSettingsSets", true);
	INIT_SETTING(Boolean, "LoadEffects", true);
	INIT_SETTING(Boolean, "LoadRSONs", true);
	INIT_SETTING(Boolean, "OverwriteExistingFiles", false);

	Config.Save(ConfigPath);
}

void ExportManager::SaveConfigToDisk()
{
	string ExportDirectory = Config.Get<System::SettingType::String>("ExportDirectory");

	if (IO::Directory::Exists(ExportDirectory))
	{
		ExportPath = ExportDirectory;
	}
	else
	{
		ExportPath = IO::Path::Combine(ApplicationPath, "exported_files");
		Config.Remove<System::SettingType::String>("ExportDirectory");
	}

	Config.Save(IO::Path::Combine(ApplicationPath, CONFIG_PATH));
}

string ExportManager::GetMapExportPath()
{
	string Result = IO::Path::Combine(ExportPath, "maps");
	IO::Directory::CreateDirectory(Result);
	return Result;
}

void ExportManager::ExportMilesAssets(const std::unique_ptr<MilesLib>& MilesFileSystem, List<ExportAsset> ExportAssets, ExportProgressCallback ProgressCallback, CheckStatusCallback StatusCallback, Forms::Form* MainForm)
{
	std::atomic<uint32_t> AssetIndex = 0;

	std::mutex UpdateMutex;
	uint32_t CurrentProgress = 0;
	string ExportDirectory = ExportPath;

	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "sounds"));

	Threading::ParallelTask([&MilesFileSystem, &ExportAssets, &ProgressCallback, &StatusCallback, &MainForm, &AssetIndex, &CurrentProgress, &UpdateMutex, ExportDirectory]
	{
		bool IsCancel = false;

		while (AssetIndex < ExportAssets.Count() && !IsCancel)
		{
			unsigned int AssetToConvert = AssetIndex++;

			if (AssetToConvert >= ExportAssets.Count())
				continue;

			ExportAsset& Asset = ExportAssets[AssetToConvert];
			MilesAudioAsset& AudioAsset = MilesFileSystem->Assets[Asset.AssetHash];
			string  Path = IO::Path::Combine(ExportDirectory, "sounds");
			bool  UseSubfolders = ExportManager::Config.Get<System::SettingType::Boolean>("AudioLanguageFolders");
			if (UseSubfolders && AudioAsset.LocalizeIndex != -1) {
				auto &Name = LanguageName((MilesLanguageID)AudioAsset.LocalizeIndex);
				IO::Directory::CreateDirectory(IO::Path::Combine(Path, Name));
				Path = IO::Path::Combine(Path, Name);
			}
			Path = IO::Path::Combine(Path, AudioAsset.Name + ".wav");

			bool bSuccess = MilesFileSystem->ExtractAsset(AudioAsset, Path);

			if (!bSuccess)
			{
				((LegionMain*)MainForm)->SetAssetError(Asset.AssetIndex);
				continue;
			}

			IsCancel = StatusCallback(Asset.AssetIndex, MainForm);

			{
				std::lock_guard<std::mutex> UpdateLock(UpdateMutex);
				uint32_t NewProgress = (uint32_t)(((float)(AssetToConvert + 1) / (float)ExportAssets.Count()) * 100.f);

				if (NewProgress > CurrentProgress)
				{
					CurrentProgress = NewProgress;
					ProgressCallback(NewProgress, MainForm, false);
				}
			}
		}
		ProgressCallback(100, MainForm, true);
	});

}

void ExportManager::ExportRpakAssets(const std::unique_ptr<RpakLib>& RpakFileSystem, List<ExportAsset> ExportAssets, ExportProgressCallback ProgressCallback, CheckStatusCallback StatusCallback, Forms::Form* MainForm)
{
	std::atomic<uint32_t> AssetIndex = 0;

	std::mutex UpdateMutex;
	uint32_t CurrentProgress = 0;
	string ExportDirectory = ExportPath;

	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "images"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "materials"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "models"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "animations"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "anim_sequences"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "subtitles"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "datatables"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "shadersets"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "settings"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "rson"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "rui"));

	RpakFileSystem->InitializeModelExporter((ModelExportFormat_t)Config.Get<System::SettingType::Integer>("ModelFormat"));
	RpakFileSystem->InitializeAnimExporter((AnimExportFormat_t)Config.Get<System::SettingType::Integer>("AnimFormat"));
	RpakFileSystem->InitializeImageExporter((ImageExportFormat_t)Config.Get<System::SettingType::Integer>("ImageFormat"));

	Threading::ParallelTask([&RpakFileSystem, &ExportAssets, &ProgressCallback, &StatusCallback, &MainForm, &AssetIndex, &CurrentProgress, &UpdateMutex, ExportDirectory]
	{
		(void)CoInitializeEx(0, COINIT_MULTITHREADED);

		bool IsCancel = false;

		while (AssetIndex < ExportAssets.Count() && !IsCancel)
		{
			auto AssetToConvert = AssetIndex++;

			if (AssetToConvert >= ExportAssets.Count())
				continue;

			auto& Asset = ExportAssets[AssetToConvert];
			auto& AssetToExport = RpakFileSystem->Assets[Asset.AssetHash];

			switch (AssetToExport.AssetType)
			{
			case (uint32_t)AssetType_t::Texture:
				RpakFileSystem->ExportTexture(AssetToExport, IO::Path::Combine(ExportDirectory, "images"), true);
				break;
			case (uint32_t)AssetType_t::UIIA:
				RpakFileSystem->ExportUIIA(AssetToExport, IO::Path::Combine(ExportDirectory, "images"));
				break;
			case (uint32_t)AssetType_t::Material:
				RpakFileSystem->ExportMaterial(AssetToExport, IO::Path::Combine(ExportDirectory, "materials"));
				break;
			case (uint32_t)AssetType_t::Model:
				RpakFileSystem->ExportModel(AssetToExport, IO::Path::Combine(ExportDirectory, "models"), IO::Path::Combine(ExportDirectory, "animations"));
				break;
			case (uint32_t)AssetType_t::AnimationRig:
				RpakFileSystem->ExportAnimationRig(AssetToExport, IO::Path::Combine(ExportDirectory, "animations"));
				break;
			case (uint32_t)AssetType_t::Animation:
				RpakFileSystem->ExportAnimationSeq(AssetToExport, IO::Path::Combine(ExportDirectory, "anim_sequences"));
				break;
			case (uint32_t)AssetType_t::DataTable:
				RpakFileSystem->ExportDataTable(AssetToExport, IO::Path::Combine(ExportDirectory, "datatables"));
				break;
			case (uint32_t)AssetType_t::Subtitles:
				RpakFileSystem->ExportSubtitles(AssetToExport, IO::Path::Combine(ExportDirectory, "subtitles"));
				break;
			case (uint32_t)AssetType_t::ShaderSet:
				RpakFileSystem->ExportShaderSet(AssetToExport, IO::Path::Combine(ExportDirectory, "shadersets"));
				break;
			case (uint32_t)AssetType_t::UIImageAtlas:
				RpakFileSystem->ExportUIImageAtlas(AssetToExport, IO::Path::Combine(ExportDirectory, "atlases"));
				break;
			case (uint32_t)AssetType_t::Settings:
				RpakFileSystem->ExportSettings(AssetToExport, IO::Path::Combine(ExportDirectory, "settings"));
				break;
			case (uint32_t)AssetType_t::SettingsLayout:
				RpakFileSystem->ExportSettingsLayout(AssetToExport, IO::Path::Combine(ExportDirectory, "settings_layouts"));
				break;
			case (uint32_t)AssetType_t::RSON:
				RpakFileSystem->ExportRSON(AssetToExport, IO::Path::Combine(ExportDirectory, "rson"));
				break;
			case (uint32_t)AssetType_t::RUI:
				RpakFileSystem->ExportRUI(AssetToExport, IO::Path::Combine(ExportDirectory, "rui"));
				break;
			}

			IsCancel = StatusCallback(Asset.AssetIndex, MainForm);

			{
				std::lock_guard<std::mutex> UpdateLock(UpdateMutex);
				auto NewProgress = (uint32_t)(((float)AssetToConvert / (float)ExportAssets.Count()) * 100.f);

				if (NewProgress > CurrentProgress)
				{
					CurrentProgress = NewProgress;
					ProgressCallback(NewProgress, MainForm, false);
				}
			}
		}

		CoUninitialize();
	});

	ProgressCallback(100, MainForm, true);
}

void ExportManager::ExportMdlAssets(const std::unique_ptr<MdlLib>& MdlFS, List<string>& ExportAssets)
{
	std::atomic<uint32_t> AssetIndex = 0;

	std::mutex UpdateMutex;
	uint32_t CurrentProgress = 0;
	string ExportDirectory = ExportPath;

	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "models"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "animations"));

	MdlFS->InitializeModelExporter((ModelExportFormat_t)Config.Get<System::SettingType::Integer>("ModelFormat"));
	MdlFS->InitializeAnimExporter((AnimExportFormat_t)Config.Get<System::SettingType::Integer>("AnimFormat"));

	Threading::ParallelTask([&MdlFS, &ExportAssets, &AssetIndex, &CurrentProgress, &UpdateMutex, ExportDirectory]
	{
		(void)CoInitializeEx(0, COINIT_MULTITHREADED);

		bool IsCancel = false;

		while (AssetIndex < ExportAssets.Count() && !IsCancel)
		{
			auto AssetToConvert = AssetIndex++;

			if (AssetToConvert >= ExportAssets.Count())
				continue;

			auto& Asset = ExportAssets[AssetToConvert];
			MdlFS->ExportMDLv53(Asset, ExportDirectory);
		}

		CoUninitialize();
	});
}

void ExportManager::ExportAssetList(std::unique_ptr<List<ApexAsset>>& AssetList, string RpakName, const string& FilePath)
{
	string ExportDirectory = IO::Path::Combine(ExportPath, "lists");
	IO::Directory::CreateDirectory(ExportDirectory);
	
	List<string> NameList;
	for (auto& Asset : *AssetList)
		NameList.EmplaceBack(Asset.Name);
		
	NameList.Sort([](const string& lhs, const string& rhs) { return lhs.Compare(rhs) < 0; });

	if (FilePath.EndsWith(".rpak"))
	{
		IO::File::WriteAllLines(IO::Path::Combine(ExportDirectory, RpakName + ".txt"), NameList);
		string ListName = RpakName + ".txt";

		g_Logger.Info("Exported List: %s\n", ListName.ToCString());
	}
	else if (FilePath.EndsWith(".mbnk"))
	{
		IO::File::WriteAllLines(IO::Path::Combine(ExportDirectory, "audio.txt"), NameList);

		g_Logger.Info("Exported List: audio.txt\n");
	}
}
