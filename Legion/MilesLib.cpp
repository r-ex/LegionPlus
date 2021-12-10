#include "MilesLib.h"
#include "Kore.h"
#include "XXHash.h"
//#include "BinkAudioEngine.h"

#pragma pack(push, 1)
struct MilesAudioBank
{
	uint32_t Magic;
	uint32_t Version;
	uint32_t FileSize;
	uint32_t MagicBank;

	uint32_t UnknownSize;
	uint32_t Checksum;

	uint64_t NameTableOffset;

	uint64_t Zero1;
	uint64_t Zero2;
	uint64_t Zero3;

	uint64_t UnknownOffset1;
	uint64_t UnknownOffset2;

	uint64_t SourceTableOffset;
	uint64_t SourceEntryOffset;

	uint64_t UnknownOffset3;
	uint64_t UnknownOffset4;

	uint64_t EventTableOffset;
	uint64_t EventEntryOffset;

	uint64_t NameTableOffset2;
	uint64_t UnknownOffset5;

	uint64_t UnknownOffset6;
	uint64_t UnknownOffset7;

	uint64_t UnknownOffset8;

	uint32_t UnknownCount1;
	uint32_t UnknownCount2;

	uint32_t SourcesCount;
	uint32_t UnknownCount3;

	uint32_t EventCount;
	uint32_t UnknownCount4;

	uint32_t UnknownCount5;
	uint32_t UnknownCount6;

	uint32_t UnknownCount7;
	uint32_t UnknownCount8;
	uint32_t UnknownCount9;
	uint32_t UnknownCount10;
};

enum class MilesSourceLocal : int16_t
{
	None = -1,
	English = 0,
	Spanish = 1
};

struct MilesApexSourceEntry
{
	uint32_t NameOffset;

	MilesSourceLocal EntryLocal;
	uint16_t PatchIndex;

	uint32_t NameOffse2;

	uint16_t SampleRate;
	uint16_t BitRate;

	uint16_t UnknownZero3;
	uint8_t ChannelCount;
	uint8_t UnknownCount;

	float Unknown15;
	uint32_t Flags2;

	uint32_t Unknown16;

	uint32_t StreamHeaderSize;
	uint32_t StreamDataSize;
	uint64_t StreamHeaderOffset;
	uint64_t StreamDataOffset;

	uint64_t NegativeOne;

	uint32_t UnknownZero2;
	uint32_t StreamDataSize2;
};

struct MilesTitanfallSourceEntry
{
	uint64_t UnknownZero;

	uint32_t Unknown1;

	uint16_t Unknown222;
	uint16_t UnknownZero2;

	uint32_t NameOffset;

	uint16_t SampleRate;
	uint16_t BitRate;

	uint8_t ChannelCount;
	uint8_t UnknownCount;
	uint16_t UnknownZero3;

	uint32_t Unknown14;

	float Unknown15;
	uint32_t Flags2;

	uint32_t Unknown16;

	uint16_t Flags;
	uint16_t Unknown17;

	uint32_t StreamHeaderSize;
	uint32_t StreamDataSize;
	uint64_t StreamHeaderOffset;
	uint64_t StreamDataOffset;

	uint64_t NegativeOne;

	MilesSourceLocal EntryLocal;
	uint16_t PatchIndex;
	

	uint32_t StreamDataSize2;
};

struct MilesStreamBankHeader
{
	uint32_t Magic;
	uint16_t Version;

	uint16_t LocalizeIndex;
	uint32_t StreamDataOffset;
	uint16_t PatchIndex;
	uint16_t Zero;
};

struct BinkASIStream
{
	uint32_t Magic;
	uint16_t Version;

	uint16_t SampleRate;

	uint32_t FrameCount;
	uint32_t ReadChunkSize;
	uint32_t ReadDataSize;
};
#pragma pack(pop)

struct BinkASIReader
{
	IO::BinaryReader* Stream;
	uint64_t DataRead;
	uint64_t HeaderSize;
	uint64_t DataStreamOffset;
	uint64_t DataStreamSize;
};

static uint32_t MilesReadFileStream(char* Buffer, uint64_t Length, void* UserData)
{
	auto Reader = (BinkASIReader*)UserData;
	uint64_t TotalRead = 0;

	if (Reader->DataRead < Reader->HeaderSize)
	{
		auto Diff = Reader->HeaderSize - Reader->DataRead;
		auto MinDiff = min(Length, Diff);
		Reader->Stream->Read(Buffer, 0, MinDiff);
		Reader->DataRead += MinDiff;
		TotalRead += MinDiff;

		if (Reader->DataRead >= Reader->HeaderSize)
			Reader->Stream->GetBaseStream()->SetPosition(Reader->DataStreamOffset);
	}

	uint64_t LengthToRead = Length - TotalRead;
	LengthToRead = min(Reader->DataStreamSize, LengthToRead);

	Reader->Stream->Read(Buffer, TotalRead, LengthToRead);
	TotalRead += LengthToRead;
	Reader->DataStreamSize -= LengthToRead;

	return (uint32_t)TotalRead;
}

void MilesLib::Initialize()
{
}

MilesLib::MilesLib()
{
}

MilesLib::~MilesLib()
{
}

void MilesLib::MountBank(const string& Path)
{
	auto BasePath = IO::Path::GetDirectoryName(Path);

	auto Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	auto ReaderStream = Reader.GetBaseStream();

	auto BankHeader = Reader.Read<MilesAudioBank>();

	ReaderStream->SetPosition(BankHeader.SourceEntryOffset);

	if (BankHeader.Version == 0xD)
	{
		List<MilesTitanfallSourceEntry> Sources(BankHeader.EventCount, true);
		ReaderStream->Read((uint8_t*)&Sources[0], 0, sizeof(MilesTitanfallSourceEntry) * BankHeader.EventCount);

		for (auto& Entry : Sources)
		{
			ReaderStream->SetPosition(BankHeader.NameTableOffset + Entry.NameOffset);

			auto Name = Reader.ReadCString();
			
			MilesAudioAsset Asset{ Name, Entry.SampleRate, Entry.ChannelCount, Entry.StreamHeaderOffset, Entry.StreamHeaderSize, Entry.StreamDataOffset, Entry.StreamDataSize, Entry.PatchIndex, (uint32_t)Entry.EntryLocal };
			Assets.Add(Hashing::XXHash::HashString(Name), Asset);
		}
	}
	else
	{

		List<MilesApexSourceEntry> Sources(BankHeader.SourcesCount, true);
		ReaderStream->Read((uint8_t*)&Sources[0], 0, sizeof(MilesApexSourceEntry) * BankHeader.SourcesCount);

		for (auto& Entry : Sources)
		{
			ReaderStream->SetPosition(BankHeader.NameTableOffset + Entry.NameOffset);

			auto Name = Reader.ReadCString();

			MilesAudioAsset Asset{ Name, Entry.SampleRate, Entry.ChannelCount, Entry.StreamHeaderOffset, Entry.StreamHeaderSize, Entry.StreamDataOffset, Entry.StreamDataSize, Entry.PatchIndex, (uint32_t)Entry.EntryLocal };
			Assets.Add(Hashing::XXHash::HashString(Name), Asset);
		}
	}

	auto MStrFiles = IO::Directory::GetFiles(BasePath, "*.mstr");

	for (auto& MStr : MStrFiles)
	{
		auto StreamReader = IO::BinaryReader(IO::File::OpenRead(MStr));
		auto StreamHeader = StreamReader.Read<MilesStreamBankHeader>();

		uint32_t KeyIndex = ((uint32_t)StreamHeader.LocalizeIndex << 16) + StreamHeader.PatchIndex;

		MilesStreamBank NewBank{ MStr, StreamHeader.StreamDataOffset };
		StreamBanks.Add(KeyIndex, NewBank);
	}
}

void MilesLib::ExtractAsset(const MilesAudioAsset& Asset, const string& FilePath)
{
}

std::unique_ptr<List<ApexAsset>> MilesLib::BuildAssetList()
{
	auto Result = std::make_unique<List<ApexAsset>>();

	for (auto& AssetKvp : Assets)
	{
		auto& Asset = AssetKvp.Value();

		ApexAsset NewAsset;
		NewAsset.Hash = AssetKvp.first;
		NewAsset.Name = AssetKvp.second.Name;
		NewAsset.Type = ApexAssetType::Sound;
		NewAsset.Info = string::Format("Sample Rate: %d, Channels: %d", AssetKvp.second.SampleRate, AssetKvp.second.ChannelCount);

		Result->EmplaceBack(std::move(NewAsset));
	}

	return std::move(Result);
}
