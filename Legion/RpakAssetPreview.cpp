#include "RpakLib.h"

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
	string Name;

	switch (Asset.AssetType)
	{
	case (uint32_t)RpakAssetType::Texture:
		this->ExtractTexture(Asset, Result, Name);
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
	auto Material = this->ExtractMaterial(Asset, "", false, false);

	return this->BuildPreviewTexture(Material.AlbedoHash);
}