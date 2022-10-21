#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include <rtech.h>

void RpakLib::BuildMapInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	string Name = string::Format("map_%llx", Asset.NameHash);

	if (ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = Name;
	else
		Info.Name = IO::Path::GetFileNameWithoutExtension(Name).ToLower();

	Info.Type = ApexAssetType::Map;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = "N/A";
}