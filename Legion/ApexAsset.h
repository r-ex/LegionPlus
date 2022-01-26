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
	Subtitles,
};

enum class ApexAssetStatus
{
	Loaded,
	Exporting,
	Exported,
	Error,
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