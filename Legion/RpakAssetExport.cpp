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
		this->ModelExporter->ExportModel(*Model.get(), IO::Path::Combine(IO::Path::Combine(Path, Model->Name), Model->Name + "_LOD0" + (const char*)ModelExporter->ModelExtension()));
	}
}

void RpakLib::ExportMaterial(const RpakLoadAsset& Asset, const string& Path)
{
	auto Material = this->ExtractMaterial(Asset, Path, false, false);
	auto OutPath = IO::Path::Combine(Path, Material.MaterialName);

	IO::Directory::CreateDirectory(OutPath);

	(void)this->ExtractMaterial(Asset, OutPath, true, true);
}

void RpakLib::ExportTexture(const RpakLoadAsset& Asset, const string& Path, bool IncludeImageNames)
{
	auto DestinationPath = IO::Path::Combine(Path, string::Format("0x%llx%s", Asset.NameHash, (const char*)ImageExtension));

	std::unique_ptr<Assets::Texture> Texture = nullptr;
	string Name;

	this->ExtractTexture(Asset, Texture, Name);

	if (IncludeImageNames && Name.Length() > 0)
		DestinationPath = IO::Path::Combine(Path, string::Format("%s%s", IO::Path::GetFileNameWithoutExtension(Name).ToCString(), (const char*)ImageExtension));

#ifndef _DEBUG
	if (IO::File::Exists(DestinationPath))	// Ignore existing assets...
		return;
#endif

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