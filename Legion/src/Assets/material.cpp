#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"

#include <typeinfo>
#include <typeindex>

void RpakLib::BuildMaterialInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	MaterialHeader MatHeader = Reader.Read<MaterialHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, MatHeader.NameIndex, MatHeader.NameOffset));

	string MaterialName = Reader.ReadCString();

	if (ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = MaterialName;
	else
		Info.Name = IO::Path::GetFileNameWithoutExtension(MaterialName).ToLower();

	Info.Type = ApexAssetType::Material;
	Info.Status = ApexAssetStatus::Loaded;

	uint32_t TexturesCount = 0;

	switch (Asset.Version) {
	case RpakGameVersion::Apex:
		TexturesCount = (MatHeader.StreamableTexturesOffset - MatHeader.TexturesOffset) / 8;
		break;
	case RpakGameVersion::Titanfall:
	case RpakGameVersion::R2TT: // unverified but should work
		TexturesCount = (MatHeader.UnknownTFOffset - MatHeader.TexturesTFOffset) / 8;
		break;
	}
	Info.Info = string::Format("Textures: %i", TexturesCount);
}

// Shaderset reference: 0x7b69f3d87cf71a2b THEY MAY DIFFER NOT SURE.
void RpakLib::ExportMaterialCPU(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	MaterialHeader MatHeader = Reader.Read<MaterialHeader>();

	if (Asset.AssetVersion == 12)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
		MaterialHeaderV12 mathdr = Reader.Read<MaterialHeaderV12>();

		MatHeader.ShaderSetHash = mathdr.m_pShaderSet;
	}

	if (!Assets.ContainsKey(MatHeader.ShaderSetHash)) // no shaderset loaded
		return;

	RpakLoadAsset ShaderSetAsset = Assets[MatHeader.ShaderSetHash];
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

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

	MaterialCPUHeader MatCPUHdr = Reader.Read<MaterialCPUHeader>();

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
	ss << "}";

	RpakStream->SetPosition(this->GetFileOffset(Asset, MatHeader.NameIndex, MatHeader.NameOffset));

	string DestinationPath = IO::Path::Combine(Path, string::Format("%s.h", IO::Path::GetFileNameWithoutExtension(Reader.ReadCString()).ToLower().ToCString()));

	std::ofstream out(DestinationPath.ToCString(), std::ios::out);
	out << ss.str();
	out.close();
}

void RpakLib::ExportMaterial(const RpakLoadAsset& Asset, const string& Path)
{
	RMdlMaterial Material = this->ExtractMaterial(Asset, Path, false, false);
	string OutPath = IO::Path::Combine(Path, Material.MaterialName);

	if (!Utils::ShouldWriteFile(OutPath))
		return;

	IO::Directory::CreateDirectory(OutPath);

	if (ExportManager::Config.GetBool("ExportMatCPU"))
		this->ExportMaterialCPU(Asset, OutPath);

	(void)this->ExtractMaterial(Asset, OutPath, true, true);
}

RMdlMaterial RpakLib::ExtractMaterial(const RpakLoadAsset& Asset, const string& Path, bool IncludeImages, bool IncludeImageNames)
{
	RMdlMaterial Result;

	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	MaterialHeader MatHeader = Reader.Read<MaterialHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, MatHeader.NameIndex, MatHeader.NameOffset));

	Result.MaterialName = IO::Path::GetFileNameWithoutExtension(Reader.ReadCString());

	List<ShaderResBinding> PixelShaderResBindings;

	bool shadersetLoaded = Assets.ContainsKey(MatHeader.ShaderSetHash);

	if (shadersetLoaded)
	{
		RpakLoadAsset ShaderSetAsset = Assets[MatHeader.ShaderSetHash];
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
	g_Logger.Info("> ShaderSet: %llx (%s)\n", MatHeader.ShaderSetHash, shadersetLoaded ? "LOADED" : "NOT LOADED");

	const uint64_t TextureTable = (Asset.Version == RpakGameVersion::Apex) ? this->GetFileOffset(Asset, MatHeader.TexturesIndex, MatHeader.TexturesOffset) : this->GetFileOffset(Asset, MatHeader.TexturesTFIndex, MatHeader.TexturesTFOffset);
	uint32_t TexturesCount = (Asset.Version == RpakGameVersion::Apex) ? 0x10 : 0x11;

	// we're actually gonna ignore the hardcoded value for apex materials
	if (Asset.Version == RpakGameVersion::Apex)
	{
		TexturesCount = (MatHeader.StreamableTexturesOffset - MatHeader.TexturesOffset) / 8;
		g_Logger.Info("> %i textures:\n", TexturesCount);
	}
	else {
		TexturesCount = (MatHeader.UnknownTFOffset - MatHeader.TexturesTFOffset) / 8;
		g_Logger.Info("> %i textures:\n", TexturesCount);
	}

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

			if (PixelShaderResBindings.Count() > 0 && i < PixelShaderResBindings.Count())
			{
				string ResName = PixelShaderResBindings[i].Name;
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