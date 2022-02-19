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
	uint16_t VirtualSegmentBlockCount;		// * 0xC

	uint32_t PatchIndex;

	uint32_t DescriptorCount;
	uint32_t AssetEntryCount;
	uint32_t GuidDescriptorCount;
	uint32_t RelationsCount;

	uint8_t Unk[0x1c];
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
	uint16_t VirtualSegmentBlockCount;		// * 0xC

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
	uint32_t PageCount;		// * 0xC

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
	uint32_t PatchDataSize;				// Total size of the patch edit stream data (Following all data blocks)
	uint32_t PatchSegmentIndex;			// Index into RpakVirtualSegmentBlock[], this entire virtual block is read FIRST, before first asset
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

	uint16_t HighestPageNum; // number of the highest page that is used by this asset
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

	uint16_t Un1;
	uint16_t Un2;

	uint32_t DataSpliceIndex;			

	uint32_t Un4;
	uint32_t Un5;
	uint32_t Un6;

	uint32_t SubHeaderSize;
	uint32_t Flags;
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

	uint32_t StartSegmentIndex;
	List<RpakSegmentBlock> SegmentBlocks;

	List<string> StarpakReferences;
	Dictionary<uint64_t, uint64_t> StarpakMap;

	List<string> OptimalStarpakReferences;
	Dictionary<uint64_t, uint64_t> OptimalStarpakMap;

	uint64_t EmbeddedStarpakOffset;
	uint64_t EmbeddedStarpakSize;

	Dictionary<uint64_t, RpakApexAssetEntry> AssetHashmap;
	List<uint64_t> DescriptorList;

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

	RpakLoadAsset() = default;
	RpakLoadAsset(uint64_t NameHash, uint32_t FileIndex, uint32_t AssetType, uint32_t SubHeaderIndex, uint32_t SubHeaderOffset, uint32_t SubHeaderSize, uint32_t RawDataIndex, uint32_t RawDataOffset, uint64_t StarpakOffset, uint64_t OptimalStarpakOffset, RpakGameVersion Version, uint32_t AssetVersion);
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

enum class RpakAssetType : uint32_t
{
	Model = 0x5F6C646D,
	Texture = 0x72747874,
	TextureAnimated = 0x6e617874,
	UIIA = 0x61696975,
	DataTable = 0x6C627464,
	Settings = 0x73677473,
	Material = 0x6C74616D,
	AnimationRig = 0x67697261,
	Animation = 0x71657361,
	Subtitles = 0x74627573,
	ShaderSet = 0x73646873,
	Shader = 0x72646873,
};

enum class RpakModelExportFormat
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

enum class RpakAnimExportFormat
{
	SEAnim,
	Cast
};

enum class RpakImageExportFormat
{
	Dds,
	Png,
	Tiff
};

enum class RpakSubtitlesExportFormat
{
	CSV,
	TXT
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

// these are all of the possible texture formats in a txtr asset
// TextureHeader.Format is used as an index into this vector
static std::vector<DXGI_FORMAT> TxtrFormatToDXGI{
	DXGI_FORMAT_BC1_UNORM,
	DXGI_FORMAT_BC1_UNORM_SRGB,
	DXGI_FORMAT_BC2_UNORM,
	DXGI_FORMAT_BC2_UNORM_SRGB,
	DXGI_FORMAT_BC3_UNORM,
	DXGI_FORMAT_BC3_UNORM_SRGB,
	DXGI_FORMAT_BC4_UNORM,
	DXGI_FORMAT_BC4_SNORM,
	DXGI_FORMAT_BC5_UNORM,
	DXGI_FORMAT_BC5_SNORM,
	DXGI_FORMAT_BC6H_UF16,
	DXGI_FORMAT_BC6H_SF16,
	DXGI_FORMAT_BC7_UNORM,
	DXGI_FORMAT_BC7_UNORM_SRGB,
	DXGI_FORMAT_R32G32B32A32_FLOAT,
	DXGI_FORMAT_R32G32B32A32_UINT,
	DXGI_FORMAT_R32G32B32A32_SINT,
	DXGI_FORMAT_R32G32B32_FLOAT,
	DXGI_FORMAT_R32G32B32_UINT,
	DXGI_FORMAT_R32G32B32_SINT,
	DXGI_FORMAT_R16G16B16A16_FLOAT,
	DXGI_FORMAT_R16G16B16A16_UNORM,
	DXGI_FORMAT_R16G16B16A16_UINT,
	DXGI_FORMAT_R16G16B16A16_SNORM,
	DXGI_FORMAT_R16G16B16A16_SINT,
	DXGI_FORMAT_R32G32_FLOAT,
	DXGI_FORMAT_R32G32_UINT,
	DXGI_FORMAT_R32G32_SINT,
	DXGI_FORMAT_R10G10B10A2_UNORM,
	DXGI_FORMAT_R10G10B10A2_UINT,
	DXGI_FORMAT_R11G11B10_FLOAT,
	DXGI_FORMAT_R8G8B8A8_UNORM,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	DXGI_FORMAT_R8G8B8A8_UINT,
	DXGI_FORMAT_R8G8B8A8_SNORM,
	DXGI_FORMAT_R8G8B8A8_SINT,
	DXGI_FORMAT_R16G16_FLOAT,
	DXGI_FORMAT_R16G16_UNORM,
	DXGI_FORMAT_R16G16_UINT,
	DXGI_FORMAT_R16G16_SNORM,
	DXGI_FORMAT_R16G16_SINT,
	DXGI_FORMAT_R32_FLOAT,
	DXGI_FORMAT_R32_UINT,
	DXGI_FORMAT_R32_SINT,
	DXGI_FORMAT_R8G8_UNORM,
	DXGI_FORMAT_R8G8_UINT,
	DXGI_FORMAT_R8G8_SNORM,
	DXGI_FORMAT_R8G8_SINT,
	DXGI_FORMAT_R16_FLOAT,
	DXGI_FORMAT_R16_UNORM,
	DXGI_FORMAT_R16_UINT,
	DXGI_FORMAT_R16_SNORM,
	DXGI_FORMAT_R16_SINT,
	DXGI_FORMAT_R8_UNORM,
	DXGI_FORMAT_R8_UINT,
	DXGI_FORMAT_R8_SNORM,
	DXGI_FORMAT_R8_SINT,
	DXGI_FORMAT_A8_UNORM,
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
	DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
	DXGI_FORMAT_D32_FLOAT,
	DXGI_FORMAT_D16_UNORM,
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
	std::unique_ptr<List<ApexAsset>> BuildAssetList(bool Models, bool Anims, bool Images, bool Materials, bool UIImages, bool DataTables);
	// Builds the preview model mesh
	std::unique_ptr<Assets::Model> BuildPreviewModel(uint64_t Hash);
	// Builds the preview texture
	std::unique_ptr<Assets::Texture> BuildPreviewTexture(uint64_t Hash);
	// Builds the preview material
	std::unique_ptr<Assets::Texture> BuildPreviewMaterial(uint64_t Hash);

	// Initializes a model exporter
	void InitializeModelExporter(RpakModelExportFormat Format = RpakModelExportFormat::SEModel);
	// Initializes a anim exporter
	void InitializeAnimExporter(RpakAnimExportFormat Format = RpakAnimExportFormat::SEAnim);
	// Initializes a image exporter
	void InitializeImageExporter(RpakImageExportFormat Format = RpakImageExportFormat::Dds);

	// RpakAssetExport.cpp
	void ExportModel(const RpakLoadAsset& Asset, const string& Path, const string& AnimPath);
	void ExportMaterial(const RpakLoadAsset& Asset, const string& Path);
	void ExportTexture(const RpakLoadAsset& Asset, const string& Path, bool IncludeImageNames);
	void ExportUIIA(const RpakLoadAsset& Asset, const string& Path);
	void ExportAnimationRig(const RpakLoadAsset& Asset, const string& Path);
	void ExportDataTable(const RpakLoadAsset& Asset, const string& Path);
	void ExportSubtitles(const RpakLoadAsset& Asset, const string& Path);
	void ExportShaderSet(const RpakLoadAsset& Asset, const string& Path);
	List<List<DataTableColumnData>> ExtractDataTable(const RpakLoadAsset& Asset);

	// Used by the BSP system.
	RMdlMaterial ExtractMaterial(const RpakLoadAsset& Asset, const string& Path, bool IncludeImages, bool IncludeImageNames);

private:
	std::array<RpakFile, MAX_LOADED_FILES> LoadedFiles;
	uint32_t LoadedFileIndex;

	List<string> LoadFileQueue;

	// The exporter formats for models and anims
	std::unique_ptr<Assets::Exporters::Exporter> ModelExporter;
	std::unique_ptr<Assets::Exporters::Exporter> AnimExporter;

	// The exporter formats for images
	imstring ImageExtension;
	Assets::SaveFileType ImageSaveType;

	std::unique_ptr<IO::MemoryStream> GetFileStream(const RpakLoadAsset& Asset);
	bool IsValidDescriptor(const RpakLoadAsset& Asset, uint32_t SegmentIndex, uint32_t SegmentOffset);
	uint64_t GetFileOffset(const RpakLoadAsset& Asset, uint32_t SegmentIndex, uint32_t SegmentOffset);
	uint64_t GetEmbeddedStarpakOffset(const RpakLoadAsset& asset);
	std::unique_ptr<IO::FileStream> GetStarpakStream(const RpakLoadAsset& Asset, bool Optimal);

	void BuildModelInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildAnimInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildRawAnimInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildMaterialInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildTextureInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildUIIAInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildDataTableInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildSubtitleInfo(const RpakLoadAsset& Asset, ApexAsset& Info);
	void BuildShaderSetInfo(const RpakLoadAsset& Asset, ApexAsset& Info);

	// RpakAssetExtract.cpp
	std::unique_ptr<Assets::Model> ExtractModel(const RpakLoadAsset& Asset, const string& Path, const string& AnimPath, bool IncludeMaterials, bool IncludeAnimations);
	void ExtractModelLod(IO::BinaryReader& Reader, const std::unique_ptr<IO::MemoryStream>& RpakStream, string Name, uint64_t Offset, const std::unique_ptr<Assets::Model>& Model, RMdlFixupPatches& Fixup, uint32_t SubHeaderSize, bool IncludeMaterials);
	void ExtractTexture(const RpakLoadAsset& Asset, std::unique_ptr<Assets::Texture>& Texture, string& Name);
	void ExtractUIIA(const RpakLoadAsset& Asset, std::unique_ptr<Assets::Texture>& Texture);
	void ExtractAnimation(const RpakLoadAsset& Asset, const List<Assets::Bone>& Skeleton, const string& Path);
	List<Assets::Bone> ExtractSkeleton(IO::BinaryReader& Reader, uint64_t SkeletonOffset);
	//List<List<DataTableColumnData>> ExtractDataTable(const RpakLoadAsset& Asset);
	List<SubtitleEntry> ExtractSubtitles(const RpakLoadAsset& Asset);
	void ExtractShader(const RpakLoadAsset& Asset, const string& Path);

	string GetSubtitlesNameFromHash(uint64_t Hash);
	void ParseRAnimBoneTranslationTrack(const RAnimBoneFlag& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex);
	void ParseRAnimBoneRotationTrack(const RAnimBoneFlag& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex);
	void ParseRAnimBoneScaleTrack(const RAnimBoneFlag& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex);

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