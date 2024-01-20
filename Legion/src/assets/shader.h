#pragma once

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
	int Size;
};

struct ShaderResBinding
{
	string Name;
	D3D_SHADER_INPUT_TYPE Type;
	uint32_t BindPoint;
	uint32_t BindCount;
};