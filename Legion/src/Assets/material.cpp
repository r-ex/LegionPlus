#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"

#include <typeinfo>
#include <typeindex>

const char* s_MaterialTypes[] = {
	"RGDU",
	"RGDP",
	"RGDC",
	"SKNU",
	"SKNP",
	"SKNC",
	"WLDU",
	"WLDC",
	"PTCU",
	"PTCS",
	"RGBS",
};

void RpakLib::BuildMaterialInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	MaterialHeader hdr;
	
	if (Asset.Version == RpakGameVersion::Apex)
	{
		if (Asset.AssetVersion >= 16)
		{
			MaterialHeaderV16 hdr_v16 = Reader.Read<MaterialHeaderV16>();
			hdr.FromV16(hdr_v16);
		}
		else hdr = Reader.Read<MaterialHeader>();

		Info.DebugInfo = string::Format("type: %s", s_MaterialTypes[hdr.materialType]);

	}
	else
	{
		MaterialHeaderV12 temp = Reader.Read<MaterialHeaderV12>();

		hdr.pName = temp.pName;
		hdr.textureHandles = temp.textureHandles;
		hdr.streamingTextureHandles = temp.streamingTextureHandles;
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, hdr.pName.Index, hdr.pName.Offset));

	string MaterialName = Reader.ReadCString();

	if (ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = MaterialName;
	else
		Info.Name = IO::Path::GetFileNameWithoutExtension(MaterialName).ToLower();

	Info.Type = ApexAssetType::Material;
	Info.Status = ApexAssetStatus::Loaded;

	uint32_t TexturesCount = (hdr.streamingTextureHandles.Offset - hdr.textureHandles.Offset) / 8;

	Info.Info = string::Format("Textures: %i", TexturesCount);
}

void RpakLib::ExportMatCPUAsRaw(const RpakLoadAsset& Asset, MaterialHeader& MatHdr, MaterialCPUHeader& MatCPUHdr, std::ofstream& oStream)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, MatCPUHdr.m_nData.Index, MatCPUHdr.m_nData.Offset));

	std::unique_ptr<char[]> buffer(new char[MatCPUHdr.m_nDataSize]);

	RpakStream->Read((uint8_t*)buffer.get(), 0, MatCPUHdr.m_nDataSize);

	oStream.write((char*)buffer.get(), MatCPUHdr.m_nDataSize);
}

void RpakLib::ExportMatCPUAsStruct(const RpakLoadAsset& Asset, MaterialHeader& MatHdr, MaterialCPUHeader& MatCPUHdr, std::ofstream& oStream)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	if (Asset.AssetVersion == 12)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
		MaterialHeaderV12 mathdr = Reader.Read<MaterialHeaderV12>();

		MatHdr.shaderSetGuid = mathdr.shaderSetGuid;
	}

	if (!Assets.ContainsKey(MatHdr.shaderSetGuid)) // no shaderset loaded
		return;

	RpakLoadAsset ShaderSetAsset = Assets[MatHdr.shaderSetGuid];
	ShaderSetHeader ShaderSetHeader = ExtractShaderSet(ShaderSetAsset);

	uint64_t PixelShaderGuid = ShaderSetHeader.PixelShaderHash;

	if (ShaderSetAsset.AssetVersion <= 11)
		PixelShaderGuid = ShaderSetHeader.OldPixelShaderHash;

	if (ShaderSetAsset.AssetVersion == 8)
		PixelShaderGuid = ShaderSetHeader.PixelShaderHashTF;

	if (!Assets.ContainsKey(PixelShaderGuid)) // no pixel shader
		return;

	List<ShaderVar> ShaderVars = ExtractShaderVars(Assets[PixelShaderGuid], "CBufUberStatic");

	if (!ShaderVars.Count()) // no shader vars matching our buffer
		return;

	RpakStream->SetPosition(this->GetFileOffset(Asset, MatCPUHdr.m_nData.Index, MatCPUHdr.m_nData.Offset));

	std::unique_ptr<char[]> buffer(new char[MatCPUHdr.m_nDataSize]);

	RpakStream->Read((uint8_t*)buffer.get(), 0, MatCPUHdr.m_nDataSize);

	std::ostringstream ss;
	ss << "struct " << "CBufUberStatic\n{\n";

	char* ptr = buffer.get();
	for (auto& it : ShaderVars)
	{
		ss << "\t";

		switch (it.Type)
		{
		case D3D_SVT_INT:
		{
			string str = string::Format("uint32_t %s = %u;", it.Name.ToCString(), *reinterpret_cast<uint32_t*>(ptr));
			ss << str.ToCString();
			break;
		}
		case D3D_SVT_FLOAT:
		{
			int elementCount = it.Size / sizeof(float);

			string str = "";
			switch (elementCount)
			{
			case 1:
				str = string::Format("float %s = %f;", it.Name.ToCString(), *reinterpret_cast<float*>(ptr));
				break;
			case 2:
			{
				Vector2 vec = *reinterpret_cast<Vector2*>(ptr);
				str = string::Format("Vector2 %s = { %f, %f };", it.Name.ToCString(), vec.X, vec.Y);
				break;
			}
			case 3:
			{
				Vector3 vec = *reinterpret_cast<Vector3*>(ptr);
				str = string::Format("Vector3 %s = { %f, %f, %f };", it.Name.ToCString(), vec.X, vec.Y, vec.Z);
				break;
			}
			default:
			{
				string valStr = "";
				for (int i = 0; i < elementCount; ++i)
				{
					valStr += string::Format("%f", *reinterpret_cast<float*>(ptr + (i * sizeof(float))));

					if (i != elementCount - 1)
						valStr += ", ";
				}

				str = string::Format("float %s[%u] = { %s };", it.Name.ToCString(), elementCount, valStr.ToCString());
				break;
			}
			}
			ss << str.ToCString();
			break;
		}
		default:
			string str = string::Format("char UNIMPLEMENTED_%s[%u];", it.Name.ToCString(), it.Size);
			ss << str.ToCString();
			break;
		}
		ptr += it.Size;

		ss << "\n";
	};
	ss << "};";

	oStream << ss.str();
}

// Shaderset reference: 0x7b69f3d87cf71a2b THEY MAY DIFFER NOT SURE.
void RpakLib::ExportMaterialCPU(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

	MaterialCPUHeader MatCPUHdr = Reader.Read<MaterialCPUHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	MaterialHeader MatHeader = Reader.Read<MaterialHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, MatHeader.pName.Index, MatHeader.pName.Offset));

	auto ExportFormat = (MatCPUExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("MatCPUFormat");

	switch (ExportFormat)
	{
	case MatCPUExportFormat_t::None:
	{
		return;
	}
	case MatCPUExportFormat_t::Struct:
	{
		string DestinationPath = IO::Path::Combine(Path, string::Format("%s.h", IO::Path::GetFileNameWithoutExtension(Reader.ReadCString()).ToLower().ToCString()));
		std::ofstream out(DestinationPath.ToCString(), std::ios::out);
		ExportMatCPUAsStruct(Asset, MatHeader, MatCPUHdr, out);
		out.close();
		break;
	}
	case MatCPUExportFormat_t::CPU:
	{
		string DestinationPath = IO::Path::Combine(Path, string::Format("%s.cpu", IO::Path::GetFileNameWithoutExtension(Reader.ReadCString()).ToLower().ToCString()));
		std::ofstream out(DestinationPath.ToCString(), std::ios::out);
		ExportMatCPUAsRaw(Asset, MatHeader, MatCPUHdr, out);
		out.close();
		break;
	}
	default:
		return;
	}
}

void RpakLib::ExportMaterial(const RpakLoadAsset& Asset, const string& Path)
{
	RMdlMaterial Material = this->ExtractMaterial(Asset, Path, false, false);
	string OutPath = IO::Path::Combine(Path, Material.MaterialName);

	if (!Utils::ShouldWriteFile(OutPath))
		return;

	IO::Directory::CreateDirectory(OutPath);

	this->ExportMaterialCPU(Asset, OutPath);

	(void)this->ExtractMaterial(Asset, OutPath, true, true);
}

RMdlMaterial RpakLib::ExtractMaterial(const RpakLoadAsset& Asset, const string& Path, bool IncludeImages, bool IncludeImageNames)
{
	RMdlMaterial Result;

	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	MaterialHeader hdr;

	if (Asset.Version == RpakGameVersion::Apex)
	{
		if (Asset.AssetVersion >= 16)
		{
			MaterialHeaderV16 hdr_v16 = Reader.Read<MaterialHeaderV16>();
			hdr.FromV16(hdr_v16);
		}
		else hdr = Reader.Read<MaterialHeader>();
	}
	else
	{
		MaterialHeaderV12 temp = Reader.Read<MaterialHeaderV12>();

		hdr.pName = temp.pName;
		hdr.textureHandles = temp.textureHandles;
		hdr.streamingTextureHandles = temp.streamingTextureHandles;
		hdr.shaderSetGuid = temp.shaderSetGuid;
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, hdr.pName.Index, hdr.pName.Offset));

	string fullMaterialName = Reader.ReadCString();
	Result.MaterialName = ExportManager::Config.GetBool("UseFullPaths") ? fullMaterialName : IO::Path::GetFileNameWithoutExtension(fullMaterialName);
	Result.FullMaterialName = fullMaterialName;

	List<ShaderResBinding> PixelShaderResBindings;

	bool shadersetLoaded = Assets.ContainsKey(hdr.shaderSetGuid);

	if (shadersetLoaded)
	{
		RpakLoadAsset ShaderSetAsset = Assets[hdr.shaderSetGuid];
		ShaderSetHeader ShaderSetHeader = ExtractShaderSet(ShaderSetAsset);

		uint64_t PixelShaderGuid = ShaderSetHeader.PixelShaderHash;

		if (ShaderSetAsset.AssetVersion <= 11)
		{
			PixelShaderGuid = ShaderSetHeader.OldPixelShaderHash;
		}

		if (Assets.ContainsKey(PixelShaderGuid))
		{
			PixelShaderResBindings = ExtractShaderResourceBindings(Assets[PixelShaderGuid], D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE);
		}
		else {
			g_Logger.Warning("Shaderset for material '%s' referenced a pixel shader that is not currently loaded. Unable to associate texture types.\n", Result.MaterialName.ToCString());
		}
	}

	g_Logger.Info("\nMaterial Info for '%s' (%llX)\n", Result.MaterialName.ToCString(), Asset.NameHash);
	g_Logger.Info("> ShaderSet: %llx (%s)\n", hdr.shaderSetGuid, shadersetLoaded ? "LOADED" : "NOT LOADED");

	const uint64_t TextureTable = this->GetFileOffset(Asset, hdr.textureHandles.Index, hdr.textureHandles.Offset); // (Asset.Version == RpakGameVersion::Apex) ? : this->GetFileOffset(Asset, hdr.TexturesTFIndex, hdr.TexturesTFOffset);
	uint32_t TexturesCount = (hdr.streamingTextureHandles.Offset - hdr.textureHandles.Offset) / 8;
	g_Logger.Info("> %i textures:\n", TexturesCount);

	uint32_t bindingIdx = 0;

	// These textures have named slots
	for (uint32_t i = 0; i < TexturesCount; i++)
	{
		RpakStream->SetPosition(TextureTable + ((uint64_t)i * 8));

		uint64_t TextureHash = Reader.Read<uint64_t>();
		string TextureName = "";
		bool bOverridden = false;
		bool bNormalRecalculate = false;

		if (TextureHash != 0)
		{
			TextureName = string::Format("0x%llx%s", TextureHash, (const char*)ImageExtension);

			if (PixelShaderResBindings.Count() > 0 && bindingIdx < PixelShaderResBindings.Count())
			{
				string ResName = PixelShaderResBindings[bindingIdx].Name;
				if (!ExportManager::Config.GetBool("UseTxtrGuids"))
				{
					TextureName = string::Format("%s_%s%s", Result.MaterialName.ToCString(), ResName.ToCString(), (const char*)ImageExtension);
				}
				bOverridden = true;

				if (ResName == "normalTexture")
					bNormalRecalculate = true;
			}

			if (Asset.Version == RpakGameVersion::Apex)
				g_Logger.Info(">> %i: 0x%llx - %s\n", i, TextureHash, bOverridden ? TextureName.ToCString() : "(no assigned name)");

			switch (i)
			{
			case 0:
				Result.AlbedoHash = TextureHash;
				Result.AlbedoMapName = TextureName;
				break;
			case 1:
				Result.NormalHash = TextureHash;
				Result.NormalMapName = TextureName;
				break;
			case 2:
				Result.GlossHash = TextureHash;
				Result.GlossMapName = TextureName;
				break;
			case 3:
				Result.SpecularHash = TextureHash;
				Result.SpecularMapName = TextureName;
				break;
			case 4:
				Result.EmissiveHash = TextureHash;
				Result.EmissiveMapName = TextureName;
				break;
			case 5:
				Result.AmbientOcclusionHash = TextureHash;
				Result.AmbientOcclusionMapName = TextureName;
				break;
			case 6:
				Result.CavityHash = TextureHash;
				Result.CavityMapName = TextureName;
				break;
			}
			bindingIdx++;

		}
		else
		{
			if (Asset.Version == RpakGameVersion::Apex)
				g_Logger.Info(">> %i: 0x0 - %s\n", i, "(no assigned name)");
		}

		// Extract to disk if need be
		if (IncludeImages && Assets.ContainsKey(TextureHash))
		{
			RpakLoadAsset& Asset = Assets[TextureHash];
			// Make sure the data we got to is a proper texture
			if (Asset.AssetType == (uint32_t)AssetType_t::Texture)
			{
				ExportTexture(Asset, Path, IncludeImageNames, bOverridden ? TextureName : string(), bNormalRecalculate);
			}
		}
	}

	g_Logger.Info("\n");

	return Result;
}

std::unique_ptr<Assets::Texture> RpakLib::BuildPreviewMaterial(uint64_t Hash)
{
	if (!this->Assets.ContainsKey(Hash))
		return nullptr;

	auto& Asset = this->Assets[Hash];
	RMdlMaterial Material = this->ExtractMaterial(Asset, "", false, false);

	return this->BuildPreviewTexture(Material.AlbedoHash);
}