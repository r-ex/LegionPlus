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

	EffectHeader EffectHdr = Reader.Read<EffectHeader>();

	string Name;
	string PCFPath;
	try {
		if (EffectHdr.EffectData.Index || EffectHdr.EffectData.Offset)
		{
			RpakStream->SetPosition(this->GetFileOffset(Asset, EffectHdr.EffectData.Index, EffectHdr.EffectData.Offset));
			EffectData Data = Reader.Read<EffectData>();

			RpakStream->SetPosition(this->GetFileOffset(Asset, Data.PCF.Index, Data.PCF.Offset));
			PCFPath = Reader.ReadCString();

			RpakStream->SetPosition(this->GetFileOffset(Asset, Data.EffectName.Index, Data.EffectName.Offset));
			RPakPtr Ptr = Reader.Read<RPakPtr>();
			RpakStream->SetPosition(this->GetFileOffset(Asset, Ptr.Index, Ptr.Offset));
			Name = Reader.ReadCString();

			RpakStream->SetPosition(this->GetFileOffset(Asset, Data.ParticleSystemOperator.Index, Data.ParticleSystemOperator.Offset));
			Ptr = Reader.Read<RPakPtr>();
			RpakStream->SetPosition(this->GetFileOffset(Asset, Ptr.Index, Ptr.Offset));
			string PSOName = Reader.ReadCString();
		}
	}
	catch (...)
	{

	}


	if (ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = Name;
	else
		Info.Name = IO::Path::GetFileNameWithoutExtension(Name).ToLower();

	Info.Type = ApexAssetType::Effect;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = PCFPath;
}