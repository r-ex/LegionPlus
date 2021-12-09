#pragma once

#include <cstdint>
#include <array>
#include "StringBase.h"
#include "ListBase.h"
#include "DictionaryBase.h"
#include "MemoryStream.h"
#include "FileStream.h"
#include "BinaryReader.h"

#include "RpakAssets.h"
#include "ApexAsset.h"

#include "Model.h"
#include "Exporter.h"
#include "RpakLib.h"

// A class that handles converting RBSP (*.bsp) files from Apex Legends.
class RBspLib
{
public:
	RBspLib();
	~RBspLib() = default;

	// The exporter formats for models and anims
	std::unique_ptr<Assets::Exporters::Exporter> ModelExporter;

	// Initializes a model exporter
	void InitializeModelExporter(RpakModelExportFormat Format = RpakModelExportFormat::SEModel);

	// Exports an on-disk bsp asset
	void ExportPropContainer(std::unique_ptr<IO::MemoryStream>& Stream, const string& Name, const string& Path);
	void ExportBsp(const std::unique_ptr<RpakLib>& RpakFileSystem, const string& Asset, const string& Path);
};
