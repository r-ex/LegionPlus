#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"

void RpakLib::BuildShaderSetInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	Info.Name = string::Format("shaderset_0x%llx", Asset.NameHash);
	Info.Type = ApexAssetType::ShaderSet;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = "N/A";
}

void RpakLib::ExportShaderSet(const RpakLoadAsset& Asset, const string& Path)
{
	string ShaderSetPath = IO::Path::Combine(Path, string::Format("0x%llx", Asset.NameHash));

	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	ShaderSetHeader Header = Reader.Read<ShaderSetHeader>();

	uint64_t PixelShaderGuid = Header.PixelShaderHash;
	uint64_t VertexShaderGuid = Header.VertexShaderHash;

	if (Asset.AssetVersion <= 11)
	{
		PixelShaderGuid = Header.OldPixelShaderHash;
		VertexShaderGuid = Header.OldVertexShaderHash;
	}
	if (!IO::Directory::Exists(ShaderSetPath))
		IO::Directory::CreateDirectory(ShaderSetPath);

	if (Assets.ContainsKey(PixelShaderGuid))
	{
		string PixelShaderPath = IO::Path::Combine(ShaderSetPath, string::Format("0x%llx_ps.fxc", PixelShaderGuid));
		this->ExtractShader(Assets[PixelShaderGuid], PixelShaderPath);
	}

	if (Assets.ContainsKey(VertexShaderGuid))
	{
		string VertexShaderPath = IO::Path::Combine(ShaderSetPath, string::Format("0x%llx_vs.fxc", VertexShaderGuid));
		this->ExtractShader(Assets[VertexShaderGuid], VertexShaderPath);
	}
}

void RpakLib::ExtractShader(const RpakLoadAsset& Asset, const string& Path)
{
	if (!Utils::ShouldWriteFile(Path))
		return;

	if (Asset.RawDataIndex == -1 || Asset.RawDataOffset == -1)
		return;

	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);



	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

	ShaderDataHeader DataHeader = Reader.Read<ShaderDataHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, DataHeader.ByteCodeIndex, DataHeader.ByteCodeOffset));

	char* bcBuf = new char[DataHeader.DataSize];

	Reader.Read(bcBuf, 0, DataHeader.DataSize);

	std::ofstream shaderOut(Path.ToCString(), std::ios::binary | std::ios::out);
	shaderOut.write(bcBuf, DataHeader.DataSize);
	shaderOut.close();
}

ShaderSetHeader RpakLib::ExtractShaderSet(const RpakLoadAsset& Asset)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	ShaderSetHeader Header = Reader.Read<ShaderSetHeader>();

	return Header;
}

List<ShaderVar> RpakLib::ExtractShaderVars(const RpakLoadAsset& Asset, D3D_SHADER_VARIABLE_TYPE VarsType)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	List<ShaderVar> Vars;

	if (Asset.RawDataIndex == -1)
		return Vars;

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

	ShaderDataHeader DataHeader = Reader.Read<ShaderDataHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, DataHeader.ByteCodeIndex, DataHeader.ByteCodeOffset));

	const uint64_t BasePos = RpakStream->GetPosition();
	DXBCHeader hdr = Reader.Read<DXBCHeader>();

	List<uint32_t> ChunkOffsets(hdr.ChunkCount, true);
	Reader.Read((uint8_t*)&ChunkOffsets[0], 0, hdr.ChunkCount * sizeof(uint32_t));

	for (uint32_t i = 0; i < hdr.ChunkCount; ++i)
		ChunkOffsets[i] += BasePos;

	for (auto& ChunkOffset : ChunkOffsets)
	{
		RpakStream->SetPosition(ChunkOffset);

		if (Reader.Read<uint32_t>() == 'FEDR') // Resource Definitions
		{
			RpakStream->SetPosition(ChunkOffset);

			RDefHeader RDefHdr = Reader.Read<RDefHeader>();

			if (RDefHdr.MajorVersion >= 5)
			{
				RpakStream->Seek(32, IO::SeekOrigin::Current); // skip RD11 header
			}

			uint64_t ConstBufferPos = (ChunkOffset + 8) + RDefHdr.ConstBufferOffset;

			for (uint32_t i = 0; i < RDefHdr.ConstBufferCount; ++i)
			{
				RpakStream->SetPosition(ConstBufferPos + (i * sizeof(RDefConstBuffer)));
				RDefConstBuffer ConstBuffer = Reader.Read<RDefConstBuffer>();

				for (uint32_t j = 0; j < ConstBuffer.VariableCount; ++j)
				{
					RpakStream->SetPosition((ChunkOffset + 8) + ConstBuffer.VariableOffset + (j * sizeof(RDefCBufVar)));

					RDefCBufVar CBufVar = Reader.Read<RDefCBufVar>();

					uint64_t NameOffset = ChunkOffset + 8 + CBufVar.NameOffset;
					uint64_t TypeOffset = ChunkOffset + 8 + CBufVar.TypeOffset;

					RpakStream->SetPosition(NameOffset);
					string Name = Reader.ReadCString();

					RpakStream->SetPosition(TypeOffset);
					RDefCBufVarType Type = Reader.Read<RDefCBufVarType>();

					ShaderVar Var;

					Var.Name = Name;
					Var.Type = (D3D_SHADER_VARIABLE_TYPE)Type.Type;

					// make sure that the VarsType arg is actually specified and then check if this var matches that type
					if (Var.Type != D3D_SVT_FORCE_DWORD && Var.Type == VarsType)
						Vars.EmplaceBack(Var);
				}
			}
			break;
		}
	}
	return Vars;
}

List<ShaderResBinding> RpakLib::ExtractShaderResourceBindings(const RpakLoadAsset& Asset, D3D_SHADER_INPUT_TYPE InputType)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	List<ShaderResBinding> ResBindings;

	if (Asset.RawDataIndex == -1)
		return ResBindings;

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

	ShaderDataHeader DataHeader = Reader.Read<ShaderDataHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, DataHeader.ByteCodeIndex, DataHeader.ByteCodeOffset));

	const uint64_t BasePos = RpakStream->GetPosition();
	auto hdr = Reader.Read<DXBCHeader>();

	List<uint32_t> ChunkOffsets(hdr.ChunkCount, true);
	Reader.Read((uint8_t*)&ChunkOffsets[0], 0, hdr.ChunkCount * sizeof(uint32_t));

	for (uint32_t i = 0; i < hdr.ChunkCount; ++i)
		ChunkOffsets[i] += BasePos;

	for (auto& ChunkOffset : ChunkOffsets)
	{
		RpakStream->SetPosition(ChunkOffset);

		if (Reader.Read<uint32_t>() == 'FEDR') // Resource Definitions
		{
			RpakStream->SetPosition(ChunkOffset);

			RDefHeader RDefHdr = Reader.Read<RDefHeader>();

			if (RDefHdr.MajorVersion >= 5)
			{
				RpakStream->Seek(32, IO::SeekOrigin::Current); // skip RD11 header
			}

			uint64_t ResBindingPos = (ChunkOffset + 8) + RDefHdr.ResBindingOffset;

			for (uint32_t i = 0; i < RDefHdr.ResBindingCount; ++i)
			{
				RpakStream->SetPosition(ResBindingPos + (i * sizeof(RDefResBinding)));
				RDefResBinding ResBinding = Reader.Read<RDefResBinding>();

				uint64_t NameOffset = ChunkOffset + 8 + ResBinding.NameOffset;

				RpakStream->SetPosition(NameOffset);
				string Name = Reader.ReadCString();

				ShaderResBinding Res;
				Res.Name = Name;
				Res.Type = ResBinding.InputType;

				if (Res.Type == InputType)
				{
					ResBindings.EmplaceBack(Res);
				}
			}
			break;
		}
	}
	return ResBindings;
}