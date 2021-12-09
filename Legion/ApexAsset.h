#pragma once

#include "StringBase.h"

enum class ApexAssetType
{
	Model,
	AnimationSet,
	Image,
	Material,
	DataTable,
	Sound,
};

enum class ApexAssetStatus
{
	Loaded,
	Exporting,
	Exported,
};

struct ApexAsset
{
	uint64_t Hash;

	string Name;
	ApexAssetType Type;
	ApexAssetStatus Status;
	string Info;

	ApexAsset();
};