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
	ShaderSet,
	UIImage,
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
	uint32_t Version;
	string Info;
	string DebugInfo = ""; // advanced info i suppose

	ApexAsset();
};