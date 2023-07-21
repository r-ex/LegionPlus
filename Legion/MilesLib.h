#pragma once

//#include "MemoryModule.h"
#include "StringBase.h"
#include "DictionaryBase.h"
#include "ListBase.h"
#include "ApexAsset.h"

struct FORMATCHUNK
{
	int chunkID = 0x20746d66; // fmt
	long chunkSize = 0x10;
	short formatTag = 3;
	unsigned short channels;
	unsigned long sampleRate;
	unsigned long avgBytesPerSecond;
	unsigned short blockAlign;
	unsigned short bitsPerSample;
};

struct DATACHUNK
{
	int chunkID = 0x61746164; // data
	long chunkSize;
};

struct WAVEHEADER
{
	int groupID = 0x46464952; // RIFF
	long size;
	int riffType = 0x45564157; // WAVE

	FORMATCHUNK fmt;
	DATACHUNK data;
};

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
	int32_t LocalizeIndex;

	MilesAudioAsset() = default;
};

struct MilesStreamBank
{
	string Path;
	uint32_t StreamDataOffset;

	MilesStreamBank() = default;
};

enum class MilesLanguageID : int16_t
{
	INVAILD = -2,
	None = -1,
	English = 0,
	French = 1,
	German = 2,
	Spanish = 3,
	Italian = 4,
	Japanese = 5,
	Polish = 6,
	Russian = 7,
	Mandarin = 8,
	Korean = 9,

	// for r2 export, these don't exist in apex (why respawn???)
	FAKEPORTUGUESE,
	FAKELATINSPAN,

	UNKNOWN,
	COUNT,

	
};

enum class MilesLanguageIDTitanfall : short
{
	INVAILD = -2,
	None = -1,
	English = 0,
	French = 1,
	German = 2,
	Spanish = 3,
	Italian = 4,
	Japanese = 5,
	Polish = 6,
	Portuguese = 7,
	Russian = 8,
	TChinese = 9, // traditional chinese (mandarin)
	MSpanish = 10, // latin/mexican spanish

	_MILESLANGCOUNT
};

MilesLanguageID ApexLangFromTF(MilesLanguageIDTitanfall langIn);
MilesLanguageIDTitanfall TFLangFromApex(MilesLanguageID langIn);


const String&
LanguageName(MilesLanguageID lang);

class MilesLib
{
public:
	void Initialize();

	MilesLib();
	~MilesLib();

	// Mounts a Miles Mbnk file
	void MountBank(const string& Path);
	// Extracts a Miles audio file
	bool ExtractAsset(const MilesAudioAsset& Asset, const string& FilePath);

	// Builds the viewer list of assets
	std::unique_ptr<List<ApexAsset>> BuildAssetList();

	// A list of loaded assets
	Dictionary<uint64_t, MilesAudioAsset> Assets;

private:

	// A list of streaming audio bank files
	Dictionary<uint32_t, MilesStreamBank> StreamBanks;

	uint32_t MbnkVersion;
};