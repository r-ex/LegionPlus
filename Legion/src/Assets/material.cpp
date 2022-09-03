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

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

	MaterialCPUHeader MatCPUHdr = Reader.Read<MaterialCPUHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, MatCPUHdr.m_nData.Index, MatCPUHdr.m_nData.Offset));

	MaterialCPUData MatCPUData = Reader.Read<MaterialCPUData>();

	enum class Types : int
	{
		UINT32,
		FLOAT,
		UVMATRIX,
		VECTOR2,
		VECTOR3
	};

	std::function<void(std::ostringstream&, const string&, void*, Types)> fnAddToStream = [](std::ostringstream& ss, const string& member, void* ptr, Types type)
	{
		ss << "\t";

		switch (type)
		{
		case Types::UINT32:
		{
			string string = string.Format("uint32_t %s = %u;", member.ToCString(), *reinterpret_cast<uint32_t*>(ptr));
			ss << string.ToCString();
			break;
		}
		case Types::FLOAT:
		{
			string string = string.Format("float %s = %f;", member.ToCString(), *reinterpret_cast<float*>(ptr));
			ss << string.ToCString();
			break;
		}
		case Types::UVMATRIX:
		{
			UVTransformMatrix uvMatr = *reinterpret_cast<UVTransformMatrix*>(ptr);
			string string = string.Format("struct %s\n\t{\n\t\tfloat uvScaleX = %f;\n\t\tfloat uvRotationX = %f;\n\t\tfloat uvRotationY = %f;\n\t\tfloat uvScaleY = %f;\n\t\tfloat uvTranslateX = %f;\n\t\tfloat uvTranslateY = %f;\n\t}\n", member.ToCString(), uvMatr.uvScaleX, uvMatr.uvRotationX, uvMatr.uvRotationY, uvMatr.uvScaleY, uvMatr.uvTranslateX, uvMatr.uvTranslateY);
			ss << string.ToCString();
			break;
		}
		case Types::VECTOR2:
		{
			Vector2 vec = *reinterpret_cast<Vector2*>(ptr);
			string string = string.Format("Vector2 %s = { %f, %f };", member.ToCString(), vec.X, vec.Y);
			ss << string.ToCString();
			break;
		}
		case Types::VECTOR3:
		{
			Vector3 vec = *reinterpret_cast<Vector3*>(ptr);
			string string = string.Format("Vector3 %s = { %f, %f, %f };", member.ToCString(), vec.X, vec.Y, vec.Z);
			ss << string.ToCString();
			break;
		}
		}

		ss << "\n";
	};

	std::ostringstream ss;
	ss << "struct " << "CBufUberStatic" << "\n{\n";

#define GET_STRUCT_MEMBER_NAME(name) [](string str) { str = str.Substring(str.IndexOf(".") + 1); return str; } (string(#name))
#define ADDSTREAM(ss, mem, type) fnAddToStream(ss, GET_STRUCT_MEMBER_NAME(mem), &mem, type);

	ADDSTREAM(ss, MatCPUData.c_uv1, Types::UVMATRIX);
	ADDSTREAM(ss, MatCPUData.c_uv2, Types::UVMATRIX);
	ADDSTREAM(ss, MatCPUData.c_uv3, Types::UVMATRIX);
	ADDSTREAM(ss, MatCPUData.c_uv4, Types::UVMATRIX);
	ADDSTREAM(ss, MatCPUData.c_uv5, Types::UVMATRIX);

	ADDSTREAM(ss, MatCPUData.c_uvDistortionIntensity, Types::VECTOR2);
	ADDSTREAM(ss, MatCPUData.c_uvDistortion2Intensity, Types::VECTOR2);

	ADDSTREAM(ss, MatCPUData.c_L0_scatterDistanceScale, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_layerBlendRamp, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_opacity, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_useAlphaModulateSpecular, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_alphaEdgeFadeExponent, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_alphaEdgeFadeInner, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_alphaEdgeFadeOuter, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_useAlphaModulateEmissive, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_emissiveEdgeFadeExponent, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_emissiveEdgeFadeInner, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_emissiveEdgeFadeOuter, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_alphaDistanceFadeScale, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_alphaDistanceFadeBias, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_alphaTestReference, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_aspectRatioMulV, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_shadowBias, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_shadowBiasStatic, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_dofOpacityLuminanceScale, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_tsaaDepthAlphaThreshold, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_tsaaMotionAlphaThreshold, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_tsaaMotionAlphaRamp, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_tsaaResponsiveFlag, Types::UINT32);

	ADDSTREAM(ss, MatCPUData.c_outlineColorSDF, Types::VECTOR3);
	ADDSTREAM(ss, MatCPUData.c_outlineWidthSDF, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_shadowColorSDF, Types::VECTOR3);
	ADDSTREAM(ss, MatCPUData.c_shadowWidthSDF, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_insideColorSDF, Types::VECTOR3);

	ADDSTREAM(ss, MatCPUData.c_outsideAlphaScalarSDF, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_glitchStrength, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_vertexDisplacementScale, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_innerFalloffWidthSDF, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_innerEdgeOffsetSDF, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_dropShadowOffsetSDF, Types::VECTOR2);

	ADDSTREAM(ss, MatCPUData.c_normalMapEdgeWidthSDF, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_shadowFalloffSDF, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_L0_scatterAmount, Types::VECTOR2);
	ADDSTREAM(ss, MatCPUData.c_L0_scatterRatio, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_L0_transmittanceIntensityScale, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_vertexDisplacementDirection, Types::VECTOR2);

	ADDSTREAM(ss, MatCPUData.c_L0_transmittanceAmount, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_L0_transmittanceDistortionAmount, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_zUpBlendingMinAngleCos, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_zUpBlendingMaxAngleCos, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_zUpBlendingVertexAlpha, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_L0_albedoTint, Types::VECTOR3);

	ADDSTREAM(ss, MatCPUData.c_depthBlendScalar, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_L0_emissiveTint, Types::VECTOR3);

	ADDSTREAM(ss, MatCPUData.c_subsurfaceMaterialID, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_L0_perfSpecColor, Types::VECTOR3);

	ADDSTREAM(ss, MatCPUData.c_L0_perfGloss, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_L1_albedoTint, Types::VECTOR3);

	ADDSTREAM(ss, MatCPUData.c_L1_perfGloss, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_L1_emissiveTint, Types::VECTOR3);
	ADDSTREAM(ss, MatCPUData.c_L1_perfSpecColor, Types::VECTOR3);

	ADDSTREAM(ss, MatCPUData.c_splineMinPixelPercent, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_L0_anisoSpecCosSinTheta, Types::VECTOR2);
	ADDSTREAM(ss, MatCPUData.c_L1_anisoSpecCosSinTheta, Types::VECTOR2);

	ADDSTREAM(ss, MatCPUData.c_L0_anisoSpecStretchAmount, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_L1_anisoSpecStretchAmount, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_L0_emissiveHeightFalloff, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_L1_emissiveHeightFalloff, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_L1_transmittanceIntensityScale, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_L1_transmittanceAmount, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_L1_transmittanceDistortionAmount, Types::FLOAT);

	ADDSTREAM(ss, MatCPUData.c_L1_scatterDistanceScale, Types::FLOAT);
	ADDSTREAM(ss, MatCPUData.c_L1_scatterAmount, Types::VECTOR3);
	ADDSTREAM(ss, MatCPUData.c_L1_scatterRatio, Types::FLOAT);
#undef GET_STRUCT_MEMBER_NAME
#undef ADDSTREAM

	ss << "}";

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	MaterialHeader MatHeader = Reader.Read<MaterialHeader>();

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