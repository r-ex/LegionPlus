#include "pch.h"
#include "ApexAsset.h"

ApexAsset::ApexAsset()
{
	Name = "<Error>";
	Status = ApexAssetStatus::Loaded;
	Type = ApexAssetType::Model;
	Info = "N/A";
}
