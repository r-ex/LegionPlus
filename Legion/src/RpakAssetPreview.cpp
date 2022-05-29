#include "pch.h"
#include "RpakLib.h"

std::unique_ptr<Assets::Texture> RpakLib::BuildPreviewTexture(uint64_t Hash)
{
	if (!this->Assets.ContainsKey(Hash))
		return nullptr;

	auto& Asset = this->Assets[Hash];

	std::unique_ptr<Assets::Texture> Result = nullptr;
	string Name;

	switch (Asset.AssetType)
	{
	case (uint32_t)AssetType_t::Texture:
		this->ExtractTexture(Asset, Result, Name);
		return Result;
	case (uint32_t)AssetType_t::UIIA:
		this->ExtractUIIA(Asset, Result);
		return Result;
	case (uint32_t)AssetType_t::UIImageAtlas:
	{
		auto RpakStream = this->GetFileStream(Asset);
		IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

		UIAtlasHeader AtlasHdr = Reader.Read<UIAtlasHeader>();

		if (!this->Assets.ContainsKey(AtlasHdr.TextureGuid))
			return nullptr;

		this->ExtractTexture(this->Assets[AtlasHdr.TextureGuid], Result, Name);
		return Result;
	}
	default:
		return nullptr;
	}
}