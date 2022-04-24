#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include "rtech.h"
#include <File.h>

void RpakLib::ExportModel(const RpakLoadAsset& Asset, const string& Path, const string& AnimPath)
{
	auto Model = this->ExtractModel(Asset, Path, AnimPath, true, true);

	if (Model && this->ModelExporter)
	{
		string DestinationPath = IO::Path::Combine(IO::Path::Combine(Path, Model->Name), Model->Name + "_LOD0" + (const char*)ModelExporter->ModelExtension());

		if (Utils::ShouldWriteFile(DestinationPath))
			this->ModelExporter->ExportModel(*Model.get(), DestinationPath);
	}
}

void RpakLib::ExportMaterial(const RpakLoadAsset& Asset, const string& Path)
{
	auto Material = this->ExtractMaterial(Asset, Path, false, false);
	auto OutPath = IO::Path::Combine(Path, Material.MaterialName);

	if (!Utils::ShouldWriteFile(OutPath))
		return;

	IO::Directory::CreateDirectory(OutPath);

	(void)this->ExtractMaterial(Asset, OutPath, true, true);
}

void RpakLib::ExportTexture(const RpakLoadAsset& Asset, const string& Path, bool IncludeImageNames, string NameOverride, bool NormalRecalculate)
{
	string DestinationName = NameOverride == "" ? string::Format("0x%llx%s", Asset.NameHash, (const char*)ImageExtension) : NameOverride;
	auto DestinationPath = IO::Path::Combine(Path, DestinationName);

	std::unique_ptr<Assets::Texture> Texture = nullptr;
	string Name;

	this->ExtractTexture(Asset, Texture, Name);

	if (IncludeImageNames && Name.Length() > 0)
		DestinationPath = IO::Path::Combine(Path, string::Format("%s%s", IO::Path::GetFileNameWithoutExtension(Name).ToCString(), (const char*)ImageExtension));

	if (!Utils::ShouldWriteFile(DestinationPath))
		return;
	
	try
	{
		if (Texture != nullptr)
		{
			if (NormalRecalculate)
			{
				auto NormalRecalcType = (eNormalRecalcType)ExportManager::Config.Get<System::SettingType::Integer>("NormalRecalcType");

				Assets::TranscodeType Type = Assets::TranscodeType::NormalMapBC5OpenGl;

				switch (NormalRecalcType)
				{
				case eNormalRecalcType::None:
					break;
				case eNormalRecalcType::DirectX:
					Texture->Transcode(Assets::TranscodeType::NormalMapBC5);
					break;
				case eNormalRecalcType::OpenGl:
					Texture->Transcode(Assets::TranscodeType::NormalMapBC5OpenGl);
					break;
				}
			}
			Texture->Save(DestinationPath, ImageSaveType);
		}
	}
	catch (...)
	{
		// Nothing, the thread attempted to export an image that already exists...
	}
}

void RpakLib::ExportUIIA(const RpakLoadAsset& Asset, const string& Path)
{
	auto DestinationPath = IO::Path::Combine(Path, string::Format("0x%llx%s", Asset.NameHash, (const char*)ImageExtension));

	if (!Utils::ShouldWriteFile(DestinationPath))	// Ignore existing assets...
		return;

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

	auto FullAnimSetName = Reader.ReadCString();
	auto AnimSetName = IO::Path::GetFileNameWithoutExtension(FullAnimSetName);
	auto AnimSetPath = IO::Path::Combine(Path, AnimSetName);

	IO::Directory::CreateDirectory(AnimSetPath);

	auto AnimFormat = (RpakAnimExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("AnimFormat");

	if (AnimFormat == RpakAnimExportFormat::RAnim)
	{
		auto SkeletonOffset = this->GetFileOffset(Asset, RigHeader.SkeletonIndex, RigHeader.SkeletonOffset);
		RpakStream->SetPosition(SkeletonOffset);

		auto SkeletonHeader = Reader.Read<RMdlSkeletonHeader>();

		RpakStream->SetPosition(SkeletonOffset);

		char* skelBuf = new char[SkeletonHeader.DataSize];
		Reader.Read(skelBuf, 0, SkeletonHeader.DataSize);

		std::ofstream skelOut(IO::Path::Combine(AnimSetPath, AnimSetName + ".rrig"), std::ios::out | std::ios::binary);
		skelOut.write(skelBuf, SkeletonHeader.DataSize);
		skelOut.close();

		// todo: rseq
		return;
	}

	const auto Skeleton = this->ExtractSkeleton(Reader, this->GetFileOffset(Asset, RigHeader.SkeletonIndex, RigHeader.SkeletonOffset));

	const uint64_t ReferenceOffset = this->GetFileOffset(Asset, RigHeader.AnimationReferenceIndex, RigHeader.AnimationReferenceOffset);

	for (uint32_t i = 0; i < RigHeader.AnimationReferenceCount; i++)
	{
		RpakStream->SetPosition(ReferenceOffset + ((uint64_t)i * 0x8));

		auto AnimHash = Reader.Read<uint64_t>();

		// excluded by DFS
		if (AnimHash == 0xdf5)
		{
			g_Logger.Warning("unable to export anim idx %i for '%s' because it is self-excluded\n", i, FullAnimSetName.ToCString());
			continue;
		}

		if (!Assets.ContainsKey(AnimHash))
		{
			g_Logger.Warning("missing anim 0x%llx for '%s'\n", AnimHash, FullAnimSetName.ToCString());
			continue;
		}

		// We need to make sure the skeleton is kept alive (copied) here...
		this->ExtractAnimation(Assets[AnimHash], Skeleton, AnimSetPath);
	}
}

void RpakLib::ExportDataTable(const RpakLoadAsset& Asset, const string& Path)
{


	auto Format = (RpakSubtitlesExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("TextFormat");

	string sExtension = "";

	switch (Format)
	{
	case RpakSubtitlesExportFormat::CSV:
		sExtension = ".csv";
		break;
	case RpakSubtitlesExportFormat::TXT:
		sExtension = ".txt";
		break;
	}
	
	auto DestinationPath = IO::Path::Combine(Path, string::Format("0x%llx", Asset.NameHash) + sExtension);

	if (!Utils::ShouldWriteFile(DestinationPath))
		return;

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
	auto Format = (RpakSubtitlesExportFormat)ExportManager::Config.Get<System::SettingType::Integer>("TextFormat");

	string sExtension = "";

	switch (Format)
	{
	case RpakSubtitlesExportFormat::CSV:
		sExtension = ".csv";
		break;
	case RpakSubtitlesExportFormat::TXT:
		sExtension = ".txt";
		break;
	}

	auto DestinationPath = IO::Path::Combine(Path, GetSubtitlesNameFromHash(Asset.NameHash) + sExtension);

	if (!Utils::ShouldWriteFile(DestinationPath))
		return;

	auto Subtitles = this->ExtractSubtitles(Asset);

	std::ofstream subt_out(DestinationPath.ToCString(), std::ios::out);

	switch (Format)
	{
	case RpakSubtitlesExportFormat::CSV:
	{
		subt_out << "color,text\n";
		for (auto& Entry : Subtitles)
		{
			subt_out << "\"#" << Vector3ToHexColor(Entry.Color) << "\",\"" << Entry.SubtitleText << "\"\n";
		}
		break;
	}
	case RpakSubtitlesExportFormat::TXT:
	{
		for (auto& Entry : Subtitles)
		{
			subt_out << Entry.SubtitleText << "\n";
		}
		break;
	}
	default:
		g_Logger.Warning("Attempted to export Subtitles asset with an invalid format (%i)\n", Format);
		break;
	}

	subt_out.close();
}

void RpakLib::ExportShaderSet(const RpakLoadAsset& Asset, const string& Path)
{
	auto ShaderSetPath = IO::Path::Combine(Path, string::Format("0x%llx", Asset.NameHash));

	auto RpakStream = this->GetFileStream(Asset);
	auto Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	auto Header = Reader.Read<ShaderSetHeader>();

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
		auto PixelShaderPath = IO::Path::Combine(ShaderSetPath, string::Format("0x%llx_ps.fxc", PixelShaderGuid));
		this->ExtractShader(Assets[PixelShaderGuid], PixelShaderPath);
	}

	if (Assets.ContainsKey(VertexShaderGuid))
	{
		auto VertexShaderPath = IO::Path::Combine(ShaderSetPath, string::Format("0x%llx_vs.fxc", VertexShaderGuid));
		this->ExtractShader(Assets[VertexShaderGuid], VertexShaderPath);
	}
}

void RpakLib::ExportUIImageAtlas(const RpakLoadAsset& Asset, const string& Path)
{
	auto AtlasPath = IO::Path::Combine(Path, string::Format("0x%llx", Asset.NameHash));

	if (!IO::Directory::Exists(AtlasPath))
		IO::Directory::CreateDirectory(AtlasPath);

	this->ExtractUIImageAtlas(Asset, AtlasPath);
}
