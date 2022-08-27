#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include <DDS.h>
#include <rtech.h>

void RpakLib::BuildTextureInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	TextureHeader TexHeader = Reader.Read<TextureHeader>();

	string Name = "";

	uint16_t TextureWidth = 0;
	uint16_t TextureHeight = 0;
	uint32_t NameIndex = 0;
	uint32_t NameOffset = 0;

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	switch (Asset.AssetVersion)
	{
	case 9:
	{
		auto TexHeader = Reader.Read<TextureHeaderV9>();

		TextureWidth = TexHeader.Width;
		TextureHeight = TexHeader.Height;
		NameIndex = TexHeader.NameIndex;
		NameOffset = TexHeader.NameOffset;
		break;
	}
	default:
	{
		auto TexHeader = Reader.Read<TextureHeader>();

		TextureWidth = TexHeader.Width;
		TextureHeight = TexHeader.Height;
		NameIndex = TexHeader.NameIndex;
		NameOffset = TexHeader.NameOffset;
		break;
	}
	}

	if (NameIndex || NameOffset)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, NameIndex, NameOffset));

		Name = Reader.ReadCString();
	}

	if (Name.Length() > 0)
		Info.Name = ExportManager::Config.GetBool("UseFullPaths") ? Name : IO::Path::GetFileNameWithoutExtension(Name);
	else
		Info.Name = string::Format("texture_0x%llx", Asset.NameHash);

	Info.Type = ApexAssetType::Image;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = string::Format("Width: %d Height %d", TextureWidth, TextureHeight);
}

void RpakLib::ExportTexture(const RpakLoadAsset& Asset, const string& Path, bool IncludeImageNames, string NameOverride, bool NormalRecalculate)
{
	string DestinationName = NameOverride == "" ? string::Format("0x%llx%s", Asset.NameHash, (const char*)ImageExtension) : NameOverride;
	string DestinationPath = IO::Path::Combine(Path, DestinationName);

	std::unique_ptr<Assets::Texture> Texture = nullptr;
	string Name;

	this->ExtractTexture(Asset, Texture, Name);

	if (IncludeImageNames && Name.Length() > 0)
		DestinationPath = IO::Path::Combine(Path, string::Format("%s%s", IO::Path::GetFileNameWithoutExtension(Name).ToCString(), (const char*)ImageExtension));

	if (!Utils::ShouldWriteFile(DestinationPath))
		return;

	try
	{
		if (Texture != nullptr)
		{
			if (NormalRecalculate)
			{
				NormalRecalcType_t NormalRecalcType = (NormalRecalcType_t)ExportManager::Config.Get<System::SettingType::Integer>("NormalRecalcType");

				Assets::TranscodeType Type = Assets::TranscodeType::NormalMapBC5OpenGl;

				switch (NormalRecalcType)
				{
				case NormalRecalcType_t::None:
					break;
				case NormalRecalcType_t::DirectX:
					Texture->Transcode(Assets::TranscodeType::NormalMapBC5);
					break;
				case NormalRecalcType_t::OpenGl:
					Texture->Transcode(Assets::TranscodeType::NormalMapBC5OpenGl);
					break;
				}
			}
			Texture->Save(DestinationPath, ImageSaveType);
		}
	}
	catch (...)
	{
		// Nothing, the thread attempted to export an image that already exists...
	}
}

void RpakLib::ExtractTexture(const RpakLoadAsset& Asset, std::unique_ptr<Assets::Texture>& Texture, string& Name)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	TextureHeader TexHeader = Reader.Read<TextureHeader>();

	if (Asset.AssetVersion >= 9)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
		auto TexHeaderV9 = Reader.Read<TextureHeaderV9>();

		TexHeader.NameIndex = TexHeaderV9.NameIndex;
		TexHeader.NameOffset = TexHeaderV9.NameOffset;
		TexHeader.Width = TexHeaderV9.Width;
		TexHeader.Height = TexHeaderV9.Height;
		TexHeader.Format = TexHeaderV9.Format;
		TexHeader.DataSize = TexHeaderV9.DataSize;
	}

	Assets::DDSFormat Fmt;

	Fmt.Format = TxtrFormatToDXGI[TexHeader.Format];

	if (TexHeader.NameIndex || TexHeader.NameOffset)
	{
		uint64_t NameOffset = this->GetFileOffset(Asset, TexHeader.NameIndex, TexHeader.NameOffset);

		RpakStream->SetPosition(NameOffset);

		Name = Reader.ReadCString();
	}
	else {
		Name = "";
	}

	Texture = std::make_unique<Assets::Texture>(TexHeader.Width, TexHeader.Height, Fmt.Format);

	uint64_t BlockSize = Texture->BlockSize();

	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;

	uint64_t Offset = 0;
	std::unique_ptr<IO::FileStream> StarpakStream = nullptr;
	bool bStreamed = false;

	if (Asset.OptimalStarpakOffset != -1)
	{
		Offset = ActualOptStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, true);

		if (this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap.ContainsKey(Asset.OptimalStarpakOffset))
		{
			if (Asset.AssetVersion != 9)
			{
				Offset += (this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap[Asset.OptimalStarpakOffset] - BlockSize);
				bStreamed = true;
			}
			else
			{
				auto BufferSize = this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap[Asset.OptimalStarpakOffset];
				auto Buffer = std::make_unique<uint8_t[]>(BufferSize);

				// Get location of compress starpakstream buffer.
				auto TempStream = this->GetStarpakStream(Asset, true);
				TempStream->SetPosition(ActualOptStarpakOffset);
				TempStream->Read(Buffer.get(), 0, BufferSize);

				// Decompress starpak texture.
				auto BufferResult = RTech::DecompressStreamedBuffer(Buffer.get(), BlockSize, (uint8_t)CompressionType::OODLE);
				BufferResult->Read(Texture->GetPixels(), 0, BlockSize);
				BufferResult.get()->Close();

				return;
			}
		}
		else
		{
			///
			// Support for finding the next highest quality mip that has a valid data source (i.e. no missing starpak)
			// This should be relatively easy
			///
			g_Logger.Warning("OptStarpak for asset 0x%llx is not loaded. Output may be incorrect/weird\n", Asset.NameHash);

			///
			// Use non-streamed data instead to make up for the missing starpak this data WILL NOT fit the intended higher quality image size
			// so the resulting image will be totally messed up
			//
			// ???: why didnt this originally just check if it also had non-opt starpak offsets and use that for the image?
			//      then at least the image would be higher quality than the highest permanent mip
			///
			if (Asset.AssetVersion != 9)
				Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset) + (TexHeader.DataSize - BlockSize);
			else if (Asset.Version == RpakGameVersion::R2TT)
				Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset);
			else
				Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset);
		}
	}
	else if (Asset.StarpakOffset != -1)
	{
		Offset = ActualStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, false);

		if (this->LoadedFiles[Asset.FileIndex].StarpakMap.ContainsKey(Asset.StarpakOffset))
		{
			if (Asset.AssetVersion != 9)
			{
				Offset += (this->LoadedFiles[Asset.FileIndex].StarpakMap[Asset.StarpakOffset] - BlockSize);
				bStreamed = true;
			}
			else if (Asset.Version == RpakGameVersion::R2TT) 
			{
				bStreamed = true;
			}
			else
			{
				auto BufferSize = this->LoadedFiles[Asset.FileIndex].StarpakMap[Asset.StarpakOffset];
				auto Buffer = std::make_unique<uint8_t[]>(BufferSize);

				// Get location of compress starpakstream buffer.
				auto TempStream = this->GetStarpakStream(Asset, false);
				TempStream->SetPosition(ActualStarpakOffset);
				TempStream->Read(Buffer.get(), 0, BufferSize);

				// Decompress starpak texture.
				auto BufferResult = RTech::DecompressStreamedBuffer(Buffer.get(), BlockSize, (uint8_t)CompressionType::OODLE);
				BufferResult->Read(Texture->GetPixels(), 0, BlockSize);
				BufferResult.get()->Close();

				return;
			}
		}
		else
		{
			g_Logger.Warning("Starpak for asset 0x%llx is not loaded. Output may be incorrect/weird\n", Asset.NameHash);

			if (Asset.AssetVersion != 9)
				Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset) + (TexHeader.DataSize - BlockSize);
			else if (Asset.Version == RpakGameVersion::R2TT)
				Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset);
			else
				this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset);
		}
	}
	else if (Asset.RawDataIndex != -1 && Asset.RawDataIndex >= this->LoadedFiles[Asset.FileIndex].StartSegmentIndex)
	{
		//
		// All texture data is inline in rpak, we can calculate without anything else
		//

		if (Asset.AssetVersion != 9)
			Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset) + (TexHeader.DataSize - BlockSize);
		else if (Asset.Version == RpakGameVersion::R2TT)
			Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset);
		else
			Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset);
	}
	else
	{
		// No bank found
		return;
	}

	if (bStreamed)
	{
		StarpakStream->SetPosition(Offset);
		StarpakStream->Read(Texture->GetPixels(), 0, BlockSize);
	}
	else {
		RpakStream->SetPosition(Offset);
		RpakStream->Read(Texture->GetPixels(), 0, BlockSize);
	}

	// this is a kinda dumb check but i'm assuming we'll never see rpak v6 on anything other than ps4
	if (Asset.Version == RpakGameVersion::R2TT)
	{
		auto UTexture = std::make_unique<Assets::Texture>(TexHeader.Width, TexHeader.Height, Fmt.Format);

		uint8_t bpp = Texture->GetBpp();
		int vp = (bpp * 2);

		int pixbl = Texture->Pixbl();
		if (pixbl == 1)
			vp = bpp / 8;

		int blocksY = TexHeader.Height / pixbl;
		int blocksX = TexHeader.Width / pixbl;

		uint8_t tempArray[16]{};
		int tmp = 0;

		for (int i = 0; i < (blocksY + 7) / 8; i++)
		{
			for (int j = 0; j < (blocksX + 7) / 8; j++)
			{
				for (int k = 0; k < 64; k++)
				{
					int mr = Assets::Texture::Morton(k, 8, 8);
					int v0 = mr / 8;
					int v1 = mr % 8;

					std::memcpy(tempArray, Texture->GetPixels() + tmp, vp);
					tmp += vp;

					if (j * 8 + v1 < blocksX && i * 8 + v0 < blocksY)
					{
						int dstIdx = (vp) * ((i * 8 + v0) * blocksX + j * 8 + v1);
						std::memcpy(UTexture->GetPixels() + dstIdx, tempArray, vp);
					}
				}
			}
		}

		Texture = std::move(UTexture);
	}
}