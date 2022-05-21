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
	default:
		return nullptr;
	}
}