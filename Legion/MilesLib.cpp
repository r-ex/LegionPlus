#include "pch.h"
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

struct MilesApexS3SourceEntry
{
	char pad_0000[12]; //0x0000

	uint16_t EntryLocal; //0x000C
	uint16_t PatchIndex; //0x000E
	
	uint32_t NameOffset; //0x0010
	
	uint16_t SampleRate; //0x0014
	uint16_t BitRate; //0x0016
	
	char pad_0018[2]; //0x0018
	
	uint8_t ChannelCount; //0x001A
	
	char pad_001B[21]; //0x001B
	
	uint32_t StreamHeaderSize; //0x0030
	uint32_t StreamDataSize; //0x0034
	uint64_t StreamHeaderOffset; //0x0038
	uint64_t StreamDataOffset; //0x0040
	
	char pad_0048[4]; //0x0048
	
	uint32_t TemplateId; //0x004C
	
	char pad_0050[4]; //0x0050
	
	uint32_t StreamDataSize2; //0x0054
};
static_assert(sizeof(MilesApexS3SourceEntry) == 0x58);

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
static_assert(sizeof(MilesTitanfallSourceEntry) == 0x58);

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

static uint32_t MilesReadFileStream_TF2(void* UserData, char* Buffer, uint64_t Length)
{
	return MilesReadFileStream(Buffer, Length, UserData);
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
		// TF|2
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
		if (BankHeader.Version == 40) {
			// S11.1
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
		else if (BankHeader.Version == 32) {
			// S3
			ReaderStream->SetPosition(*(uint64_t*)(uintptr_t(&BankHeader) + 0x48));
			const auto SourcesCount = *(uint32_t*)(uintptr_t(&BankHeader) + 0x98);
			const auto NameTableOffset = *(uint64_t*)(uintptr_t(&BankHeader) + 0x70);
			List<MilesApexS3SourceEntry> Sources(SourcesCount, true);
			ReaderStream->Read((uint8_t*)&Sources[0], 0, sizeof(MilesApexS3SourceEntry) * SourcesCount);

			for (auto& Entry : Sources)
			{
				ReaderStream->SetPosition(NameTableOffset + Entry.NameOffset);

				auto Name = Reader.ReadCString();

				MilesAudioAsset Asset{ Name, Entry.SampleRate, Entry.ChannelCount, Entry.StreamHeaderOffset, Entry.StreamHeaderSize, Entry.StreamDataOffset, Entry.StreamDataSize, Entry.PatchIndex, (uint32_t)Entry.EntryLocal };
				Assets.Add(Hashing::XXHash::HashString(Name), Asset);
			}
		}
		else {
			throw new std::exception("Unknown MBNK version!");
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
	uint32_t KeyIndex = ((uint32_t)Asset.LocalizeIndex << 16) + Asset.PatchIndex;
	if (!StreamBanks.ContainsKey(KeyIndex))
		return;
	
	const auto& Bank = StreamBanks[KeyIndex];
	auto Reader = IO::BinaryReader(IO::File::OpenRead(Bank.Path));
	auto ReaderStream = Reader.GetBaseStream();

	static uintptr_t binkawin = 0;
	if (!binkawin) {
		binkawin = (uintptr_t)LoadLibraryA("binkawin64.dll");
	}
	if (!binkawin)
		//throw new std::exception("Failed to load binkawin64.dll!");
		return;

	// Dynamically get a table
	static uintptr_t binka = 0;
	if (!binka) {
		const auto proc = uintptr_t(GetProcAddress(HMODULE(binkawin), "MilesDriverRegisterBinkAudio")) + 3;
		const auto offset = *(uint32_t*)proc;
		binka = proc + 4 + offset;
	}

	// Determine if version is supported or not...
	static bool check = false;
	static bool version_tf2 = false;
	if (!check) {
		if ((*(uintptr_t*)(binka + 7 * 8) != 0) && (*(uintptr_t*)(binka + 7 * 8) != 0x0A09080605040302)) {
			binka = 0;
			FreeLibrary(HMODULE(binkawin));
			//throw new std::exception("Unsupported version with bigger table!");
			return;
		}
		const auto dosHeader = PIMAGE_DOS_HEADER(binkawin);
		const auto imageNTHeaders = PIMAGE_NT_HEADERS(binkawin + dosHeader->e_lfanew);
		version_tf2 = (imageNTHeaders->FileHeader.TimeDateStamp <= 0x57E48A0C);
		check = true;
	}

	// function types in the table
	using metadata_f_t = uintptr_t(__fastcall*)(void* data, size_t size, uint16_t* channels, uint32_t* sample_rate, uint32_t* samples_count, uint32_t* adw4);
	
	using open_stream_f_t = uintptr_t(__fastcall*)(void* data, void*, void* reader, void* user_data);
	using open_stream_tf2_f_t = uintptr_t(__fastcall*)(void* user_data, void* data, void*, void* reader);
	
	using decoder_f_t = size_t(__fastcall*)(void* data, void* decoded, size_t size, size_t size2, void* reader, void* user_data);
	using deocder_tf2_f_t = size_t(__fastcall*)(void* user_data, void* data, void* decoded, size_t size, void* reader);

	const auto metadata = *(metadata_f_t*)(binka + 8);
	uint16_t channels;
	uint32_t sample_rate, samples_count;
	uint32_t adw4[4];
	uint8_t header[24];
	ReaderStream->SetPosition(Asset.PreloadOffset);
	ReaderStream->Read(header, 0, sizeof(header));
	metadata(header, sizeof(header), &channels, &sample_rate, &samples_count, adw4);

	ReaderStream->SetPosition(Asset.PreloadOffset);
	auto allocd = std::vector<uint8_t>(adw4[0], 0);
	BinkASIReader UserData{ &Reader, 0, Asset.PreloadSize, Asset.StreamOffset + Bank.StreamDataOffset, 0 };
	if (version_tf2) {
		// Let's hope someone won't use some old ass lib which doesn't expect the right header
		const auto open_stream = *(open_stream_tf2_f_t*)(binka + 16);
		open_stream(&UserData, allocd.data(), nullptr, MilesReadFileStream_TF2);
	}
	else {
		// we assume it's S3, this is the same that OG Legion uses...
		const auto open_stream = *(open_stream_f_t*)(binka + 16);
		// TODO: error check - should return 2
		open_stream(allocd.data(), nullptr, MilesReadFileStream, &UserData);
	}

	UserData.DataStreamSize = *(uint32_t*)(allocd.data() + 16ull) - Asset.PreloadSize;

	auto decoded = std::vector<float>(channels * 64ull);
	auto decoded_desh = std::vector<float>(channels * 64ull);

	size_t ret = 0;
	// TODO: potentially break on hitting the required sample count?
	auto Writer = IO::File::OpenWrite(FilePath);
	do {
		if (version_tf2) {
			const auto decode = *(deocder_tf2_f_t*)(binka + 24);
			ret = decode(&UserData, allocd.data(), decoded.data(), 64, MilesReadFileStream_TF2);
		}
		else {
			const auto decode = *(decoder_f_t*)(binka + 24);
			ret = decode(allocd.data(), decoded.data(), 64, 64, MilesReadFileStream, &UserData);
		}

		size_t desh_pos = 0;
		for (size_t j = 0; j < 4; j++) {
			for (size_t i = 0; i < 16; i++) {
				for (size_t chan = 0; chan < channels; chan++) {
					decoded_desh[desh_pos++] = decoded[(j * 16) + i + (chan * 64)];
				}
			}
		}

		// TODO: proper container...
		if (ret > 0) {
			Writer->Write((uint8_t*)decoded_desh.data(), 0, decoded_desh.size() * 4);
		}
	} while (ret == 64);
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
