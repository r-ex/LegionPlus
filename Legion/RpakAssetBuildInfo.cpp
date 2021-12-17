#include "RpakLib.h"
#include "Path.h"

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

	string Name = "";

	if (TexHeader.NameOffset)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, TexHeader.NameIndex, TexHeader.NameOffset));

		Name = Reader.ReadCString();
	}

	if (Name.Length() > 0)
		Info.Name = IO::Path::GetFileNameWithoutExtension(Name);
	else
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