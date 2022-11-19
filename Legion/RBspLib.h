#pragma once

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

#pragma pack(push, 1)
struct BSPHeader_t
{
	int ident;
	short version;
	short bExternal; // yea this would be better as a bool and char padding but idc so cry about it
	int mapRevision;
	int lastLump;
};
#pragma pack(pop)

enum class ApexRBspLumps
{
	TEXTURES = 0x2,
	VERTICES = 0x3,
	MODELS = 0xE,
	SURFACE_NAMES = 0xF,
	NORMALS = 0x1E,
	GAME_LUMPS = 0x23,
	VERTEX_UNLIT = 0x47,
	VERTEX_LIT_FLAT = 0x48,
	VERTEX_LIT_BUMP = 0x49,
	VERTEX_UNLIT_TS = 0x4A,
	FACES = 0x4F,
	MESHES = 0x50,
	MATERIALS = 0x52
};

enum class TF2RBspLumps
{
	TEXTURES = 0x2,
	VERTICES = 0x3,
	MODELS = 0xE,
	NORMALS = 0x1E,
	GAME_LUMPS = 0x23,
	SURFACE_NAMES = 0x2B,
	VERTEX_UNLIT = 0x47,
	VERTEX_LIT_FLAT = 0x48,
	VERTEX_LIT_BUMP = 0x49,
	VERTEX_UNLIT_TS = 0x4A,
	FACES = 0x4F,
	MESHES = 0x50,
	MATERIALS = 0x52
};

#define R2_BSP_VERSION 0x25

// A class that handles converting RBSP (*.bsp) files from Apex Legends.
class RBspLib
{
public:
	RBspLib();
	~RBspLib() = default;

	// The exporter formats for models and anims
	std::unique_ptr<Assets::Exporters::Exporter> ModelExporter;

	// Initializes a model exporter
	void InitializeModelExporter(ModelExportFormat_t Format = ModelExportFormat_t::SEModel);

	// Exports an on-disk bsp asset
	void ExportPropContainer(std::unique_ptr<IO::MemoryStream>& Stream, const string& Name, const string& Path);
	void ExportBsp(const std::unique_ptr<RpakLib>& RpakFileSystem, const string& Asset, const string& Path);

private:
	List<string> PropModelNames;

	void ExportApexBsp(const std::unique_ptr<RpakLib>& RpakFileSystem, std::unique_ptr<IO::FileStream>& Stream, BSPHeader_t Header, const string& Asset, const string& Path);
	void ExportTitanfall2Bsp(const std::unique_ptr<RpakLib>& RpakFileSystem, std::unique_ptr<IO::FileStream>& Stream, BSPHeader_t Header, const string& Asset, const string& Path);
};
