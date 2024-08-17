#include "pch.h"
#include "MilesLib.h"
#include "Kore.h"
#include "XXHash.h"
//#include "BinkAudioEngine.h"
#include "RadAudio/RadAudioDecoder.h"

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
	IO::BinaryReader Reader = IO::BinaryReader(IO::File::OpenRead(Bank.Path));
	IO::Stream* ReaderStream = Reader.GetBaseStream();

	// Dynamically get a table
	static MilesASIDecoder* decoder = 0;

	ReaderStream->SetPosition(Asset.PreloadOffset);

	// function types in the table
	using metadata_f_t = uintptr_t(__fastcall*)(void* data, size_t size, uint16_t* channels, uint32_t* sample_rate, uint32_t* samples_count, uint32_t* adw4, uint32_t* sizeRequired);
	
	using open_stream_f_t = uintptr_t(__fastcall*)(void* data, size_t* data_size, void* reader, void* user_data);

	using unk20_f_t = size_t(__fastcall*)(void* data, uint32_t a2, uint32_t* a3, uint32_t* a4);
	using unk18_f_t = size_t(__fastcall*)(void* data);

	// !!!NEW!!!
	using gbs_f_t = size_t(__fastcall*)(void* data, void* stream_data, size_t stream_data_size, uint32_t* consumed, uint32_t* block_size, uint32_t* required_size);
	using decoder_new_f_t = size_t(__fastcall*)(void* data, void* stream_data, size_t stream_data_len, void* out_data, size_t out_data_size, uint32_t* consumed, uint32_t* samples);

	// 0 - data size for opening stream
	// 1 - max block size
	// 2 - max number of samples per decode call
	// 3 - 
	uint32_t parsedSizeInfo[4]{};

	// This used to be a minimal header buffer, but Rad Audio requires the buffer to have more than just the
	// base file header, so we might as well read the whole of the asset's preload data into the buffer
	uint8_t* preloadDataBuffer = new uint8_t[Asset.PreloadSize];
	ReaderStream->Read(preloadDataBuffer, 0, Asset.PreloadSize);

	if (preloadDataBuffer[0] == 'A' && preloadDataBuffer[1] == 'D' && preloadDataBuffer[2] == 'A' && preloadDataBuffer[3] == 'R')
	{
		//printf("Using Rad Audio Decoder\n");
		decoder = reinterpret_cast<MilesASIDecoder*>(GetRadAudioDecoder());
	}
	else
	{
		g_Logger.Warning("Failed to decode audio. This build only supports the Rad Audio Codec for audio extraction.\n");
		return false;
	}

	uint16_t channels = 0;
	uint32_t sampleRate = 0;
	uint32_t numSamples = 0;

	const auto ASI_stream_parse_metadata = static_cast<metadata_f_t>(decoder->ASI_stream_parse_metadata);

	// RadA's decoder also has a 7th argument that contains the same value as parsedSizeInfo[0]
	size_t metadata_res = ASI_stream_parse_metadata(preloadDataBuffer, Asset.PreloadSize, &channels, &sampleRate, &numSamples, parsedSizeInfo, nullptr);

	// Reset cursor back to the data to be read
	ReaderStream->SetPosition(Asset.PreloadOffset);

	auto containerData = std::vector<uint8_t>(parsedSizeInfo[0], 0);
	BinkASIReader UserData{ &Reader, 0, Asset.PreloadSize, Asset.StreamOffset + Bank.StreamDataOffset, 0 };

	// TODO: error check - should return 2
	const auto ASI_open_stream = static_cast<open_stream_f_t>(decoder->ASI_open_stream);

	size_t allocatedDataSize = containerData.size();
	ASI_open_stream(containerData.data(), &allocatedDataSize, MilesReadFileStream, &UserData);

	// not entirely sure what this does
	const auto ASI_notify_seek = static_cast<unk18_f_t>(decoder->ASI_notify_seek);
	ASI_notify_seek(containerData.data());

	// offset is to a "file_size" member in the bink audio file header
	//UserData.DataStreamSize = *(uint32_t*)(containerData.data() + 0x10) - Asset.PreloadSize;
	UserData.DataStreamSize = *(uint64_t*)(containerData.data() + 0x18) - Asset.PreloadSize;

	std::vector<float> interleavedBuffer = std::vector<float>(channels * numSamples);
	float* outputBuffer = interleavedBuffer.data();

	std::vector<char> stream_data;

	// Buffer for holding the decoded data for each decode_block call.
	// parsedSizeInfo[2] is the max number of samples per decode
	std::vector<float> radDecodedData(channels * parsedSizeInfo[2]);

	size_t totalFramesDecoded = 0;
	uint32_t minInputBufferSize = 0; // start off with 0 bytes for input buffer so we can ask the decoder what it wants

	while (totalFramesDecoded < numSamples)
	{
		// Clear the decode buffer just in case something goes wrong
		memset(radDecodedData.data(), 0, radDecodedData.size() * 4);

		const auto get_block_size = static_cast<gbs_f_t>(decoder->ASI_get_block_size);
		const auto decode_block = static_cast<decoder_new_f_t>(decoder->ASI_decode_block);

		uint32_t bytesConsumed = 0;
		uint32_t blockSize = 0;

		// If we have not yet established the smallest that our input buffer can be, call getblocksize once to find out
		if (minInputBufferSize == 0)
		{
			get_block_size(containerData.data(), stream_data.data(), 0, &bytesConsumed, &blockSize, &minInputBufferSize);

			// Fetch the smallest possible amount of data to populate the input buffer.
			// Future decode iterations will include this minimum buffer size in their read operation
			stream_data.resize(minInputBufferSize);
			MilesReadFileStream(stream_data.data(), stream_data.size(), &UserData);
		}

		// Make a call to the decoder to find out how much data it wants for the next decode
		get_block_size(containerData.data(), stream_data.data(), stream_data.size(), &bytesConsumed, &blockSize, &minInputBufferSize);

		if (blockSize == 0xFFFF)
			break;

		const size_t oldSize = stream_data.size();
		stream_data.resize(blockSize + minInputBufferSize);
		MilesReadFileStream(stream_data.data() + oldSize, stream_data.size() - oldSize, &UserData);

		get_block_size(containerData.data(), stream_data.data(), stream_data.size(), &bytesConsumed, &blockSize, &minInputBufferSize);

		// if we have now got a valid decode input buffer
		if (blockSize != 0xFFFF)
		{
			uint32_t decodeBytesConsumed = 0;
			uint32_t samplesDecoded = 0;

			decode_block(containerData.data(), stream_data.data(), stream_data.size(), radDecodedData.data(), radDecodedData.size() * sizeof(float), &decodeBytesConsumed, &samplesDecoded);
			
			if (decodeBytesConsumed == 0)
			{
				printf("Finished decoding.\n");
				//break;
			}

			// The decoder provides us with a non-interleaved buffer which means that
			// each channel's data is separate out into separate locations within the decode buffer
			// before writing to file, the data must be brought back together
			// e.g.: (L - left channel, R - right channel)
			// non-interleaved: LLLLLLRRRRRR
			// interleaved:     LRLRLRLRLRLR
			// https://stackoverflow.com/a/17883834
			// 
			// This may not be valid for other decoders, as miles uses parsedSizeInfo[3] to identify the decoded data format
			// and decide how to process the audio immediately after decoding
			for (int channelIdx = 0; channelIdx < channels; ++channelIdx)
			{
				const float* const channelSampleBuffer = radDecodedData.data() + (parsedSizeInfo[2] * channelIdx);

				for (uint32_t sampleIdx = 0; sampleIdx < samplesDecoded; ++sampleIdx)
				{
					// Index in the output buffer from which the channels of this sample begin
					const size_t outputIdx = static_cast<size_t>(channels) * (sampleIdx + totalFramesDecoded);

					outputBuffer[outputIdx + channelIdx] = channelSampleBuffer[sampleIdx];
				}
			}

			// Add number of samples decoded to the total to keep track of when we are done decoding the whole thing
			totalFramesDecoded += samplesDecoded;
			
			const size_t unconsumedInputBytes = stream_data.size() - decodeBytesConsumed;

			// Allocate temporary buffer to store the unconsumed input bytes until the main input buffer can be resized
			char* tempBuffer = new char[unconsumedInputBytes];

			// Copy unconsumed bytes to the temporary buffer
			memcpy(tempBuffer, stream_data.data() + decodeBytesConsumed, unconsumedInputBytes);

			// Resize the input buffer to the size of the temporary buffer
			stream_data.resize(unconsumedInputBytes);

			// Copy the bytes from the temporary buffer to the actual input buffer
			memcpy(stream_data.data(), tempBuffer, stream_data.size());

			// Release the temporary buffer
			delete[] tempBuffer;
			tempBuffer = nullptr;
		}
		else
		{
			//printf("Received blockSize = 0xFFFF after %lld decoded samples\n", totalFramesDecoded);
			break;
		}

	}

	std::unique_ptr<IO::Stream> Writer = IO::File::OpenWrite(FilePath);

	WAVEHEADER hdr;

	Writer->Write((uint8_t*)&hdr, 0, sizeof(WAVEHEADER));

	Writer->Write(reinterpret_cast<uint8_t*>(interleavedBuffer.data()), 0, interleavedBuffer.size() * sizeof(float));

	const uint64_t DataSize = interleavedBuffer.size() * sizeof(float);
	hdr.size = DataSize + 36;

	hdr.fmt.channels = channels;
	hdr.fmt.sampleRate = sampleRate;
	hdr.fmt.blockAlign = DataSize / numSamples;
	hdr.fmt.bitsPerSample = ((DataSize * 8) / numSamples)/channels;

	hdr.data.chunkSize = DataSize;

	hdr.fmt.avgBytesPerSecond = hdr.fmt.blockAlign * sampleRate;

	Writer->Seek(0, IO::SeekOrigin::Begin);

	Writer->Write((uint8_t*)&hdr, 0, sizeof(WAVEHEADER));
	Writer->Close();
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
