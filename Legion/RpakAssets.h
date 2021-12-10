#pragma once

#include <cstdint>
#include "Vector2.h"
#include "Vector3.h"
#include "Quaternion.h"
#include "ListBase.h"

// Game structures
// APEX
#pragma pack(push, 1)
struct TextureHeader
{
	uint64_t NameHash;
	uint64_t Unknown;

	uint16_t Width;
	uint16_t Height;

	uint8_t Un1;
	uint8_t Un2;
	uint8_t Format;		// Maps to a DXGI format
	uint8_t Un3;

	uint32_t DataSize;	// This is the total amount of image data across all banks
	uint32_t Unknown2;
	uint8_t Unknown3;
	uint8_t MipLevels;	// Usually 0x8/0x9
	uint8_t MipLevelsStreamed;

	uint8_t UnknownPad[0x15];
};

struct TextureAnimatedHeader
{
	uint32_t Index1;
	uint32_t Offset1;

	uint32_t Index2;
	uint32_t Offset2;

	uint32_t Index3;
	uint32_t Offset3;
};

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
	uint32_t Flags;
	uint32_t StringOffset;
	uint32_t ColumnDataType;
	uint32_t RowDataOffset;
};

struct SettingsHeader
{
	uint64_t Hash;
	
	uint32_t KvpIndex;
	uint32_t KvpOffset;

	uint32_t NameIndex;
	uint32_t NameOffset;

	uint32_t StringBufferIndex;
	uint32_t StringBufferOffset;

	uint8_t Unk1[0x18];

	uint32_t KvpBufferSize;

	uint8_t Unk2[0xC];
};

struct PatchHeader
{
	uint32_t Version;
	uint32_t Count;

	uint32_t EntriesIndex;
	uint32_t EntriesOffset;

	uint32_t PatchesIndex;
	uint32_t PatchesOffset;
};

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

struct ModelHeaderS68
{
	uint32_t SkeletonIndex;
	uint32_t SkeletonOffset;
	uint64_t Padding;

	uint32_t NameIndex;
	uint32_t NameOffset;
	uint64_t Padding2;

	uint32_t MeshIndex;
	uint32_t MeshOffset;
	uint64_t Padding3;
	
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
	uint32_t SkeletonIndex;
	uint32_t SkeletonOffset;
	uint64_t Padding;

	uint32_t NameIndex;
	uint32_t NameOffset;
	uint64_t Padding2;

	uint32_t MeshIndex;
	uint32_t MeshOffset;
	uint64_t Padding3;

	uint32_t BlockIndex1;
	uint32_t BlockOffset1;
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

struct ShaderHeader
{
	uint64_t Padding;
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

struct ShaderSetHeader {
	uint8_t Unknown1[0x18];
	uint16_t Count1;
	uint16_t Count2;
	uint16_t Count3;
	uint8_t Byte1;
	uint8_t Byte2;

	uint8_t Unknown2[0x20];

	uint64_t VertexShaderHash;
	uint64_t PixelShaderHash;
};

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

struct RMdlSkeletonHeader
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

	uint32_t Flags;

	uint32_t BoneCount;
	uint32_t BoneDataOffset;

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

	uint8_t Unknown3[0x44];
	uint32_t OffsetToBoneRemapInfo;
	uint32_t BoneRemapCount;
};

struct RMdlTitanfallSkeletonHeader
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

	uint32_t Flags;

	uint32_t BoneCount;
	uint32_t BoneDataOffset;

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

	uint32_t SubmeshLodsOffsetOg;

	uint8_t Unknown3[0x98];

	uint32_t SubmeshLodsOffset;
	uint32_t MeshOffset;
};

struct RMdlBone
{
	uint32_t NameOffset;		// Relative to current position
	int32_t ParentIndex;

	uint8_t UnknownNegativeOne[0x18];

	Math::Vector3 Position;		// Local
	Math::Quaternion Rotation;	// Local

	uint8_t Padding[0x78];
};

struct RMdlTitanfallBone
{
	uint32_t NameOffset;		// Relative to current position
	int32_t ParentIndex;

	uint8_t UnknownNegativeOne[0x18];

	Math::Vector3 Position;		// Local
	Math::Quaternion Rotation;	// Local

	uint8_t UnknownData[0x24];

	float RotationScale[3];

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

struct RMdlMeshHeader
{
	uint32_t Magic;
	uint32_t Version;
	uint32_t Hash;

	uint32_t NumLods;
	uint32_t NumLodVertCounts[8];

	uint32_t NumFixups;
	uint32_t FixupOffset;

	uint32_t VertexOffset;
	uint32_t TangentOffset;
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

	uint64_t VertexOffset;
	uint64_t VertexCount;		// 0x1 each aka, in bytes

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
	uint32_t Magic;		// 0x47567430	'0tVG'
	uint32_t Version;	// 0x1
	uint32_t Padding;
	uint32_t LodCount;	// If 0x1, this IS the first and only lod, if > 0x1, MORE 0tVG headers follow PER lod count

	uint8_t Unknown[0x14];
	uint32_t DataSize;	// Total size of data + header in starpak
	uint32_t SubmeshCount;
	uint32_t Padding2;
	uint32_t StreamFlags;
	uint8_t Unknown2[0xC];	// We're either at 
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

struct RMdlVGSubmeshOld
{
	uint32_t Flags1;					// Flags that pertain to this submesh
	uint32_t Flags2;					// Also flags that pertain to this submesh
	uint32_t VertexOffsetBytes;			// Offset into vertex buffer by bytes
	uint32_t VertexBufferStride;		// Stride in bytes of the vertex buffer
	uint32_t VertexCount;				// Count of vertices used
	uint32_t Int6;
	uint32_t ExtendedWeightsOffset;		// Offset into the extended weights buffer
	uint32_t Int8;
	uint32_t IndexOffset;				// Some form of index offset
	uint32_t IndexCount;				// Some form of index count
	uint32_t VertexOffset2;				// Some form of vertex offset (Not always used??)
	uint32_t VertexCount2;				// some form of vertex count
	uint32_t StripIndex;				// Index into the strips structs
	uint32_t Int14;
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

struct RMdlPackedVertexPosition
{
	uint32_t _Value[2];

	Math::Vector3 Unpack()
	{
		return Math::Vector3(0, 0, 0);
		// TODO(rx): re-implement this from dump
		// Redacted
	}
};

struct RMdlPackedVertexNormal
{
	uint32_t _Value;

	Math::Vector3 Unpack()
	{
		return Math::Vector3(0, 0, 0);
		// TODO(rx): re-implement this from dump
        // Redacted
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

struct RMdlBodyPart
{
	uint32_t NumModels;
	uint32_t ModelOffset;
};

struct RMdlTitanfallBodyPart
{
	uint32_t NameOffset;
	uint32_t NumModels;
	uint32_t BaseIndex;
	uint32_t ModelOffset;
};

struct RMdlModel
{
	uint32_t NumLods;
	uint32_t LodOffset;
};

struct RMdlTitanfallModel
{
	char Name[0x40];

	uint32_t ModelType;
	float BoundingRadius;

	uint32_t NumMeshes;
	uint32_t MeshOffset;
	uint32_t NumVertices;
	uint32_t VertexIndex;
	uint32_t TangentsIndex;

	uint8_t Unk1[0x38];
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
	uint32_t UnknownIndex;
	uint32_t UnknownOffset;

	uint8_t Unknown3[0x28];

	uint32_t TexturesTFIndex;
	uint32_t TexturesTFOffset;
};

struct StarpakStreamEntry
{
	uint64_t Offset;
	uint64_t Size;
};
#pragma pack(pop)

// Validate all game structures
// APEX
static_assert(sizeof(TextureHeader) == 0x38, "Invalid Header Size");
static_assert(sizeof(DataTableHeader) == 0x28, "Invalid Header Size");
static_assert(sizeof(DataTableColumn) == 0x10, "Invalid Header Size");
static_assert(sizeof(SettingsHeader) == 0x48, "Invalid Header Size");
static_assert(sizeof(PatchHeader) == 0x18, "Invalid Header Size");
static_assert(sizeof(UIIAHeader) == 0x40, "Invalid Header Size");
static_assert(sizeof(SettingsKeyValue) == 0x8, "Invalid Header Size");
static_assert(sizeof(SettingsKeyValuePair) == 0x10, "Invalid Header Size");
static_assert(sizeof(ModelHeaderS68) == 0x68, "Invalid Header Size");
static_assert(sizeof(ModelHeaderS80) == 0x80, "Invalid Header Size");
static_assert(sizeof(AnimHeader) == 0x30, "Invalid Header Size");
static_assert(sizeof(AnimRigHeader) == 0x28, "Invalid Header Size");
static_assert(sizeof(RMdlBone) == 0xB4, "Invalid Header Size");
//static_assert(sizeof(RMdlSkeletonHeader) == 0x118, "Invalid Header Size");
static_assert(sizeof(RMdlMeshStreamHeader) == 0x24, "Invalid Header Size");
static_assert(sizeof(RMdlMeshHeader) == 0x40, "Invalid Header Size");
static_assert(sizeof(RMdlFixup) == 0xC, "Invalid Header Size");
static_assert(sizeof(RMdlVertex) == 0x30, "Invalid Header Size");
static_assert(sizeof(RMdlBodyPart) == 0x8, "Invalid Header Size");
static_assert(sizeof(RMdlModel) == 0x8, "Invalid Header Size");
static_assert(sizeof(RMdlLod) == 0xC, "Invalid Header Size");
static_assert(sizeof(RMdlSubmesh) == 0x9, "Invalid Header Size");
static_assert(sizeof(RMdlStripGroup) == 0x19, "Invalid Header Size");
static_assert(sizeof(RMdlStripVert) == 0x9, "Invalid Header Size");
static_assert(sizeof(RMdlStrip) == 0x1B, "Invalid Header Size");
static_assert(sizeof(RMdlExtendedWeight) == 0x4, "Invalid Header Size");
static_assert(sizeof(RMdlLodSubmesh) == 0x4C, "Invalid Header Size");
static_assert(sizeof(RMdlTexture) == 0xC, "Invalid Header Size");
static_assert(sizeof(RAnimHeader) == 0x40, "Invalid Header Size");
static_assert(sizeof(StarpakStreamEntry) == 0x10, "Invalid Header Size");

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
};

struct RUIImageTile
{
	uint32_t Offset : 24;
	uint32_t Opcode : 8;
};