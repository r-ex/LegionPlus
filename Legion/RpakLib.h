#pragma once

#include "StringBase.h"
#include "ListBase.h"
#include "DictionaryBase.h"
#include "MemoryStream.h"
#include "FileStream.h"
#include "BinaryReader.h"

#include "RpakAssets.h"
#include "ApexAsset.h"

// For when using the previewer
#include "Model.h"
#include "Texture.h"
#include "Exporter.h"
#include "rmdlstructs.h"

#define MAX_LOADED_FILES 4096

#pragma pack(push, 1)
struct RpakBaseHeader
{
	uint32_t Magic;
	uint16_t Version;
	uint16_t Flags;
};

struct RpakApexHeader
{
	uint32_t Magic;
	uint16_t Version;
	uint8_t Flags;
	bool IsCompressed;
	uint64_t CreatedFileTime;
	uint64_t Hash;

	uint64_t CompressedSize;
	uint64_t EmbeddedStarpakOffset;
	uint64_t Padding;
	uint64_t DecompressedSize;
	uint64_t EmbeddedStarpakSize;
	uint64_t Padding2;

	uint16_t StarpakReferenceSize;
	uint16_t StarpakOptReferenceSize;
	uint16_t VirtualSegmentCount;			// * 0x10
	uint16_t MemPageCount;		// * 0xC

	uint32_t PatchIndex;

	uint32_t DescriptorCount;
	uint32_t AssetEntryCount;
	uint32_t GuidDescriptorCount;
	uint32_t RelationsCount;

	char Unk[0x10];
	uint32_t PageOffset; // Size not verified.
	char Unk1[0x8];
};

struct RpakTitanfallHeader
{
	uint32_t Magic;
	uint16_t Version;
	uint16_t Flags;
	uint64_t CreatedFileTime;
	uint64_t Hash;

	uint64_t CompressedSize;
	uint64_t Padding1;
	uint64_t DecompressedSize;
	uint64_t Padding2;

	uint16_t StarpakReferenceSize;
	uint16_t VirtualSegmentCount;			// * 0x10
	uint16_t MemPageCount;		// * 0xC

	uint16_t PatchIndex;

	uint32_t DescriptorCount;
	uint32_t AssetEntryCount;
	uint32_t GuidDescriptorCount;
	uint32_t UnknownSixedBlockCount;

	uint32_t UnknownSeventhBlockCount;
	uint32_t UnknownEighthBlockCount;
};

struct RpakHeaderV6
{
	uint32_t Magic;
	uint16_t Version;
	uint16_t Flags;

	uint64_t CreatedFileTime;
	uint64_t Hash;

	uint64_t FileSize;
	uint64_t Padding;
	uint64_t Padding2;

	uint32_t StarpakReferenceSize;
	uint32_t VirtualSegmentCount;			// * 0x10
	uint32_t MemPageCount;		// * 0xC

	uint32_t DescriptorCount;
	uint32_t AssetEntryCount;
	uint32_t GuidDescriptorCount;
	uint32_t UnknownSixthBlockCount;

	uint32_t UnknownSeventhBlockCount;
	uint32_t UnknownEighthBlockCount;
	uint32_t what;
};

struct RpakDescriptor
{
	uint32_t PageIdx;
	uint32_t PageOffset;
};

struct RpakVirtualSegment
{
	uint32_t DataFlag;	// Flags (Streamed, Internal...?)
	uint32_t DataType;	// What this data contains...?
	uint64_t DataSize;	// The size of the data block... (Sometimes bigger than data in-file, probably also contains streamed shit...)
};

struct RpakVirtualSegmentBlock
{
	uint32_t VirtualSegmentIndex;	// Corrosponds to a data block entry
	uint32_t Flags;					// Unknown right now
	uint32_t DataSize;				// Total size of the block
};

struct RpakUnknownBlockFive
{
	uint32_t Size;
	uint32_t Flags;
};

struct RpakFileRelation
{
	uint32_t AssetIndex;
};

struct RpakPatchHeader
{
	uint32_t PatchDataSize; // Total size of the patch edit stream data (Following all data blocks)
	uint32_t PatchSegmentIndex; // Index into RpakVirtualSegmentBlock[], this entire virtual block is read FIRST, before first asset
};

struct RpakPatchCompressPair
{
	uint64_t CompressedSize;
	uint64_t DecompressedSize;
};

struct RpakApexAssetEntry
{
	uint64_t NameHash;
	uint64_t Padding;

	uint32_t SubHeaderDataBlockIndex; // "head"
	uint32_t SubHeaderDataBlockOffset;
	uint32_t RawDataBlockIndex; // "cpu"
	uint32_t RawDataBlockOffset;

	uint64_t StarpakOffset;
	uint64_t OptimalStarpakOffset;

	uint16_t PageEnd; // number of the highest page that is used by this asset
	uint16_t Un2;

	uint32_t RelationsStartIndex;

	uint32_t UsesStartIndex;
	uint32_t RelationsCount;
	uint32_t UsesCount;

	uint32_t SubHeaderSize;
	uint32_t Version;
	uint32_t Magic;
};

struct RpakTitanfallAssetEntry
{
	uint64_t NameHash;
	uint64_t Padding;

	uint32_t SubHeaderDataBlockIndex;
	uint32_t SubHeaderDataBlockOffset;
	uint32_t RawDataBlockIndex;
	uint32_t RawDataBlockOffset;

	uint64_t StarpakOffset;

	uint16_t PageEnd;
	uint16_t Un2;

	uint32_t RelationsStartIndex;

	uint32_t UsesStartIndex;
	uint32_t RelationsCount;
	uint32_t UsesCount;

	uint32_t SubHeaderSize;
	uint32_t Version;
	uint32_t Magic;
};
#pragma pack(pop)

struct RpakSegmentBlock
{
	uint64_t Offset;
	uint64_t Size;

	RpakSegmentBlock() = default;
	RpakSegmentBlock(uint64_t Offset, uint64_t Size);
};

enum class RpakGameVersion : uint32_t
{
	R2TT      = 0x6, // Titanfall 2 Tech Test
	Titanfall = 0x7, // Titanfall 2
	Apex      = 0x8, // Apex Legends
};

class RpakFile
{
public:
	RpakFile();
	~RpakFile() = default;

	RpakGameVersion Version;

	uint64_t CreatedTime; // actually FILETIME but uint64_t is easier to compare
	uint64_t Hash;

	uint32_t StartSegmentIndex;
	List<RpakSegmentBlock> SegmentBlocks;

	List<string> StarpakReferences;
	Dictionary<uint64_t, uint64_t> StarpakMap;

	List<string> OptimalStarpakReferences;
	Dictionary<uint64_t, uint64_t> OptimalStarpakMap;

	uint64_t EmbeddedStarpakOffset;
	uint64_t EmbeddedStarpakSize;

	Dictionary<uint64_t, RpakApexAssetEntry> AssetHashmap;

	std::unique_ptr<uint8_t[]> SegmentData;
	uint64_t SegmentDataSize;

	std::unique_ptr<uint8_t[]> PatchData;
	uint64_t PatchDataSize;
};

struct RpakLoadAsset
{
	uint64_t NameHash;
	uint32_t FileIndex;
	uint32_t RpakFileIndex;
	uint32_t AssetVersion;
	RpakGameVersion Version;
	uint32_t AssetType;

	uint32_t SubHeaderIndex;
	uint32_t SubHeaderOffset;
	uint32_t SubHeaderSize;
	uint32_t RawDataIndex;
	uint32_t RawDataOffset;

	uint64_t StarpakOffset;
	uint64_t OptimalStarpakOffset;

	RpakFile* PakFile;

	RpakLoadAsset() = default;
	RpakLoadAsset(uint64_t NameHash, uint32_t FileIndex, uint32_t AssetType, uint32_t SubHeaderIndex, uint32_t SubHeaderOffset, uint32_t SubHeaderSize, uint32_t RawDataIndex, uint32_t RawDataOffset, uint64_t StarpakOffset, uint64_t OptimalStarpakOffset, RpakGameVersion Version, uint32_t AssetVersion, RpakFile* PakFile);
};

// Shared
static_assert(sizeof(RpakPatchHeader) == 0x8, "Invalid header size");
static_assert(sizeof(RpakUnknownBlockFive) == 0x8, "Invalid header size");
static_assert(sizeof(RpakFileRelation) == 0x4, "Invalid header size");
static_assert(sizeof(RpakVirtualSegment) == 0x10, "Invalid header size");

// Apex
static_assert(sizeof(RpakApexHeader) == 0x80, "Invalid header size");
static_assert(sizeof(RpakApexAssetEntry) == 0x50, "Invalid header size");

// Titanfall
static_assert(sizeof(RpakTitanfallHeader) == 0x58, "Invalid header size");
static_assert(sizeof(RpakTitanfallAssetEntry) == 0x48, "Invalid header size");

enum class AssetType_t : uint32_t
{
	Model = '_ldm', // mdl_ - 0x5F6C646D
	Texture = 'rtxt', // txtr - 0x72747874
	TextureAnimated = 'naxt', // txan - 0x6e617874
	UIIA = 'aiiu', // uiia - 0x61696975
	DataTable = 'lbtd', // dtbl - 0x6C627464
	Settings = 'sgts', // stgs - 0x73677473
	SettingsLayout = 'tlts', // stlt - 0x746c7473
	Material = 'ltam', // matl - 0x6C74616D
	AnimationRig = 'gira', // arig - 0x67697261
	Animation = 'qesa', // aseq - 0x71657361
	Subtitles = 'tbus', // subt - 0x74627573
	ShaderSet = 'sdhs', // shds - 0x73646873
	Shader = 'rdhs', // shdr - 0x72646873
	UIImageAtlas = 'gmiu', // uimg - 0x676D6975
	RSON = 'nosr', // rson - 0x72736F6E
	RUI = 'iu', // ui - 0x75690000
	Map = 'pamr', // rmap - 0x70616D72
	Effect = 'tcfe', // efct - 0x74636665
};

enum class ModelExportFormat_t
{
	SEModel,
	OBJ,
	XNALaraText,
	XNALaraBinary,
	SMD,
	XModel,
	Maya,
	FBX,
	Cast,
	RMDL
};

enum class AnimExportFormat_t
{
	SEAnim,
	Cast,
	RAnim
};

enum class MatCPUExportFormat_t
{
	None,
	Struct,
	CPU
};

enum class AudioExportFormat_t
{
	WAV,
	BinkA
};

enum class ImageExportFormat_t
{
	Dds,
	Png,
	Tiff,
	Tga,
};

enum class TextExportFormat_t
{
	CSV,
	TXT
};

enum class NormalRecalcType_t
{
	None,
	DirectX,
	OpenGl
};

// this is bad
enum class SubtitleLanguageHash : uint64_t
{
	English = 0x655f79f11377196a,
	French = 0x3d5404cd608d7068,
	German = 0x2f5ecc4608e4c647,
	Italian = 0x9234d8a930fe0ab6,
	Japanese = 0x8cabeba1b904a76b,
	Korean = 0x818477c5f5ce54dc,
	MSpanish = 0x2ccda3809873d4bd,
	Polish = 0x8d4239b4dc6f2169,
	Portuguese = 0x33b3081c5185bace,
	Russian = 0x770d3abd7e3e286,
	SChinese = 0x4468b090e02fd5e9,
	TChinese = 0x92a5954c745ecd89,
	Spanish = 0xe4d57ced5b16779,
};

static std::map<SubtitleLanguageHash, string> SubtitleLanguageMap{
	{SubtitleLanguageHash::English, "english"},
	{SubtitleLanguageHash::French, "french"},
	{SubtitleLanguageHash::German, "german"},
	{SubtitleLanguageHash::Italian, "italian"},
	{SubtitleLanguageHash::Japanese, "japanese"},
	{SubtitleLanguageHash::Korean, "korean"},
	{SubtitleLanguageHash::MSpanish, "mspanish"},
	{SubtitleLanguageHash::Polish, "polish"},
	{SubtitleLanguageHash::Portuguese, "portuguese"},
	{SubtitleLanguageHash::Russian, "russian"},
	{SubtitleLanguageHash::SChinese, "schinese"},
	{SubtitleLanguageHash::TChinese, "tchinese"},
	{SubtitleLanguageHash::Spanish, "spanish"},
};

class RpakLib
{
public:
	RpakLib();
	~RpakLib() = default;

	void LoadRpaks(const List<string>& Paths);
	void LoadRpak(const string& Path, bool Dump = false);
	void PatchAssets();

	Dictionary<uint64_t, RpakLoadAsset> Assets;

	bool m_bModelExporterInitialized = false;
	bool m_bAnimExporterInitialized = false;
	bool m_bImageExporterInitialized = false;

	// Builds the viewer list of assets
	std::unique_ptr<List<ApexAsset>> BuildAssetList(const std::array<bool, 11>& arrAssets);
	// Builds the preview model mesh
	std::unique_ptr<Assets::Model> BuildPreviewModel(uint64_t Hash);
	// Builds the preview texture
	std::unique_ptr<Assets::Texture> BuildPreviewTexture(uint64_t Hash);
	// Builds the preview material
	std::unique_ptr<Assets::Texture> BuildPreviewMaterial(uint64_t Hash);

	// Initializes a model exporter
	void InitializeModelExporter(ModelExportFormat_t Format = ModelExportFormat_t::SEModel);
	// Initializes a anim exporter
	void InitializeAnimExporter(AnimExportFormat_t Format = AnimExportFormat_t::SEAnim);
	// Initializes a image exporter
	void InitializeImageExporter(ImageExportFormat_t Format = ImageExportFormat_t::Dds);

	void ExportModel(const RpakLoadAsset& Asset, const string& Path, const string& AnimPath);
	void ExportMaterial(const RpakLoadAsset& Asset, const string& Path);
	void ExportMaterialCPU(const RpakLoadAsset& Asset, const string& Path);
	void ExportMatCPUAsStruct(const RpakLoadAsset& Asset, MaterialHeader& MatHdr, MaterialCPUHeader& MatCPUHdr, std::ofstream& oStream);
	void ExportMatCPUAsRaw(const RpakLoadAsset& Asset, MaterialHeader& MatHdr, MaterialCPUHeader& MatCPUHdr, std::ofstream& oStream);
	void ExportTexture(const RpakLoadAsset& asset, const string& path, bool includeImageNames, string nameOverride = "", bool normalRecalculate = false);
	void ExportUIIA(const RpakLoadAsset& Asset, const string& Path);
	void ExportAnimationRig(const RpakLoadAsset& Asset, const string& Path);
	void ExportAnimationRig_V5(const RpakLoadAsset& Asset, const string& Path);
	void ExportAnimationSeq(const RpakLoadAsset& Asset, const string& Path);
	void ExportDataTable(const RpakLoadAsset& Asset, const string& Path);
	void ExportSubtitles(const RpakLoadAsset& Asset, const string& Path);
	void ExportShaderSet(const RpakLoadAsset& Asset, const string& Path);
	void ExportUIImageAtlas(const RpakLoadAsset& Asset, const string& Path);
	void ExportSettings(const RpakLoadAsset& Asset, const string& Path);
	void ExportSettingsLayout(const RpakLoadAsset& Asset, const string& Path);
	void ExportRSON(const RpakLoadAsset& Asset, const string& Path);
	void ExportQC(int assetVersion, const string& Path, const string& modelPath, char* rmdlBuf, char* phyBuf = nullptr);
	void ExportRUI(const RpakLoadAsset& Asset, const string& Path);

	List<List<DataTableColumnData>> ExtractDataTable(const RpakLoadAsset& Asset);
	List<ShaderVar> ExtractShaderVars(const RpakLoadAsset& Asset, const std::string& CBufName = "", D3D_SHADER_VARIABLE_TYPE Type = D3D_SVT_FORCE_DWORD); // default value as a type that should never be used
	List<ShaderResBinding> ExtractShaderResourceBindings(const RpakLoadAsset& Asset, D3D_SHADER_INPUT_TYPE InputType);

	// Used by the BSP system.
	RMdlMaterial ExtractMaterial(const RpakLoadAsset& Asset, const string& Path, bool IncludeImages, bool IncludeImageNames);

private:
	std::array<RpakFile, MAX_LOADED_FILES> LoadedFiles;
	uint32_t LoadedFileIndex;

	List<string> LoadFileQueue;
	List<string> LoadedFilePaths;

	// The exporter formats for models and anims
	std::unique_ptr<Assets::Exporters::Exporter> ModelExporter;
	std::unique_ptr<Assets::Exporters::Exporter> AnimExporter;

	// The exporter formats for images
	imstring ImageExtension;
	Assets::SaveFileType ImageSaveType;

	std::unique_ptr<IO::MemoryStream> GetFileStream(const RpakLoadAsset& Asset);
	uint64_t GetFileOffset(const RpakLoadAsset& Asset, uint32_t SegmentIndex, uint32_t SegmentOffset);
	uint64_t GetFileOffset(const RpakLoadAsset& Asset, RPakPtr& ptr);
	uint64_t GetEmbeddedStarpakOffset(const RpakLoadAsset& asset);
	std::unique_ptr<IO::FileStream> GetStarpakStream(const RpakLoadAsset& Asset, bool Optimal);

	string ReadStringFromPointer(const RpakLoadAsset& Asset, const RPakPtr& ptr);
	string ReadStringFromPointer(const RpakLoadAsset& Asset, uint32_t index, uint32_t offset);

private:
	// purpose: set up asset list entries
	void BuildModelInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildAnimInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildRawAnimInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildMaterialInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildTextureInfo(const RpakLoadAsset& asset, ApexAsset& assetInfo);
	void BuildUIIAInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildDataTableInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildSubtitleInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildShaderSetInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildUIImageAtlasInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildSettingsInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildMapInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildEffectInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildSettingsLayoutInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildRSONInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildRUIInfo(const RpakLoadAsset& Asset, ApexAsset& Info);

	std::unique_ptr<Assets::Model> ExtractModel(const RpakLoadAsset& Asset, const string& Path, const string& AnimPath, bool IncludeMaterials, bool IncludeAnimations);
	std::unique_ptr<Assets::Model> ExtractModel_V16(const RpakLoadAsset& Asset, const string& Path, const string& AnimPath, bool IncludeMaterials, bool IncludeAnimations);
	void ExtractModelLod(IO::BinaryReader& Reader, const std::unique_ptr<IO::MemoryStream>& RpakStream, string Name, uint64_t Offset, const std::unique_ptr<Assets::Model>& Model, RMdlFixupPatches& Fixup, uint32_t Version, bool IncludeMaterials);
	void ExtractModelLod_V14(IO::BinaryReader& Reader, const std::unique_ptr<IO::MemoryStream>& RpakStream, string Name, uint64_t Offset, const std::unique_ptr<Assets::Model>& Model, RMdlFixupPatches& Fixup, uint32_t Version, bool IncludeMaterials);
	void ExtractModelLod_V16(IO::BinaryReader& Reader, const std::unique_ptr<IO::MemoryStream>& RpakStream, string Name, uint64_t Offset, const std::unique_ptr<Assets::Model>& Model, RMdlFixupPatches& Fixup, uint32_t Version, bool IncludeMaterials);
	void ExtractModelLodOld(IO::BinaryReader& Reader, const std::unique_ptr<IO::MemoryStream>& RpakStream, string Name, uint64_t Offset, const std::unique_ptr<Assets::Model>& Model, RMdlFixupPatches& Fixup, uint32_t Version, bool IncludeMaterials);
	void ExtractTexture(const RpakLoadAsset& asset, std::unique_ptr<Assets::Texture>& texture, string& name);
	void ExtractUIIA(const RpakLoadAsset& Asset, std::unique_ptr<Assets::Texture>& Texture);
	void ExtractAnimation_V11(const RpakLoadAsset& Asset, const List<Assets::Bone>& Skeleton, const string& Path);
	void ExtractAnimation(const RpakLoadAsset& Asset, const List<Assets::Bone>& Skeleton, const string& Path);
	List<Assets::Bone> ExtractSkeleton(IO::BinaryReader& Reader, uint64_t SkeletonOffset, uint32_t Version, int mdlHeaderSize = 0);
	List<Assets::Bone> ExtractSkeleton_V16(IO::BinaryReader& Reader, uint64_t SkeletonOffset, uint32_t Version, int mdlHeaderSize=0);
	//List<List<DataTableColumnData>> ExtractDataTable(const RpakLoadAsset& Asset);
	List<SubtitleEntry> ExtractSubtitles(const RpakLoadAsset& Asset);
	void ExtractShader(const RpakLoadAsset& Asset, const string& OutputDirPath, const string& Path);
	ShaderSetHeader ExtractShaderSet(const RpakLoadAsset& Asset);
	void ExtractUIImageAtlas(const RpakLoadAsset& Asset, const string& Path);
	void ExtractSettings(const RpakLoadAsset& Asset, const string& Path, const string& Name, const SettingsHeader& Header);
	SettingsLayout ExtractSettingsLayout(const RpakLoadAsset& Asset);

	string ExtractAnimationRig(const RpakLoadAsset& Asset);
	string ExtractAnimationSeq(const RpakLoadAsset& Asset);

	void ExtractRSON(const RpakLoadAsset& Asset, const string& Path);
	void ExtractRUI(const RpakLoadAsset& Asset, const string& Path);

	void ExtractTextureName(const RpakLoadAsset& asset, string& name);

	void R_WriteRSONFile(const RpakLoadAsset& Asset, std::ofstream& out, IO::BinaryReader & Reader, RSONNode node, int level);

	string GetSubtitlesNameFromHash(uint64_t Hash);
	void CalcBonePosition(const mstudio_rle_anim_t& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex);
	void CalcBoneQuaternion(const mstudio_rle_anim_t& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex);
	void CalcBoneScale(const mstudio_rle_anim_t& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex);

	bool ValidateAssetPatchStatus(const RpakLoadAsset& Asset);
	bool ValidateAssetStreamStatus(const RpakLoadAsset& Asset);

	bool MountRpak(const string& Path, bool Dump);
	void MountStarpak(const string& Path, uint32_t FileIndex, uint32_t StarpakIndex, bool Optimal);

	bool MountApexRpak(const string& Path, bool Dump);
	bool ParseApexRpak(const string& RpakPath, std::unique_ptr<IO::MemoryStream>& ParseStream);
	bool MountTitanfallRpak(const string& Path, bool Dump);
	bool ParseTitanfallRpak(const string& RpakPath, std::unique_ptr<IO::MemoryStream>& ParseStream);
	bool MountR2TTRpak(const string& Path, bool Dump);
	bool ParseR2TTRpak(const string& RpakPath, std::unique_ptr<IO::MemoryStream>& ParseStream);
};