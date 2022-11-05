#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include <DDS.h>
#include <rtech.h>

void RpakLib::ExtractTextureName(const RpakLoadAsset& Asset, string& Name)
{
	// anything above v8 definitely doesn't have a name, so no point trying
	if (Asset.AssetVersion > 8)
		return;

	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	TextureHeaderV8 txtrHdr = Reader.Read<TextureHeaderV8>();

	if (txtrHdr.name.Index || txtrHdr.name.Offset)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, txtrHdr.name.Index, txtrHdr.name.Offset));

		Name = Reader.ReadCString();
	}
}

void RpakLib::BuildTextureInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	TextureHeader txtrHdr;

	if (Asset.AssetVersion >= 9)
	{
		TextureHeaderV9 txtrHdrV9 = Reader.Read<TextureHeaderV9>();
		txtrHdr.name = txtrHdrV9.name;
		txtrHdr.width = txtrHdrV9.width;
		txtrHdr.height = txtrHdrV9.height;
	}
	else
	{
		TextureHeaderV8 txtrHdrV8 = Reader.Read<TextureHeaderV8>();
		txtrHdr.name = txtrHdrV8.name;
		txtrHdr.width = txtrHdrV8.width;
		txtrHdr.height = txtrHdrV8.height;
	}

	string txtrName = "";
	if (txtrHdr.name.Value)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, txtrHdr.name));
		txtrName = Reader.ReadCString();
	}

	if (txtrName.Length() > 0)
		Info.Name = ExportManager::Config.GetBool("UseFullPaths") ? txtrName : IO::Path::GetFileNameWithoutExtension(txtrName);
	else
		Info.Name = string::Format("texture_0x%llx", Asset.NameHash);

	Info.Type = ApexAssetType::Image;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = string::Format("Width: %d Height %d", txtrHdr.width, txtrHdr.height);
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

uint64_t CalculateHighestMipOffset(const TextureHeader& txtrHdr, const uint8_t& mipCount)
{
	uint64_t retOffset = 0;

	uint32_t mipLevel = mipCount;
	for (int i = 1; i < mipCount; i++)
	{
		--mipLevel;
		if (txtrHdr.arraySize)
		{
			int mipWidth = 0;
			if (txtrHdr.width >> mipLevel > 1)
				mipWidth = (txtrHdr.width >> mipLevel) - 1;

			int mipHeight = 0;
			if (txtrHdr.height >> mipLevel > 1)
				mipHeight = (txtrHdr.height >> mipLevel) - 1;

			uint8_t x = s_pBytesPerPixel[txtrHdr.imageFormat].first;
			uint8_t y = s_pBytesPerPixel[txtrHdr.imageFormat].second;

			uint32_t bytesPerPixelWidth = (y + mipWidth) >> (y >> 1);
			uint32_t bytesPerPixelHeight = (y + mipHeight) >> (y >> 1);
			uint32_t sliceWidth = x * (y >> (y >> 1));

			uint32_t rowPitch = sliceWidth * bytesPerPixelWidth;
			uint32_t slicePitch = x * bytesPerPixelWidth * bytesPerPixelHeight;

			for (int a = 0; a < txtrHdr.arraySize; a++)
			{
				retOffset += ((uint64_t)(slicePitch + 15) & 0xFFFFFFF0);
			}
		}
	}

	return retOffset;
}

void RpakLib::ExtractTexture(const RpakLoadAsset& Asset, std::unique_ptr<Assets::Texture>& Texture, string& Name)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	TextureHeader txtrHdr;

	if (Asset.AssetVersion >= 9)
	{
		TextureHeaderV9 txtrHdrV9 = Reader.Read<TextureHeaderV9>();

		txtrHdr.name = txtrHdrV9.name;
		txtrHdr.width = txtrHdrV9.width;
		txtrHdr.height = txtrHdrV9.height;
		txtrHdr.imageFormat = txtrHdrV9.imageFormat;
		txtrHdr.dataSize = txtrHdrV9.dataSize;
		txtrHdr.permanentMipCount = txtrHdrV9.permanentMipCount;
		txtrHdr.streamedMipCount = txtrHdrV9.streamedMipCount;
		txtrHdr.optStreamedMipCount = txtrHdrV9.optStreamedMipCount;
	}
	else
	{
		TextureHeaderV8 txtrHdrV8 = Reader.Read<TextureHeaderV8>();

		txtrHdr.name = txtrHdrV8.name;
		txtrHdr.width = txtrHdrV8.width;
		txtrHdr.height = txtrHdrV8.height;
		txtrHdr.imageFormat = txtrHdrV8.imageFormat;
		txtrHdr.dataSize = txtrHdrV8.dataSize;
		txtrHdr.permanentMipCount = txtrHdrV8.permanentMipCount;
		txtrHdr.streamedMipCount = txtrHdrV8.streamedMipCount;
		txtrHdr.optStreamedMipCount = txtrHdrV8.optStreamedMipCount;
		txtrHdr.unkMip = txtrHdrV8.unkMip;
	}

	if (txtrHdr.name.Value)
	{
		uint64_t NameOffset = this->GetFileOffset(Asset, txtrHdr.name);

		RpakStream->SetPosition(NameOffset);

		Name = Reader.ReadCString();
	}
	else {
		Name = "";
	}

	Assets::DDSFormat ddsFormat;

	ddsFormat.Format = TxtrFormatToDXGI[txtrHdr.imageFormat];

	Texture = std::make_unique<Assets::Texture>(txtrHdr.width, txtrHdr.height, ddsFormat.Format);

	std::unique_ptr<IO::FileStream> starpakStream = nullptr;
	uint64_t starpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t optStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;

	std::unique_ptr<IO::MemoryStream> decompStarpakStream = nullptr;
	uint64_t highestMipOffset = 0;
	uint64_t blockSize = Texture->BlockSize();

	bool isVersionWithCompression = Asset.AssetVersion >= 9;

	if (isVersionWithCompression)
	{
		auto decompressBuffer = [](std::unique_ptr<IO::FileStream>& starpakStream, uint64_t bufferSize, uint64_t starpakOffset, uint64_t blockSize) -> std::unique_ptr<IO::MemoryStream>
		{
			auto Buffer = std::make_unique<uint8_t[]>(bufferSize);

			// Get location of compress starpakstream buffer.
			starpakStream->SetPosition(starpakOffset);
			starpakStream->Read(Buffer.get(), 0, bufferSize);

			// Decompress starpak texture.
			return RTech::DecompressStreamedBuffer(Buffer.get(), blockSize, (uint8_t)CompressionType::OODLE);
		};

		if (Asset.OptimalStarpakOffset != -1) // Is txtr data in opt starpak?
		{
			starpakStream = this->GetStarpakStream(Asset, true);
			highestMipOffset = optStarpakOffset;

			if (this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap.ContainsKey(Asset.OptimalStarpakOffset))
			{
				decompStarpakStream = std::move(decompressBuffer(starpakStream, this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap[Asset.OptimalStarpakOffset], optStarpakOffset, blockSize));
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

				// FIX FIX FIX
				highestMipOffset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset);
			}
		}
		else if (Asset.StarpakOffset != -1) // Is txtr data in starpak?
		{
			starpakStream = this->GetStarpakStream(Asset, false);
			highestMipOffset = starpakOffset;

			if (this->LoadedFiles[Asset.FileIndex].StarpakMap.ContainsKey(Asset.StarpakOffset))
			{
				decompStarpakStream = std::move(decompressBuffer(starpakStream, this->LoadedFiles[Asset.FileIndex].StarpakMap[Asset.StarpakOffset], optStarpakOffset, blockSize));
			}
			else
			{
				g_Logger.Warning("Starpak for asset 0x%llx is not loaded. Output may be incorrect/weird\n", Asset.NameHash);

				// FIX FIX FIX
				highestMipOffset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset);
			}
		}
		else if (Asset.RawDataIndex != -1 && Asset.RawDataIndex >= this->LoadedFiles[Asset.FileIndex].StartSegmentIndex) // Is txtr data in RPak?
		{
			highestMipOffset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset) + CalculateHighestMipOffset(txtrHdr, txtrHdr.permanentMipCount);
		}
		else
		{
			g_Logger.Warning("Asset 0x%llx has no valid data.\n", Asset.NameHash);
			return;
		}
	}
	else
	{
		if (Asset.OptimalStarpakOffset != -1) // Is txtr data in opt starpak?
		{
			starpakStream = this->GetStarpakStream(Asset, true);
			highestMipOffset = optStarpakOffset;

			if (this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap.ContainsKey(Asset.OptimalStarpakOffset))
			{
				if (!txtrHdr.unkMip)
					highestMipOffset += (this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap[Asset.OptimalStarpakOffset] - blockSize);
			}
			else
			{
				g_Logger.Warning("OptStarpak for asset 0x%llx is not loaded. Output may be incorrect/weird\n", Asset.NameHash);
				highestMipOffset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset) + (txtrHdr.dataSize - blockSize);
			}
		}
		else if (Asset.StarpakOffset != -1) // Is txtr data in starpak?
		{
			starpakStream = this->GetStarpakStream(Asset, false);
			highestMipOffset = starpakOffset;

			if (this->LoadedFiles[Asset.FileIndex].StarpakMap.ContainsKey(Asset.StarpakOffset))
			{
				if (!txtrHdr.unkMip)
					highestMipOffset += (this->LoadedFiles[Asset.FileIndex].StarpakMap[Asset.StarpakOffset] - blockSize);
			}
			else
			{
				g_Logger.Warning("Starpak for asset 0x%llx is not loaded. Output may be incorrect/weird\n", Asset.NameHash);
				highestMipOffset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset) + (txtrHdr.dataSize - blockSize);
			}
		}
		else if (Asset.RawDataIndex != -1 && Asset.RawDataIndex >= this->LoadedFiles[Asset.FileIndex].StartSegmentIndex) // Is txtr data in RPak?
		{
			if (!txtrHdr.unkMip)
				highestMipOffset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset) + (txtrHdr.dataSize - blockSize);
		}
		else
		{
			g_Logger.Warning("Asset 0x%llx has no valid data.\n", Asset.NameHash);
			return;
		}
	}

	if (decompStarpakStream)
	{
		decompStarpakStream->Read(Texture->GetPixels(), 0, blockSize);
		decompStarpakStream->Close();
	}
	else if (starpakStream)
	{
		starpakStream->SetPosition(highestMipOffset);
		starpakStream->Read(Texture->GetPixels(), 0, blockSize);
	}
	else 
	{
		RpakStream->SetPosition(highestMipOffset);
		RpakStream->Read(Texture->GetPixels(), 0, blockSize);
	}

	// unswizzle ps4 textures
	if (txtrHdr.unk == 8)
	{
		auto UTexture = std::make_unique<Assets::Texture>(txtrHdr.width, txtrHdr.height, ddsFormat.Format);

		uint8_t bpp = Texture->GetBpp();
		int vp = (bpp * 2);

		int pixbl = Texture->Pixbl();
		if (pixbl == 1)
			vp = bpp / 8;

		int blocksY = txtrHdr.height / pixbl;
		int blocksX = txtrHdr.width / pixbl;

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

	// unswizzle switch textures
	/*else if (TexHeader.compressionType == 9)
	{
		// stub for now because there's other issues
	}*/
}