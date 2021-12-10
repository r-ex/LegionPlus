#pragma once

#include <cstdint>
//#include "MemoryModule.h"
#include "StringBase.h"
#include "DictionaryBase.h"
#include "ListBase.h"
#include "ApexAsset.h"

struct MilesAudioAsset
{
	string Name;
	uint32_t SampleRate;
	uint32_t ChannelCount;

	uint64_t PreloadOffset;
	uint32_t PreloadSize;
	uint64_t StreamOffset;
	uint32_t StreamSize;

	uint32_t PatchIndex;
	uint32_t LocalizeIndex;

	MilesAudioAsset() = default;
};

struct MilesStreamBank
{
	string Path;
	uint32_t StreamDataOffset;

	MilesStreamBank() = default;
};

class MilesLib
{
public:
	void Initialize();

	MilesLib();
	~MilesLib();

	// Mounts a Miles Mbnk file
	void MountBank(const string& Path);
	// Extracts a Miles audio file
	void ExtractAsset(const MilesAudioAsset& Asset, const string& FilePath);

	// Builds the viewer list of assets
	std::unique_ptr<List<ApexAsset>> BuildAssetList();

	// A list of loaded assets
	Dictionary<uint64_t, MilesAudioAsset> Assets;

private:

	// A list of streaming audio bank files
	Dictionary<uint32_t, MilesStreamBank> StreamBanks;
};