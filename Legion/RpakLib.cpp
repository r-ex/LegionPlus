#include <vector>
#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>
#include <sstream>

#include "RpakLib.h"
#include "Path.h"
#include "File.h"
#include "Directory.h"
#include "DDS.h"
#include "Half.h"
#include "Texture.h"
#include "Model.h"
#include "BinaryReader.h"

// Asset export formats
#include "CoDXAssetExport.h"
#include "XNALaraBinary.h"
#include "XNALaraAscii.h"
#include "WavefrontOBJ.h"
#include "KaydaraFBX.h"
#include "CastAsset.h"
#include "ValveSMD.h"
#include "AutodeskMaya.h"
#include "SEAsset.h"

// Rpak asset formats
#include "rtech.h"
#include "RpakImageTiles.h"

RpakFile::RpakFile()
	: SegmentData(nullptr), StartSegmentIndex(0), SegmentDataSize(0), PatchData(nullptr), PatchDataSize(0), Version(RpakGameVersion::Apex), EmbeddedStarpakOffset(0), EmbeddedStarpakSize(0)
{
}

RpakLib::RpakLib()
	: LoadedFileIndex(0), ImageExtension(".dds"), ImageSaveType(Assets::SaveFileType::Dds)
{
}

void RpakLib::LoadRpaks(const List<string>& Paths)
{
	List<string> LoadedFiles;

	for (auto& Rpak : Paths)
	{
		// Ignore duplicate files triggered by loading multiple rpaks at once.
		if (LoadedFiles.Contains(Rpak))
			continue;

		this->LoadRpak(Rpak, false);

		// Copy over to loaded, clear for next file
		for (auto& Loaded : this->LoadFileQueue)
		{
			LoadedFiles.EmplaceBack(Loaded);
		}
		this->LoadFileQueue.Clear();
	}
}

void RpakLib::LoadRpak(const string& Path, bool Dump)
{
	this->LoadFileQueue.EmplaceBack(Path);
	uint32_t LoadFileQueue = 0;

	while (LoadFileQueue < this->LoadFileQueue.Count() && this->MountRpak(this->LoadFileQueue[LoadFileQueue++], Dump));
}

void RpakLib::PatchAssets()
{
	// This is a dictionary of failed stream assets
	Dictionary<uint64_t, RpakLoadAsset> PatchedStreamAssets;

	// We must load this way...
	for (uint32_t i = 0; i < this->LoadedFileIndex; i++)
	{
		auto& LoadedFile = this->LoadedFiles[i];

		for (auto& Kvp : LoadedFile.AssetHashmap)
		{
			if (!this->Assets.ContainsKey(Kvp.first))
			{
				auto Asset = RpakLoadAsset(
					Kvp.second.NameHash,
					i,
					Kvp.second.Magic,
					Kvp.second.SubHeaderDataBlockIndex,
					Kvp.second.SubHeaderDataBlockOffset,
					Kvp.second.SubHeaderSize,
					Kvp.second.RawDataBlockIndex,
					Kvp.second.RawDataBlockOffset,
					Kvp.second.StarpakOffset,
					Kvp.second.OptimalStarpakOffset,
					LoadedFile.Version
				);

				// All assets must follow this patch sequence
				if (this->ValidateAssetPatchStatus(Asset))
				{
					// If we have a model or texture, it must also pass the stream test
					if (Asset.AssetType == (uint32_t)RpakAssetType::Model
						|| Asset.AssetType == (uint32_t)RpakAssetType::Texture
						|| Asset.AssetType == (uint32_t)RpakAssetType::UIIA)
					{
						if (this->ValidateAssetStreamStatus(Asset))
						{
							this->Assets.Add(Kvp.first, Asset);
						}
						else if (!PatchedStreamAssets.ContainsKey(Kvp.first))
						{
							PatchedStreamAssets.Add(Kvp.first, Asset);
						}
					}
					else
					{
						this->Assets.Add(Kvp.first, Asset);
					}
				}
			}
		}
	}

	// We will attempt to brute force patch assets...
	for (auto& Kvp : PatchedStreamAssets)
	{
		for (uint32_t i = 0; i < this->LoadedFileIndex; i++)
		{
			if (i == Kvp.second.FileIndex)
				continue;

			auto& File = this->LoadedFiles[i];

			if (File.AssetHashmap.ContainsKey(Kvp.first))
			{
				auto& Asset = File.AssetHashmap[Kvp.first];

				Kvp.second.RpakFileIndex = i;
				Kvp.second.StarpakOffset = Asset.StarpakOffset;
				Kvp.second.OptimalStarpakOffset = Asset.OptimalStarpakOffset;

				if (this->ValidateAssetStreamStatus(Kvp.second))
				{
					this->Assets.Add(Kvp.first, Kvp.second);
					break;
				}
			}
		}
	}

	// Clean up the old cache of assets
	for (uint32_t i = 0; i < this->LoadedFileIndex; i++)
	{
		this->LoadedFiles[i].AssetHashmap.Clear();
	}
}

std::unique_ptr<List<ApexAsset>> RpakLib::BuildAssetList(bool Models, bool Anims, bool Images, bool Materials, bool UIImages, bool DataTables)
{
	auto Result = std::make_unique<List<ApexAsset>>();

	for (auto& AssetKvp : Assets)
	{
		auto& Asset = AssetKvp.Value();

		ApexAsset NewAsset;
		NewAsset.Hash = AssetKvp.first;

		switch (Asset.AssetType)
		{
		case (uint32_t)RpakAssetType::Model:
			if (!Models)
				continue;
			BuildModelInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::AnimationRig:
			if (!Anims)
				continue;
			BuildAnimInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::Material:
			if (!Materials)
				continue;
			BuildMaterialInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::Texture:
			if (!Images)
				continue;
			BuildTextureInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::UIIA:
			if (!UIImages)
				continue;
			BuildUIIAInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::DataTable:
			if (!DataTables)
				continue;
			BuildDataTableInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::Subtitles:
			// todo: subtitle loading setting
			BuildSubtitleInfo(Asset, NewAsset);
			break;
		default:
			continue;
		}

		Result->EmplaceBack(std::move(NewAsset));
	}

	return std::move(Result);
}

std::unique_ptr<Assets::Model> RpakLib::BuildPreviewModel(uint64_t Hash)
{
	if (!this->Assets.ContainsKey(Hash))
		return nullptr;

	auto& Asset = this->Assets[Hash];

	if (Asset.AssetType != (uint32_t)RpakAssetType::Model)
		return nullptr;

	return std::move(this->ExtractModel(Asset, "", "", false, false));
}

std::unique_ptr<Assets::Texture> RpakLib::BuildPreviewTexture(uint64_t Hash)
{
	if (!this->Assets.ContainsKey(Hash))
		return nullptr;

	auto& Asset = this->Assets[Hash];

	std::unique_ptr<Assets::Texture> Result = nullptr;

	switch (Asset.AssetType)
	{
	case (uint32_t)RpakAssetType::Texture:
		this->ExtractTexture(Asset, Result);
		return Result;
	case (uint32_t)RpakAssetType::UIIA:
		this->ExtractUIIA(Asset, Result);
		return Result;
	default:
		return nullptr;
	}
}

std::unique_ptr<Assets::Texture> RpakLib::BuildPreviewMaterial(uint64_t Hash)
{
	if (!this->Assets.ContainsKey(Hash))
		return nullptr;

	auto& Asset = this->Assets[Hash];
	auto Material = this->ExtractMaterial(Asset, "", false);

	return this->BuildPreviewTexture(Material.AlbedoHash);
}

void RpakLib::InitializeModelExporter(RpakModelExportFormat Format)
{
	switch (Format)
	{
	case RpakModelExportFormat::Maya:
		ModelExporter = std::make_unique<Assets::Exporters::AutodeskMaya>();
		break;
	case RpakModelExportFormat::OBJ:
		ModelExporter = std::make_unique<Assets::Exporters::WavefrontOBJ>();
		break;
	case RpakModelExportFormat::XNALaraText:
		ModelExporter = std::make_unique<Assets::Exporters::XNALaraAscii>();
		break;
	case RpakModelExportFormat::XNALaraBinary:
		ModelExporter = std::make_unique<Assets::Exporters::XNALaraBinary>();
		break;
	case RpakModelExportFormat::SMD:
		ModelExporter = std::make_unique<Assets::Exporters::ValveSMD>();
		break;
	case RpakModelExportFormat::XModel:
		ModelExporter = std::make_unique<Assets::Exporters::CoDXAssetExport>();
		break;
	case RpakModelExportFormat::FBX:
		ModelExporter = std::make_unique<Assets::Exporters::KaydaraFBX>();
		break;
	case RpakModelExportFormat::Cast:
		ModelExporter = std::make_unique<Assets::Exporters::CastAsset>();
		break;
	default:
		ModelExporter = std::make_unique<Assets::Exporters::SEAsset>();
		break;
	}

	m_bModelExporterInitialized = true;
}

void RpakLib::InitializeAnimExporter(RpakAnimExportFormat Format)
{
	switch (Format)
	{
	case RpakAnimExportFormat::Cast:
		AnimExporter = std::make_unique<Assets::Exporters::CastAsset>();
		break;
	default:
		AnimExporter = std::make_unique<Assets::Exporters::SEAsset>();
		break;
	}

	m_bAnimExporterInitialized = true;
}

void RpakLib::InitializeImageExporter(RpakImageExportFormat Format)
{
	switch (Format)
	{
	case RpakImageExportFormat::Dds:
		ImageSaveType = Assets::SaveFileType::Dds;
		ImageExtension = Assets::Texture::GetExtensionForType(ImageSaveType);
		break;
	case RpakImageExportFormat::Png:
		ImageSaveType = Assets::SaveFileType::Png;
		ImageExtension = Assets::Texture::GetExtensionForType(ImageSaveType);
		break;
	case RpakImageExportFormat::Tiff:
		ImageSaveType = Assets::SaveFileType::Tiff;
		ImageExtension = Assets::Texture::GetExtensionForType(ImageSaveType);
		break;
	}

	m_bImageExporterInitialized = true;
}

void RpakLib::ExportModel(const RpakLoadAsset& Asset, const string& Path, const string& AnimPath)
{
	auto Model = this->ExtractModel(Asset, Path, AnimPath, true, true);

	if (Model && this->ModelExporter)
	{
		this->ModelExporter->ExportModel(*Model.get(), IO::Path::Combine(IO::Path::Combine(Path, Model->Name), Model->Name + "_LOD0" + (const char*)ModelExporter->ModelExtension()));
	}
}

void RpakLib::ExportMaterial(const RpakLoadAsset& Asset, const string& Path)
{
	auto Material = this->ExtractMaterial(Asset, Path, false);
	auto OutPath = IO::Path::Combine(Path, Material.MaterialName);

	IO::Directory::CreateDirectory(OutPath);

	(void)this->ExtractMaterial(Asset, OutPath, true);
}

void RpakLib::ExportTexture(const RpakLoadAsset& Asset, const string& Path)
{
	auto DestinationPath = IO::Path::Combine(Path, string::Format("0x%llx%s", Asset.NameHash, (const char*)ImageExtension));

#ifndef _DEBUG
	if (IO::File::Exists(DestinationPath))	// Ignore existing assets...
		return;
#endif

	std::unique_ptr<Assets::Texture> Texture = nullptr;

	this->ExtractTexture(Asset, Texture);

	try
	{
		if (Texture != nullptr)
			Texture->Save(DestinationPath, ImageSaveType);
	}
	catch (...)
	{
		// Nothing, the thread attempted to export an image that already exists...
	}
}

std::unique_ptr<IO::MemoryStream> DecompressStreamedBuffer(const uint8_t* Data, uint64_t& DataSize, uint8_t Format)
{
	if (Format == 0x1)
	{
		/* 
		*  Below might be only 16 params actually.
		*  If I didn't fuck up with math.
		*  char v23[8]; // [rsp+40h] [rbp-E8h] BYREF\
		*  memset_0(v23, 0, 176ui64);
		*/
		//std::int64_t params[18];
		rpak_decomp_state state;

		// sig to containing function in retail: (+0x195) E8 ? ? ? ? 0F B6 06 3C 01 
		// v13 = RTech::DecompressedSize(*(_QWORD*)(a1 + 32),*(_QWORD*)(a1 + 40), -1i64, *(unsigned int*)(a1 + 8), 0i64, 0i64);
		// Compiler decided here to make that function have 6 args, -1 is inlined in our function.
		// Porter originally calculated a mask for the -1 input field
		// RpakCalcMask(0x400000)
		/*
		*  v7 = 1;
		*  v8 = 0x200000i64;
		*  do
		*  {
		*	 ++v7;
		*     8 >>= 1;
		*  } while ( v8 );
		* 
		* (1 << v7) - 1 as input for the -1 param.
		*/

		uint32_t dSize = g_pRtech->DecompressPakfileInit(&state, (uint8_t*)Data, DataSize, 0, 0);

		std::vector<std::uint8_t> pakbuf(dSize, 0);

		//params[1] = std::int64_t(pakbuf.data());
		//params[3] = -1i64;
		state.out_mask = UINT64_MAX;
		state.out = uint64_t(pakbuf.data());

		// Porter originally passed 0x400000ui64 here after calculating the actual rpak mask, we will push pakbuf.size() for the time being.
		// Retail passes 16 as the buffer_size
		/*
		*  v14 = *(__int64 (__fastcall ***)(_QWORD, _QWORD, __int64))(a1 + 56);
        *  *(_DWORD *)(a1 + 12) = decompressed_size;
        *  v15 = (*v14)(v14, decompressed_size, 16i64);
		*/
		std::uint8_t decomp_result = g_pRtech->DecompressPakFile((int64_t*)&state, dSize, pakbuf.size());

		return std::make_unique<IO::MemoryStream>((uint8_t*)Data, 0, DataSize);
	}
	else if (Format == 0x2)
	{
		auto State = std::make_unique<uint8_t[]>(0x25000);
		g_pRtech->DecompressSnowflakeInit((long long)&State.get()[0], (int64_t)Data, DataSize);

		auto EditState = (__int64*)&State.get()[0];
		auto DecompressedSize = EditState[0x48D3];

		auto Result = new uint8_t[DecompressedSize]{};

		EditState[0x48DA] = (__int64)Result;
		EditState[0x48DB] = 0;

		g_pRtech->DecompressSnowflake((long long)&State.get()[0], DataSize, DecompressedSize);

		DataSize = EditState[0x48db];

		return std::make_unique<IO::MemoryStream>(Result, 0, EditState[0x48db]);
	}
	else
	{
		DataSize = 0;
		return nullptr;
	}
}

void RpakLib::ExportUIIA(const RpakLoadAsset& Asset, const string& Path)
{
	auto DestinationPath = IO::Path::Combine(Path, string::Format("0x%llx%s", Asset.NameHash, (const char*)ImageExtension));

#ifndef _DEBUG
	if (IO::File::Exists(DestinationPath))	// Ignore existing assets...
		return;
#endif

	std::unique_ptr<Assets::Texture> Texture = nullptr;

	this->ExtractUIIA(Asset, Texture);

	try
	{
		if (Texture != nullptr)
			Texture->Save(DestinationPath, ImageSaveType);
	}
	catch (...)
	{
		// Nothing, the thread attempted to export an image that already exists...
	}
}

void RpakLib::ExportAnimationRig(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto RigHeader = Reader.Read<AnimRigHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, RigHeader.NameIndex, RigHeader.NameOffset));

	auto AnimSetName = IO::Path::GetFileNameWithoutExtension(Reader.ReadCString());
	auto AnimSetPath = IO::Path::Combine(Path, AnimSetName);

	IO::Directory::CreateDirectory(AnimSetPath);

	const uint64_t ReferenceOffset = this->GetFileOffset(Asset, RigHeader.AnimationReferenceIndex, RigHeader.AnimationReferenceOffset);
	const auto Skeleton = this->ExtractSkeleton(Reader, this->GetFileOffset(Asset, RigHeader.SkeletonIndex, RigHeader.SkeletonOffset));

	for (uint32_t i = 0; i < RigHeader.AnimationReferenceCount; i++)
	{
		RpakStream->SetPosition(ReferenceOffset + ((uint64_t)i * 0x8));

		auto AnimHash = Reader.Read<uint64_t>();

		if (!Assets.ContainsKey(AnimHash))
			continue;	// Should never happen

		// We need to make sure the skeleton is kept alive (copied) here...
		this->ExtractAnimation(Assets[AnimHash], Skeleton, AnimSetPath);
	}
}

void RpakLib::ExportDataTable(const RpakLoadAsset& Asset, const string& Path)
{
	auto DestinationPath = IO::Path::Combine(Path, string::Format("0x%llx.csv", Asset.NameHash));

	auto DataTable = this->ExtractDataTable(Asset);

	std::ofstream dtbl_out(DestinationPath.ToCString(), std::ios::out);

	for (int i = 0; i < DataTable.Count(); ++i)
	{
		List<DataTableColumnData> Row = DataTable[i];

		for (int c = 0; c < Row.Count(); ++c)
		{
			DataTableColumnData cd = Row[c];

			switch (cd.Type)
			{
			case DataTableColumnDataType::Bool:
				dtbl_out << cd.bValue;
				break;
			case DataTableColumnDataType::Int:
				dtbl_out << cd.iValue;
				break;
			case DataTableColumnDataType::Float:
				dtbl_out << cd.fValue;
				break;
			case DataTableColumnDataType::Vector:
			{
				dtbl_out << "\"<" << cd.vValue.X << "," << cd.vValue.Y << "," << cd.vValue.Z << ">\"";
				break;
			}
			case DataTableColumnDataType::Asset:
			{
				dtbl_out << "\"" + cd.assetValue + "\"";
				break;
			}
			case DataTableColumnDataType::AssetNoPrecache:
			{
				dtbl_out << "\"" + cd.assetNPValue + "\"";
				break;
			}
			case DataTableColumnDataType::StringT:
			{
				dtbl_out << "\"" + cd.stringValue + "\"";
				break;
			}
			}
			if (c != Row.Count() - 1)
			{
				dtbl_out << ",";
			}
			else {
				dtbl_out << "\n";
			}
		}
	}

	dtbl_out.close();

}

string Vector3ToHexColor(Math::Vector3 vec)
{
	std::stringstream stream;

	stream << std::hex << (uint16_t)vec.X << (uint16_t)vec.Y << (uint16_t)vec.Z;

	return stream.str().c_str();
}

void RpakLib::ExportSubtitles(const RpakLoadAsset& Asset, const string& Path)
{
	auto DestinationPath = IO::Path::Combine(Path, GetSubtitlesNameFromHash(Asset.NameHash) + ".csv");

	auto Subtitles = this->ExtractSubtitles(Asset);

	std::ofstream subt_out(DestinationPath.ToCString(), std::ios::out);

	subt_out << "color,text\n";
	for (auto& Entry : Subtitles)
	{
		subt_out << "\"#" << Vector3ToHexColor(Entry.Color) << "\",\"" << Entry.SubtitleText << "\"\n";
	}
	subt_out.close();
}

std::unique_ptr<IO::MemoryStream> RpakLib::GetFileStream(const RpakLoadAsset& Asset)
{
	auto& File = this->LoadedFiles[Asset.FileIndex];

	return std::move(std::make_unique<IO::MemoryStream>(File.SegmentData.get(), 0, File.SegmentDataSize, false, true));
}

uint64_t RpakLib::GetFileOffset(const RpakLoadAsset& Asset, uint32_t SegmentIndex, uint32_t SegmentOffset)
{
	return (this->LoadedFiles[Asset.FileIndex].SegmentBlocks[SegmentIndex - this->LoadedFiles[Asset.FileIndex].StartSegmentIndex].Offset + SegmentOffset);
}

uint64_t RpakLib::GetEmbeddedStarpakOffset(const RpakLoadAsset& Asset)
{
	return this->LoadedFiles[Asset.FileIndex].EmbeddedStarpakOffset;
}

std::unique_ptr<IO::FileStream> RpakLib::GetStarpakStream(const RpakLoadAsset& Asset, bool Optimal)
{
	if (Optimal)
	{
		uint64_t OptStarpakIndex = Asset.OptimalStarpakOffset & 0xFF;
#if _DEBUG
		printf("Load starpak: %s\n", this->LoadedFiles[Asset.RpakFileIndex].OptimalStarpakReferences[OptStarpakIndex].ToCString());
#endif
		return std::move(IO::File::OpenRead(this->LoadedFiles[Asset.RpakFileIndex].OptimalStarpakReferences[OptStarpakIndex]));
	}
	else
	{
		uint64_t StarpakPatchIndex = Asset.StarpakOffset & 0xFF;
#if _DEBUG
		printf("Load starpak: %s\n", this->LoadedFiles[Asset.RpakFileIndex].StarpakReferences[StarpakPatchIndex].ToCString());
#endif
		return std::move(IO::File::OpenRead(this->LoadedFiles[Asset.RpakFileIndex].StarpakReferences[StarpakPatchIndex]));
	}
}

void RpakLib::BuildModelInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	ModelHeaderS68 ModHeader{};
	if (Asset.SubHeaderSize <= 0x68)
	{
		ModHeader = Reader.Read<ModelHeaderS68>();
	}
	else
	{
		auto ModHeaderTmp = Reader.Read<ModelHeaderS80>();
		std::memcpy(&ModHeader, &ModHeaderTmp, offsetof(ModelHeaderS80, DataFlags));
		std::memcpy(&ModHeader.AnimSequenceIndex, &ModHeader.AnimSequenceIndex, sizeof(uint32_t) * 3);
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, ModHeader.NameIndex, ModHeader.NameOffset));

	Info.Name = IO::Path::GetFileNameWithoutExtension(Reader.ReadCString()).ToLower();
	Info.Type = ApexAssetType::Model;

	RpakStream->SetPosition(this->GetFileOffset(Asset, ModHeader.SkeletonIndex, ModHeader.SkeletonOffset));

	auto SkeletonHeader = Reader.Read<RMdlSkeletonHeader>();

	if (ModHeader.AnimSequenceCount > 0)
	{
		Info.Info = string::Format("Bones: %d, Meshes: %d, Animations: %d", SkeletonHeader.BoneCount, SkeletonHeader.BodyPartCount, ModHeader.AnimSequenceCount);
	}
	else
	{
		Info.Info = string::Format("Bones: %d, Meshes: %d", SkeletonHeader.BoneCount, SkeletonHeader.BodyPartCount);
	}
}

void RpakLib::BuildAnimInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto RigHeader = Reader.Read<AnimRigHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, RigHeader.NameIndex, RigHeader.NameOffset));

	Info.Name = IO::Path::GetFileNameWithoutExtension(Reader.ReadCString()).ToLower();
	Info.Type = ApexAssetType::AnimationSet;
	Info.Status = ApexAssetStatus::Loaded;

	RpakStream->SetPosition(this->GetFileOffset(Asset, RigHeader.SkeletonIndex, RigHeader.SkeletonOffset));

	auto SkeletonHeader = Reader.Read<RMdlSkeletonHeader>();

	Info.Info = string::Format("Animations: %d, Bones: %d", RigHeader.AnimationReferenceCount, SkeletonHeader.BoneCount);
}

void RpakLib::BuildRawAnimInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto AnHeader = Reader.Read<AnimHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, AnHeader.NameIndex, AnHeader.NameOffset));

	auto AnimName = IO::Path::GetFileNameWithoutExtension(Reader.ReadCString());

	Info.Name = AnimName.ToLower();
	Info.Type = ApexAssetType::AnimationSet;
	Info.Status = ApexAssetStatus::Loaded;
}

void RpakLib::BuildMaterialInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto MatHeader = Reader.Read<MaterialHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, MatHeader.NameIndex, MatHeader.NameOffset));

	Info.Name = IO::Path::GetFileNameWithoutExtension(Reader.ReadCString()).ToLower();
	Info.Type = ApexAssetType::Material;
	Info.Status = ApexAssetStatus::Loaded;
}

void RpakLib::BuildTextureInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto TexHeader = Reader.Read<TextureHeader>();

	Info.Name = string::Format("texture_0x%llx", Asset.NameHash);
	Info.Type = ApexAssetType::Image;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = string::Format("Width: %d Height %d", TexHeader.Width, TexHeader.Height);
}

void RpakLib::BuildUIIAInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto TexHeader = Reader.Read<UIIAHeader>();

	Info.Name = string::Format("uiimage_0x%llx", Asset.NameHash);
	Info.Type = ApexAssetType::Image;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = string::Format("Width: %d Height %d", TexHeader.Width, TexHeader.Height);
}

void RpakLib::BuildDataTableInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
	auto DtblHeader = Reader.Read<DataTableHeader>();

	Info.Name = string::Format("datatable_0x%llx", Asset.NameHash);
	Info.Type = ApexAssetType::DataTable;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = string::Format("Columns: %d Rows: %d", DtblHeader.ColumnCount, DtblHeader.RowCount);
}

void RpakLib::BuildSubtitleInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	Info.Name = GetSubtitlesNameFromHash(Asset.NameHash);
	Info.Type = ApexAssetType::Subtitles;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = "N/A";
}

std::unique_ptr<Assets::Model> RpakLib::ExtractModel(const RpakLoadAsset& Asset, const string& Path, const string& AnimPath, bool IncludeMaterials, bool IncludeAnimations)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);
	auto Model = std::make_unique<Assets::Model>(0, 0);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	ModelHeaderS68 ModHeader{};
	if (Asset.SubHeaderSize <= 0x68)
	{
		ModHeader = Reader.Read<ModelHeaderS68>();
	}
	else
	{
		auto ModHeaderTmp = Reader.Read<ModelHeaderS80>();
		std::memcpy(&ModHeader, &ModHeaderTmp, offsetof(ModelHeaderS80, DataFlags));
		std::memcpy(&ModHeader.AnimSequenceIndex, &ModHeader.AnimSequenceIndex, sizeof(uint32_t) * 3);
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, ModHeader.NameIndex, ModHeader.NameOffset));

	auto ModelName = IO::Path::GetFileNameWithoutExtension(Reader.ReadCString());
	auto ModelPath = IO::Path::Combine(Path, ModelName);
	auto TexturePath = IO::Path::Combine(ModelPath, "_images");
	auto AnimationPath = IO::Path::Combine(AnimPath, ModelName);

	Model->Name = ModelName;

	if (IncludeMaterials)
	{
		IO::Directory::CreateDirectory(ModelPath);
		IO::Directory::CreateDirectory(TexturePath);
	}

	const uint64_t SkeletonOffset = this->GetFileOffset(Asset, ModHeader.SkeletonIndex, ModHeader.SkeletonOffset);

	Model->Bones = std::move(ExtractSkeleton(Reader, SkeletonOffset));
	Model->GenerateGlobalTransforms(true, true); // We need global transforms

	if (IncludeAnimations && ModHeader.AnimSequenceCount > 0)
	{
		IO::Directory::CreateDirectory(AnimationPath);

		RpakStream->SetPosition(this->GetFileOffset(Asset, ModHeader.AnimSequenceIndex, ModHeader.AnimSequenceOffset));

		for (uint32_t i = 0; i < ModHeader.AnimSequenceCount; i++)
		{
			auto AnimHash = Reader.Read<uint64_t>();

			if (!Assets.ContainsKey(AnimHash))
				continue;	// Should never happen

			// We need to make sure the skeleton is kept alive (copied) here...
			this->ExtractAnimation(Assets[AnimHash], Model->Bones, AnimationPath);
		}
	}

	RpakStream->SetPosition(SkeletonOffset);

	auto SkeletonHeader = Reader.Read<RMdlSkeletonHeader>();

	RpakStream->SetPosition(SkeletonOffset + SkeletonHeader.TextureOffset);

	List<RMdlTexture> MaterialBuffer(SkeletonHeader.TextureCount, true);
	RpakStream->Read((uint8_t*)&MaterialBuffer[0], 0, SkeletonHeader.TextureCount * sizeof(RMdlTexture));

	List<uint8_t> BoneRemapTable(SkeletonHeader.BoneRemapCount, true);

	if (SkeletonHeader.BoneRemapCount == 0)
	{
		for (uint32_t i = 0; i < 0xFF; i++)
		{
			BoneRemapTable.EmplaceBack(i);
		}
	}
	else
	{
		RpakStream->SetPosition(SkeletonOffset + (SkeletonHeader.DataSize - SkeletonHeader.BoneRemapCount));
		RpakStream->Read((uint8_t*)&BoneRemapTable[0], 0, SkeletonHeader.BoneRemapCount);
	}

	RpakStream->SetPosition(SkeletonOffset + SkeletonHeader.SubmeshLodsOffset);

	RMdlFixupPatches Fixups{};
	Fixups.MaterialPath = TexturePath;
	Fixups.Materials = &MaterialBuffer;
	Fixups.BoneRemaps = &BoneRemapTable;
	Fixups.FixupTableOffset = (SkeletonOffset + SkeletonHeader.SubmeshLodsOffset);

	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t StarpakPatchIndex = Asset.StarpakOffset & 0xFF;
	uint64_t OptStarpakIndex = Asset.OptimalStarpakOffset & 0xFF;

	uint64_t Offset = 0;
	std::unique_ptr<IO::FileStream> StarpakStream = nullptr;

	if (Asset.OptimalStarpakOffset != -1)
	{
		Offset = ActualOptStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, true);
	}
	else if (Asset.StarpakOffset != -1)
	{
		Offset = ActualStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, false);
	}
	else
	{
		// An error occured while parsing
		return nullptr;
	}

	auto StarpakReader = IO::BinaryReader(StarpakStream.get(), true);
	this->ExtractModelLod(StarpakReader, RpakStream, ModelName, Offset, Model, Fixups, IncludeMaterials);

	return std::move(Model);
}

void RpakLib::ExtractModelLod(IO::BinaryReader& Reader, const std::unique_ptr<IO::MemoryStream>& RpakStream, string Name, uint64_t Offset, const std::unique_ptr<Assets::Model>& Model, RMdlFixupPatches& Fixup, bool IncludeMaterials)
{
	auto BaseStream = Reader.GetBaseStream();

	BaseStream->SetPosition(Offset);

	auto VGHeader = Reader.Read<RMdlVGHeader>();

	switch (VGHeader.StreamFlags)
	{
	case 0x10:	// submeshes follow direct
		break;
	case 0x60:	// vg header
	case 0x80:	// vg header
	case 0x90:	// vg header
	case 0xb0:	// vg header
	case 0xc0:	// vg header
	{
		uint32_t skip = 0;
		while (true)
		{
			BaseStream->SetPosition(Offset + 0x10 * skip + sizeof(RMdlVGHeader));
			if (Reader.Read<uint32_t>() == 0x47567430)
			{
				break;
			}
			skip++;
		}
		BaseStream->SetPosition(Offset + 0x10 * skip + sizeof(RMdlVGHeader));
		VGHeader = Reader.Read<RMdlVGHeader>();
	}
	break;
	case 0x40:	// skip 0x10 * lodcount
	case 0x50:	// skip 0x10 * lodcount
		BaseStream->SetPosition(Offset + (0x10 * VGHeader.LodCount) + sizeof(RMdlVGHeader));
		break;
	case 0x20:	// skip 0x10
		BaseStream->SetPosition(Offset + 0x10 + sizeof(RMdlVGHeader));
		break;
	default:
#if _DEBUG
		printf("Unknown mesh command: 0x%x\n", VGHeader.StreamFlags);
#endif
		break;
	}

	// Offsets in submesh are relative to the submesh we're reading...
	const auto SubmeshPointer = BaseStream->GetPosition();

	// We need to read the submeshes
	List<RMdlVGSubmesh> SubmeshBuffer(VGHeader.SubmeshCount, true);
	Reader.Read((uint8_t*)&SubmeshBuffer[0], 0, VGHeader.SubmeshCount * sizeof(RMdlVGSubmesh));

	// Loop and read submeshes
	for (uint32_t s = 0; s < VGHeader.SubmeshCount; s++)
	{
		auto& Submesh = SubmeshBuffer[s];

		// We have buffers per submesh now thank god
		List<uint8_t> VertexBuffer(Submesh.VertexCountBytes, true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh)) + offsetof(RMdlVGSubmesh, VertexOffset) + Submesh.VertexOffset);
		Reader.Read((uint8_t*)&VertexBuffer[0], 0, Submesh.VertexCountBytes);

		List<uint16_t> IndexBuffer(Submesh.IndexPacked.Count, true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh)) + offsetof(RMdlVGSubmesh, IndexOffset) + Submesh.IndexOffset);
		Reader.Read((uint8_t*)&IndexBuffer[0], 0, Submesh.IndexPacked.Count * sizeof(uint16_t));

		List<RMdlExtendedWeight> ExtendedWeights(Submesh.ExtendedWeightsCount / sizeof(RMdlExtendedWeight), true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh)) + offsetof(RMdlVGSubmesh, ExtendedWeightsOffset) + Submesh.ExtendedWeightsOffset);
		Reader.Read((uint8_t*)&ExtendedWeights[0], 0, Submesh.ExtendedWeightsCount);

		List<RMdlVGExternalWeights> ExternalWeightsBuffer(Submesh.ExternalWeightsCount, true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh)) + offsetof(RMdlVGSubmesh, ExternalWeightsOffset) + Submesh.ExternalWeightsOffset);
		Reader.Read((uint8_t*)&ExternalWeightsBuffer[0], 0, Submesh.ExternalWeightsCount * sizeof(RMdlVGExternalWeights));

		List<RMdlVGStrip> StripBuffer(Submesh.StripsCount, true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh)) + offsetof(RMdlVGSubmesh, StripsOffset) + Submesh.StripsOffset);
		Reader.Read((uint8_t*)&StripBuffer[0], 0, Submesh.StripsCount * sizeof(RMdlVGStrip));

		// Ignore a submesh that has no strips, otherwise there is no mesh.
		// This is likely also determined by flags == 0x0, but this is a good check.
		if (Submesh.StripsCount == 0)
		{
			continue;
		}

		auto& BoneRemapBuffer = *Fixup.BoneRemaps;

		auto& Mesh = Model->Meshes.Emplace(0x10, (((Submesh.Flags2 & 0x2) == 0x2) ? 2 : 1));	// max weights / max uvs
		auto& Strip = StripBuffer[0];

		auto VertexBufferPtr = (uint8_t*)&VertexBuffer[0];
		auto FaceBufferPtr = (uint16_t*)&IndexBuffer[0];

		// Cache these here, flags in the submesh dictate what to use
		Math::Vector3 Position{};
		Math::Vector3 Normal{};
		Math::Vector2 UVs{};
		Assets::VertexColor Color{};

		for (uint32_t v = 0; v < Submesh.VertexCount; v++)
		{
			uint32_t Shift = 0;

			if ((Submesh.Flags1 & 0x1) == 0x1)
			{
				Position = *(Math::Vector3*)(VertexBufferPtr + Shift);
				Shift += sizeof(Math::Vector3);
			}
			else if ((Submesh.Flags1 & 0x2) == 0x2)
			{
				Position = (*(RMdlPackedVertexPosition*)(VertexBufferPtr + Shift)).Unpack();
				Shift += sizeof(RMdlPackedVertexPosition);
			}

			RMdlPackedVertexWeights Weights{};

			if ((Submesh.Flags1 & 0x5000) == 0x5000)
			{
				Weights = *(RMdlPackedVertexWeights*)(VertexBufferPtr + Shift);
				Shift += sizeof(RMdlPackedVertexWeights);
			}

			Normal = (*(RMdlPackedVertexNormal*)(VertexBufferPtr + Shift)).Unpack();
			Shift += sizeof(RMdlPackedVertexNormal);

			if ((Submesh.Flags1 & 0x10) == 0x10)
			{
				Color = *(Assets::VertexColor*)(VertexBufferPtr + Shift);
				Shift += sizeof(Assets::VertexColor);
			}

			UVs = *(Math::Vector2*)(VertexBufferPtr + Shift);
			Shift += sizeof(Math::Vector2);

			auto Vertex = Mesh.Vertices.Emplace(Position, Normal, Color, UVs);

			if ((Submesh.Flags2 & 0x2) == 0x2)
			{
				Vertex.SetUVLayer(*(Math::Vector2*)(VertexBufferPtr + Shift), 1);
				Shift += sizeof(Math::Vector2);
			}

			if ((Submesh.Flags1 & 0x5000) == 0x5000)
			{
				auto& ExternalWeights = ExternalWeightsBuffer[v];

				if (ExtendedWeights.Count() > 0)
				{
					//
					// These models have complex extended weights
					//

					uint32_t ExtendedWeightsIndex = (uint32_t)Weights.BlendIds[2] << 16;
					ExtendedWeightsIndex |= (uint32_t)Weights.BlendWeights[1];

					float CurrentWeightTotal = (float)(Weights.BlendWeights[0] + 1) / (float)0x8000;
					uint32_t WeightsIndex = 0;

					Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], CurrentWeightTotal }, WeightsIndex++);

					uint32_t ExtendedCounter = 1;
					uint32_t ExtendedComparer = 0;
					uint32_t ExtendedIndexShift = 0;
					while (true)
					{
						ExtendedComparer = (ExtendedCounter >= Weights.BlendIds[3]);
						if (ExtendedComparer != 0)
							break;

						ExtendedIndexShift = ExtendedWeightsIndex + ExtendedCounter;
						ExtendedIndexShift = ExtendedIndexShift + -1;

						auto& ExtendedWeight = ExtendedWeights[ExtendedIndexShift];

						float ExtendedValue = (float)(ExtendedWeight.Weight + 1) / (float)0x8000;
						uint32_t ExtendedIndex = BoneRemapBuffer[ExtendedWeight.BoneId];

						Vertex.SetWeight({ ExtendedIndex, ExtendedValue }, WeightsIndex++);

						CurrentWeightTotal += ExtendedValue;
						ExtendedCounter = ExtendedCounter + 1;
					}

					if (Weights.BlendIds[0] != Weights.BlendIds[1] && CurrentWeightTotal < 1.0f)
					{
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], 1.0f - CurrentWeightTotal }, WeightsIndex);
					}
				}
				else
				{
					//
					// These models have 3 or less weights
					//

					if (ExternalWeights.NumWeights == 0x1)
					{
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], 1.0f }, 0);
					}
					else if (ExternalWeights.NumWeights == 0x2)
					{
						float CurrentWeightTotal = (float)(Weights.BlendWeights[0] + 1) / (float)0x8000;

						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], CurrentWeightTotal }, 0);
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], 1.0f - CurrentWeightTotal }, 1);
					}
					else if (ExternalWeights.NumWeights == 0x3)
					{
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], ExternalWeights.SimpleWeights[0] }, 0);
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], ExternalWeights.SimpleWeights[1] }, 1);
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[2]], ExternalWeights.SimpleWeights[2] }, 2);
					}
				}
			}
			else if (Model->Bones.Count() > 0)
			{
				// Only a default weight is needed
				Vertex.SetWeight({ 0, 1.f }, 0);
			}

			VertexBufferPtr += Submesh.VertexBufferStride;
		}

		for (uint32_t f = 0; f < (Strip.IndexCount / 3); f++)
		{
			auto i1 = *(uint16_t*)FaceBufferPtr;
			auto i2 = *(uint16_t*)(FaceBufferPtr + 1);
			auto i3 = *(uint16_t*)(FaceBufferPtr + 2);

			Mesh.Faces.EmplaceBack(i1, i2, i3);

			FaceBufferPtr += 3;
		}

		RpakStream->SetPosition(Fixup.FixupTableOffset);

		auto SubmeshLodReader = IO::BinaryReader(RpakStream.get(), true);
		auto SubmeshLod = SubmeshLodReader.Read<RMdlLodSubmesh>();
		auto& Material = (*Fixup.Materials)[SubmeshLod.Index];

		if (SubmeshLod.Index < Fixup.Materials->Count() && Assets.ContainsKey(Material.MaterialHash))
		{
			auto& MaterialAsset = Assets[Material.MaterialHash];

			auto ParsedMaterial = this->ExtractMaterial(MaterialAsset, Fixup.MaterialPath, IncludeMaterials);
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
			Mesh.MaterialIndices.EmplaceBack(-1);
		}

		// Add an extra slot for the extra UV Layer if present
		if ((Submesh.Flags2 & 0x2) == 0x2)
			Mesh.MaterialIndices.EmplaceBack(-1);

		Fixup.FixupTableOffset += sizeof(RMdlLodSubmesh);
	}
}

RMdlMaterial RpakLib::ExtractMaterial(const RpakLoadAsset& Asset, const string& Path, bool IncludeImages)
{
	RMdlMaterial Result;

	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto MatHeader = Reader.Read<MaterialHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, MatHeader.NameIndex, MatHeader.NameOffset));

	Result.MaterialName = IO::Path::GetFileNameWithoutExtension(Reader.ReadCString());

	RpakStream->SetPosition(this->GetFileOffset(Asset, MatHeader.TypeIndex, MatHeader.TypeOffset));

	const uint64_t TextureTable = (Asset.Version == RpakGameVersion::Apex) ? this->GetFileOffset(Asset, MatHeader.TexturesIndex, MatHeader.TexturesOffset) : this->GetFileOffset(Asset, MatHeader.TexturesTFIndex, MatHeader.TexturesTFOffset);
	const uint32_t TexturesCount = (Asset.Version == RpakGameVersion::Apex) ? 0x10 : 0x11;

	// These textures have named slots
	for (uint32_t i = 0; i < TexturesCount; i++)
	{
		RpakStream->SetPosition(TextureTable + ((uint64_t)i * 8));

		auto TextureHash = Reader.Read<uint64_t>();

		if (TextureHash != 0)
		{
			auto TextureName = string::Format("0x%llx%s", TextureHash, (const char*)ImageExtension);

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
			auto& Asset = Assets[TextureHash];
			// Make sure the data we got to is a proper texture
			if (Asset.AssetType == (uint32_t)RpakAssetType::Texture)
			{
				ExportTexture(Asset, Path);
			}
		}
	}

	return Result;
}

void RpakLib::ExtractTexture(const RpakLoadAsset& Asset, std::unique_ptr<Assets::Texture>& Texture)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto TexHeader = Reader.Read<TextureHeader>();

	Assets::DDSFormat Fmt;

	switch (TexHeader.Format)
	{
	case 0x0:	// DXT1 No-Alpha
	case 0x1:	// DXT1 Alpha
		Fmt.Format = DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM;
		break;
	case 0x6:	// BC4
		Fmt.Format = DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM;
		break;
	case 0x8:	// BC5
		Fmt.Format = DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM;
		break;
	case 0xA:
	case 0xB:
		Fmt.Format = DXGI_FORMAT::DXGI_FORMAT_BC6H_UF16;
		break;
	case 0xC:
	case 0xD:
		Fmt.Format = DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM;
		break;
	case 0x1f:
	case 0x20:
		Fmt.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case 0x2C:
		Fmt.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM;
		break;
	case 0x35:
		Fmt.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		break;
	default:
#if _DEBUG
		printf("0x%llx 0x%llx 0x%x size 0x%x\n", Asset.OptimalStarpakOffset, Asset.StarpakOffset, Asset.RawDataOffset, TexHeader.DataSize);
		printf("0x%llx %d %d Unknown format: 0x%x\n", Asset.NameHash, TexHeader.Width, TexHeader.Height, TexHeader.Format);
		__debugbreak();
#endif
		return;
	}

	Texture = std::make_unique<Assets::Texture>(TexHeader.Width, TexHeader.Height, Fmt.Format);

	uint32_t BlockSize = Texture->BlockSize();

	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;

	uint64_t Offset = 0;
	std::unique_ptr<IO::FileStream> StarpakStream = nullptr;

	if (Asset.OptimalStarpakOffset != -1)
	{
		Offset = ActualOptStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, true);

		if (this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap.ContainsKey(Asset.OptimalStarpakOffset))
		{
			Offset += (this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap[Asset.OptimalStarpakOffset] - BlockSize);
		}
		else
		{
			return;
		}
	}
	else if (Asset.StarpakOffset != -1)
	{
		Offset = ActualStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, false);

		if (this->LoadedFiles[Asset.FileIndex].StarpakMap.ContainsKey(Asset.StarpakOffset))
		{
			Offset += (this->LoadedFiles[Asset.FileIndex].StarpakMap[Asset.StarpakOffset] - BlockSize);
		}
		else
		{
			return;
		}
	}
	else if (Asset.RawDataIndex != -1 && Asset.RawDataIndex >= this->LoadedFiles[Asset.FileIndex].StartSegmentIndex)
	{
		//
		// All texture data is inline in rpak, we can calculate without anything else
		//

		uint64_t Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset) + (TexHeader.DataSize - BlockSize);

		RpakStream->SetPosition(Offset);
		RpakStream->Read(Texture->GetPixels(), 0, BlockSize);
		return;
	}
	else
	{
		// No bank found
		return;
	}

	StarpakStream->SetPosition(Offset);
	StarpakStream->Read(Texture->GetPixels(), 0, BlockSize);
}

void RpakLib::ExtractUIIA(const RpakLoadAsset& Asset, std::unique_ptr<Assets::Texture>& Texture)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto TexHeader = Reader.Read<UIIAHeader>();
	auto UnpackedShift = (~(unsigned __int8)((*(unsigned __int16*)&TexHeader.Flags) >> 5) & 2);

	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;

	std::unique_ptr<IO::MemoryStream> StarpakStream = nullptr;

	if (Asset.OptimalStarpakOffset != -1 && Asset.OptimalStarpakOffset != 0)
	{
		auto TempStream = this->GetStarpakStream(Asset, true);

		if (this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap.ContainsKey(Asset.OptimalStarpakOffset))
		{
			auto BufferSize = this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap[Asset.OptimalStarpakOffset];
			auto CompressedBuffer = std::make_unique<uint8_t[]>(BufferSize);

			TempStream->SetPosition(ActualOptStarpakOffset);
			TempStream->Read(CompressedBuffer.get(), 0, BufferSize);

			StarpakStream = DecompressStreamedBuffer(CompressedBuffer.get(), BufferSize, TexHeader.Flags.CompressionType);
		}
		else
		{
			return;
		}
	}
	else if (Asset.StarpakOffset != -1 && Asset.StarpakOffset != 0)
	{
		auto TempStream = this->GetStarpakStream(Asset, false);

		if (this->LoadedFiles[Asset.FileIndex].StarpakMap.ContainsKey(Asset.StarpakOffset))
		{
			auto BufferSize = this->LoadedFiles[Asset.FileIndex].StarpakMap[Asset.StarpakOffset];
			auto CompressedBuffer = std::make_unique<uint8_t[]>(BufferSize);

			TempStream->SetPosition(ActualStarpakOffset);
			TempStream->Read(CompressedBuffer.get(), 0, BufferSize);

			StarpakStream = DecompressStreamedBuffer(CompressedBuffer.get(), BufferSize, TexHeader.Flags.CompressionType);
		}
		else
		{
			return;
		}
	}
	else if (Asset.StarpakOffset == 0)
	{
		RpakStream->SetPosition(this->LoadedFiles[Asset.FileIndex].EmbeddedStarpakOffset);

		auto BufferSize = this->LoadedFiles[Asset.FileIndex].EmbeddedStarpakSize;
		auto CompressedBuffer = std::make_unique<uint8_t[]>(BufferSize);

		RpakStream->Read(CompressedBuffer.get(), 0, BufferSize);

		StarpakStream = DecompressStreamedBuffer(CompressedBuffer.get(), BufferSize, TexHeader.Flags.CompressionType);
	}

	if (Asset.RawDataIndex != -1 && Asset.RawDataIndex >= this->LoadedFiles[Asset.FileIndex].StartSegmentIndex)
	{
		//
		// All texture data is inline in rpak, we can calculate without anything else
		//

		RUIImage RImage{};
		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));
		RpakStream->Read((uint8_t*)&RImage, 0, sizeof(RUIImage));

		if (RImage.HighResolutionWidth + UnpackedShift > 2)
			RImage.HighResolutionWidth += UnpackedShift;
		if (RImage.HighResolutionHeight + UnpackedShift > 2)
			RImage.HighResolutionHeight += UnpackedShift;
		if (RImage.LowResolutionWidth + UnpackedShift > 2)
			RImage.LowResolutionWidth += UnpackedShift;
		if (RImage.LowResolutionHeight + UnpackedShift > 2)
			RImage.LowResolutionHeight += UnpackedShift;

		auto UseHighResolution = StarpakStream != nullptr;

		auto WidthBlocks = UseHighResolution ? (RImage.HighResolutionWidth + 29) / 31 : (RImage.LowResolutionWidth + 29) / 31;
		auto HeightBlocks = UseHighResolution ? (RImage.HighResolutionHeight + 29) / 31 : (RImage.LowResolutionHeight + 29) / 31;
		auto TotalBlocks = WidthBlocks * HeightBlocks;
		auto Width = WidthBlocks * 32;
		auto Height = HeightBlocks * 32;

		uint64_t Offset = UseHighResolution ? 0 : this->GetFileOffset(Asset, RImage.BufferIndex, RImage.BufferOffset);

		if (UseHighResolution)
		{
			RpakStream = std::move(StarpakStream);
		}
		RpakStream->SetPosition(Offset);

		auto CodePoints = std::make_unique<RUIImageTile[]>(TotalBlocks);
		auto CodePointsSize = TotalBlocks * sizeof(RUIImageTile);

		// Check if we have tables for the tiles, if not, generate the opcodes directly.
		if (!UseHighResolution && (!TexHeader.Flags.HasLowTable || !TexHeader.Flags.LowTableBc7))
		{
			for (uint32_t i = 0; i < TotalBlocks; i++)
			{
				CodePoints[i].Opcode = TexHeader.Flags.LowTableBc7 ? 0x41 : 0x40;
				CodePoints[i].Offset = i * (TexHeader.Flags.LowTableBc7 ? 1024 : 512);
			}
			CodePointsSize = 0;
		}
		else if (UseHighResolution && (!TexHeader.Flags.HasHighTable || !TexHeader.Flags.HighTableBc7))
		{
			for (uint32_t i = 0; i < TotalBlocks; i++)
			{
				CodePoints[i].Opcode = TexHeader.Flags.HighTableBc7 ? 0x41 : 0x40;
				CodePoints[i].Offset = i * (TexHeader.Flags.HighTableBc7 ? 1024 : 512);
			}
			CodePointsSize = 0;
		}
		else
		{
			RpakStream->Read((uint8_t*)CodePoints.get(), 0, CodePointsSize);
		}

		uint32_t NumBc1Blocks = 0;
		uint32_t NumBc7Blocks = 0;

		for (uint32_t i = 0; i < TotalBlocks; i++)
		{
			// This opcode requires us to copy an existing opcode.
			if (CodePoints[i].Opcode == 0xC0)
			{
				CodePoints[i] = CodePoints[CodePoints[i].Offset];
			}

			// Standard block compressed opcodes (Bc1/Bc7).
			if (CodePoints[i].Opcode == 0x40)
			{
				NumBc1Blocks++;
			}
			else if (CodePoints[i].Opcode == 0x41)
			{
				NumBc7Blocks++;
			}
		}

		if (NumBc1Blocks)
		{
			//
			// We are now at the Bc1 blocks.
			//

			auto Bc1Destination = std::make_unique<uint8_t[]>(TotalBlocks * 512);
			auto Bc1Texture = std::make_unique<Assets::Texture>(Width, Height, DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM);

			//
			// Copy over the blocks
			//

			for (uint32_t y = 0; y < HeightBlocks; y++)
			{
				for (uint32_t x = 0; x < WidthBlocks; x++)
				{
					auto& Point = CodePoints[x + (y * WidthBlocks)];
					auto BlockOffset = (x * 512) + ((y * WidthBlocks) * 512);

					if (Point.Opcode != 0x40)
					{
						std::memcpy(Bc1Destination.get() + BlockOffset, RUIImageTileBc1, sizeof(RUIImageTileBc1));
						continue;
					}

					RpakStream->SetPosition(Point.Offset + Offset);

					// Bc1Destination contains 32x32 BC1 512 bytes swizzled, copy to texture location.
					RpakStream->Read(Bc1Destination.get() + BlockOffset, 0, 512);
				}
			}

			//
			// Unswizzle Bc1 blocks.
			//

			auto Num5 = Height / 4;
			auto Num6 = Width / 4;
			constexpr uint32_t Bc1Bpp2x = 4 * 2;

			uint64_t Bc1Offset = 0;

			for (uint32_t i = 0; i < Num5; i++)
			{
				for (uint32_t j = 0; j < Num6; j++)
				{
					int mx = j;
					int my = i;

					g_pRtech->UnswizzleBlock(j, i, Num6, 2, mx, my);
					g_pRtech->UnswizzleBlock(mx, my, Num6, 4, mx, my);
					g_pRtech->UnswizzleBlock(mx, my, Num6, 8, mx, my);

					uint64_t destination = Bc1Bpp2x * (my * Num6 + mx);

					std::memcpy(Bc1Texture->GetPixels() + destination, Bc1Destination.get() + Bc1Offset, Bc1Bpp2x);
					Bc1Offset += Bc1Bpp2x;
				}
			}

			Bc1Texture->ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
			Texture = std::move(Bc1Texture);
		}
		else
		{
			// We didn't have an initial texture to use, to initialize the resulting texture
			Texture = std::make_unique<Assets::Texture>(Width, Height, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
		}

		if (NumBc7Blocks)
		{
			//
			// We are now at the Bc7 blocks.
			//

			auto Bc7Destination = std::make_unique<uint8_t[]>(TotalBlocks * 1024);
			auto Bc7Texture = std::make_unique<Assets::Texture>(Width, Height, DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM);

			//
			// Copy over the blocks
			//

			for (uint32_t y = 0; y < HeightBlocks; y++)
			{
				for (uint32_t x = 0; x < WidthBlocks; x++)
				{
					auto& Point = CodePoints[x + (y * WidthBlocks)];
					auto BlockOffset = (x * 1024) + ((y * WidthBlocks) * 1024);

					if (Point.Opcode != 0x41)
					{
						std::memcpy(Bc7Destination.get() + BlockOffset, RUIImageTileBc7, sizeof(RUIImageTileBc7));
						continue;
					}

					RpakStream->SetPosition(Point.Offset + Offset);

					// Bc1Destination contains 32x32 BC7 1024 bytes swizzled, copy to texture location.
					RpakStream->Read(Bc7Destination.get() + BlockOffset, 0, 1024);
				}
			}

			//
			// Unswizzle Bc1 blocks.
			//

			auto Num5 = Height / 4;
			auto Num6 = Width / 4;
			constexpr uint32_t Bc7Bpp2x = 8 * 2;

			uint64_t Bc7Offset = 0;

			for (uint32_t i = 0; i < Num5; i++)
			{
				for (uint32_t j = 0; j < Num6; j++)
				{
					int mx = j;
					int my = i;

					g_pRtech->UnswizzleBlock(j, i, Num6, 2, mx, my);
					g_pRtech->UnswizzleBlock(mx, my, Num6, 4, mx, my);
					g_pRtech->UnswizzleBlock(mx, my, Num6, 8, mx, my);

					uint64_t destination = Bc7Bpp2x * (my * Num6 + mx);


					std::memcpy(Bc7Texture->GetPixels() + destination, Bc7Destination.get() + Bc7Offset, Bc7Bpp2x);
					Bc7Offset += Bc7Bpp2x;
				}
			}

			Bc7Texture->ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);

			if (NumBc1Blocks)
			{
				// Stitch the 32x32 uncompressed tiles to the final image.
				for (uint32_t y = 0; y < HeightBlocks; y++)
				{
					for (uint32_t x = 0; x < WidthBlocks; x++)
					{
						auto& Point = CodePoints[x + (y * WidthBlocks)];

						if (Point.Opcode == 0x41)
						{
							// TODO(rx): reimplement?
							//Texture->CopyTextureSlice(*Bc7Texture, { (x * 32),(y * 32),32,32 }, (x * 32), (y * 32));
						}
					}
				}
			}
			else
			{
				// We are the final image
				Texture = std::move(Bc7Texture);
			}
		}
	}
}

void RpakLib::ExtractAnimation(const RpakLoadAsset& Asset, const List<Assets::Bone>& Skeleton, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto AnHeader = Reader.Read<AnimHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, AnHeader.NameIndex, AnHeader.NameOffset));

	auto AnimName = IO::Path::GetFileNameWithoutExtension(Reader.ReadCString());

	const uint64_t AnimationOffset = this->GetFileOffset(Asset, AnHeader.AnimationIndex, AnHeader.AnimationOffset);

	RpakStream->SetPosition(AnimationOffset);

	auto AnimSequenceHeader = Reader.Read<RAnimHeader>();

	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t StarpakPatchIndex = Asset.StarpakOffset & 0xFF;
	uint64_t OptStarpakIndex = Asset.OptimalStarpakOffset & 0xFF;

	uint64_t OffsetOfStarpakData = 0;
	std::unique_ptr<IO::FileStream> StarpakStream = nullptr;

	if (Asset.OptimalStarpakOffset != -1)
	{
		OffsetOfStarpakData = ActualOptStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, true);
	}
	else if (Asset.StarpakOffset != -1)
	{
		OffsetOfStarpakData = ActualStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, false);
	}

	auto StarpakReader = IO::BinaryReader(StarpakStream.get(), true);

	for (uint32_t i = 0; i < AnimSequenceHeader.AnimationCount; i++)
	{
		RpakStream->SetPosition(AnimationOffset + AnimSequenceHeader.AnimationOffset + ((uint64_t)i * 0x4));

		auto AnimDataOffset = Reader.Read<uint32_t>();

		RpakStream->SetPosition(AnimationOffset + AnimDataOffset);

		auto offseq = RpakStream->GetPosition();

		auto AnimDataHeader = Reader.Read<RAnimSequenceHeader>();

		if (!(AnimDataHeader.Flags & 0x20000))
			continue;

		auto Anim = std::make_unique<Assets::Animation>(Skeleton.Count());
		auto AnimCurveType = Assets::AnimationCurveMode::Absolute;

		if (AnimDataHeader.Flags & 0x4)
		{
			AnimCurveType = Assets::AnimationCurveMode::Additive;
		}

		for (auto& Bone : Skeleton)
		{
			Anim->Bones.EmplaceBack(Bone.Name(), Bone.Parent(), Bone.LocalPosition(), Bone.LocalRotation());

			auto& CurveNodes = Anim->GetNodeCurves(Bone.Name());

			// Inject curve nodes here, we can use the purge empty later to remove them
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::RotateQuaternion, AnimCurveType);
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::TranslateX, AnimCurveType);
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::TranslateY, AnimCurveType);
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::TranslateZ, AnimCurveType);
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::ScaleX, AnimCurveType);
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::ScaleY, AnimCurveType);
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::ScaleZ, AnimCurveType);
		}

		const auto AnimHeaderPointer = AnimationOffset + AnimDataOffset;

		for (uint32_t Frame = 0; Frame < AnimDataHeader.FrameCount; Frame++)
		{
			uint32_t ChunkTableIndex = 0;
			uint32_t ChunkFrame = Frame;
			uint32_t FrameCountOneLess = 0;
			uint32_t FirstChunk = AnimDataHeader.FirstChunkOffset;
			uint64_t ChunkDataOffset = 0;
			uint32_t IsChunkInStarpak = 0;
			uint64_t ResultDataPtr = 0;

			if (!AnimDataHeader.FrameMedianCount)
			{
				// Nothing here
				goto nomedian;
			}
			else if (ChunkFrame >= AnimDataHeader.FrameSplitCount)
			{
				auto FrameCount = AnimDataHeader.FrameCount;
				auto ChunkFrameMinusSplitCount = ChunkFrame - AnimDataHeader.FrameSplitCount;
				if (FrameCount <= AnimDataHeader.FrameMedianCount || ChunkFrame != FrameCount - 1)
				{
					ChunkTableIndex = ChunkFrameMinusSplitCount / AnimDataHeader.FrameMedianCount + 1;
					ChunkFrame = ChunkFrame - (AnimDataHeader.FrameMedianCount * (ChunkFrameMinusSplitCount / AnimDataHeader.FrameMedianCount)) - AnimDataHeader.FrameSplitCount;
				}
				else
				{
					ChunkFrame = 0;
					ChunkTableIndex = (FrameCount - AnimDataHeader.FrameSplitCount - 1) / AnimDataHeader.FrameMedianCount + 2;
				}
			}

			ChunkDataOffset = AnimDataHeader.OffsetToChunkOffsetsTable + 8 * (uint64_t)ChunkTableIndex;

			RpakStream->SetPosition(AnimHeaderPointer + ChunkDataOffset);
			FirstChunk = Reader.Read<uint32_t>();

			RpakStream->SetPosition(AnimHeaderPointer + ChunkDataOffset + 4);
			IsChunkInStarpak = Reader.Read<uint32_t>();

			if (IsChunkInStarpak)
			{
				auto v13 = AnimDataHeader.SomeDataOffset;
				if (v13)
				{
					ResultDataPtr = v13 + FirstChunk;
				}
				else
				{
					RpakStream->SetPosition(AnimHeaderPointer + ChunkDataOffset);
					auto v14 = Reader.Read<uint32_t>();
					ResultDataPtr = OffsetOfStarpakData + v14;
				}
			}
			else
			{
			nomedian:
				ResultDataPtr = AnimHeaderPointer + FirstChunk;
			}

			char BoneFlags[256]{};

			if (IsChunkInStarpak)
			{
				StarpakStream->SetPosition(ResultDataPtr);
				StarpakStream->Read((uint8_t*)BoneFlags, 0, ((4 * (uint64_t)Skeleton.Count() + 7) / 8 + 1) & 0xFFFFFFFFFFFFFFFE);
			}
			else
			{
				RpakStream->SetPosition(ResultDataPtr);
				RpakStream->Read((uint8_t*)BoneFlags, 0, ((4 * (uint64_t)Skeleton.Count() + 7) / 8 + 1) & 0xFFFFFFFFFFFFFFFE);
			}

			for (uint32_t b = 0; b < Skeleton.Count(); b++)
			{
				uint32_t Shift = 4 * (b % 2);
				char BoneTrackFlags = BoneFlags[b / 2] >> Shift;

				if (BoneTrackFlags & 0x7)
				{
					uint64_t TrackDataRead = 0;

					auto BoneDataFlags = IsChunkInStarpak ? StarpakReader.Read<RAnimBoneFlag>() : Reader.Read<RAnimBoneFlag>();
					auto BoneTrackData = IsChunkInStarpak ? StarpakReader.Read((BoneDataFlags.Size > 0) ? BoneDataFlags.Size - sizeof(uint16_t) : 0, TrackDataRead) : Reader.Read((BoneDataFlags.Size > 0) ? BoneDataFlags.Size - sizeof(uint16_t) : 0, TrackDataRead);

					uint16_t* BoneTrackDataPtr = (uint16_t*)BoneTrackData.get();

					// Set this so when we do translations we will know whether or not to add the rest position onto it...
					BoneDataFlags.bAdditiveCustom = (AnimCurveType == Assets::AnimationCurveMode::Additive);

					if (BoneTrackFlags & 0x1)
						ParseRAnimBoneTranslationTrack(BoneDataFlags, &BoneTrackDataPtr, Anim, b, ChunkFrame, Frame);
					if (BoneTrackFlags & 0x2)
						ParseRAnimBoneRotationTrack(BoneDataFlags, &BoneTrackDataPtr, Anim, b, ChunkFrame, Frame);
					if (BoneTrackFlags & 0x4)
						ParseRAnimBoneScaleTrack(BoneDataFlags, &BoneTrackDataPtr, Anim, b, ChunkFrame, Frame);
				}
			}
		}

		Anim->RemoveEmptyNodes();

		try
		{
			this->AnimExporter->ExportAnimation(*Anim.get(), IO::Path::Combine(Path, AnimName + string::Format("_%d", i) + (const char*)this->AnimExporter->AnimationExtension()));
		}
		catch (...)
		{
		}
	}
}

List<Assets::Bone> RpakLib::ExtractSkeleton(IO::BinaryReader& Reader, uint64_t SkeletonOffset)
{
	auto RpakStream = Reader.GetBaseStream();

	RpakStream->SetPosition(SkeletonOffset);

	auto SkeletonHeader = Reader.Read<RMdlSkeletonHeader>();

	auto Result = List<Assets::Bone>(SkeletonHeader.BoneCount);

	for (uint32_t i = 0; i < SkeletonHeader.BoneCount; i++)
	{
		auto Position = SkeletonOffset + SkeletonHeader.BoneDataOffset + (i * sizeof(RMdlBone));

		RpakStream->SetPosition(Position);
		auto Bone = Reader.Read<RMdlBone>();
		RpakStream->SetPosition(Position + Bone.NameOffset);

		auto TagName = Reader.ReadCString();

		Result.EmplaceBack(TagName, Bone.ParentIndex, Bone.Position, Bone.Rotation);
	}

	return Result;
}

List<List<DataTableColumnData>> RpakLib::ExtractDataTable(const RpakLoadAsset& Asset)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto DtblHeader = Reader.Read<DataTableHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, DtblHeader.ColumnHeaderBlock, DtblHeader.ColumnHeaderOffset));

	List<DataTableColumn> Columns;
	//List<string> ColumnNames;
	List<List<DataTableColumnData>> Data;

	for (int i = 0; i < DtblHeader.ColumnCount; ++i)
	{
		DataTableColumn col;

		uint32_t id = Reader.Read<uint32_t>();
		uint32_t offset = Reader.Read<uint32_t>();

		col.Unk0Seek = this->GetFileOffset(Asset, id, offset);

		col.Unk8 = Reader.Read<uint64_t>();
		col.Type = Reader.Read<uint32_t>();
		col.RowOffset = Reader.Read<uint32_t>();

		Columns.EmplaceBack(col);
	}

	List<DataTableColumnData> ColumnNameData;

	for (int i = 0; i < DtblHeader.ColumnCount; ++i)
	{
		DataTableColumn col = Columns[i];

		RpakStream->SetPosition(col.Unk0Seek);
		string name = Reader.ReadCString();

		//ColumnNames.EmplaceBack();
		DataTableColumnData cd;

		cd.stringValue = name;
		cd.Type = DataTableColumnDataType::StringT;

		ColumnNameData.EmplaceBack(cd);
	}

	Data.EmplaceBack(ColumnNameData);

	uint64_t rows_seek = this->GetFileOffset(Asset, DtblHeader.RowHeaderBlock, DtblHeader.RowHeaderOffset);

	for (int i = 0; i < DtblHeader.RowCount; ++i)
	{
		List<DataTableColumnData> RowData;

		for (int c = 0; c < DtblHeader.ColumnCount; ++c)
		{
			DataTableColumn col = Columns[c];

			uint64_t base_pos = rows_seek + (DtblHeader.RowStride * i);
			RpakStream->SetPosition(base_pos + col.RowOffset);

			DataTableColumnData d;

			d.Type = (DataTableColumnDataType)col.Type;

			switch (col.Type)
			{
			case DataTableColumnDataType::Bool:
				d.bValue = Reader.Read<uint32_t>() != 0;
				break;
			case DataTableColumnDataType::Int:
				d.iValue = Reader.Read<int32_t>();
				break;
			case DataTableColumnDataType::Float:
				d.fValue = Reader.Read<float>();
				break;
			case DataTableColumnDataType::Vector:
				d.vValue = Reader.Read<Math::Vector3>();
				break;
			case DataTableColumnDataType::Asset:
			{
				uint32_t id = Reader.Read<uint32_t>();
				uint32_t off = Reader.Read<uint32_t>();
				uint64_t pos = this->GetFileOffset(Asset, id, off);
				RpakStream->SetPosition(pos);
				d.assetValue = Reader.ReadCString();
				break;
			}
			case DataTableColumnDataType::AssetNoPrecache:
			{
				uint32_t id = Reader.Read<uint32_t>();
				uint32_t off = Reader.Read<uint32_t>();
				uint64_t pos = this->GetFileOffset(Asset, id, off);
				RpakStream->SetPosition(pos);

				d.assetNPValue = Reader.ReadCString();
				break;
			}
			case DataTableColumnDataType::StringT:
			{
				uint32_t id = Reader.Read<uint32_t>();
				uint32_t off = Reader.Read<uint32_t>();
				uint64_t pos = this->GetFileOffset(Asset, id, off);
				RpakStream->SetPosition(pos);

				d.stringValue = Reader.ReadCString();
				break;
			}

			}
			RowData.EmplaceBack(d);
		}
		Data.EmplaceBack(RowData);
	}
	return Data;
}

List<SubtitleEntry> RpakLib::ExtractSubtitles(const RpakLoadAsset& Asset)
{
	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto SubtHdr = Reader.Read<SubtitleHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, SubtHdr.EntriesIndex, SubtHdr.EntriesOffset));

	List<SubtitleEntry> Subtitles;

	std::regex ClrRegex("<clr:([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3})>");


	while (!RpakStream->GetIsEndOfFile())
	{
		SubtitleEntry se;
		
		string temp_string = Reader.ReadCString();

		std::smatch sm;

		std::string s(temp_string);

		std::regex_search(s, sm, ClrRegex);

		if (sm.size() == 4)
			se.Color = Math::Vector3(atof(sm[1].str().c_str()), atof(sm[2].str().c_str()), atof(sm[3].str().c_str()));
		else
			se.Color = Math::Vector3(255, 255, 255);

		se.SubtitleText = temp_string.Substring(sm[0].str().length());

		Subtitles.EmplaceBack(se);
	}
	return Subtitles;
}

string RpakLib::GetSubtitlesNameFromHash(uint64_t Hash)
{
	if (SubtitleLanguageMap.count((SubtitleLanguageHash)Hash))
		return "subtitles_" + SubtitleLanguageMap[(SubtitleLanguageHash)Hash];

	return string::Format("subt_0x%llx", Hash);
}

void RpakLib::ParseRAnimBoneTranslationTrack(const RAnimBoneFlag& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	uint16_t* TranslationDataPtr = *BoneTrackData;

	if (!BoneFlags.bDynamicTranslation)
	{
		auto& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

		// TranslateX/Y/Z
		Curves[1].Keyframes.EmplaceBack(FrameIndex, Math::Half(TranslationDataPtr[0]).ToFloat());
		Curves[2].Keyframes.EmplaceBack(FrameIndex, Math::Half(TranslationDataPtr[1]).ToFloat());
		Curves[3].Keyframes.EmplaceBack(FrameIndex, Math::Half(TranslationDataPtr[2]).ToFloat());

		*BoneTrackData += 3;	// Advance over the size of the data
	}
	else
	{
		uint16_t TranslationFlags = TranslationDataPtr[2];
		float TranslationScale = *(float*)TranslationDataPtr;

		uint8_t* TranslationDataX = (uint8_t*)TranslationDataPtr + (TranslationFlags & 0x1FFF) + 4;	// Data for x

		uint64_t DataYOffset = *((uint8_t*)TranslationDataPtr + 6);
		uint64_t DataZOffset = *((uint8_t*)TranslationDataPtr + 7);

		uint8_t* TranslationDataY = &TranslationDataX[2 * DataYOffset];	// Data for y
		uint8_t* TranslationDataZ = &TranslationDataX[2 * DataZOffset];	// Data for z

		auto& Bone = Anim->Bones[BoneIndex].LocalPosition();

		float Result[3]{ Bone.X, Bone.Y, Bone.Z };

		uint32_t TranslationIndex = 0;
		uint32_t v32 = 0xF;

		uint8_t* dataPtrs[] = { TranslationDataX,TranslationDataY,TranslationDataZ };

		float TranslationFinal = 0, TimeScale = 0 /*Might not be 'TimeScale'*/;
		float Time = 0;	// time but doesn't matter
		do
		{
			if (_bittest((const long*)&TranslationFlags, v32))
			{
				g_pRtech->DecompressDynamicTrack(Frame, dataPtrs[TranslationIndex], TranslationScale, &TranslationFinal, &TimeScale);

				if (BoneFlags.bAdditiveCustom)
					Result[TranslationIndex] = (float)((float)((float)(1.0 - Time) * TranslationFinal) + (float)(TimeScale * Time));
				else
					Result[TranslationIndex] = (float)((float)((float)(1.0 - Time) * TranslationFinal) + (float)(TimeScale * Time)) + Result[TranslationIndex];
			}

			--v32;
			++TranslationIndex;
		} while (TranslationIndex < 3);

		auto& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

		// TranslateX/Y/Z
		Curves[1].Keyframes.EmplaceBack(FrameIndex, Result[0]);
		Curves[2].Keyframes.EmplaceBack(FrameIndex, Result[1]);
		Curves[3].Keyframes.EmplaceBack(FrameIndex, Result[2]);

		*BoneTrackData += 4;	// Advance over the size of the data
	}
}

void RpakLib::ParseRAnimBoneRotationTrack(const RAnimBoneFlag& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	uint16_t* RotationDataPtr = *BoneTrackData;

	struct Quat64
	{
		uint64_t X : 21;
		uint64_t Y : 21;
		uint64_t Z : 21;
		uint64_t WNeg : 1;
	};

	if (!BoneFlags.bDynamicRotation)
	{
		auto PackedQuat = *(Quat64*)RotationDataPtr;

		Math::Quaternion Quat;

		Quat.X = ((int)PackedQuat.X - 1048576) * (1 / 1048576.5f);
		Quat.Y = ((int)PackedQuat.Y - 1048576) * (1 / 1048576.5f);
		Quat.Z = ((int)PackedQuat.Z - 1048576) * (1 / 1048576.5f);
		Quat.W = std::sqrt(1 - Quat.X * Quat.X - Quat.Y * Quat.Y - Quat.Z * Quat.Z);

		if (PackedQuat.WNeg)
			Quat.W = -Quat.W;

		// RotateQuaternion
		Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name())[0].Keyframes.Emplace(FrameIndex, Math::Quaternion(Quat[0], Quat[1], Quat[2], Quat[3]));

		*BoneTrackData += 4;	// Advance over the size of the data
	}
	else
	{
		uint16_t RotationFlags = RotationDataPtr[0];

		uint8_t* TranslationDataX = (uint8_t*)RotationDataPtr + (RotationFlags & 0x1FFF);	// Data for x

		uint64_t DataYOffset = *((uint8_t*)RotationDataPtr + 2);
		uint64_t DataZOffset = *((uint8_t*)RotationDataPtr + 3);

		uint8_t* TranslationDataY = &TranslationDataX[2 * DataYOffset];	// Data for y
		uint8_t* TranslationDataZ = &TranslationDataX[2 * DataZOffset];	// Data for z


		auto BoneRotation = Anim->Bones[BoneIndex].LocalRotation().ToEulerAngles();

		float EulerResult[4]{ Math::MathHelper::DegreesToRadians(BoneRotation.X),Math::MathHelper::DegreesToRadians(BoneRotation.Y),Math::MathHelper::DegreesToRadians(BoneRotation.Z),0 };

		uint32_t v31 = 0;
		uint32_t v32 = 0xF;

		uint8_t* dataPtrs[] = { TranslationDataX,TranslationDataY,TranslationDataZ };

		float TranslationFinal = 0, TimeScale = 0 /*Might not be 'TimeScale'*/;
		float a2 = 0;	// time?
		do
		{
			if (_bittest((const long*)&RotationFlags, v32))
			{
				g_pRtech->DecompressDynamicTrack(Frame, dataPtrs[v31], 0.00019175345f, &TranslationFinal, &TimeScale);
				EulerResult[v31] = TranslationFinal;
			}

			--v32;
			++v31;
		} while (v31 < 3);

		Math::Quaternion Result;
		g_pRtech->DecompressConvertRotation((const __m128i*) & EulerResult[0], (float*)&Result);

		Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name())[0].Keyframes.Emplace(FrameIndex, Result);

		*BoneTrackData += 2;	// Advance over the size of the data
	}
}

void RpakLib::ParseRAnimBoneScaleTrack(const RAnimBoneFlag& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	uint16_t* ScaleDataPtr = *BoneTrackData;

	if (!BoneFlags.bDynamicScale)
	{
		auto& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

		// ScaleX/Y/Z
		Curves[4].Keyframes.EmplaceBack(FrameIndex, Math::Half(ScaleDataPtr[0]).ToFloat());
		Curves[5].Keyframes.EmplaceBack(FrameIndex, Math::Half(ScaleDataPtr[1]).ToFloat());
		Curves[6].Keyframes.EmplaceBack(FrameIndex, Math::Half(ScaleDataPtr[2]).ToFloat());

		*BoneTrackData += 3;	// Advance over the size of the data
	}
	else
	{
		uint32_t ScaleFlags = ScaleDataPtr[0];

		uint8_t* TranslationDataX = (uint8_t*)ScaleDataPtr + (ScaleFlags & 0x1FFF);	// Data for x

		uint64_t DataYOffset = *((uint8_t*)ScaleDataPtr + 2);
		uint64_t DataZOffset = *((uint8_t*)ScaleDataPtr + 3);

		uint8_t* TranslationDataY = &TranslationDataX[2 * DataYOffset];	// Data for y
		uint8_t* TranslationDataZ = &TranslationDataX[2 * DataZOffset];	// Data for z

		auto& BoneScale = Anim->Bones[BoneIndex].Scale();

		float Result[3]{ BoneScale.X, BoneScale.Y, BoneScale.Z };

		uint32_t v31 = 0;
		uint32_t v32 = 0xF;

		uint8_t* dataPtrs[] = { TranslationDataX,TranslationDataY,TranslationDataZ };

		float TranslationFinal = 0, TimeScale = 0 /*Might not be 'TimeScale'*/;
		float a2 = 0;	// time but doesn't matter
		do
		{
			if (_bittest((const long*)&ScaleFlags, v32))
			{
				g_pRtech->DecompressDynamicTrack(Frame, dataPtrs[v31], 0.0030518509f, &TranslationFinal, &TimeScale);
				Result[v31] = (float)((float)((float)(1.0 - a2) * TranslationFinal) + (float)(TimeScale * a2)) + Result[v31];
			}

			--v32;
			++v31;
		} while (v31 < 3);

		auto& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

		// ScaleX/Y/Z
		Curves[4].Keyframes.EmplaceBack(FrameIndex, Result[0]);
		Curves[5].Keyframes.EmplaceBack(FrameIndex, Result[1]);
		Curves[6].Keyframes.EmplaceBack(FrameIndex, Result[2]);

		*BoneTrackData += 2;	// Advance over the size of the data
	}
}

bool RpakLib::ValidateAssetPatchStatus(const RpakLoadAsset& Asset)
{
	auto& LoadedFile = this->LoadedFiles[Asset.FileIndex];

	if (Asset.SubHeaderIndex >= LoadedFile.StartSegmentIndex)
	{
		// The sub header and data blocks are within the packages...
		// We now need to check if the asset subheader blocks are within the package...
		auto RpakStream = this->GetFileStream(Asset);
		auto Reader = IO::BinaryReader(RpakStream.get(), true);

		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

		switch (Asset.AssetType)
		{
		case (uint32_t)RpakAssetType::Model:
		{
			if (Asset.SubHeaderSize <= 0x68)
			{
				auto SubHeader = Reader.Read<ModelHeaderS68>();
				return (SubHeader.NameIndex >= LoadedFile.StartSegmentIndex && SubHeader.SkeletonIndex >= LoadedFile.StartSegmentIndex);
			}
			else
			{
				auto SubHeader = Reader.Read<ModelHeaderS80>();
				return (SubHeader.NameIndex >= LoadedFile.StartSegmentIndex && SubHeader.SkeletonIndex >= LoadedFile.StartSegmentIndex);
			}
		}
		case (uint32_t)RpakAssetType::Texture:
		{
			auto SubHeader = Reader.Read<TextureHeader>();
			return (SubHeader.DataSize > 0);
		}
		case (uint32_t)RpakAssetType::UIIA:
		{
			auto SubHeader = Reader.Read<UIIAHeader>();
			return (SubHeader.Width > 0 && SubHeader.Height > 0);
		}
		case (uint32_t)RpakAssetType::Material:
		{
			auto SubHeader = Reader.Read<MaterialHeader>();
			if (Asset.Version == RpakGameVersion::Apex)
				return (SubHeader.NameIndex >= LoadedFile.StartSegmentIndex && SubHeader.TexturesIndex >= LoadedFile.StartSegmentIndex);
			else
				return (SubHeader.NameIndex >= LoadedFile.StartSegmentIndex && SubHeader.TexturesTFIndex >= LoadedFile.StartSegmentIndex);
		}
		case (uint32_t)RpakAssetType::AnimationRig:
		{
			auto SubHeader = Reader.Read<AnimRigHeader>();
			return (SubHeader.NameIndex >= LoadedFile.StartSegmentIndex && SubHeader.SkeletonIndex >= LoadedFile.StartSegmentIndex && SubHeader.AnimationReferenceIndex >= LoadedFile.StartSegmentIndex);
		}
		case (uint32_t)RpakAssetType::Animation:
		{
			auto SubHeader = Reader.Read<AnimHeader>();
			return (SubHeader.AnimationIndex >= LoadedFile.StartSegmentIndex);
		}
		case (uint32_t)RpakAssetType::DataTable:
		{
			auto SubHeader = Reader.Read<DataTableHeader>();
			return (SubHeader.ColumnCount != 0 && SubHeader.RowCount != 0);
		}
		case (uint32_t)RpakAssetType::Subtitles:
		{
			return true;
		}
		default:
			return false;
		}
	}

	// Outside bounds of app
	return false;
}

bool RpakLib::ValidateAssetStreamStatus(const RpakLoadAsset& Asset)
{
	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t StarpakPatchIndex = Asset.StarpakOffset & 0xFF;
	uint64_t OptStarpakIndex = Asset.OptimalStarpakOffset & 0xFF;

	if (Asset.OptimalStarpakOffset != -1 && Asset.OptimalStarpakOffset != 0)
	{
		if (!this->LoadedFiles[Asset.RpakFileIndex].OptimalStarpakMap.ContainsKey(Asset.OptimalStarpakOffset))
			return false;
	}

	if (Asset.StarpakOffset != -1 && Asset.StarpakOffset != 0)
	{
		if (!this->LoadedFiles[Asset.RpakFileIndex].StarpakMap.ContainsKey(Asset.StarpakOffset))
			return false;
	}

	return true;
}

bool RpakLib::MountRpak(const string& Path, bool Dump)
{
	auto Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	auto BaseHeader = Reader.Read<RpakBaseHeader>();

	if (BaseHeader.Magic != 0x6B615052)
	{
		return false;
	}

	switch (BaseHeader.Version)
	{
	case (uint32_t)RpakGameVersion::Apex:
		return this->MountApexRpak(Path, Dump);
	case (uint32_t)RpakGameVersion::Titanfall:
		return this->MountTitanfallRpak(Path, Dump);
	default:
		return false;
	}
}

bool RpakLib::ParseApexRpak(const string& RpakPath, std::unique_ptr<IO::MemoryStream>& ParseStream)
{
	auto Reader = IO::BinaryReader(ParseStream.get(), true);
	auto RpakRoot = IO::Path::GetDirectoryName(RpakPath);
	auto Header = Reader.Read<RpakApexHeader>();
	auto File = &this->LoadedFiles[this->LoadedFileIndex++];

	RpakPatchHeader PatchHeader{};
	RpakPatchCompressPair PatchCompressPairs[16]{};
	uint16_t PatchIndicesToFile[16]{};

	if (Header.PatchIndex)
	{
		ParseStream->Read((uint8_t*)&PatchHeader, 0, sizeof(RpakPatchHeader));
		ParseStream->Read((uint8_t*)&PatchCompressPairs, 0, sizeof(RpakPatchCompressPair) * Header.PatchIndex);
		ParseStream->Read((uint8_t*)&PatchIndicesToFile, 0, sizeof(uint16_t) * Header.PatchIndex);
	}

	uint32_t StarpakLen = Header.StarpakReferenceSize;
	while (StarpakLen > 0)
	{
		auto Starpak = Reader.ReadCString();

		if (Starpak.Length() > 0)
		{
			auto Path = IO::Path::Combine(RpakRoot, IO::Path::GetFileName(Starpak));
			this->MountStarpak(Path, this->LoadedFileIndex - 1, File->StarpakReferences.Count(), false);
			File->StarpakReferences.EmplaceBack(Path);
		}

		StarpakLen -= Starpak.Length() + sizeof(char);
	}
	StarpakLen = Header.StarpakOptReferenceSize;
	while (StarpakLen > 0)
	{
		auto Starpak = Reader.ReadCString();

		if (Starpak.Length() > 0)
		{
			auto Path = IO::Path::Combine(RpakRoot, IO::Path::GetFileName(Starpak));
			this->MountStarpak(Path, this->LoadedFileIndex - 1, File->OptimalStarpakReferences.Count(), true);
			File->OptimalStarpakReferences.EmplaceBack(Path);
		}

		StarpakLen -= Starpak.Length() + sizeof(char);
	}

	// We need to load the rest of the data before applying a patch stream
	List<RpakVirtualSegment> VirtualSegments(Header.VirtualSegmentCount, true);
	List<RpakVirtualSegmentBlock> VirtualSegmentBlocks(Header.VirtualSegmentBlockCount, true);
	List<RpakUnknownBlockThree> UnknownBlockThrees(Header.UnknownThirdBlockCount, true);
	List<RpakApexAssetEntry> AssetEntries(Header.AssetEntryCount, true);

	// Faster loading here by reading to the buffers directly
	ParseStream->Read((uint8_t*)&VirtualSegments[0], 0, sizeof(RpakVirtualSegment) * Header.VirtualSegmentCount);
	ParseStream->Read((uint8_t*)&VirtualSegmentBlocks[0], 0, sizeof(RpakVirtualSegmentBlock) * Header.VirtualSegmentBlockCount);
	ParseStream->Read((uint8_t*)&UnknownBlockThrees[0], 0, sizeof(RpakUnknownBlockThree) * Header.UnknownThirdBlockCount);
	ParseStream->Read((uint8_t*)&AssetEntries[0], 0, sizeof(RpakApexAssetEntry) * Header.AssetEntryCount);

	// The fifth and sixth blocks appear to only
	// be used for streaming images / starpak stuff, not always there
	ParseStream->Seek(sizeof(RpakUnknownBlockFive) * Header.UnknownFifthBlockCount, IO::SeekOrigin::Current);
	ParseStream->Seek(sizeof(RpakUnknownBlockSix) * Header.UnknownSixedBlockCount, IO::SeekOrigin::Current);

	// At this point, we need to check if we have to switch to a patch edit stream
	if (Header.PatchIndex)
	{
		File->PatchData = std::make_unique<uint8_t[]>(PatchHeader.PatchDataSize);
		File->PatchDataSize = PatchHeader.PatchDataSize;

		ParseStream->Read(File->PatchData.get(), 0, PatchHeader.PatchDataSize);
	}

	auto BufferRemaining = ParseStream->GetLength() - ParseStream->GetPosition();

	uint64_t Offset = 0;
	for (uint32_t i = PatchHeader.PatchSegmentIndex; i < Header.VirtualSegmentBlockCount; i++)
	{
		File->SegmentBlocks.EmplaceBack(Offset, VirtualSegmentBlocks[i].DataSize);
		Offset += VirtualSegmentBlocks[i].DataSize;
	}

	for (auto& Asset : AssetEntries)
	{
		File->AssetHashmap.Add(Asset.NameHash, Asset);
	}
	File->StartSegmentIndex = PatchHeader.PatchSegmentIndex;
	File->SegmentData = std::make_unique<uint8_t[]>(BufferRemaining);
	File->SegmentDataSize = BufferRemaining;
	File->EmbeddedStarpakOffset = Header.EmbeddedStarpakOffset - ParseStream->GetPosition();
	File->EmbeddedStarpakSize = Header.EmbeddedStarpakSize;

	ParseStream->Read(File->SegmentData.get(), 0, BufferRemaining);

	if (this->LoadedFileIndex == 1)
	{
		auto BasePath = IO::Path::GetDirectoryName(RpakPath);
		auto FileNameNoExt = IO::Path::GetFileNameWithoutExtension(RpakPath);

		// Trim off the () if exists
		if (FileNameNoExt.Contains("("))
			FileNameNoExt = FileNameNoExt.Substring(0, FileNameNoExt.IndexOf("("));

		auto FinalPath = IO::Path::Combine(BasePath, FileNameNoExt);

		for (uint32_t i = 0; i < Header.PatchIndex; i++)
		{
			auto PatchIndexToFile = PatchIndicesToFile[i];

			if (PatchIndexToFile == 0)
				this->LoadFileQueue.EmplaceBack(string::Format("%s.rpak", FinalPath.ToCString()));
			else
				this->LoadFileQueue.EmplaceBack(string::Format("%s(%02d).rpak", FinalPath.ToCString(), PatchIndexToFile));
		}
	}

	return true;
}

bool RpakLib::ParseTitanfallRpak(const string& RpakPath, std::unique_ptr<IO::MemoryStream>& ParseStream)
{
	auto Reader = IO::BinaryReader(ParseStream.get(), true);
	auto RpakRoot = IO::Path::GetDirectoryName(RpakPath);
	auto Header = Reader.Read<RpakTitanfallHeader>();
	auto File = &this->LoadedFiles[this->LoadedFileIndex++];

	// Default version is apex, 0x8, must make sure this is set.
	File->Version = RpakGameVersion::Titanfall;

	RpakPatchHeader PatchHeader{};
	RpakPatchCompressPair PatchCompressPairs[16]{};
	uint16_t PatchIndicesToFile[16]{};

	if (Header.PatchIndex)
	{
		ParseStream->Read((uint8_t*)&PatchHeader, 0, sizeof(RpakPatchHeader));
		ParseStream->Read((uint8_t*)&PatchCompressPairs, 0, sizeof(RpakPatchCompressPair) * Header.PatchIndex);
		ParseStream->Read((uint8_t*)&PatchIndicesToFile, 0, sizeof(uint16_t) * Header.PatchIndex);
	}

	uint32_t StarpakLen = Header.StarpakReferenceSize;
	while (StarpakLen > 0)
	{
		auto Starpak = Reader.ReadCString();

		if (Starpak.Length() > 0)
		{
			auto Path = IO::Path::Combine(RpakRoot, IO::Path::GetFileName(Starpak));
			this->MountStarpak(Path, this->LoadedFileIndex - 1, File->StarpakReferences.Count(), false);
			File->StarpakReferences.EmplaceBack(Path);
		}

		StarpakLen -= Starpak.Length() + sizeof(char);
	}

	// We need to load the rest of the data before applying a patch stream
	List<RpakVirtualSegment> VirtualSegments;
	List<RpakVirtualSegmentBlock> VirtualSegmentBlocks;
	List<RpakUnknownBlockThree> UnknownBlockThrees;
	List<RpakTitanfallAssetEntry> AssetEntries;

	for (uint32_t i = 0; i < Header.VirtualSegmentCount; i++)
	{
		VirtualSegments.EmplaceBack(Reader.Read<RpakVirtualSegment>());
	}
	for (uint32_t i = 0; i < Header.VirtualSegmentBlockCount; i++)
	{
		VirtualSegmentBlocks.EmplaceBack(Reader.Read<RpakVirtualSegmentBlock>());
	}
	for (uint32_t i = 0; i < Header.UnknownThirdBlockCount; i++)
	{
		UnknownBlockThrees.EmplaceBack(Reader.Read<RpakUnknownBlockThree>());
	}
	for (uint32_t i = 0; i < Header.AssetEntryCount; i++)
	{
		AssetEntries.EmplaceBack(Reader.Read<RpakTitanfallAssetEntry>());
	}

	// The fifth and sixth blocks appear to only
	// be used for streaming images / starpak stuff, not always there
	ParseStream->Seek(sizeof(RpakUnknownBlockFive) * Header.UnknownFifthBlockCount, IO::SeekOrigin::Current);
	ParseStream->Seek(sizeof(RpakUnknownBlockSix) * Header.UnknownSixedBlockCount, IO::SeekOrigin::Current);

	// 7th and 8th blocks are weird and useless
	ParseStream->Seek(sizeof(uint32_t) * Header.UnknownSeventhBlockCount, IO::SeekOrigin::Current);
	ParseStream->Seek(Header.UnknownEighthBlockCount, IO::SeekOrigin::Current);

	// At this point, we need to check if we have to switch to a patch edit stream
	if (Header.PatchIndex)
	{
		File->PatchData = std::make_unique<uint8_t[]>(PatchHeader.PatchDataSize);
		File->PatchDataSize = PatchHeader.PatchDataSize;

		ParseStream->Read(File->PatchData.get(), 0, PatchHeader.PatchDataSize);
	}

	auto BufferRemaining = ParseStream->GetLength() - ParseStream->GetPosition();

	uint64_t Offset = 0;
	for (uint32_t i = PatchHeader.PatchSegmentIndex; i < Header.VirtualSegmentBlockCount; i++)
	{
		File->SegmentBlocks.EmplaceBack(Offset, VirtualSegmentBlocks[i].DataSize);
		Offset += VirtualSegmentBlocks[i].DataSize;
	}

	for (auto& Asset : AssetEntries)
	{
		RpakApexAssetEntry NewAsset{};
		NewAsset.OptimalStarpakOffset = (uint64_t)(-1);

		std::memcpy(&NewAsset, &Asset, 40);
		std::memcpy(((uint8_t*)&NewAsset) + 48, ((uint8_t*)&Asset) + 40, 32);

		File->AssetHashmap.Add(Asset.NameHash, NewAsset);
	}
	File->StartSegmentIndex = PatchHeader.PatchSegmentIndex;
	File->SegmentData = std::make_unique<uint8_t[]>(BufferRemaining);
	File->SegmentDataSize = BufferRemaining;

	ParseStream->Read(File->SegmentData.get(), 0, BufferRemaining);

	if (this->LoadedFileIndex == 1)
	{
		auto BasePath = IO::Path::GetDirectoryName(RpakPath);
		auto FileNameNoExt = IO::Path::GetFileNameWithoutExtension(RpakPath);

		// Trim off the () if exists
		if (FileNameNoExt.Contains("("))
			FileNameNoExt = FileNameNoExt.Substring(0, FileNameNoExt.IndexOf("("));

		auto FinalPath = IO::Path::Combine(BasePath, FileNameNoExt);

		for (uint32_t i = 0; i < Header.PatchIndex; i++)
		{
			auto PatchIndexToFile = PatchIndicesToFile[i];
			if (PatchIndexToFile == 0)
				this->LoadFileQueue.EmplaceBack(string::Format("%s.rpak", FinalPath.ToCString()));
			else
				this->LoadFileQueue.EmplaceBack(string::Format("%s(%02d).rpak", FinalPath.ToCString(), PatchIndexToFile));
		}
	}

	return true;
}

void RpakLib::MountStarpak(const string& Path, uint32_t FileIndex, uint32_t StarpakIndex, bool Optimal)
{
	auto& File = this->LoadedFiles[FileIndex];
	auto Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	auto StarpakStream = Reader.GetBaseStream();

	StarpakStream->SetPosition(StarpakStream->GetLength() - sizeof(uint64_t));

	auto EntryCount = Reader.Read<uint64_t>();

	StarpakStream->SetPosition(StarpakStream->GetLength() - sizeof(uint64_t) - (sizeof(StarpakStreamEntry) * EntryCount));

	for (uint32_t i = 0; i < EntryCount; i++)
	{
		auto Entry = Reader.Read<StarpakStreamEntry>();

		if (Optimal)
		{
			File.OptimalStarpakMap.Add(Entry.Offset + StarpakIndex, Entry.Size);
		}
		else
		{
			File.StarpakMap.Add(Entry.Offset + StarpakIndex, Entry.Size);
		}
	}
}

bool RpakLib::MountApexRpak(const string& Path, bool Dump)
{
	auto Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	auto Header = Reader.Read<RpakApexHeader>();

	if (Header.CompressedSize == Header.DecompressedSize)
	{
		auto Stream = std::make_unique<IO::MemoryStream>();

		Reader.GetBaseStream()->SetPosition(0);
		Reader.GetBaseStream()->CopyTo(Stream.get());
		Stream->SetPosition(0);

		return ParseApexRpak(Path, Stream);
	}

	auto CompressedBuffer = std::make_unique<uint8_t[]>(Header.CompressedSize);

	Reader.Read(CompressedBuffer.get() + sizeof(RpakApexHeader), 0, Header.CompressedSize - sizeof(RpakApexHeader));

	//std::int64_t params[18];
	rpak_decomp_state state;

	uint32_t dSize = g_pRtech->DecompressPakfileInit(&state, CompressedBuffer.get(), Header.CompressedSize, 0, sizeof(RpakApexHeader));

	std::vector<std::uint8_t> pakbuf(dSize, 0);

	//params[1] = std::int64_t(pakbuf.data());
	//params[3] = -1i64;
	state.out_mask = UINT64_MAX;
	state.out = uint64_t(pakbuf.data());

	std::uint8_t decomp_result = g_pRtech->DecompressPakFile((int64_t*)&state, dSize, pakbuf.size());

	std::memcpy(pakbuf.data(), &Header, sizeof(RpakApexHeader));

	auto ResultStream = std::make_unique<IO::MemoryStream>(pakbuf.data(), 0, Header.DecompressedSize, true, false, true);

#if _DEBUG
	if (Dump)
	{
		auto OutStream = IO::File::Create(IO::Path::Combine("D:\\", IO::Path::GetFileName(Path)));
		ResultStream->CopyTo(OutStream.get());
		ResultStream->SetPosition(0);
	}
#endif
	bool r = ParseApexRpak(Path, ResultStream);

	printf("Parse: %i\n", r);

	return r;
}

bool RpakLib::MountTitanfallRpak(const string& Path, bool Dump)
{
	auto Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	auto Header = Reader.Read<RpakTitanfallHeader>();

	if (Header.CompressedSize == Header.DecompressedSize)
	{
		auto Stream = std::make_unique<IO::MemoryStream>();

		Reader.GetBaseStream()->SetPosition(0);
		Reader.GetBaseStream()->CopyTo(Stream.get());

		return ParseTitanfallRpak(Path, Stream);
	}

	auto CompressedBuffer = std::make_unique<uint8_t[]>(Header.CompressedSize);

	Reader.Read(CompressedBuffer.get() + sizeof(RpakTitanfallHeader), 0, Header.CompressedSize - sizeof(RpakTitanfallHeader));

	//std::int64_t params[18];
	rpak_decomp_state state;

	uint32_t dSize = g_pRtech->DecompressPakfileInit(&state, CompressedBuffer.get(), Header.CompressedSize, 0, sizeof(RpakTitanfallHeader));

	std::vector<std::uint8_t> pakbuf(dSize, 0);

	//params[1] = std::int64_t(pakbuf.data());
	//params[3] = -1i64;
	state.out_mask = UINT64_MAX;
	state.out = uint64_t(pakbuf.data());

	std::uint8_t decomp_result = g_pRtech->DecompressPakFile((int64_t*)&state, dSize, pakbuf.size());

	std::memcpy(pakbuf.data(), &Header, sizeof(RpakTitanfallHeader));

	auto ResultStream = std::make_unique<IO::MemoryStream>(pakbuf.data(), 0, Header.DecompressedSize, true, false, true);

#if _DEBUG
	if (Dump)
	{
		auto OutStream = IO::File::Create(IO::Path::Combine("D:\\", IO::Path::GetFileName(Path)));
		ResultStream->CopyTo(OutStream.get());
		ResultStream->SetPosition(0);
	}
#endif
	return ParseTitanfallRpak(Path, ResultStream);
}

RpakLoadAsset::RpakLoadAsset(uint64_t NameHash, uint32_t FileIndex, uint32_t AssetType, uint32_t SubHeaderIndex, uint32_t SubHeaderOffset, uint32_t SubHeaderSize, uint32_t RawDataIndex, uint32_t RawDataOffset, uint64_t StarpakOffset, uint64_t OptimalStarpakOffset, RpakGameVersion Version)
	: NameHash(NameHash), FileIndex(FileIndex), RpakFileIndex(FileIndex), AssetType(AssetType), SubHeaderIndex(SubHeaderIndex), SubHeaderOffset(SubHeaderOffset), SubHeaderSize(SubHeaderSize), RawDataIndex(RawDataIndex), RawDataOffset(RawDataOffset), StarpakOffset(StarpakOffset), OptimalStarpakOffset(OptimalStarpakOffset), Version(Version)
{
}

RpakSegmentBlock::RpakSegmentBlock(uint64_t Offset, uint64_t Size)
	: Offset(Offset), Size(Size)
{
}
