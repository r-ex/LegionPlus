#pragma once

#include "Form.h"
#include "ListBase.h"
#include "ExportAsset.h"
#include "Settings.h"
#include "MilesLib.h"
#include "RpakLib.h"
#include "MdlLib.h"

typedef void (ExportProgressCallback)(uint32_t Progress, Forms::Form* MainForm, bool Finished);
typedef bool (CheckStatusCallback)(int32_t AssetIndex, Forms::Form* MainForm);

// Handles exporting assets from the various filesystems...
class ExportManager
{
public:

	// Our settings object
	static System::Settings Config;
	// Our application path
	static string ApplicationPath;
	// Our export path
	static string ExportPath;

	// Initializes the exporter (Load settings / paths)
	static void InitializeExporter();
	// Saves the current memory settings to disk
	static void SaveConfigToDisk();

	// The path used to export BSP models
	static string GetMapExportPath();

	// Handles exporting miles sound assets in parallel
	static void ExportMilesAssets(const std::unique_ptr<MilesLib>& MilesFileSystem, List<ExportAsset> ExportAssets, ExportProgressCallback ProgressCallback, CheckStatusCallback StatusCallback, Forms::Form* MainForm);
	// Handles exporting rpak assets in parallel
	static void ExportRpakAssets(const std::unique_ptr<RpakLib>& RpakFileSystem, List<ExportAsset> ExportAssets, ExportProgressCallback ProgressCallback, CheckStatusCallback StatusCallback, Forms::Form* MainForm);
	// Handles exporting vpk assets in parallel
	static void ExportMdlAssets(const std::unique_ptr<MdlLib>& MdlFS, List<string>& ExportAssets);
	// Write a list of loaded assets to disk
	static void ExportAssetList(std::unique_ptr<List<ApexAsset>>& AssetList, string RpakName, const string& FilePath);
};