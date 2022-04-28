#include "pch.h"
#include "ExportManager.h"
#include "ParallelTask.h"
#include "Path.h"
#include "Directory.h"
#include "File.h"
#include "Environment.h"
#include "LegionMain.h"

System::Settings ExportManager::Config = System::Settings();
string ExportManager::ApplicationPath = "";
string ExportManager::ExportPath = "";

void ExportManager::InitializeExporter()
{
	ApplicationPath = System::Environment::GetApplicationPath();

	auto ConfigPath = IO::Path::Combine(ApplicationPath, "LegionPlus.cfg");

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
		auto ExportDirectory = Config.Get<System::SettingType::String>("ExportDirectory");

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

	if (!Config.Has("ModelFormat"))
		Config.Set<System::SettingType::Integer>("ModelFormat", (uint32_t)ModelExportFormat_t::Cast);
	if (!Config.Has("AnimFormat"))
		Config.Set<System::SettingType::Integer>("AnimFormat", (uint32_t)AnimExportFormat_t::Cast);
	if (!Config.Has("ImageFormat"))
		Config.Set<System::SettingType::Integer>("ImageFormat", (uint32_t)ImageExportFormat_t::Dds);

	if (!Config.Has("LoadModels"))
		Config.Set<System::SettingType::Boolean>("LoadModels", true);
	if (!Config.Has("LoadAnimations"))
		Config.Set<System::SettingType::Boolean>("LoadAnimations", true);
	if (!Config.Has("LoadImages"))
		Config.Set<System::SettingType::Boolean>("LoadImages", true);
	if (!Config.Has("LoadMaterials"))
		Config.Set<System::SettingType::Boolean>("LoadMaterials", true);
	if (!Config.Has("LoadUIImages"))
		Config.Set<System::SettingType::Boolean>("LoadUIImages", true);
	if (!Config.Has("LoadDataTables"))
		Config.Set<System::SettingType::Boolean>("LoadDataTables", true);
	if (!Config.Has("LoadShaderSets"))
		Config.Set<System::SettingType::Boolean>("LoadShaderSets", true);
	if (!Config.Has("OverwriteExistingFiles"))
		Config.Set<System::SettingType::Boolean>("OverwriteExistingFiles", false);
		
	Config.Save(ConfigPath);
}

void ExportManager::SaveConfigToDisk()
{
	auto ExportDirectory = Config.Get<System::SettingType::String>("ExportDirectory");

	if (IO::Directory::Exists(ExportDirectory))
	{
		ExportPath = ExportDirectory;
	}
	else
	{
		ExportPath = IO::Path::Combine(ApplicationPath, "exported_files");
		Config.Remove<System::SettingType::String>("ExportDirectory");
	}

	Config.Save(IO::Path::Combine(ApplicationPath, "LegionPlus.cfg"));
}

string ExportManager::GetMapExportPath()
{
	auto Result = IO::Path::Combine(ExportPath, "maps");
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
			auto AssetToConvert = AssetIndex++;

			if (AssetToConvert >= ExportAssets.Count())
				continue;

			auto& Asset = ExportAssets[AssetToConvert];
			auto& AudioAsset = MilesFileSystem->Assets[Asset.AssetHash];
			auto  Path = IO::Path::Combine(ExportDirectory, "sounds");
			auto  UseSubfolders = ExportManager::Config.Get<System::SettingType::Boolean>("AudioLanguageFolders");
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
				auto NewProgress = (uint32_t)(((float)(AssetToConvert + 1) / (float)ExportAssets.Count()) * 100.f);

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
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "subtitles"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "datatables"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "shadersets"));

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
			MdlFS->ExportRMdl(Asset, ExportDirectory);
		}

		CoUninitialize();
	});
}

void ExportManager::ExportRpakAssetList(std::unique_ptr<List<ApexAsset>>& AssetList, string RpakName)
{
	String ExportDirectory = IO::Path::Combine(ExportPath, "lists");
	IO::Directory::CreateDirectory(ExportDirectory);
	List<String> NameList;
	for (auto& Asset : *AssetList)
		NameList.EmplaceBack(Asset.Name);

	NameList.Sort([](const String& lhs, const String& rhs) { return lhs.Compare(rhs) < 0; });
	IO::File::WriteAllLines(IO::Path::Combine(ExportDirectory, RpakName + ".txt"), NameList);

	auto ListName = RpakName + ".txt";

	g_Logger.Info("Exported List: %s\n", ListName.ToCString());
}
