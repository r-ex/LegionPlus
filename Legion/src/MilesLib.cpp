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
	uint32_t DialogueCount;

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

struct MilesApexSourceEntry
{
	uint32_t NameOffset;

	MilesLanguageID EntryLocal;
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

	MilesLanguageID EntryLocal; //0x000C
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

	MilesLanguageIDTitanfall EntryLocal;
	uint16_t PatchIndex;

	uint32_t StreamDataSize2;
};
static_assert(sizeof(MilesTitanfallSourceEntry) == 0x58);

struct MilesStreamBankHeader
{
	uint32_t Magic;
	uint16_t Version;

	MilesLanguageID LocalizeIndex;

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

MilesLanguageID ApexLangFromTF(MilesLanguageIDTitanfall langIn)
{
	switch (langIn)
	{
	case MilesLanguageIDTitanfall::None:
	case MilesLanguageIDTitanfall::English:
	case MilesLanguageIDTitanfall::French:
	case MilesLanguageIDTitanfall::German:
	case MilesLanguageIDTitanfall::Spanish:
	case MilesLanguageIDTitanfall::Italian:
	case MilesLanguageIDTitanfall::Japanese:
	case MilesLanguageIDTitanfall::Polish:
		return static_cast<MilesLanguageID>(langIn);
		break;
	case MilesLanguageIDTitanfall::Portuguese:
		return MilesLanguageID::FAKEPORTUGUESE;
		break;
	case MilesLanguageIDTitanfall::Russian:
		return MilesLanguageID::Russian;
		break;
	case MilesLanguageIDTitanfall::TChinese:
		return MilesLanguageID::Mandarin;
		// unsure
		break;
	case MilesLanguageIDTitanfall::MSpanish:
		return MilesLanguageID::FAKELATINSPAN;
		break;
	case MilesLanguageIDTitanfall::_MILESLANGCOUNT:
	default:
		return MilesLanguageID::INVAILD;
		break;
	}

	return MilesLanguageID::None;
}

MilesLanguageIDTitanfall TFLangFromApex(MilesLanguageID langIn)
{
	switch (langIn)
	{
	case MilesLanguageID::None:
	case MilesLanguageID::English:
	case MilesLanguageID::French:
	case MilesLanguageID::German:
	case MilesLanguageID::Spanish:
	case MilesLanguageID::Italian:
	case MilesLanguageID::Japanese:
	case MilesLanguageID::Polish:
		return static_cast<MilesLanguageIDTitanfall>(langIn);
		break;
	case MilesLanguageID::Russian:
		return MilesLanguageIDTitanfall::Russian;
		break;
	case MilesLanguageID::Mandarin:
		return MilesLanguageIDTitanfall::TChinese;
		break;
	case MilesLanguageID::FAKEPORTUGUESE:
		return MilesLanguageIDTitanfall::Portuguese;
		break;
	case MilesLanguageID::FAKELATINSPAN:
		return MilesLanguageIDTitanfall::MSpanish;
		break;
	case MilesLanguageID::Korean:
	case MilesLanguageID::UNKNOWN:
	case MilesLanguageID::COUNT:
	default:
		return MilesLanguageIDTitanfall::INVAILD;
		break;
	}

	return MilesLanguageIDTitanfall::None;
}

const String&
LanguageName(MilesLanguageID lang) {
	static const String LanguageNames[(int16_t)MilesLanguageID::COUNT + 1] = {
		"English",
		"French",
		"German",
		"Spanish",
		"Italian",
		"Japanese",
		"Polish",
		"Russian",
		"Mandarin",
		"Korean",
		"Portuguese(TF2)",
		"Latin Spanish(TF2)",
		"Unknown",
		"Sounds",
	};
	if (MilesLanguageID::English <= lang && lang < MilesLanguageID::COUNT) {
		return LanguageNames[(int32_t)lang];
	}
	if (lang == MilesLanguageID::None) {
		return LanguageNames[(int16_t)MilesLanguageID::COUNT];
	}
	return LanguageNames[(int32_t)MilesLanguageID::UNKNOWN];
}

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

	this->MbnkVersion = BankHeader.Version;

	ReaderStream->SetPosition(BankHeader.SourceEntryOffset);

	auto SelectedLanguage = (MilesLanguageID)ExportManager::Config.Get<System::SettingType::Integer>("AudioLanguage");

	if (BankHeader.Version == 0xB)
	{
		// R2TT - only english audio exists
		ReaderStream->SetPosition(*(uint64_t*)(uintptr_t(&BankHeader) + 0x48));
		const auto NameTableOffset = *(uint64_t*)(uintptr_t(&BankHeader) + 0x70);
		const auto SourcesCount = *(uint32_t*)(uintptr_t(&BankHeader) + 0xA0);
		List<MilesTitanfallSourceEntry> Sources(SourcesCount, true);
		ReaderStream->Read((uint8_t*)&Sources[0], 0, sizeof(MilesTitanfallSourceEntry) * SourcesCount);

		for (auto& Entry : Sources)
		{
			ReaderStream->SetPosition(NameTableOffset + Entry.NameOffset);

			auto Name = Reader.ReadCString();

			MilesAudioAsset Asset{ Name, Entry.SampleRate, Entry.ChannelCount, Entry.StreamHeaderOffset, Entry.StreamHeaderSize, Entry.StreamDataOffset, Entry.StreamDataSize, Entry.PatchIndex, (int32_t)Entry.EntryLocal };
			Assets.Add(Hashing::XXHash::HashString(Name), Asset);
		}
	}
	else if (BankHeader.Version > 0xB && BankHeader.Version <= 0xD)
	{
		// TF|2
		ReaderStream->SetPosition(*(uint64_t*)(uintptr_t(&BankHeader) + 0x48)); // SourcesOffset
		const auto NameTableOffset = *(uint64_t*)(uintptr_t(&BankHeader) + 0x70);
		const auto LanguageSourcesCount = *(uint32_t*)(uintptr_t(&BankHeader) + 0x9C);
		auto SourcesCount = *(uint32_t*)(uintptr_t(&BankHeader) + 0xA0);

		SourcesCount += (LanguageSourcesCount * (uint32_t)TFLangFromApex(SelectedLanguage));

		List<MilesTitanfallSourceEntry> Sources(SourcesCount, true);
		ReaderStream->Read((uint8_t*)&Sources[0], 0, sizeof(MilesTitanfallSourceEntry) * SourcesCount);

		for (auto& Entry : Sources)
		{
			ReaderStream->SetPosition(NameTableOffset + Entry.NameOffset);

			auto Name = Reader.ReadCString();

			// fix lang because titanfall is slightly different
			MilesLanguageID fixedLang = ApexLangFromTF(static_cast<MilesLanguageIDTitanfall>(Entry.EntryLocal));

			if (fixedLang == MilesLanguageID::None || fixedLang == SelectedLanguage)
			{
				MilesAudioAsset Asset{ Name, Entry.SampleRate, Entry.ChannelCount, Entry.StreamHeaderOffset, Entry.StreamHeaderSize, Entry.StreamDataOffset, Entry.StreamDataSize, Entry.PatchIndex, (int32_t)fixedLang};
				Assets.Add(Hashing::XXHash::HashString(Name), Asset);
			}
		}
	}
	else
	{
		if (BankHeader.Version >= 40) {
			// S11.1
			auto SoundCount = BankHeader.SourcesCount - BankHeader.DialogueCount;
			{ 
				// Gather non-voiced audio files
				List<MilesApexSourceEntry> SoundSources(SoundCount, true);
				ReaderStream->SetPosition(BankHeader.SourceEntryOffset);
				ReaderStream->Read((uint8_t*)&SoundSources[0], 0, sizeof(MilesApexSourceEntry) * SoundCount);
				for (auto& Entry : SoundSources)
				{
					ReaderStream->SetPosition(BankHeader.NameTableOffset + Entry.NameOffset);
					auto Name = Reader.ReadCString();
					MilesAudioAsset Asset{ Name, Entry.SampleRate, Entry.ChannelCount, Entry.StreamHeaderOffset, Entry.StreamHeaderSize, Entry.StreamDataOffset, Entry.StreamDataSize, Entry.PatchIndex, (int32_t)Entry.EntryLocal };
					Assets.Add(Hashing::XXHash::HashString(Name), Asset);
				}
			}

			{
				// Gather voiced audio files in the selected language
				List<MilesApexSourceEntry> DialogueSources(BankHeader.DialogueCount, true);
				ReaderStream->SetPosition(BankHeader.SourceEntryOffset + sizeof(MilesApexSourceEntry) * (SoundCount + (int32_t)SelectedLanguage * BankHeader.DialogueCount));
				ReaderStream->Read((uint8_t*)&DialogueSources[0], 0, sizeof(MilesApexSourceEntry) * BankHeader.DialogueCount);
				for (auto& Entry : DialogueSources)
				{
					ReaderStream->SetPosition(BankHeader.NameTableOffset + Entry.NameOffset);
					auto Name = Reader.ReadCString();
					MilesAudioAsset Asset{ Name, Entry.SampleRate, Entry.ChannelCount, Entry.StreamHeaderOffset, Entry.StreamHeaderSize, Entry.StreamDataOffset, Entry.StreamDataSize, Entry.PatchIndex, (int32_t)Entry.EntryLocal };
					Assets.Add(Hashing::XXHash::HashString(Name), Asset);
				}
			}
		}
		else if (BankHeader.Version >= 28 && BankHeader.Version <= 32) {
			// S2 -> S3
			ReaderStream->SetPosition(*(uint64_t*)(uintptr_t(&BankHeader) + 0x48));
			const auto NameTableOffset = *(uint64_t*)(uintptr_t(&BankHeader) + 0x70);
			const auto LanguageSourcesCount = *(uint32_t*)(uintptr_t(&BankHeader) + 0x94);
			auto SourcesCount = *(uint32_t*)(uintptr_t(&BankHeader) + 0x98);

			SourcesCount += (LanguageSourcesCount * (uint32_t)SelectedLanguage);

			List<MilesApexS3SourceEntry> Sources(SourcesCount, true);
			ReaderStream->Read((uint8_t*)&Sources[0], 0, sizeof(MilesApexS3SourceEntry) * SourcesCount);

			for (auto& Entry : Sources)
			{
				ReaderStream->SetPosition(NameTableOffset + Entry.NameOffset);

				auto Name = Reader.ReadCString();

				if (Entry.EntryLocal == MilesLanguageID::None || Entry.EntryLocal == SelectedLanguage)
				{
					MilesAudioAsset Asset{ Name, Entry.SampleRate, Entry.ChannelCount, Entry.StreamHeaderOffset, Entry.StreamHeaderSize, Entry.StreamDataOffset, Entry.StreamDataSize, Entry.PatchIndex, (uint32_t)Entry.EntryLocal };
					Assets.Add(Hashing::XXHash::HashString(Name), Asset);
				}
			}
		}
		else {
			g_Logger.Warning("Unknown MBNK Version: %i\n", BankHeader.Version);
			throw std::exception("Unknown MBNK version!");
		}
	}

	auto Language = LanguageName(SelectedLanguage);

	struct MStrFile {
		MilesLanguageID languageID;
		int32_t patch;
		string filename;
	};

	auto Paths = IO::Directory::GetFiles(BasePath, "*.mstr");

	for (auto& Path : Paths)
	{
		MilesStreamBankHeader StreamHeader;
		try {
			auto StreamReader = IO::BinaryReader(IO::File::OpenRead(Path));
			StreamHeader = StreamReader.Read<MilesStreamBankHeader>();
		}
		catch (...) { continue; }

		if (this->MbnkVersion >= 11 && this->MbnkVersion <= 13)
			StreamHeader.LocalizeIndex = ApexLangFromTF(static_cast<MilesLanguageIDTitanfall>(StreamHeader.LocalizeIndex));

		if (StreamHeader.Magic != 0x43535452) {
			g_Logger.Warning("File %s has .mstr extension but wrong magic number\n", Path.ToCString());
			continue;
		}
		if (StreamHeader.LocalizeIndex != MilesLanguageID::None && StreamHeader.LocalizeIndex != SelectedLanguage) continue;

		g_Logger.Info("Loaded %s (patch %d) audio bank: %s\n", LanguageName(StreamHeader.LocalizeIndex).ToCString(), StreamHeader.PatchIndex, Path.ToCString());

		uint32_t KeyIndex = ((uint32_t)StreamHeader.LocalizeIndex << 16) + StreamHeader.PatchIndex;

		MilesStreamBank NewBank{ Path, StreamHeader.StreamDataOffset };
		StreamBanks.Add(KeyIndex, NewBank);
	}
}

bool MilesLib::ExtractAsset(const MilesAudioAsset& Asset, const string& FilePath)
{
	uint32_t KeyIndex = ((uint32_t)Asset.LocalizeIndex << 16) + Asset.PatchIndex;

	if (!StreamBanks.ContainsKey(KeyIndex))
		return false;
	
	const auto& Bank = StreamBanks[KeyIndex];
	auto Reader = IO::BinaryReader(IO::File::OpenRead(Bank.Path));
	auto ReaderStream = Reader.GetBaseStream();

	static uintptr_t binkawin = 0;
	if (!binkawin) {
		if ((binkawin = (uintptr_t)LoadLibraryA("binkawin64.dll")) == 0)
		{
			HKEY hKey = HKEY_LOCAL_MACHINE;
			HKEY resKey;
			string installDir;
			char buf[1024]{};
			DWORD BufferSize = 1025;

			// check origin for the apex installation directory
			if (RegGetValueA(hKey, "SOFTWARE\\Respawn\\Apex", "Install Dir", RRF_RT_ANY, NULL, (PVOID)&buf, &BufferSize) != ERROR_SUCCESS)
			{
				// origin apex was not found; check steam
				// this is bad. users can have apex installed on steam on a different drive to the steam installation and this won't find it
				if (RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", "SteamPath", RRF_RT_ANY, NULL, (PVOID)&buf, &BufferSize) != ERROR_SUCCESS)
				{
					g_Logger.Warning("no apex installation found. please bug this if you have apex and provide your installation path\n");
					return false;
				}
				else {
					installDir = IO::Path::Combine(buf, "steamapps");
					installDir = IO::Path::Combine(installDir, "common");
					installDir = IO::Path::Combine(installDir, "Apex Legends");
				}
			}
			else {
				installDir = buf;
			}

			SetDllDirectoryA(installDir.ToCString());
			binkawin = (uintptr_t)LoadLibraryA("binkawin64.dll");

		}
	}
	if (!binkawin)
	{
		//throw new std::exception("Failed to load binkawin64.dll!");
		g_Logger.Warning("!!! - Unable to export audio asset: Failed to load binkawin64.dll (make sure that you have apex installed or the required dlls in the same directory as LegionPlus.exe)\n");
		return false;
	}

	// Dynamically get a table
	static uintptr_t binka = 0;
	if (!binka) {
		const auto proc = uintptr_t(GetProcAddress(HMODULE(binkawin), "MilesDriverRegisterBinkAudio")) + 3;

		if (proc == 3)
			return false;

		const auto offset = *(uint32_t*)proc;
		binka = proc + 4 + offset;
	}

	// Determine if version is supported or not...
	static bool check = false;
	static bool version_tf2 = false;
	static bool version_retail = false;
	if (!check) {
		if ((*(uintptr_t*)(binka + 7 * 8) != 0) && (*(uintptr_t*)(binka + 7 * 8) != 0x0A09080605040302)) {
			version_retail = true;
			check = true;
		}
		else {
			const auto dosHeader = PIMAGE_DOS_HEADER(binkawin);
			const auto imageNTHeaders = PIMAGE_NT_HEADERS(binkawin + dosHeader->e_lfanew);
			version_tf2 = (imageNTHeaders->FileHeader.TimeDateStamp <= 0x57E48A0C);
			check = true;
		}
	}

	// function types in the table
	using metadata_f_t = uintptr_t(__fastcall*)(void* data, size_t size, uint16_t* channels, uint32_t* sample_rate, uint32_t* samples_count, uint32_t* adw4);
	
	using open_stream_f_t = uintptr_t(__fastcall*)(void* data, void*, void* reader, void* user_data);
	using open_stream_tf2_f_t = uintptr_t(__fastcall*)(void* user_data, void* data, void*, void* reader);
	
	using decoder_f_t = size_t(__fastcall*)(void* data, void* decoded, size_t size, size_t size2, void* reader, void* user_data);
	using deocder_tf2_f_t = size_t(__fastcall*)(void* user_data, void* data, void* decoded, size_t size, void* reader);

	using unk20_f_t = size_t(__fastcall*)(void* data, uint32_t a2, uint32_t* a3, uint32_t* a4);
	using unk18_f_t = size_t(__fastcall*)(void* data);

	// !!!NEW!!!
	using gbs_f_t = size_t(__fastcall*)(void* data, void* stream_data, size_t stream_data_size, uint32_t* consumed, uint32_t* block_size, uint32_t* required_size);
	using decoder_new_f_t = size_t(__fastcall*)(void* data, void* stream_data, size_t stream_data_len, void* out_data, size_t out_data_size, uint32_t* consumed, uint32_t* samples);

	const auto metadata = *(metadata_f_t*)(binka + 8);
	uint16_t channels;
	uint32_t sample_rate, samples_count;
	uint32_t adw4[4];
	uint8_t header[24];
	ReaderStream->SetPosition(Asset.PreloadOffset);
	ReaderStream->Read(header, 0, sizeof(header));
	metadata(header, sizeof(header), &channels, &sample_rate, &samples_count, adw4);

	ReaderStream->SetPosition(Asset.PreloadOffset);

	if ((AudioExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("AudioFormat") == AudioExportFormat_t::BinkA)
	{
		char* preloadBuf = new char[Asset.PreloadSize];
		ReaderStream->Read((uint8_t*)preloadBuf, 0, Asset.PreloadSize);

		ReaderStream->SetPosition(Asset.StreamOffset + Bank.StreamDataOffset);

		uint32_t StreamDataSize = *(uint32_t*)(header + 16) - Asset.PreloadSize;

		char* streamDataBuf = new char[StreamDataSize];
		ReaderStream->Read((uint8_t*)streamDataBuf, 0, StreamDataSize);

		std::ofstream out_file(IO::Path::ChangeExtension(FilePath, "binka").ToCString(), std::ios::out | std::ios::binary);
		out_file.write(preloadBuf, Asset.PreloadSize);
		out_file.write(streamDataBuf, StreamDataSize);
		out_file.close();
		return true;
	}


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

	/*{
		const auto unk20 = *(unk20_f_t*)(binka + 40);
		uint32_t tmp;
		unk20(allocd.data(), 0, &tmp, nullptr);
	}*/
	// Reset blending frames
	if (version_retail) {
		const auto blend = *(unk18_f_t*)(binka + 24);
		blend(allocd.data());
	}
	else {
		const auto unk18 = *(unk18_f_t*)(binka + 32);
		unk18(allocd.data());
	}

	UserData.DataStreamSize = *(uint32_t*)(allocd.data() + 16ull) - Asset.PreloadSize;

	size_t decoded_size = channels * 64ull;
	if (version_retail) {
		// We can't make any mistake in the size...
		// I think that's the pure max?
		decoded_size = channels * adw4[2];
	}
	auto decoded = std::vector<float>(version_retail ? 0 : decoded_size);
	auto decoded_desh = std::vector<float>(version_retail ? 0 : decoded_size);
	auto decoded_short = std::vector<uint16_t>(version_retail ? decoded_size : 0);

	size_t ret = 0;
	// TODO: potentially break on hitting the required sample count?
	auto Writer = IO::File::OpenWrite(FilePath);

	WAVEHEADER hdr;

	uint64_t DataSize = 0;

	Writer->Write((uint8_t*)&hdr, 0, sizeof(WAVEHEADER));

	std::vector<char> stream_data(8);

	do {
		if (version_retail) {
			// Welcome to my cult where we perform this ritual...

			// Read first 8 bytes...
			MilesReadFileStream(stream_data.data(), 8, &UserData);

			// Get required block size
			const auto get_block_size = *(gbs_f_t*)(binka + 7*8);
			uint32_t consumed, block_size, req;
			get_block_size(allocd.data(), stream_data.data(), 8, &consumed, &block_size, &req);

			if (block_size == 65535) {
				break;
			}

			if (block_size > (UserData.HeaderSize + UserData.DataStreamSize - UserData.DataRead)) {
				break;
			}

			// Resize and read everything else, allocation will happen ONLY if new_size>capacity
			stream_data.resize(block_size);
			MilesReadFileStream(stream_data.data() + 8, block_size - 8, &UserData);

			// Now we can finally decode...
			const auto decode = *(decoder_new_f_t*)(binka + 6*8);
			uint32_t consumed_decode, samples;
			decode(allocd.data(), stream_data.data(), stream_data.size(), decoded_short.data(), decoded_short.size() * 2, &consumed_decode, &samples);
			ret = samples * channels; // ???

			// Debug assert?
			assert(consumed_decode == block_size);

			// Resize just to be safe...
			stream_data.resize(8);
		}
		else if (version_tf2) {
			const auto decode = *(deocder_tf2_f_t*)(binka + 24);
			ret = decode(&UserData, allocd.data(), decoded.data(), 64, MilesReadFileStream_TF2);
		}
		else {
			const auto decode = *(decoder_f_t*)(binka + 24);
			ret = decode(allocd.data(), decoded.data(), 64, 64, MilesReadFileStream, &UserData);
		}

		if (!version_retail) {
			size_t desh_pos = 0;
			for (size_t j = 0; j < 4; j++) {
				for (size_t i = 0; i < 16; i++) {
					for (size_t chan = 0; chan < channels; chan++) {
						decoded_desh[desh_pos++] = decoded[(j * 16) + i + (chan * 64)];
					}
				}
			}
		}
		else {
			// Welcome to my another ritual of stereo packed encoding
			//if (channels > 2) { // remove for stereo
			if (channels > 1) { // remove for mono
				// WAV expects all channels at once meanwhile MSS gives us 2 channels per big sample thingie?
				// Or it just decodes everything in a big chunk?
				size_t pos = 0;
				auto decoded_short_copy = decoded_short;
				auto samples = ret / channels; // E - Effiecency 
				for (size_t i = 0; i < samples; i++) {
					//for (size_t chan = 0; chan < (channels / 2); chan++) { // remove for stereo
					for (size_t chan = 0; chan < channels; chan++) { // remove for mono
						decoded_short[pos++] = decoded_short_copy[(chan * samples) + i];
						//decoded_short[pos++] = decoded_short_copy[(chan * samples) + i + 1]; // remove for stereo
					}
					// This is stereo related
					/*
					if (channels % 2) {
						assert(false); // ???
					}
					*/
				}
			}
		}

		if (ret > 0) {
			if (version_retail) {
				DataSize += ret * 2;
				Writer->Write((uint8_t*)decoded_short.data(), 0, ret * 2);
			}
			else {
				DataSize += decoded_desh.size() * 4;
				Writer->Write((uint8_t*)decoded_desh.data(), 0, decoded_desh.size() * 4);
			}
		}
	} while ((ret == 64) || (version_retail && ret));

	hdr.size = DataSize + 36;

	hdr.fmt.channels = channels;
	hdr.fmt.sampleRate = sample_rate;
	hdr.fmt.blockAlign = DataSize / samples_count;
	hdr.fmt.bitsPerSample = ((DataSize * 8) / samples_count)/channels;

	hdr.data.chunkSize = DataSize;

	if (version_retail) {
		// Yes, this is required, don't ask me why
		hdr.fmt.formatTag = 1; // WAVE_FORMAT_PCM
		hdr.fmt.blockAlign = 2 * channels;
		hdr.fmt.bitsPerSample = 16;
	}

	hdr.fmt.avgBytesPerSecond = hdr.fmt.blockAlign * sample_rate;

	Writer->Seek(0, IO::SeekOrigin::Begin);

	Writer->Write((uint8_t*)&hdr, 0, sizeof(WAVEHEADER));
	g_Logger.Info("Successfully exported %s\n", FilePath.ToCString());
	return true;
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
		String Language = AssetKvp.second.LocalizeIndex == -1 ? String("None") : LanguageName((MilesLanguageID)AssetKvp.second.LocalizeIndex);
		NewAsset.Info = string::Format("Language: %s, Sample Rate: %d, Channels: %d", Language.ToCString(), AssetKvp.second.SampleRate, AssetKvp.second.ChannelCount);
		NewAsset.Version = this->MbnkVersion;

		Result->EmplaceBack(std::move(NewAsset));
	}

	return std::move(Result);
}
