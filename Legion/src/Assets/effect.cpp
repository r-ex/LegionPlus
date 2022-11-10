#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include <rtech.h>

// N1094_CL456479 (Season 3 Launch) won't work with this because the base rpak (effects.rpak) is patched itself.
void RpakLib::BuildEffectInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	string name;
	string path;

	if (Asset.AssetVersion >= 5)
	{
		EffectHeaderV10 effectHdr = Reader.Read<EffectHeaderV10>();

		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

		EffectDataV10 effectData = Reader.Read<EffectDataV10>();

		RpakStream->SetPosition(this->GetFileOffset(Asset, effectData.effectName));
		name = Reader.ReadCString();

		RpakStream->SetPosition(this->GetFileOffset(Asset, effectData.effectPath));
		path = Reader.ReadCString();
	}
	else if (Asset.AssetVersion == 4)
	{
		EffectDataV10 effectData = Reader.Read<EffectDataV10>();

		// .. Don't ask, that is one way to get the name in V4..
		RpakStream->SetPosition(this->GetFileOffset(Asset, effectData.effectName));
		RPakPtr ptr = Reader.Read<RPakPtr>();
		RpakStream->SetPosition(this->GetFileOffset(Asset, ptr));
		ptr = Reader.Read<RPakPtr>();
		RpakStream->SetPosition(this->GetFileOffset(Asset, ptr));
		name = Reader.ReadCString();

		// Last ptr in chain decides to become sentinent sometimes and turn into different data.
		//RpakStream->SetPosition(this->GetFileOffset(Asset, effectData.effectPath));
		//ptr = Reader.Read<RPakPtr>();
		//RpakStream->SetPosition(this->GetFileOffset(Asset, ptr));
		//ptr = Reader.Read<RPakPtr>();
		//RpakStream->SetPosition(this->GetFileOffset(Asset, ptr));
		//path = Reader.ReadCString();
	}
	else
	{
		EffectHeaderV3 effectHdr = Reader.Read<EffectHeaderV3>();

		if (effectHdr.effectData.Index || effectHdr.effectData.Offset)
		{
			RpakStream->SetPosition(this->GetFileOffset(Asset, effectHdr.effectData.Index, effectHdr.effectData.Offset));
			EffectDataV3 effectData = Reader.Read<EffectDataV3>();

			RpakStream->SetPosition(this->GetFileOffset(Asset, effectData.pcf));
			path = Reader.ReadCString();

			RpakStream->SetPosition(this->GetFileOffset(Asset, effectData.effectName));
			RPakPtr ptr = Reader.Read<RPakPtr>();
			RpakStream->SetPosition(this->GetFileOffset(Asset, ptr));
			name = Reader.ReadCString();

			RpakStream->SetPosition(this->GetFileOffset(Asset, effectData.particleSystemOperator));
			ptr = Reader.Read<RPakPtr>();
			RpakStream->SetPosition(this->GetFileOffset(Asset, ptr));
			string PSOName = Reader.ReadCString();
		}
	}

	if (ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = name;
	else
		Info.Name = IO::Path::GetFileNameWithoutExtension(name).ToLower();

	Info.Type = ApexAssetType::Effect;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = path;
}