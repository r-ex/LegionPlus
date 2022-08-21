#include "pch.h"
#include "MdlLib.h"
#include "File.h"
#include "Directory.h"
#include "Path.h"
#include "Half.h"

#include "SEAsset.h"
#include "AutodeskMaya.h"
#include "WavefrontOBJ.h"
#include "XNALaraAscii.h"
#include "XNALaraBinary.h"
#include "ValveSMD.h"
#include "KaydaraFBX.h"
#include "CoDXAssetExport.h"

#include "RBspLib.h"
#include "CastAsset.h"

RBspLib::RBspLib()
{
}

void RBspLib::InitializeModelExporter(ModelExportFormat_t Format)
{
	switch (Format)
	{
	case ModelExportFormat_t::Maya:
		ModelExporter = std::make_unique<Assets::Exporters::AutodeskMaya>();
		break;
	case ModelExportFormat_t::OBJ:
		ModelExporter = std::make_unique<Assets::Exporters::WavefrontOBJ>();
		break;
	case ModelExportFormat_t::XNALaraText:
		ModelExporter = std::make_unique<Assets::Exporters::XNALaraAscii>();
		break;
	case ModelExportFormat_t::XNALaraBinary:
		ModelExporter = std::make_unique<Assets::Exporters::XNALaraBinary>();
		break;
	case ModelExportFormat_t::SMD:
		ModelExporter = std::make_unique<Assets::Exporters::ValveSMD>();
		break;
	case ModelExportFormat_t::XModel:
		ModelExporter = std::make_unique<Assets::Exporters::CoDXAssetExport>();
		break;
	case ModelExportFormat_t::FBX:
		ModelExporter = std::make_unique<Assets::Exporters::KaydaraFBX>();
		break;
	case ModelExportFormat_t::Cast:
		ModelExporter = std::make_unique<Assets::Exporters::CastAsset>();
		break;
	default:
		ModelExporter = std::make_unique<Assets::Exporters::SEAsset>();
		break;
	}
}

#pragma pack(push, 1)
struct RBspTexture
{
	uint32_t NameIndex;
	uint32_t Width;
	uint32_t Height;
	uint32_t Flags;
};

struct TFRBspTexture
{
	Math::Vector3 Reflectivity;
	uint32_t NameIndex;
	uint32_t Width;
	uint32_t Height;
	uint32_t ViewWidth;
	uint32_t ViewHeight;
	uint32_t Flags;
};

struct RBspLumpHeader
{
	uint32_t Offset;
	uint32_t DataSize;
	uint32_t Version;
	uint32_t UncompressedSize;
};

struct RBspModel
{
	float Mins[3];
	float Maxs[3];
	uint32_t MeshStart;
	uint32_t MeshCount;
	uint32_t Unknowns[8];
};

struct TFRBspModel
{
	float Mins[3];
	float Maxs[3];
	uint32_t MeshStart;
	uint32_t MeshCount;
};

struct RBspMesh
{
	uint32_t FaceStart;
	uint16_t FaceCount;
	uint16_t Unknown1;
	uint32_t Unknowns[3];
	uint16_t Unknown2;
	uint16_t MaterialIndex;
	uint32_t Flags;
};

struct RBspMaterial
{
	uint16_t TextureIndex;
	uint16_t LightmapIndex;
	uint16_t Unknowns[2];
	uint32_t VertexOffset;
};

struct RBspVertexLitBump
{
	uint32_t PositionIndex;
	uint32_t NormalIndex;
	Math::Vector2 UVs;
	uint32_t NegativeOne;
	Math::Vector2 UVs2;
	uint8_t RGBA[4];
};

struct TFRBspVertexLitBump
{
	uint32_t PositionIndex;
	uint32_t NormalIndex;
	Math::Vector2 UVs;
	Math::Vector2 UVs2;
	Math::Vector2 UVs3;
	uint32_t Unknowns[3];
};

struct RBspVertexLitFlat
{
	uint32_t PositionIndex;
	uint32_t NormalIndex;
	Math::Vector2 UVs;
	uint32_t Unknown;
};

struct RBspVertexUnlit
{
	uint32_t PositionIndex;
	uint32_t NormalIndex;
	Math::Vector2 UVs;
	uint32_t Unknown;
};

struct RBspVertexUnlitTS
{
	uint32_t PositionIndex;
	uint32_t NormalIndex;
	Math::Vector2 UVs;
	uint32_t Unknown;
	uint32_t Unknown2;
};

struct TFRBspVertexUnlitTS
{
	uint32_t PositionIndex;
	uint32_t NormalIndex;
	Math::Vector2 UVs;
	uint32_t Unknown[3];
};

struct RBspPropsHeader
{
	uint32_t Version;
	uint32_t Magic;
	uint32_t Flags;
	uint64_t Hash;
	uint32_t NameCount;
};

struct RBspProp
{
	Math::Vector3 Position;
	Math::Vector3 Rotation;
	float Scale;
	uint16_t NameIndex;

	uint8_t Padding[0x22];
};
#pragma pack(pop)

void RBspLib::ExportPropContainer(std::unique_ptr<IO::MemoryStream>& Stream, const string& Name, const string& Path)
{
	auto Reader = IO::BinaryReader(Stream.get(), true);
	auto Output = IO::BinaryWriter(IO::File::Create(IO::Path::Combine(Path, Name + ".mprt")));
	auto Header = Reader.Read<RBspPropsHeader>();

	List<string> Names;

	for (uint32_t i = 0; i < Header.NameCount; i++)
	{
		char Buffer[0x80]{};
		Reader.Read(Buffer, 0, 0x80);

		Names.EmplaceBack(string(Buffer));
	}

	auto PropCount = Reader.Read<uint32_t>();
	Stream->Seek(0x8, IO::SeekOrigin::Current);

	Output.Write<uint32_t>(0x7472706D);
	Output.Write<uint32_t>(0x3);
	Output.Write<uint32_t>(PropCount);

	for (uint32_t i = 0; i < PropCount; i++)
	{
		auto Prop = Reader.Read<RBspProp>();
		auto Name = IO::Path::GetFileNameWithoutExtension(Names[Prop.NameIndex]).ToLower();

		if (!this->PropModelNames.Contains(Name))
			this->PropModelNames.EmplaceBack(Name);

		Output.WriteCString(Name);
		Output.Write<Math::Vector3>(Prop.Position);
		Output.Write<Math::Vector3>(Math::Vector3(Prop.Rotation.Z, Prop.Rotation.X, Prop.Rotation.Y));
		Output.Write<float>(Prop.Scale);
	}
}

#define READ_LUMP(X, D) \
			if (Header.IsEntirelyStreamed) { \
				ReadExternalLumpFile(BasePath, D, Lumps[D].DataSize, X); \
			} else { \
				Stream->SetPosition(Lumps[D].Offset); \
				Stream->Read((uint8_t*)&X[0], 0, Lumps[D].DataSize); \
			}

template<typename T>
static void ReadExternalLumpFile(const string& BasePath, uint32_t Lump, uint32_t DataSize, List<T>& Data)
{
	string Path = string::Format("%s.bsp.%04x.bsp_lump", BasePath.ToCString(), Lump);

	if (IO::File::Exists(Path))
		IO::File::OpenRead(Path)->Read((uint8_t*)&Data[0], 0, DataSize);
	//else
	//{
	//	string ex = "Couldn't find file " + IO::Path::GetFileName(Path) + "\nMake sure you have exported all .bsp_lump files for this map";
	//	throw std::exception(ex.ToCString());
	//}
}

static void ReadExternalLumpArrayFile(const string& BasePath, uint32_t Lump, uint32_t DataSize, uint8_t* Data)
{
	string Path = string::Format("%s.bsp.%04x.bsp_lump", BasePath.ToCString(), Lump);

	if(IO::File::Exists(Path))
		IO::File::OpenRead(Path)->Read(Data, 0, DataSize);
	else
	{
		string ex = "Couldn't find file " + IO::Path::GetFileName(Path) + "\nMake sure you have exported all .bsp_lump files for this map";
		throw std::exception(ex.ToCString());
	}
	
}

void RBspLib::ExportBsp(const std::unique_ptr<RpakLib>& RpakFileSystem, const string& Asset, const string& Path)
{
	std::unique_ptr<IO::FileStream> Stream = IO::File::OpenRead(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(Stream.get(), true);
	RBspHeader Header = Reader.Read<RBspHeader>();

	if (Header.Magic != 0x50534272) // rBSP
		throw std::exception("Invalid bsp file magic. Expected 'rBSP'");

	switch (Header.Version)
	{
	case 0x25:
		ExportTitanfall2Bsp(RpakFileSystem, Stream, Header, Asset, Path);
		break;
	default:
		if (Header.Version < 0x2F)
			throw std::exception(string::Format("Unsupported bsp version: %i", Header.Version));

		ExportApexBsp(RpakFileSystem, Stream, Header, Asset, Path);
		break;
	}
}

void RBspLib::ExportApexBsp(const std::unique_ptr<RpakLib>& RpakFileSystem, std::unique_ptr<IO::FileStream>& Stream, RBspHeader Header, const string& Asset, const string& Path)
{
	auto Model = std::make_unique<Assets::Model>(0, 0);

	auto ModelName = IO::Path::GetFileNameWithoutExtension(Asset);
	auto ModelPath = IO::Path::Combine(Path, ModelName);
	auto TexturePath = IO::Path::Combine(ModelPath, "_images");
	auto BasePath = IO::Path::Combine(IO::Path::GetDirectoryName(Asset), IO::Path::GetFileNameWithoutExtension(Asset));

	IO::Directory::CreateDirectory(TexturePath);

	Model->Name = ModelName;

	const uint32_t TEXTURES = 0x2;
	const uint32_t VERTICES = 0x3;
	const uint32_t MODELS = 0xE;
	const uint32_t SURFACE_NAMES = 0xF;
	const uint32_t NORMALS = 0x1E;
	const uint32_t GAME_LUMPS = 0x23;
	const uint32_t VERTEX_UNLIT = 0x47;
	const uint32_t VERTEX_LIT_FLAT = 0x48;
	const uint32_t VERTEX_LIT_BUMP = 0x49;
	const uint32_t VERTEX_UNLIT_TS = 0x4A;
	const uint32_t FACES = 0x4F;
	const uint32_t MESHES = 0x50;
	const uint32_t MATERIALS = 0x52;

	auto Lumps = List<RBspLumpHeader>(Header.NumLumpsMinusOne + 1, true);
	Stream->Read((uint8_t*)&Lumps[0], 0, sizeof(RBspLumpHeader) * (Header.NumLumpsMinusOne + 1));

	auto Models = List<RBspModel>(Lumps[MODELS].DataSize / sizeof(RBspModel), true);
	READ_LUMP(Models, MODELS);

	auto Meshes = List<RBspMesh>(Lumps[MESHES].DataSize / sizeof(RBspMesh), true);
	READ_LUMP(Meshes, MESHES);

	auto Materials = List<RBspMaterial>(Lumps[MATERIALS].DataSize / sizeof(RBspMaterial), true);
	READ_LUMP(Materials, MATERIALS);

	auto Textures = List<RBspTexture>(Lumps[TEXTURES].DataSize / sizeof(RBspTexture), true);
	READ_LUMP(Textures, TEXTURES);

	auto NameTable = List<uint8_t>(Lumps[SURFACE_NAMES].DataSize, true);
	READ_LUMP(NameTable, SURFACE_NAMES);

	auto Faces = List<uint16_t>(Lumps[FACES].DataSize / sizeof(uint16_t), true);
	READ_LUMP(Faces, FACES);

	auto Vertices = List<Math::Vector3>(Lumps[VERTICES].DataSize / sizeof(Math::Vector3), true);
	READ_LUMP(Vertices, VERTICES);
	auto VertexNormals = List<Math::Vector3>(Lumps[NORMALS].DataSize / sizeof(Math::Vector3), true);
	READ_LUMP(VertexNormals, NORMALS);

	auto VertexLitBumps = List<RBspVertexLitBump>(Lumps[VERTEX_LIT_BUMP].DataSize / sizeof(RBspVertexLitBump), true);
	READ_LUMP(VertexLitBumps, VERTEX_LIT_BUMP);
	auto VertexLitFlat = List<RBspVertexLitFlat>(Lumps[VERTEX_LIT_FLAT].DataSize / sizeof(RBspVertexLitFlat), true);
	READ_LUMP(VertexLitFlat, VERTEX_LIT_FLAT);
	auto VertexUnlit = List<RBspVertexUnlit>(Lumps[VERTEX_UNLIT].DataSize / sizeof(RBspVertexUnlit), true);
	READ_LUMP(VertexUnlit, VERTEX_UNLIT);
	auto VertexUnlitTS = List<RBspVertexUnlitTS>(Lumps[VERTEX_UNLIT_TS].DataSize / sizeof(RBspVertexUnlitTS), true);
	READ_LUMP(VertexUnlitTS, VERTEX_UNLIT_TS);

	std::unique_ptr<List<ApexAsset>> RpakMaterials;
	Dictionary<string, RpakLoadAsset> RpakMaterialLookup;

	// make sure that RpakFileSystem actually exists (i.e. an rpak is loaded)
	if (RpakFileSystem)
	{
		std::array<bool, 8> bAssets = {
			false, // Model
			false, // Animation
			false, // Texture/Images
			true,  // Material
			false, // UIIA
			false, // Datatables
			false, // ShaderSets
			false, // SettingsSets
		};

		RpakMaterials = RpakFileSystem->BuildAssetList(bAssets);

		for (auto& Mat : *RpakMaterials)
		{
			RpakMaterialLookup.Add(Mat.Name, RpakFileSystem->Assets[Mat.Hash]);
		}
	}

	for (auto& BspModel : Models)
	{
		for (uint32_t m = BspModel.MeshStart; m < (BspModel.MeshStart + BspModel.MeshCount); m++)
		{
			auto& BspMesh = Meshes[m];

			if (BspMesh.FaceCount <= 0)
				continue;

			auto FaceLump = BspMesh.Flags & 0x600;

			auto& Material = Materials[BspMesh.MaterialIndex];
			auto& Texture = Textures[Material.TextureIndex];
			auto& Mesh = Model->Meshes.Emplace(0, 1);

			auto MaterialName = string((const char*)&NameTable[Texture.NameIndex]);
			auto CleanedMaterialName = IO::Path::GetFileNameWithoutExtension(MaterialName).ToLower();

			int32_t MaterialIndex = -1;

			if (RpakMaterialLookup.ContainsKey(CleanedMaterialName))
			{
				auto MaterialAsset = RpakMaterialLookup[CleanedMaterialName];
				auto ParsedMaterial = RpakFileSystem->ExtractMaterial(MaterialAsset, TexturePath, true, false);
				auto MaterialIndex = Model->AddMaterial(ParsedMaterial.MaterialName, ParsedMaterial.AlbedoHash);

				auto& MaterialInstance = Model->Materials[MaterialIndex];

				if (ParsedMaterial.AlbedoMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::Albedo, { "_images\\" + ParsedMaterial.AlbedoMapName, ParsedMaterial.AlbedoHash });
				if (ParsedMaterial.NormalMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::Normal, { "_images\\" + ParsedMaterial.NormalMapName, ParsedMaterial.NormalHash });
				if (ParsedMaterial.GlossMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::Gloss, { "_images\\" + ParsedMaterial.GlossMapName, ParsedMaterial.GlossHash });
				if (ParsedMaterial.SpecularMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::Specular, { "_images\\" + ParsedMaterial.SpecularMapName, ParsedMaterial.SpecularHash });
				if (ParsedMaterial.EmissiveMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::Emissive, { "_images\\" + ParsedMaterial.EmissiveMapName, ParsedMaterial.EmissiveHash });
				if (ParsedMaterial.AmbientOcclusionMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::AmbientOcclusion, { "_images\\" + ParsedMaterial.AmbientOcclusionMapName, ParsedMaterial.AmbientOcclusionHash });
				if (ParsedMaterial.CavityMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::Cavity, {"_images\\" + ParsedMaterial.CavityMapName, ParsedMaterial.CavityHash });

				Mesh.MaterialIndices.EmplaceBack(MaterialIndex);
			}
			else
			{
				Mesh.MaterialIndices.EmplaceBack(Model->AddMaterial(CleanedMaterialName, 0xDEADBEEF));
			}

			uint32_t FaceIndex = 0;

			if (FaceLump == 0x000)
			{
				for (uint32_t v = BspMesh.FaceStart; v < (BspMesh.FaceStart + (BspMesh.FaceCount * 3)); v += 3)
				{
					auto& V1 = VertexLitFlat[Faces[v] + Material.VertexOffset];
					auto& V2 = VertexLitFlat[Faces[v + 1] + Material.VertexOffset];
					auto& V3 = VertexLitFlat[Faces[v + 2] + Material.VertexOffset];

					Mesh.Vertices.EmplaceBack(Vertices[V1.PositionIndex], VertexNormals[V1.NormalIndex], Assets::VertexColor(), V1.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V2.PositionIndex], VertexNormals[V2.NormalIndex], Assets::VertexColor(), V2.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V3.PositionIndex], VertexNormals[V3.NormalIndex], Assets::VertexColor(), V3.UVs);

					Mesh.Faces.EmplaceBack(FaceIndex, FaceIndex + 1, FaceIndex + 2);
					FaceIndex += 3;
				}
			}
			else if (FaceLump == 0x200)
			{
				for (uint32_t v = BspMesh.FaceStart; v < (BspMesh.FaceStart + (BspMesh.FaceCount * 3)); v += 3)
				{
					auto& V1 = VertexLitBumps[Faces[v] + Material.VertexOffset];
					auto& V2 = VertexLitBumps[Faces[v + 1] + Material.VertexOffset];
					auto& V3 = VertexLitBumps[Faces[v + 2] + Material.VertexOffset];

					Mesh.Vertices.EmplaceBack(Vertices[V1.PositionIndex], VertexNormals[V1.NormalIndex], Assets::VertexColor(), V1.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V2.PositionIndex], VertexNormals[V2.NormalIndex], Assets::VertexColor(), V2.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V3.PositionIndex], VertexNormals[V3.NormalIndex], Assets::VertexColor(), V3.UVs);

					Mesh.Faces.EmplaceBack(FaceIndex, FaceIndex + 1, FaceIndex + 2);
					FaceIndex += 3;
				}
			}
			else if (FaceLump == 0x400)
			{
				for (uint32_t v = BspMesh.FaceStart; v < (BspMesh.FaceStart + (BspMesh.FaceCount * 3)); v += 3)
				{
					auto& V1 = VertexUnlit[Faces[v] + Material.VertexOffset];
					auto& V2 = VertexUnlit[Faces[v + 1] + Material.VertexOffset];
					auto& V3 = VertexUnlit[Faces[v + 2] + Material.VertexOffset];

					Mesh.Vertices.EmplaceBack(Vertices[V1.PositionIndex], VertexNormals[V1.NormalIndex], Assets::VertexColor(), V1.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V2.PositionIndex], VertexNormals[V2.NormalIndex], Assets::VertexColor(), V2.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V3.PositionIndex], VertexNormals[V3.NormalIndex], Assets::VertexColor(), V3.UVs);

					Mesh.Faces.EmplaceBack(FaceIndex, FaceIndex + 1, FaceIndex + 2);
					FaceIndex += 3;
				}
			}
			else if (FaceLump == 0x600)
			{
				for (uint32_t v = BspMesh.FaceStart; v < (BspMesh.FaceStart + (BspMesh.FaceCount * 3)); v += 3)
				{
					auto& V1 = VertexUnlitTS[Faces[v] + Material.VertexOffset];
					auto& V2 = VertexUnlitTS[Faces[v + 1] + Material.VertexOffset];
					auto& V3 = VertexUnlitTS[Faces[v + 2] + Material.VertexOffset];

					Mesh.Vertices.EmplaceBack(Vertices[V1.PositionIndex], VertexNormals[V1.NormalIndex], Assets::VertexColor(), V1.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V2.PositionIndex], VertexNormals[V2.NormalIndex], Assets::VertexColor(), V2.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V3.PositionIndex], VertexNormals[V3.NormalIndex], Assets::VertexColor(), V3.UVs);

					Mesh.Faces.EmplaceBack(FaceIndex, FaceIndex + 1, FaceIndex + 2);
					FaceIndex += 3;
				}
			}
		}
	}

	this->ModelExporter->ExportModel(*Model.get(), IO::Path::Combine(ModelPath, Model->Name + "_LOD0" + (const char*)ModelExporter->ModelExtension()));

	auto Buffer = new uint8_t[Lumps[GAME_LUMPS].DataSize];
	if (Header.IsEntirelyStreamed)
	{
		ReadExternalLumpArrayFile(BasePath, GAME_LUMPS, Lumps[GAME_LUMPS].DataSize, &Buffer[0]);
	}
	else {
		Stream->SetPosition(Lumps[GAME_LUMPS].Offset);
		Stream->Read(Buffer, 0, Lumps[GAME_LUMPS].DataSize);
	}

	auto Stream2 = std::make_unique<IO::MemoryStream>(Buffer, 0, Lumps[GAME_LUMPS].DataSize);
	this->ExportPropContainer(Stream2, Model->Name + "_LOD0", ModelPath);

	// Export all of the bsp's prop models

	auto ExportedModelsPath = IO::Path::Combine(ModelPath, "_models");
	auto ExportedModelAnimsPath = IO::Path::Combine(ExportedModelsPath, "_animations");

	if (!RpakFileSystem)
		return;

	std::array<bool, 8> bAssets = {
		true,  // Model
		false, // Animation
		false, // Texture/Images
		false, // Material
		false, // UIIA
		false, // Datatables
		false, // ShaderSets
		false, // SettingsSets
	};

	auto RpakModels = RpakFileSystem->BuildAssetList(bAssets);
	Dictionary<string, RpakLoadAsset> RpakModelLookup;

	for (auto& tm : *RpakModels)
	{
		RpakModelLookup.Add(tm.Name, RpakFileSystem->Assets[tm.Hash]);
	}

	for (uint32_t i = 0; i < this->PropModelNames.Count(); ++i)
	{
		string ModelName = this->PropModelNames[i];
		if (RpakModelLookup.ContainsKey(ModelName))
		{
			auto ModelAsset = RpakModelLookup[ModelName];
			RpakFileSystem->ExportModel(ModelAsset, ExportedModelsPath, ExportedModelAnimsPath);
		}
	}
}

void RBspLib::ExportTitanfall2Bsp(const std::unique_ptr<RpakLib>& RpakFileSystem, std::unique_ptr<IO::FileStream>& Stream, RBspHeader Header, const string& Asset, const string& Path)
{
	auto Model = std::make_unique<Assets::Model>(0, 0);

	auto ModelName = IO::Path::GetFileNameWithoutExtension(Asset);
	auto ModelPath = IO::Path::Combine(Path, ModelName);
	auto TexturePath = IO::Path::Combine(ModelPath, "_images");
	auto BasePath = IO::Path::Combine(IO::Path::GetDirectoryName(Asset), IO::Path::GetFileNameWithoutExtension(Asset));

	IO::Directory::CreateDirectory(TexturePath);

	Model->Name = ModelName;

	const uint32_t TEXTURES = 0x2;
	const uint32_t VERTICES = 0x3;
	const uint32_t MODELS = 0xE;
	const uint32_t SURFACE_NAMES = 0x2B;
	const uint32_t NORMALS = 0x1E;
	const uint32_t GAME_LUMPS = 0x23;
	const uint32_t VERTEX_UNLIT = 0x47;
	const uint32_t VERTEX_LIT_FLAT = 0x48;
	const uint32_t VERTEX_LIT_BUMP = 0x49;
	const uint32_t VERTEX_UNLIT_TS = 0x4A;
	const uint32_t FACES = 0x4F;
	const uint32_t MESHES = 0x50;
	const uint32_t MATERIALS = 0x52;

	auto Lumps = List<RBspLumpHeader>(Header.NumLumpsMinusOne + 1, true);
	Stream->Read((uint8_t*)&Lumps[0], 0, sizeof(RBspLumpHeader) * (Header.NumLumpsMinusOne + 1));

	auto Models = List<TFRBspModel>(Lumps[MODELS].DataSize / sizeof(TFRBspModel), true);
	READ_LUMP(Models, MODELS);

	auto Meshes = List<RBspMesh>(Lumps[MESHES].DataSize / sizeof(RBspMesh), true);
	READ_LUMP(Meshes, MESHES);

	auto Materials = List<RBspMaterial>(Lumps[MATERIALS].DataSize / sizeof(RBspMaterial), true);
	READ_LUMP(Materials, MATERIALS);
	//
	auto Textures = List<TFRBspTexture>(Lumps[TEXTURES].DataSize / sizeof(TFRBspTexture), true);
	READ_LUMP(Textures, TEXTURES);

	List<string> NameStringTable;
	auto NameTable = List<uint8_t>(Lumps[SURFACE_NAMES].DataSize, true);
	READ_LUMP(NameTable, SURFACE_NAMES);

	string temp;
	for (auto& e : NameTable)
	{
		temp += e;
		if (e == '\0')
		{
			NameStringTable.EmplaceBack(temp);
			temp = "";
		}
	}

	auto Faces = List<uint16_t>(Lumps[FACES].DataSize / sizeof(uint16_t), true);
	READ_LUMP(Faces, FACES);

	auto Vertices = List<Math::Vector3>(Lumps[VERTICES].DataSize / sizeof(Math::Vector3), true);
	READ_LUMP(Vertices, VERTICES);
	auto VertexNormals = List<Math::Vector3>(Lumps[NORMALS].DataSize / sizeof(Math::Vector3), true);
	READ_LUMP(VertexNormals, NORMALS);

	auto VertexLitBumps = List<TFRBspVertexLitBump>(Lumps[VERTEX_LIT_BUMP].DataSize / sizeof(TFRBspVertexLitBump), true);
	READ_LUMP(VertexLitBumps, VERTEX_LIT_BUMP);

	auto VertexLitFlat = List<RBspVertexLitFlat>(Lumps[VERTEX_LIT_FLAT].DataSize / sizeof(RBspVertexLitFlat), true);
	READ_LUMP(VertexLitFlat, VERTEX_LIT_FLAT);
	auto VertexUnlit = List<RBspVertexUnlit>(Lumps[VERTEX_UNLIT].DataSize / sizeof(RBspVertexUnlit), true);
	READ_LUMP(VertexUnlit, VERTEX_UNLIT);
	auto VertexUnlitTS = List<TFRBspVertexUnlitTS>(Lumps[VERTEX_UNLIT_TS].DataSize / sizeof(TFRBspVertexUnlitTS), true);
	READ_LUMP(VertexUnlitTS, VERTEX_UNLIT_TS);

	std::unique_ptr<List<ApexAsset>> RpakMaterials;
	Dictionary<string, RpakLoadAsset> RpakMaterialLookup;

	// make sure that RpakFileSystem actually exists (i.e. an rpak is loaded)
	if (RpakFileSystem)
	{
		std::array<bool, 8> bAssets = {
			false, // Model
			false, // Animation
			false, // Texture/Images
			true,  // Material
			false, // UIIA
			false, // Datatables
			false, // ShaderSets
			false, // SettingsSets
		};

		RpakMaterials = RpakFileSystem->BuildAssetList(bAssets);

		for (auto& Mat : *RpakMaterials)
		{
			RpakMaterialLookup.Add(Mat.Name, RpakFileSystem->Assets[Mat.Hash]);
		}
	}

	for (auto& BspModel : Models)
	{
		for (uint32_t m = BspModel.MeshStart; m < (BspModel.MeshStart + BspModel.MeshCount); m++)
		{
			auto& BspMesh = Meshes[m];

			if (BspMesh.FaceCount <= 0)
				continue;

			auto FaceLump = BspMesh.Flags & 0x600;

			auto& Material = Materials[BspMesh.MaterialIndex];
			auto& Texture = Textures[Material.TextureIndex];
			auto& Mesh = Model->Meshes.Emplace(0, 1);

			string MaterialName = NameStringTable[Texture.NameIndex];
			string CleanedMaterialName = IO::Path::GetFileNameWithoutExtension(MaterialName).ToLower();

			int32_t MaterialIndex = -1;

			if (RpakMaterialLookup.ContainsKey(CleanedMaterialName))
			{
				auto MaterialAsset = RpakMaterialLookup[CleanedMaterialName];
				auto ParsedMaterial = RpakFileSystem->ExtractMaterial(MaterialAsset, TexturePath, true, false);
				auto MaterialIndex = Model->AddMaterial(ParsedMaterial.MaterialName, ParsedMaterial.AlbedoHash);

				auto& MaterialInstance = Model->Materials[MaterialIndex];

				if (ParsedMaterial.AlbedoMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::Albedo, { "_images\\" + ParsedMaterial.AlbedoMapName, ParsedMaterial.AlbedoHash });
				if (ParsedMaterial.NormalMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::Normal, { "_images\\" + ParsedMaterial.NormalMapName, ParsedMaterial.NormalHash });
				if (ParsedMaterial.GlossMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::Gloss, { "_images\\" + ParsedMaterial.GlossMapName, ParsedMaterial.GlossHash });
				if (ParsedMaterial.SpecularMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::Specular, { "_images\\" + ParsedMaterial.SpecularMapName, ParsedMaterial.SpecularHash });
				if (ParsedMaterial.EmissiveMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::Emissive, { "_images\\" + ParsedMaterial.EmissiveMapName, ParsedMaterial.EmissiveHash });
				if (ParsedMaterial.AmbientOcclusionMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::AmbientOcclusion, { "_images\\" + ParsedMaterial.AmbientOcclusionMapName, ParsedMaterial.AmbientOcclusionHash });
				if (ParsedMaterial.CavityMapName != "")
					MaterialInstance.Slots.Add(Assets::MaterialSlotType::Cavity, { "_images\\" + ParsedMaterial.CavityMapName, ParsedMaterial.CavityHash });

				Mesh.MaterialIndices.EmplaceBack(MaterialIndex);
			}
			else
			{
				Mesh.MaterialIndices.EmplaceBack(Model->AddMaterial(CleanedMaterialName, 0xDEADBEEF));
			}

			uint32_t FaceIndex = 0;

			if (FaceLump == 0x000)
			{
				for (uint32_t v = BspMesh.FaceStart; v < (BspMesh.FaceStart + (BspMesh.FaceCount * 3)); v += 3)
				{
					auto& V1 = VertexLitFlat[Faces[v] + Material.VertexOffset];
					auto& V2 = VertexLitFlat[Faces[v + 1] + Material.VertexOffset];
					auto& V3 = VertexLitFlat[Faces[v + 2] + Material.VertexOffset];

					Mesh.Vertices.EmplaceBack(Vertices[V1.PositionIndex], VertexNormals[V1.NormalIndex], Assets::VertexColor(), V1.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V2.PositionIndex], VertexNormals[V2.NormalIndex], Assets::VertexColor(), V2.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V3.PositionIndex], VertexNormals[V3.NormalIndex], Assets::VertexColor(), V3.UVs);

					Mesh.Faces.EmplaceBack(FaceIndex, FaceIndex + 1, FaceIndex + 2);
					FaceIndex += 3;
				}
			}
			else if (FaceLump == 0x200)
			{
				for (uint32_t v = BspMesh.FaceStart; v < (BspMesh.FaceStart + (BspMesh.FaceCount * 3)); v += 3)
				{
					auto& V1 = VertexLitBumps[Faces[v] + Material.VertexOffset];
					auto& V2 = VertexLitBumps[Faces[v + 1] + Material.VertexOffset];
					auto& V3 = VertexLitBumps[Faces[v + 2] + Material.VertexOffset];

					Mesh.Vertices.EmplaceBack(Vertices[V1.PositionIndex], VertexNormals[V1.NormalIndex], Assets::VertexColor(), V1.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V2.PositionIndex], VertexNormals[V2.NormalIndex], Assets::VertexColor(), V2.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V3.PositionIndex], VertexNormals[V3.NormalIndex], Assets::VertexColor(), V3.UVs);

					Mesh.Faces.EmplaceBack(FaceIndex, FaceIndex + 1, FaceIndex + 2);
					FaceIndex += 3;
				}
			}
			else if (FaceLump == 0x400)
			{
				for (uint32_t v = BspMesh.FaceStart; v < (BspMesh.FaceStart + (BspMesh.FaceCount * 3)); v += 3)
				{
					auto& V1 = VertexUnlit[Faces[v] + Material.VertexOffset];
					auto& V2 = VertexUnlit[Faces[v + 1] + Material.VertexOffset];
					auto& V3 = VertexUnlit[Faces[v + 2] + Material.VertexOffset];

					Mesh.Vertices.EmplaceBack(Vertices[V1.PositionIndex], VertexNormals[V1.NormalIndex], Assets::VertexColor(), V1.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V2.PositionIndex], VertexNormals[V2.NormalIndex], Assets::VertexColor(), V2.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V3.PositionIndex], VertexNormals[V3.NormalIndex], Assets::VertexColor(), V3.UVs);

					Mesh.Faces.EmplaceBack(FaceIndex, FaceIndex + 1, FaceIndex + 2);
					FaceIndex += 3;
				}
			}
			else if (FaceLump == 0x600)
			{
				for (uint32_t v = BspMesh.FaceStart; v < (BspMesh.FaceStart + (BspMesh.FaceCount * 3)); v += 3)
				{
					auto& V1 = VertexUnlitTS[Faces[v] + Material.VertexOffset];
					auto& V2 = VertexUnlitTS[Faces[v + 1] + Material.VertexOffset];
					auto& V3 = VertexUnlitTS[Faces[v + 2] + Material.VertexOffset];

					Mesh.Vertices.EmplaceBack(Vertices[V1.PositionIndex], VertexNormals[V1.NormalIndex], Assets::VertexColor(), V1.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V2.PositionIndex], VertexNormals[V2.NormalIndex], Assets::VertexColor(), V2.UVs);
					Mesh.Vertices.EmplaceBack(Vertices[V3.PositionIndex], VertexNormals[V3.NormalIndex], Assets::VertexColor(), V3.UVs);

					Mesh.Faces.EmplaceBack(FaceIndex, FaceIndex + 1, FaceIndex + 2);
					FaceIndex += 3;
				}
			}
		}
	}

	this->ModelExporter->ExportModel(*Model.get(), IO::Path::Combine(ModelPath, Model->Name + "_LOD0" + (const char*)ModelExporter->ModelExtension()));

	auto Buffer = new uint8_t[Lumps[GAME_LUMPS].DataSize];
	if (Header.IsEntirelyStreamed)
	{
		ReadExternalLumpArrayFile(BasePath, GAME_LUMPS, Lumps[GAME_LUMPS].DataSize, &Buffer[0]);
	}
	else {
		Stream->SetPosition(Lumps[GAME_LUMPS].Offset);
		Stream->Read(Buffer, 0, Lumps[GAME_LUMPS].DataSize);
	}

	auto Stream2 = std::make_unique<IO::MemoryStream>(Buffer, 0, Lumps[GAME_LUMPS].DataSize);
	this->ExportPropContainer(Stream2, Model->Name + "_LOD0", ModelPath);

	// Export all of the bsp's prop models

	if (!RpakFileSystem)
		return;

	auto ExportedModelsPath = IO::Path::Combine(ModelPath, "_models");
	auto ExportedModelAnimsPath = IO::Path::Combine(ExportedModelsPath, "_animations");

	std::array<bool, 8> bAssets = {
		true,  // Model
		false, // Animation
		false, // Texture/Images
		false, // Material
		false, // UIIA
		false, // Datatables
		false, // ShaderSets
		false, // SettingsSets
	};

	auto RpakModels = RpakFileSystem->BuildAssetList(bAssets);
	Dictionary<string, RpakLoadAsset> RpakModelLookup;

	for (auto& tm : *RpakModels)
	{
		RpakModelLookup.Add(tm.Name, RpakFileSystem->Assets[tm.Hash]);
	}

	for (uint32_t i = 0; i < this->PropModelNames.Count(); ++i)
	{
		string ModelName = this->PropModelNames[i];
		if (RpakModelLookup.ContainsKey(ModelName))
		{
			auto ModelAsset = RpakModelLookup[ModelName];
			RpakFileSystem->ExportModel(ModelAsset, ExportedModelsPath, ExportedModelAnimsPath);
		}
	}
}