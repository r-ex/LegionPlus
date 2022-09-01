#pragma once

#include "Vector2.h"
#include "Vector3.h"
#include "Quaternion.h"
#include "ListBase.h"
#include <d3dcommon.h>
#include "Utils.h"

// Game structures
// APEX
#pragma pack(push, 1)
union RPakPtr
{
	struct {
		uint32_t Index;
		uint32_t Offset;
	};
	uint64_t Value;
};

enum class CompressionType : uint8_t
{
	DEFAULT = 0x0,
	PAKFILE = 0x1,
	SNOWFLAKE = 0x2,
	OODLE = 0x3
};

struct TextureHeader
{
	uint64_t NameHash;
	uint32_t NameIndex;
	uint32_t NameOffset;

	uint16_t Width;
	uint16_t Height;

	uint8_t Un1;
	uint8_t Un2;
	uint8_t Format;				  // used as an index into an array of DXGI formats
	uint8_t Un3;

	uint32_t DataSize;	          // total data size across all mips
	uint8_t Unknown2;
	uint8_t MipLevelsStreamedOpt; // mips stored in .opt.starpak
	uint8_t ArraySize;
	uint8_t LayerCount;
	uint8_t Unknown4;
	uint8_t MipLevelsPermanent;   // mips stored in .rpak
	uint8_t MipLevelsStreamed;    // mips stored in .starpak

	uint8_t UnknownPad[0x15];
};

struct TextureHeaderV9
{
	uint32_t NameIndex;
	uint32_t NameOffset;
	uint16_t Format;
	uint16_t Width;
	uint16_t Height;
	uint16_t Un1;
	uint8_t ArraySize;
	uint8_t byte11;
	uint8_t gap12;
	uint8_t char13;
	uint32_t DataSize;
	uint8_t MipLevels;
	uint8_t MipLevelsStreamed;
	uint8_t MipLevelsStreamedOpt;
};

// --- txan ---
struct TextureAnimatedHeader
{
	uint32_t Index1;
	uint32_t Offset1;

	uint32_t Index2;
	uint32_t Offset2;

	uint32_t Index3;
	uint32_t Offset3;
};

// --- uiia ---
struct UIIAFlags
{
	uint8_t HasLowTable : 1;
	uint8_t LowTableBc7 : 1;
	uint8_t HasHighTable : 1;
	uint8_t HighTableBc7 : 1;
	// 0x1 = RpakDecompress
	// 0x2 = RpakDecompressSnowflake
	uint8_t CompressionType : 2;
	uint8_t SizeShift : 2;
};

struct UIIAHeader
{
	uint64_t Zero;
	uint64_t NegativeOne;

	uint16_t AtlasCount;

	UIIAFlags Flags;
	uint8_t Padding;
	uint16_t Unknown;
	uint16_t NegativeOne2;

	uint16_t Width;
	uint16_t Height;

	uint32_t Padding2;

	float FloatTable[0x8];
};

struct RUIImage
{
	uint32_t Zero[0x8];

	uint16_t HighResolutionWidth;
	uint16_t HighResolutionHeight;

	uint16_t LowResolutionWidth;
	uint16_t LowResolutionHeight;

	uint32_t BufferIndex;
	uint32_t BufferOffset;

	uint32_t Zero2[0x4];
};

// --- dtbl ---
enum DataTableColumnDataType
{
	Bool,
	Int,
	Float,
	Vector,
	StringT,
	Asset,
	AssetNoPrecache
};

struct DataTableHeader
{
	uint32_t ColumnCount;
	uint32_t RowCount;

	uint32_t ColumnHeaderBlock;
	uint32_t ColumnHeaderOffset;
	uint32_t RowHeaderBlock;
	uint32_t RowHeaderOffset;
	uint32_t UnkHash;
	
	uint16_t Un1;
	uint16_t Un2;

	uint32_t RowStride;	// Number of bytes per row
	uint32_t Padding;
};

struct DataTableColumn
{
	uint64_t Unk0Seek;
	uint64_t Unk8;
	uint32_t Type;
	uint32_t RowOffset;
};

struct DataTableColumnData
{
	DataTableColumnDataType Type;
	bool bValue = 0;
	int iValue = -1;
	float fValue = -1;
	Math::Vector3 vValue;
	string stringValue;
	string assetValue;
	string assetNPValue;
};

// --- subt ---
struct SubtitleHeader
{
	char padding[0x10];

	uint32_t EntriesIndex;
	uint32_t EntriesOffset;
};

struct SubtitleEntry
{
	Math::Vector3 Color;
	string SubtitleText;
};

// settings structs are only tested on season 3 for now
enum class SettingsFieldType : uint16_t
{
	ST_Bool,
	ST_Int,
	ST_Float,
	ST_Float2,
	ST_Float3,
	ST_String,
	ST_Asset,
	ST_Asset_2,
	ST_Array,
	ST_Array_2,
};

// --- stgs ---
struct SettingsHeader
{
	uint64_t LayoutGUID;

	RPakPtr Values;

	RPakPtr Name;

	RPakPtr StringBuf;

	uint64_t Unk1;

	RPakPtr ModNames;

	RPakPtr Unk2;

	uint32_t KvpBufferSize;

	uint8_t Unk3[0xC];
};
ASSERT_SIZE(SettingsHeader, 0x48);

struct SettingsKeyValue
{
	uint32_t BlockIndex;
	uint32_t BlockOffset;
};

struct SettingsKeyValuePair
{
	SettingsKeyValue Key;
	SettingsKeyValue Value;
};

struct SettingsLayoutHeader
{
	RPakPtr pName;
	RPakPtr pItems;
	RPakPtr unk2;
	uint32_t unk3;
	uint32_t itemsCount;
	uint32_t unk4;
	uint32_t unk5;
	uint32_t unk6;
	uint32_t unk7;
	uint32_t unk8;
	uint32_t unk9;
	RPakPtr pStringBuf;
	RPakPtr unk11;
};

struct SettingsLayoutItem
{
	SettingsFieldType type = SettingsFieldType::ST_String;
	string name;
	uint32_t valueOffset; // offset from start of stgs value buffer
};

struct SettingsLayout
{
	string name;
	unsigned int itemsCount;
	List<SettingsLayoutItem> items;
};

// --- Ptch ---
struct PatchHeader
{
	uint32_t Version;
	uint32_t Count;

	uint32_t EntriesIndex;
	uint32_t EntriesOffset;

	uint32_t PatchesIndex;
	uint32_t PatchesOffset;
};

// ANIMATIONS
// --- aseq ---
struct AnimHeader
{
	uint32_t AnimationIndex;
	uint32_t AnimationOffset;

	uint32_t NameIndex;
	uint32_t NameOffset;

	uint8_t Unknown[0x18];

	uint32_t ModelHashIndex;
	uint32_t ModelHashOffset;
};

// --- arig ---
struct AnimRigHeader
{
	uint32_t SkeletonIndex;
	uint32_t SkeletonOffset;

	uint32_t NameIndex;
	uint32_t NameOffset;

	uint32_t Unk1;
	uint32_t AnimationReferenceCount;

	uint32_t AnimationReferenceIndex;
	uint32_t AnimationReferenceOffset;

	uint32_t Unk3;
	uint32_t Unk4;
};

struct RAnimHeader
{
	uint32_t Zero;
	uint32_t NameOffset;
	uint32_t SourceOffset;
	uint32_t Flags;

	uint64_t Unknown;

	uint32_t NotetrackCount;
	uint32_t NotetrackOffset;

	float Mins[3];
	float Maxs[3];

	uint32_t AnimationCount;
	uint32_t AnimationOffset;
};

struct RAnimTitanfallHeader
{
	uint32_t Zero;
	uint32_t NameOffset;

	float Framerate;

	uint32_t Flags;
	uint32_t FrameCount;

	uint32_t Zero1;
	uint32_t Zero2;

	uint32_t UnknownOffset;
	uint32_t FirstChunkOffset;

	uint32_t UnknownCount2;
	uint32_t UnknownOffset2;

	uint32_t Zero3;
	uint32_t Zero4;

	uint32_t OffsetToChunkOffsetsTable;
	uint32_t FrameSplitCount;

	uint8_t UnknownZero[0x20];
};

struct RAnimSequenceHeader
{
	uint32_t Zero;
	uint32_t NameOffset;

	float Framerate;

	uint32_t Flags;
	uint32_t FrameCount;

	uint64_t UnknownZero;

	uint32_t FloatTableOffset;
	uint32_t FirstChunkOffset;
	uint32_t Flags2;
	uint32_t UnknownTableOffset;
	uint32_t OffsetToChunkOffsetsTable;
	uint32_t FrameSplitCount;
	uint32_t FrameMedianCount;
	uint64_t Padding;
	uint64_t SomeDataOffset;
};

struct RAnimBoneFlag
{
	uint16_t Size : 12;
	uint16_t bAdditiveCustom : 1;
	uint16_t bDynamicScale : 1;			// If zero, one per data set
	uint16_t bDynamicRotation : 1;		// If zero, one per data set
	uint16_t bDynamicTranslation : 1;	// If zero, one per data set
};

struct RAnimTitanfallBoneFlag
{
	uint8_t Unused : 1;
	uint8_t bStaticTranslation : 1;		// If zero, one per data set
	uint8_t bStaticRotation : 1;		// If zero, one per data set
	uint8_t bStaticScale : 1;			// If zero, one per data set
	uint8_t Unused2 : 1;
	uint8_t Unused3 : 1;
	uint8_t Unused4 : 1;
};

struct RAnimBoneHeader
{
	float TranslationScale;

	uint8_t BoneIndex;
	RAnimTitanfallBoneFlag BoneFlags;
	uint8_t Flags2;
	uint8_t Flags3;
	
	union
	{
		struct
		{
			uint16_t OffsetX;
			uint16_t OffsetY;
			uint16_t OffsetZ;
			uint16_t OffsetL;
		};

		uint64_t PackedRotation;
	} RotationInfo;

	uint16_t TranslationX;
	uint16_t TranslationY;
	uint16_t TranslationZ;

	uint16_t ScaleX;
	uint16_t ScaleY;
	uint16_t ScaleZ;

	uint32_t DataSize;
};

// MODELS
// --- mdl_ ---
struct ModelHeaderS50
{
	// .rmdl
	uint32_t SkeletonIndex;
	uint32_t SkeletonOffset;

	uint32_t NameIndex;
	uint32_t NameOffset;

	// .phy
	uint32_t PhyIndex;
	uint32_t PhyOffset;
	uint64_t Padding3;

	// .vvd
	uint32_t BlockIndex1;
	uint32_t BlockOffset1;
	uint64_t Padding4;

	uint32_t Padding5;
	uint32_t StreamedDataSize;
	uint32_t DataFlags;
	uint64_t Padding6;

	uint32_t AnimSequenceCount;

};

struct ModelHeaderS68
{
	// .rmdl
	uint32_t SkeletonIndex;
	uint32_t SkeletonOffset;
	uint64_t Padding;

	uint32_t NameIndex;
	uint32_t NameOffset;
	uint64_t Padding2;

	// .phy
	uint32_t PhyIndex;
	uint32_t PhyOffset;
	uint64_t Padding3;

	// .vvd
	uint32_t BlockIndex1;
	uint32_t BlockOffset1;
	uint64_t Padding4;

	uint32_t Padding5;
	uint32_t StreamedDataSize;
	uint32_t DataFlags;
	uint64_t Padding6;

	uint32_t AnimSequenceCount;
	uint32_t AnimSequenceIndex;
	uint32_t AnimSequenceOffset;

	uint64_t Padding7;
};

struct ModelHeaderS80
{
	// .rmdl
	uint32_t SkeletonIndex;
	uint32_t SkeletonOffset;
	uint64_t Padding;

	uint32_t NameIndex;
	uint32_t NameOffset;
	uint64_t Padding2;

	// .phy
	uint32_t PhyIndex;
	uint32_t PhyOffset;
	uint64_t Padding3;

	// .vvd
	// this pointer is not always registered
	// similar data will often be streamed from a mandatory starpak
	uint32_t VGIndex1;
	uint32_t VGOffset1;
	uint64_t Padding4;

	uint32_t Padding5;
	uint32_t StreamedDataSize;
	uint32_t DataFlags;

	float Box[6];
	uint64_t Padding6;

	uint32_t AnimSequenceCount;
	uint32_t AnimSequenceIndex;
	uint32_t AnimSequenceOffset;

	uint64_t Padding7;
};

struct studiohdr_t // latest studiohdr
{
	uint32_t Magic;
	uint32_t Version;
	uint32_t Hash;
	uint32_t NameTableOffset;

	char SkeletonName[0x40];

	uint32_t DataSize;

	float EyePosition[3];
	float IllumPosition[3];
	float HullMin[3];
	float HullMax[3];
	float ViewBBMin[3];
	float ViewBBMax[3];

	uint32_t Flags; // 0x9c

	uint32_t BoneCount; // 0xa0
	uint32_t BoneDataOffset; // 0xa4

	uint32_t BoneControllerCount;
	uint32_t BoneControllerOffset;

	uint32_t HitboxCount;
	uint32_t HitboxOffset;

	uint32_t LocalAnimCount;
	uint32_t LocalAnimOffset;

	uint32_t LocalSeqCount;
	uint32_t LocalSeqOffset;

	uint32_t ActivityListVersion;
	uint32_t EventsIndexed;

	uint32_t TextureCount;
	uint32_t TextureOffset;

	uint32_t TextureDirCount;
	uint32_t TextureDirOffset;

	uint32_t SkinReferenceCount;	// Total number of references (submeshes)
	uint32_t SkinFamilyCount;		// Total skins per reference
	uint32_t SkinReferenceOffset;	// Offset to data

	uint32_t BodyPartCount;
	uint32_t BodyPartOffset;

	uint32_t AttachmentCount;
	uint32_t AttachmentOffset;

	uint8_t Unknown2[0x14];

	uint32_t NahhhO;
	uint32_t SubmeshLodsOffset;

	uint32_t Unk;
	uint32_t SubmeshLodsOffset_V14;

	uint8_t Unknown3[0x3C];
	uint32_t OffsetToBoneRemapInfo;
	uint32_t BoneRemapCount;
	uint32_t OffsetToBoneRemapInfo_V14;
	uint32_t BoneRemapCount_V14;
};

struct s3studiohdr_t // season 3 studiohdr
{
	uint32_t Magic;
	uint32_t Version;
	uint32_t Hash;
	uint32_t NameTableOffset;

	char SkeletonName[0x40];

	uint32_t DataSize;

	float EyePosition[3];
	float IllumPosition[3];
	float HullMin[3];
	float HullMax[3];
	float ViewBBMin[3];
	float ViewBBMax[3];

	uint32_t Flags; // 0x9c

	uint32_t BoneCount; // 0xa0
	uint32_t BoneDataOffset; // 0xa4

	uint32_t BoneControllerCount;
	uint32_t BoneControllerOffset;

	uint32_t HitboxCount;
	uint32_t HitboxOffset;

	uint32_t LocalAnimCount;
	uint32_t LocalAnimOffset;

	uint32_t LocalSeqCount;
	uint32_t LocalSeqOffset;

	uint32_t ActivityListVersion;
	uint32_t EventsIndexed;

	uint32_t TextureCount;
	uint32_t TextureOffset;

	uint32_t TextureDirCount;
	uint32_t TextureDirOffset;

	uint32_t SkinReferenceCount;	// Total number of references (submeshes)
	uint32_t SkinFamilyCount;		// Total skins per reference
	uint32_t SkinReferenceOffset;	// Offset to data

	uint32_t BodyPartCount;
	uint32_t BodyPartOffset;

	uint32_t AttachmentCount;
	uint32_t AttachmentOffset;

	uint8_t Unknown2[0x14];

	uint32_t meshindex;

	uint8_t Unknown3[0x64];
	uint32_t OffsetToBoneRemapInfo;
	uint32_t BoneRemapCount;
	char unk[0x40];

	// index relative to the start of streamed data
	int vtxindex;
	int vvdindex;
	int vvcindex;
	int vphyindex;

	int vtxsize;
	int vvdsize;
	int vvcsize;
	int vphysize;
};

struct r2studiohdr_t // titanfall 2 studiohdr (MDL v53)
{
	int id; // Model format ID, such as "IDST" (0x49 0x44 0x53 0x54)
	int version; // Format version number, such as 48 (0x30,0x00,0x00,0x00)
	int checksum; // This has to be the same in the phy and vtx files to load!
	int sznameindex; // This has been moved from studiohdr2 to the front of the main header.
	char name[64]; // The internal name of the model, padding with null bytes.
	                // Typically "my_model.mdl" will have an internal name of "my_model"
	int length; // Data size of MDL file in bytes.
	
	Vector3 eyeposition;	// ideal eye position
	
	Vector3 illumposition;	// illumination center
	
	Vector3 hull_min;		// ideal movement hull size
	Vector3 hull_max;			
	
	Vector3 view_bbmin;		// clipping bounding box
	Vector3 view_bbmax;		
	
	int flags;
	
	int numbones; // bones
	int boneindex;
	
	int numbonecontrollers; // bone controllers
	int bonecontrollerindex;
	
	int numhitboxsets;
	int hitboxsetindex;
	
	int numlocalanim; // animations/poses
	int localanimindex; // animation descriptions
	
	int numlocalseq; // sequences
	int localseqindex;
	
	int activitylistversion; // initialization flag - have the sequences been indexed?
	int eventsindexed;
	
	// mstudiotexture_t
	// short rpak path
	// raw textures
	int numtextures; // the material limit exceeds 128, probably 256.
	int textureindex;
	
	// this should always only be one, unless using vmts.
	// raw textures search paths
	int numcdtextures;
	int cdtextureindex;
	
	// replaceable textures tables
	int numskinref;
	int numskinfamilies;
	int skinindex;
	
	int numbodyparts;		
	int bodypartindex;
	
	int numlocalattachments;
	int localattachmentindex;
	
	int numlocalnodes;
	int localnodeindex;
	int localnodenameindex;
	
	int numflexdesc;
	int flexdescindex;
	
	int numflexcontrollers;
	int flexcontrollerindex;
	
	int numflexrules;
	int flexruleindex;
	
	int numikchains;
	int ikchainindex;
	
	// this is rui meshes, todo refind mouth count.
	int numruimeshes;
	int ruimeshindex;
	
	int numlocalposeparameters;
	int localposeparamindex;
	
	int surfacepropindex;
	
	int keyvalueindex;
	int keyvaluesize;
	
	int numlocalikautoplaylocks;
	int localikautoplaylockindex;
	
	float mass;
	int contents;
	
	// external animations, models, etc.
	int numincludemodels;
	int includemodelindex;
	
	uint32_t virtualModel;
	
	// animblock is either completely cut, this is because they no longer use .ani files.
	
	int bonetablebynameindex;
	
	// if STUDIOHDR_FLAGS_CONSTANT_DIRECTIONAL_LIGHT_DOT is set,
	// this value is used to calculate directional components of lighting 
	// on static props
	byte constdirectionallightdot;
	
	// set during load of mdl data to track *desired* lod configuration (not actual)
	// the *actual* clamped root lod is found in studiohwdata
	// this is stored here as a global store to ensure the staged loading matches the rendering
	byte rootLOD;
	
	// set in the mdl data to specify that lod configuration should only allow first numAllowRootLODs
	// to be set as root LOD:
	//	numAllowedRootLODs = 0	means no restriction, any lod can be set as root lod.
	//	numAllowedRootLODs = N	means that lod0 - lod(N-1) can be set as root lod, but not lodN or lower.
	byte numAllowedRootLODs;
	
	byte unused;
	
	float fadedistance; // set to -1 to never fade. set above 0 if you want it to fade out, distance is in feet.
	                    // player/titan models seem to inherit this value from the first model loaded in menus.
	                    // works oddly on entities, probably only meant for static props
	
	int numflexcontrollerui;
	int flexcontrolleruiindex;
	
	// used by tools only that don't cache, but persist mdl's peer data
	// engine uses virtualModel to back link to cache pointers
	// might not be correct
	uint32_t pVertexBase; // float flVertAnimFixedPointScale;
	uint32_t pIndexBase; // int surfacepropLookup;
	
	// this is in all shipped models, probably part of their asset bakery. it should be 0x2CC.
	// doesn't actually need to be written pretty sure, only four bytes when not present.
	// this is not completely true as some models simply have nothing, such as animation models.
	int mayaindex;
	
	int numsrcbonetransform;
	int srcbonetransformindex;
	
	int illumpositionattachmentindex;
	
	int linearboneindex;
	
	int m_nBoneFlexDriverCount;
	int m_nBoneFlexDriverIndex;
	
	// for static props (and maybe others)
	// Per Triangle AABB
	int aabbindex;
	int numaabb;
	int numaabb1;
	int numaabb2;
	
	int stringtableindex;
	
	// start of model combination stuff.
	// anis are no longer used from what I can tell, v52s that had them don't in v53.
	int vtxindex; // VTX
	int vvdindex; // VVD / IDSV
	int vvcindex; // VVC / IDCV 
	int vphyindex; // VPHY / IVPS
	
	int vtxsize;
	int vvdsize;
	int vvcsize;
	int vphysize;
	
	// the following 'unks' could actually be indexs.
	// one of these is probably the ANI/IDAG index
	// vertAnimFixedPointScale might be in here but I doubt it.
	
	// this data block is related to the vphy, if it's not present the data will not be written
	int unkmemberindex1; // section between vphy and vtx.?
	int numunkmember1; // only seems to be used when phy has one solid
	
	int unk;
	
	int unkindex3; // goes to the same spot as vtx normally.
	
	int unused1[60]; // god I hope
};

struct mstudiobone_t
{
	uint32_t NameOffset;		// Relative to current position
	int32_t ParentIndex;

	int BoneControllers[6]; // -1 if none

	Math::Vector3 Position;		// Local
	Math::Quaternion Rotation;	// Local

	uint8_t Padding[0x78];
};

struct r2mstudiobone_t
{
	uint32_t NameOffset;		// Relative to current position
	int32_t ParentIndex;

	int BoneControllers[6]; // -1 if none

	Math::Vector3 Position;		// Local
	Math::Quaternion Rotation;	// Local
	Math::Vector3 EulerRotation;

	Math::Vector3 PositionScale;
	Math::Vector3 RotationScale;

	uint8_t Padding[0x88];
};

struct RMdlMeshStreamHeader
{
	uint32_t Version;
	uint32_t VertCacheSize;

	uint16_t MaxBonesPerStrip;
	uint16_t MaxBonesPerTri;

	uint32_t MaxBonesPerVert;
	uint32_t Hash;

	uint32_t NumLods;
	
	uint32_t MaterialReplacementOffset;		// Add 0x8 * 0x4 to get end of file.

	uint32_t NumBodyParts;
	uint32_t BodyPartOffset;
};

struct vertexFileHeader_t
{
	int id;
	int version;
	uint32_t checksum;

	int numLODs;
	int numLODVertexes[8];

	int numFixups;
	int fixupTableStart;

	int vertexDataStart;
	int tangentDataStart;
};

struct RMdlVGHeaderOld
{
	uint32_t Magic;		// 0x47567430	'0tvg'
	uint32_t Version;	// 0x1
	uint32_t Unknown;	// Usually 0
	uint32_t DataSize;	// Total size of data + header in starpak

	uint64_t BoneRemapOffset;
	uint64_t BoneRemapCount;		// Only 1 byte each

	uint64_t SubmeshOffset;
	uint64_t SubmeshCount;		// 0x48 each

	uint64_t IndexOffset;
	uint64_t IndexCount;		// 0x2 each (uint16_t)

	uint64_t VertexBufferOffset;
	uint64_t VertexBufferSize; // 1 byte each

	uint64_t ExtendedWeightsOffset;
	uint64_t ExtendedWeightsCount;		// Only 1 byte per count

	uint64_t Unknown2Offset;
	uint64_t Unknown2Count;		// 0x30 each

	uint64_t LodOffset;
	uint64_t LodCount;			// 0x8 each

	uint64_t ExternalWeightsOffset;
	uint64_t ExternalWeightsCount;	// 0x10 each

	uint64_t StripsOffset;
	uint64_t StripsCount;			// 0x23 each
};

enum class RMdlVGStreamFlags : uint32_t
{
	Unknown1 = (1 << 0),
	Unknown2 = (1 << 1),
	Unknown3 = (1 << 2),
	Unknown4 = (1 << 3),
	ShouldAlwaysBeSet = (1 << 4),
	Unknown6 = (1 << 5),
	HasSubVGHeaders2 = (1 << 6),
	HasSubVGHeaders = (1 << 7),
};

struct RMdlVGHeader
{
	int id;		// 0x47567430	'0tVG'
	int version;	// 0x1
	int padding;
	uint32_t lodCount;	// If 0x1, this IS the first and only lod, if > 0x1, MORE 0tVG headers follow PER lod count
	uint32_t unk;
	uint32_t unk1;
	uint32_t lodOffset;
	char unk3[8];
};

struct VGLod
{
	char unk[4];
	uint32_t dataSize;
	short submeshCount;
	char unk1; // both of these bytes line up with the LOD index
	char unk2;
	float distance;
	uint64_t submeshOffset;
};

struct RMdlVGIndexCountPacked
{
	uint64_t Count : 56;
	uint64_t Type : 8;
};

struct RMdlVGSubmesh
{
	uint32_t Flags1;					// Flags that pertain to this submesh
	uint32_t Flags2;					// Also flags that pertain to this submesh
	uint32_t VertexBufferStride;		// Stride in bytes of the vertex buffer
	uint32_t VertexCount;				// Count of vertices used

	uint64_t IndexOffset;
	RMdlVGIndexCountPacked IndexPacked;	// 0x2 each (uint16_t)

	uint64_t VertexOffset;
	uint64_t VertexCountBytes;		// 0x1 each aka, in bytes

	uint64_t ExtendedWeightsOffset;
	uint64_t ExtendedWeightsCount;		// Only 1 byte per count

	uint64_t ExternalWeightsOffset;
	uint64_t ExternalWeightsCount;	// 0x10 each

	uint64_t StripsOffset;
	uint64_t StripsCount;			// 0x23 each
};

struct RMdlVGSubmesh_V14
{
	uint32_t Flags1;					// Flags that pertain to this submesh
	uint32_t Flags2;					// Also flags that pertain to this submesh
	uint32_t VertexBufferStride;		// Stride in bytes of the vertex buffer
	uint32_t VertexCount;				// Count of vertices used

	uint64_t IndexOffset;
	RMdlVGIndexCountPacked IndexPacked;	// 0x2 each (uint16_t)

	uint64_t VertexOffset;
	uint64_t VertexCountBytes;		// 0x1 each aka, in bytes

	uint64_t ExtendedWeightsOffset;
	uint64_t ExtendedWeightsCount; // idk if these are actually unused but it looks like they are

	uint64_t ExternalWeightsOffset;
	uint64_t ExternalWeightsCount;	// 0x10 each

	uint64_t StripsOffset;
	uint64_t StripsCount;			// 0x23 each

	uint64_t UnkOffset;
	uint64_t UnkCount;		// Only 1 byte per count
};

struct RMdlVGSubmeshOld
{
	uint32_t Flags1;					// Flags that pertain to this submesh
	uint32_t Flags2;					// Also flags that pertain to this submesh
	uint32_t VertexOffsetBytes;			// Offset into vertex buffer by bytes
	uint32_t VertexBufferStride;		// Stride in bytes of the vertex buffer
	uint32_t VertexCount;				// Count of vertices used
	uint32_t Int6;
	uint32_t ExtendedWeightsOffset;		// Offset into the extended weights buffer
	uint32_t ExtendedWeightsSize;		// Size or count of extended weights used by this submesh
	uint32_t IndexOffset;				// Some form of index offset
	uint32_t IndexCount;				// Some form of index count
	uint32_t VertexOffset2;				// Some form of vertex offset (Not always used??)
	uint32_t VertexCount2;				// some form of vertex count
	uint32_t StripsIndex;				// Index into the strips structs
	uint32_t StripsCount;
	uint32_t Int15;
	uint32_t Int16;
	uint32_t Int17;
	uint32_t Int18;
};

struct RMdlVGStrip
{
	uint32_t IndexCount;
	uint32_t IndexOffset;

	uint32_t VertexCount;
	uint32_t VertexOffset;

	uint16_t NumBones;

	uint8_t StripFlags;

	uint8_t Padding[0x10];
};

struct RMdlVGLod
{
	uint16_t SubmeshIndex;
	uint16_t SubmeshCount;
	float Distance;
};

struct RMdlPhyHeader
{
	uint32_t HeaderSize;
	uint32_t Id;
	uint32_t SolidCount;
	uint32_t Checksum;
	uint32_t TextOffset; // offset to the text section
};

#define LAST_IND(x,part_type)    (sizeof(x)/sizeof(part_type) - 1)
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
#  define LOW_IND(x,part_type)   LAST_IND(x,part_type)
#  define HIGH_IND(x,part_type)  0
#else
#  define HIGH_IND(x,part_type)  LAST_IND(x,part_type)
#  define LOW_IND(x,part_type)   0
#endif

#define BYTEn(x, n)   (*((uint8_t*)&(x)+n))
#define WORDn(x, n)   (*((uint16_t*)&(x)+n))
#define DWORDn(x, n)  (*((uint32_t*)&(x)+n))
#define LOBYTE(x)  BYTEn(x,LOW_IND(x,uint8_t))
#define LOWORD(x)  WORDn(x,LOW_IND(x,uint16_t))
#define LODWORD(x) DWORDn(x,LOW_IND(x,uint32_t))
#define HIBYTE(x)  BYTEn(x,HIGH_IND(x,uint8_t))
#define HIWORD(x)  WORDn(x,HIGH_IND(x,uint16_t))
#define HIDWORD(x) DWORDn(x,HIGH_IND(x,uint32_t))
#define BYTE1(x)   BYTEn(x,  1)         // byte 1 (counting from 0)
#define BYTE2(x)   BYTEn(x,  2)

struct RMdlPackedVertexPosition
{
	uint32_t _Value[2];

	Math::Vector3 Unpack()
	{
		float x, y, z;

		x = ((_Value[0] & 0x1FFFFF) * 0.0009765625) - 1024.0;
		y = ((((_Value[1] & 0x3FFu) << 11) + (_Value[0] >> 21)) * 0.0009765625) - 1024.0;
		z = ((_Value[1] >> 10) * 0.0009765625) - 2048.0;

		return Math::Vector3(x, y, z);
	}
};

struct RMdlPackedVertexNormal
{
	uint32_t _Value;

	Math::Vector3 Unpack()
	{
		float x, y, z;

		float v87 = ((2 * _Value) >> 30);
		int v88 = 255;
		if (((8 * _Value) >> 31) != 0.0)
			v88 = -255;
		float v89 = (float)v88;
		float v90 = ((_Value << 13) >> 23) + -256.0;
		float v91 = ((16 * _Value) >> 23) + -256.0;
		float v92 = ((v91 * v91) + (255.0*255.0)) + (v90 * v90);

		float v93 = sqrtf(v92);
		int v97 = 0;

		float v1, v2, v3;

		v1 = v90 * (1.0 / v93);
		v2 = v89 * (1.0 / v93);
		v3 = v91 * (1.0 / v93);
		if (v87 == 1.0)
			v97 = -1;
		else
			v97 = 0;
		if (v87 == 2.0)
		{
			x = v3;
			y = v1;
			z = v2;
		}
		else
		{
			x = v2;
			y = v3;
			z = v1;
		}
		if (!v97)
		{
			v1 = x;
			v2 = y;
			v3 = z;
		}
		return Math::Vector3(v1,v2,v3);
	}
};

struct RMdlPackedVertexWeights
{
	uint16_t BlendWeights[2];
	uint8_t BlendIds[4];
};

struct RMdlVGExternalWeights
{
	union
	{
		struct
		{
			uint16_t Weights[4];
			uint32_t Unknown;
		};

		float SimpleWeights[3];
	};

	uint8_t WeightIds[3];
	uint8_t NumWeights;
};

struct RMdlFixup
{
	uint32_t LodIndex;
	
	uint32_t VertexIndex;
	uint32_t VertexCount;
};

struct RMdlVertex
{
	union
	{
		struct
		{
			uint16_t Weights[4];
			uint32_t WeightTableIndex;
		};

		float SimpleWeights[3];
	};
	
	uint8_t WeightIds[3];
	uint8_t NumWeights;

	Math::Vector3 Position;
	Math::Vector3 Normal;
	Math::Vector2 UVs;
};

struct mstudiobodyparts_t
{
	uint32_t nummodels;
	uint32_t modelindex;
};

struct r2mstudiobodyparts_t
{
	uint32_t sznameindex;
	uint32_t nummodels;
	uint32_t base;
	uint32_t modelindex;
};

struct RMdlModel
{
	uint32_t NumLods;
	uint32_t LodOffset;
};

struct RMdlTitanfallModel
{
	char Name[0x40];

	int type;
	float boundingradius;

	int nummeshes;
	int meshindex;
	int numvertices;
	int vertexindex;
	int tangentsindex;

	int numattachments;
	int attachmentindex;

	// might be cut
	int numeyeballs;
	int eyeballindex;

	//mstudio_modelvertexdata_t vertexdata;

	int unk[4];

	int unkindex;
	int unkindex1;

	int unused[4];
};

struct RMdlLod
{
	uint32_t SubmeshCount;
	uint32_t SubmeshOffset;
	float Distance;
};

struct RMdlSubmesh
{
	uint32_t NumStripGroups;
	uint32_t StripGroupOffset;

	uint8_t Flags;
};

struct RMdlStripGroup
{
	uint32_t VertexCount;
	uint32_t VertexOffset;
	
	uint32_t IndexCount;
	uint32_t IndexOffset;

	uint32_t NumStrips;
	uint32_t StripOffset;

	uint8_t Flags;

	int numTopologyIndices;
	uint32_t topologyOffset;
};

struct RMdlStrip
{
	uint32_t IndexCount;
	uint32_t IndexOffset;
	
	uint32_t VertexCount;
	uint32_t VertexOffset;

	uint16_t NumBones;

	uint8_t StripFlags;

	uint32_t NumBoneChanges;
	uint32_t BoneChangeOffset;
};

struct RMdlStripVert
{
	uint8_t BoneWeightIndex[3];
	uint8_t NumBones;

	uint16_t VertexIndex;

	uint8_t BoneIds[3];
};

struct RMdlExtendedWeight
{
	uint16_t Weight;
	uint16_t BoneId;
};

struct RMdlLodSubmeshOld
{
	uint32_t Index;
	
	uint16_t Unknown1;
	uint16_t Unknown2;
	uint16_t Unknown3;
	uint16_t Unknown4;

	uint8_t UnknownPad[0x28];

	uint32_t LodVertCounts[8];

	uint32_t Unknown5;
	uint32_t Unknown6;
};

struct RMdlLodSubmesh
{
	uint32_t Index;

	uint16_t Unknown1;
	uint16_t Unknown2;
	uint16_t Unknown3;
	uint16_t Unknown4;

	uint8_t UnknownPad[0x18];

	uint32_t LodVertCounts[8];

	uint32_t Unknown5;
	uint32_t Unknown6;
};

struct RMdlTitanfallLodSubmesh
{
	uint32_t Index;

	uint16_t Unknown1;
	uint16_t Unknown2;
	uint16_t Unknown3;
	uint16_t Unknown4;

	uint8_t UnknownPad[0x28];

	uint32_t LodVertCounts[8];

	uint8_t UnknownPad2[0x20];
};

struct RMdlTexture
{
	uint32_t Offset;
	uint64_t MaterialHash;
};

// MATERIALS
// --- matl ---
struct MaterialHeader
{
	uint8_t Unknown[0x10];
	uint64_t Hash;

	uint32_t NameIndex;
	uint32_t NameOffset;
	uint32_t TypeIndex;
	uint32_t TypeOffset;
	uint32_t SurfaceIndex;
	uint32_t SurfaceOffset;

	uint8_t Unknown2[0x28];
	uint64_t ShaderSetHash;

	uint32_t TexturesIndex;
	uint32_t TexturesOffset;
	// Also Texture pointer, but less relevant for now.
	uint32_t StreamableTexturesIndex;
	uint32_t StreamableTexturesOffset;

	int16_t StreamableTextureCount;
	int16_t Width;
	int16_t Height;
	int16_t Unused;
	uint32_t ImageFlags; // Image Flags, they decide the tiling, stretching etc.

	uint8_t Unknown3[0x1C];

	uint32_t TexturesTFIndex;
	uint32_t TexturesTFOffset;
	uint32_t UnknownTFIndex;
	uint32_t UnknownTFOffset;
};

struct StarpakStreamEntry
{
	uint64_t Offset;
	uint64_t Size;
};


// SHADERS
// --- shdr ---
struct ShaderHeader
{
	uint32_t NameIndex;
	uint32_t NameOffset;
	uint64_t DataSize;
	uint64_t Padding2;

	uint32_t Index1;
	uint32_t Offset1;

	uint32_t Index2;
	uint32_t Offset2;
};

struct RShaderImage
{
	uint32_t DataIndex;
	uint32_t DataOffset;
	uint32_t DataSize;
	uint32_t Size2;
	uint32_t DataIndex2;
	uint32_t DataOffset2;
};

// --- shds ---
struct ShaderSetHeader {
	uint64_t VTablePadding;
	uint32_t NameIndex;
	uint32_t NameOffset;
	uint8_t Unknown1[0x8];
	uint16_t Count1;
	uint16_t TextureInputCount;
	uint16_t Count3;
	uint8_t Byte1;
	uint8_t Byte2;

	uint8_t Unknown2[0x10];

	uint64_t OldVertexShaderHash;
	uint64_t OldPixelShaderHash;

	// only used for version 12+
	uint64_t VertexShaderHash;
	uint64_t PixelShaderHash;
	uint64_t PixelShaderHashTF;

};

struct ShaderSetHeaderTF {
	uint64_t VTablePadding;
	uint32_t NameIndex;
	uint32_t NameOffset;
	uint8_t Unknown1[0x8];
	uint16_t Count1;
	uint16_t TextureInputCount;
	uint16_t Count3;
	uint8_t Byte1;
	uint8_t Byte2;

	uint8_t Unknown2[0x28];

	// only used for version 12+
	uint64_t VertexShaderHash;
	uint64_t PixelShaderHash;
};


struct ShaderDataHeader
{
	uint32_t ByteCodeIndex;
	uint32_t ByteCodeOffset;
	uint32_t DataSize;
};

// DX Shader Types
enum ShaderType : uint16_t
{
	ComputeShader = 0x4353,
	DomainShader = 0x4453,
	GeometryShader = 0x4753,
	HullShader = 0x4853,
	VertexShader = 0xFFFE,
	PixelShader = 0xFFFF,
};

// shader "DXBC" header
struct DXBCHeader
{
	char FourCC[4];
	uint32_t Checksum[4];
	uint32_t One;
	uint32_t DataSize;
	uint32_t ChunkCount;
};

struct RDefHeader
{
	uint32_t Magic; // RDEF
	uint32_t DataSize;
	uint32_t ConstBufferCount;
	uint32_t ConstBufferOffset;
	uint32_t ResBindingCount;
	uint32_t ResBindingOffset;
	uint8_t  MinorVersion;
	uint8_t  MajorVersion;
	ShaderType ShaderType;
	uint32_t Flags;
	uint32_t CompilerStringOffset;
};

struct RDefConstBuffer
{
	uint32_t NameOffset;

	uint32_t VariableCount;
	uint32_t VariableOffset;
	uint32_t DataSize;
	uint32_t Flags;
	uint32_t BufferType;
};

struct RDefCBufVar
{
	uint32_t NameOffset;
	uint32_t CBufStartOffset;
	uint32_t Size;
	uint32_t Flags;
	uint32_t TypeOffset;
	uint32_t DefaultOffset; // offset to the default value of this var
	uint32_t unk[4];
};

struct RDefCBufVarType
{
	uint16_t Class;
	uint16_t Type;
	uint16_t MatrixRows;
	uint16_t MatrixColumns;
	uint16_t ArraySize;
	uint16_t StructMemberCount;
	uint16_t FirstMemberOffset;
};

struct RDefResBinding
{
	uint32_t NameOffset;
	D3D_SHADER_INPUT_TYPE InputType;
	D3D_RESOURCE_RETURN_TYPE ReturnType;
	uint32_t ViewDimension;
	uint32_t SampleCount;
	uint32_t BindPoint;
	uint32_t BindCount;
	D3D_SHADER_INPUT_FLAGS InputFlags;
};

struct ShaderVar
{
	string Name;
	D3D_SHADER_VARIABLE_TYPE Type;
};

struct ShaderResBinding
{
	string Name;
	D3D_SHADER_INPUT_TYPE Type;
};

// uimg - ui image atlas
struct UIAtlasHeader
{
	uint64_t Unk0;
	uint16_t Width; // full dimensions of the atlas texture
	uint16_t Height;
	uint16_t TexturesCount; // number of textures
	uint16_t UnkE;
	uint32_t TextureOffsetsIndex; // texture "offsets" (idrk what this actually does tbh)
	uint32_t TextureOffsetsOffset;
	uint32_t TextureDimsIndex; // texture dimensions
	uint32_t TextureDimsOffset;
	uint32_t Unk20;
	uint32_t Unk24;
	uint32_t TextureHashesIndex; // texture hashes
	uint32_t TextureHashesOffset;
	uint32_t TextureNamesIndex; // texture paths ( not always present )
	uint32_t TextureNamesOffset;
	uint64_t TextureGuid; // asset guid for the texture that contains our images
};

struct UIAtlasUV
{
	float uv0x; // top left corner
	float uv0y;

	float uv1x;
	float uv1y;
};

struct UIAtlasOffset
{
	// these don't seem to matter all that much as long as they are a valid float number
	float f0 = 0.f;
	float f1 = 0.f;

	// endX and endY define where the edge of the image is, with 1.f being the full length of the image and 0.5f being half of the image
	float endX = 1.f;
	float endY = 1.f;

	// startX and startY define where the top left corner is in proportion to the full image dimensions
	float startX = 0.f;
	float startY = 0.f;

	// changing these 2 values causes the image to be distorted on each axis
	float unkX = 1.f;
	float unkY = 1.f;
};

struct UIAtlasImage // uiai wen
{
	string Path;
	uint32_t Hash;
	uint32_t PathTableOffset;

	UIAtlasOffset offsets; // idk anymore
	UIAtlasUV uvs;

	uint16_t Width;
	uint16_t Height;

	uint16_t PosX; // top left corner's absolute position in the atlas texture
	uint16_t PosY;
};

// --- rson ---
#define RSON_STRING 0x2
#define RSON_OBJECT 0x8
#define RSON_BOOLEAN 0x10
#define RSON_INTEGER 0x20
#define RSON_ARRAY 0x1000

struct RSONHeader
{
	int type;
	int nodeCount;
	RPakPtr pNodes;
};

struct RSONNode
{
	RPakPtr pName;
	int type;
	int valueCount;
	RPakPtr pValues;
};
#pragma pack(pop)


// Validate all game structures
// APEX
ASSERT_SIZE(TextureHeader, 0x38);
ASSERT_SIZE(DataTableHeader, 0x28);
ASSERT_SIZE(DataTableColumn, 0x18);
ASSERT_SIZE(SubtitleHeader, 0x18);
ASSERT_SIZE(SettingsHeader, 0x48);
ASSERT_SIZE(PatchHeader, 0x18);
ASSERT_SIZE(UIIAHeader, 0x40);
ASSERT_SIZE(SettingsKeyValue, 0x8);
ASSERT_SIZE(SettingsKeyValuePair, 0x10);
ASSERT_SIZE(ModelHeaderS68, 0x68);
ASSERT_SIZE(ModelHeaderS80, 0x80);
ASSERT_SIZE(AnimHeader, 0x30);
ASSERT_SIZE(AnimRigHeader, 0x28);
ASSERT_SIZE(mstudiobone_t, 0xB4);
ASSERT_SIZE(RMdlMeshStreamHeader, 0x24);
ASSERT_SIZE(vertexFileHeader_t, 0x40);
ASSERT_SIZE(RMdlFixup, 0xC);
ASSERT_SIZE(RMdlVertex, 0x30);
ASSERT_SIZE(mstudiobodyparts_t, 0x8);
ASSERT_SIZE(RMdlModel, 0x8);
ASSERT_SIZE(RMdlLod, 0xC);
ASSERT_SIZE(RMdlSubmesh, 0x9);
//ASSERT_SIZE(RMdlStripGroup, 0x19);
ASSERT_SIZE(RMdlStripVert, 0x9);
ASSERT_SIZE(RMdlStrip, 0x1B);
ASSERT_SIZE(RMdlExtendedWeight, 0x4);
ASSERT_SIZE(RMdlTexture, 0xC);
ASSERT_SIZE(RAnimHeader, 0x40);

// Game helper structs
struct RMdlFixupPatches
{
	List<RMdlTexture>* Materials;
	List<RMdlLodSubmesh>* SubmeshLods;
	List<uint8_t>* BoneRemaps;
	string MaterialPath;

	uint64_t VertexShift;
	uint64_t FixupTableOffset;
	uint64_t WeightsTableOffset;
};

struct RMdlMaterial
{
	string MaterialName;

	string AlbedoMapName;
	string NormalMapName;
	string GlossMapName;
	string SpecularMapName;
	string EmissiveMapName;
	string AmbientOcclusionMapName;
	string CavityMapName;

	uint64_t AlbedoHash;
	uint64_t NormalHash;
	uint64_t GlossHash;
	uint64_t SpecularHash;
	uint64_t EmissiveHash;
	uint64_t AmbientOcclusionHash;
	uint64_t CavityHash;
};

struct RUIImageTile
{
	uint32_t Offset : 24;
	uint32_t Opcode : 8;
};