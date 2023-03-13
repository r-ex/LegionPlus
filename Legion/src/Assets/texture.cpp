#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include <DDS.h>
#include <rtech.h>

void RpakLib::ExtractTextureName(const RpakLoadAsset& asset, string& name)
{
	// anything above v8 definitely doesn't have a name, so no point trying
	if (asset.AssetVersion > 8)
		return;

	auto rpakStream = this->GetFileStream(asset);
	IO::BinaryReader reader = IO::BinaryReader(rpakStream.get(), true);

	rpakStream->SetPosition(this->GetFileOffset(asset, asset.SubHeaderIndex, asset.SubHeaderOffset));

	TextureHeaderV8 txtrHdr = reader.Read<TextureHeaderV8>();

	if (txtrHdr.name.Index || txtrHdr.name.Offset)
	{
		rpakStream->SetPosition(this->GetFileOffset(asset, txtrHdr.name.Index, txtrHdr.name.Offset));

		name = reader.ReadCString();
	}
}

void RpakLib::BuildTextureInfo(const RpakLoadAsset& asset, ApexAsset& assetInfo)
{
	auto rpakStream = this->GetFileStream(asset);
	IO::BinaryReader reader = IO::BinaryReader(rpakStream.get(), true);

	rpakStream->SetPosition(this->GetFileOffset(asset, asset.SubHeaderIndex, asset.SubHeaderOffset));

	TextureHeader txtrHdr;

	if (asset.AssetVersion >= 9)
	{
		TextureHeaderV9 txtrHdrV9 = reader.Read<TextureHeaderV9>();
		txtrHdr.name = txtrHdrV9.name;
		txtrHdr.width = txtrHdrV9.width;
		txtrHdr.height = txtrHdrV9.height;
	}
	else
	{
		TextureHeaderV8 txtrHdrV8 = reader.Read<TextureHeaderV8>();
		txtrHdr.name = txtrHdrV8.name;
		txtrHdr.width = txtrHdrV8.width;
		txtrHdr.height = txtrHdrV8.height;
	}

	string txtrName = "";
	if (txtrHdr.name.Value)
	{
		rpakStream->SetPosition(this->GetFileOffset(asset, txtrHdr.name));
		txtrName = reader.ReadCString();
	}

	if (txtrName.Length() > 0)
		assetInfo.Name = ExportManager::Config.GetBool("UseFullPaths") ? txtrName : IO::Path::GetFileNameWithoutExtension(txtrName);
	else
		assetInfo.Name = string::Format("texture_0x%llx", asset.NameHash);

	assetInfo.Type = ApexAssetType::Image;
	assetInfo.Status = ApexAssetStatus::Loaded;
	assetInfo.Info = string::Format("Width: %d Height %d", txtrHdr.width, txtrHdr.height);
}

void RpakLib::ExportTexture(const RpakLoadAsset& asset, const string& path, bool includeImageNames, string nameOverride, bool normalRecalculate)
{
	string destName = nameOverride == "" ? string::Format("0x%llx%s", asset.NameHash, (const char*)ImageExtension) : nameOverride;
	string destPath = IO::Path::Combine(path, destName);

	std::unique_ptr<Assets::Texture> texture = nullptr;
	string name;

	this->ExtractTexture(asset, texture, name);

	if (includeImageNames && name.Length() > 0)
		destPath = IO::Path::Combine(path, string::Format("%s%s", IO::Path::GetFileNameWithoutExtension(name).ToCString(), (const char*)ImageExtension));

	if (!Utils::ShouldWriteFile(destPath))
		return;

	try
	{
		if (texture)
		{
			if (normalRecalculate)
			{
				NormalRecalcType_t NormalRecalcType = (NormalRecalcType_t)ExportManager::Config.Get<System::SettingType::Integer>("NormalRecalcType");

				Assets::TranscodeType Type = Assets::TranscodeType::NormalMapBC5OpenGl;

				switch (NormalRecalcType)
				{
				case NormalRecalcType_t::None:
					break;
				case NormalRecalcType_t::DirectX:
					texture->Transcode(Assets::TranscodeType::NormalMapBC5);
					break;
				case NormalRecalcType_t::OpenGl:
					texture->Transcode(Assets::TranscodeType::NormalMapBC5OpenGl);
					break;
				}
			}
			texture->Save(destPath, ImageSaveType);
		}
	}
	catch (...)
	{
		// Nothing, the thread attempted to export an image that already exists...
	}
}

#undef max
constexpr uint32_t ALIGNMENT_SIZE = 15;
uint64_t CalculateHighestMipOffset(const TextureHeader& txtrHdr, const uint8_t& mipCount)
{
	uint64_t retOffset = 0;

	for (int mipLevel = mipCount - 1; mipLevel >= 1; mipLevel--)
	{
		int mipWidth = std::max(0, (txtrHdr.width >> mipLevel) - 1);
		int mipHeight = std::max(0, (txtrHdr.height >> mipLevel) - 1);

		const uint8_t x = s_pBytesPerPixel[txtrHdr.imageFormat].first;
		const uint8_t y = s_pBytesPerPixel[txtrHdr.imageFormat].second;

		uint32_t bppWidth = (y + mipWidth) >> (y >> 1);
		uint32_t bppHeight = (y + mipHeight) >> (y >> 1);
		uint32_t sliceWidth = x * (y >> (y >> 1));

		uint32_t rowPitch = sliceWidth * bppWidth;
		uint32_t slicePitch = x * bppWidth * bppHeight;

		for (int a = 0; a < txtrHdr.arraySize; a++)
		{
			retOffset += ((uint64_t)(slicePitch + ALIGNMENT_SIZE) & ~ALIGNMENT_SIZE);
		}
	}

	return retOffset;
}

void RpakLib::ExtractTexture(const RpakLoadAsset& asset, std::unique_ptr<Assets::Texture>& texture, string& name)
{
	auto rpakStream = this->GetFileStream(asset);
	IO::BinaryReader reader = IO::BinaryReader(rpakStream.get(), true);

	rpakStream->SetPosition(this->GetFileOffset(asset, asset.SubHeaderIndex, asset.SubHeaderOffset));

	TextureHeader txtrHdr;

	if (asset.AssetVersion >= 9)
	{
		TextureHeaderV9 txtrHdrV9 = reader.Read<TextureHeaderV9>();

		txtrHdr.name = txtrHdrV9.name;
		txtrHdr.width = txtrHdrV9.width;
		txtrHdr.height = txtrHdrV9.height;
		txtrHdr.imageFormat = txtrHdrV9.imageFormat;
		txtrHdr.dataSize = txtrHdrV9.dataSize;
		txtrHdr.permanentMipCount = txtrHdrV9.permanentMipCount;
		txtrHdr.streamedMipCount = txtrHdrV9.streamedMipCount;
		txtrHdr.optStreamedMipCount = txtrHdrV9.optStreamedMipCount;
		txtrHdr.arraySize = txtrHdrV9.arraySize;
	}
	else
	{
		TextureHeaderV8 txtrHdrV8 = reader.Read<TextureHeaderV8>();

		txtrHdr.name = txtrHdrV8.name;
		txtrHdr.width = txtrHdrV8.width;
		txtrHdr.height = txtrHdrV8.height;
		txtrHdr.imageFormat = txtrHdrV8.imageFormat;
		txtrHdr.dataSize = txtrHdrV8.dataSize;
		txtrHdr.permanentMipCount = txtrHdrV8.permanentMipCount;
		txtrHdr.streamedMipCount = txtrHdrV8.streamedMipCount;
		txtrHdr.optStreamedMipCount = txtrHdrV8.optStreamedMipCount;
		txtrHdr.unkMip = txtrHdrV8.unkMip;
		txtrHdr.unk = txtrHdrV8.unk;
		txtrHdr.arraySize = txtrHdrV8.arraySize;
	}

	if (txtrHdr.name.Value)
	{
		uint64_t NameOffset = this->GetFileOffset(asset, txtrHdr.name);

		rpakStream->SetPosition(NameOffset);

		name = reader.ReadCString();
	}
	else {
		name = "";
	}

	Assets::DDSFormat ddsFormat;

	ddsFormat.Format = TxtrFormatToDXGI[txtrHdr.imageFormat];

	texture = std::make_unique<Assets::Texture>(txtrHdr.width, txtrHdr.height, ddsFormat.Format);

	std::unique_ptr<IO::FileStream> starpakStream = nullptr;
	uint64_t starpakOffset = asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t optStarpakOffset = asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;

	std::unique_ptr<IO::MemoryStream> decompStarpakStream = nullptr;
	uint64_t highestMipOffset = 0;
	uint64_t blockSize = texture->BlockSize();

	bool isVersionWithCompression = asset.AssetVersion >= 9;

	if (isVersionWithCompression)
	{
		auto decompressBuffer = [](std::unique_ptr<IO::FileStream>& starpakStream, uint64_t bufferSize, uint64_t starpakOffset, uint64_t blockSize) -> std::unique_ptr<IO::MemoryStream>
		{
			auto Buffer = new uint8_t[bufferSize];

			// Get location of compress starpakstream buffer.
			starpakStream->SetPosition(starpakOffset);
			starpakStream->Read(Buffer, 0, bufferSize);

			// Decompress starpak texture.
			return RTech::DecompressStreamedBuffer(Buffer, blockSize, (uint8_t)CompressionType::OODLE);
		};

		if (asset.OptimalStarpakOffset != -1) // Is txtr data in opt starpak?
		{
			starpakStream = this->GetStarpakStream(asset, true);
			highestMipOffset = optStarpakOffset;

			if (this->LoadedFiles[asset.FileIndex].OptimalStarpakMap.ContainsKey(asset.OptimalStarpakOffset))
			{
				decompStarpakStream = std::move(decompressBuffer(starpakStream, this->LoadedFiles[asset.FileIndex].OptimalStarpakMap[asset.OptimalStarpakOffset], optStarpakOffset, blockSize));
			}
			else
			{
				///
				// Support for finding the next highest quality mip that has a valid data source (i.e. no missing starpak)
				// This should be relatively easy
				///
				g_Logger.Warning("OptStarpak for asset 0x%llx is not loaded. Output may be incorrect/weird\n", asset.NameHash);

				///
				// Use non-streamed data instead to make up for the missing starpak this data WILL NOT fit the intended higher quality image size
				// so the resulting image will be totally messed up
				//
				// ???: why didnt this originally just check if it also had non-opt starpak offsets and use that for the image?
				//      then at least the image would be higher quality than the highest permanent mip
				///

				// FIX FIX FIX
				starpakStream.release();
				highestMipOffset = this->GetFileOffset(asset, asset.RawDataIndex, asset.RawDataOffset);
			}
		}
		else if (asset.StarpakOffset != -1) // Is txtr data in starpak?
		{
			starpakStream = this->GetStarpakStream(asset, false);
			highestMipOffset = starpakOffset;

			if (this->LoadedFiles[asset.FileIndex].StarpakMap.ContainsKey(asset.StarpakOffset))
			{
				decompStarpakStream = std::move(decompressBuffer(starpakStream, this->LoadedFiles[asset.FileIndex].StarpakMap[asset.StarpakOffset], starpakOffset, blockSize));
			}
			else
			{
				g_Logger.Warning("Starpak for asset 0x%llx is not loaded. Output may be incorrect/weird\n", asset.NameHash);

				// FIX FIX FIX
				starpakStream.release();
				highestMipOffset = this->GetFileOffset(asset, asset.RawDataIndex, asset.RawDataOffset);
			}
		}
		else if (asset.RawDataIndex != -1 && asset.RawDataIndex >= this->LoadedFiles[asset.FileIndex].StartSegmentIndex) // Is txtr data in RPak?
		{
			highestMipOffset = this->GetFileOffset(asset, asset.RawDataIndex, asset.RawDataOffset) + CalculateHighestMipOffset(txtrHdr, txtrHdr.permanentMipCount);
		}
		else
		{
			g_Logger.Warning("Asset 0x%llx has no valid data.\n", asset.NameHash);
			return;
		}
	}
	else
	{
		if (asset.OptimalStarpakOffset != -1) // Is txtr data in opt starpak?
		{
			starpakStream = this->GetStarpakStream(asset, true);
			highestMipOffset = optStarpakOffset;

			if (this->LoadedFiles[asset.FileIndex].OptimalStarpakMap.ContainsKey(asset.OptimalStarpakOffset))
			{
				highestMipOffset += (this->LoadedFiles[asset.FileIndex].OptimalStarpakMap[asset.OptimalStarpakOffset] - blockSize);
			}
			else
			{
				g_Logger.Warning("OptStarpak for asset 0x%llx is not loaded. Output may be incorrect/weird\n", asset.NameHash);
				starpakStream.release();
				highestMipOffset = this->GetFileOffset(asset, asset.RawDataIndex, asset.RawDataOffset) + (txtrHdr.dataSize - blockSize);
			}
		}
		else if (asset.StarpakOffset != -1) // Is txtr data in starpak?
		{
			starpakStream = this->GetStarpakStream(asset, false);
			highestMipOffset = starpakOffset;

			if (this->LoadedFiles[asset.FileIndex].StarpakMap.ContainsKey(asset.StarpakOffset))
			{
				if (!txtrHdr.unkMip)
					highestMipOffset += (this->LoadedFiles[asset.FileIndex].StarpakMap[asset.StarpakOffset] - blockSize);
			}
			else
			{
				g_Logger.Warning("Starpak for asset 0x%llx is not loaded. Output may be incorrect/weird\n", asset.NameHash);
				starpakStream.release();
				highestMipOffset = this->GetFileOffset(asset, asset.RawDataIndex, asset.RawDataOffset) + (txtrHdr.dataSize - blockSize);
			}
		}
		else if (asset.RawDataIndex != -1 && asset.RawDataIndex >= this->LoadedFiles[asset.FileIndex].StartSegmentIndex) // Is txtr data in RPak?
		{
			if (!txtrHdr.unkMip)
				highestMipOffset = this->GetFileOffset(asset, asset.RawDataIndex, asset.RawDataOffset) + (txtrHdr.dataSize - blockSize);
			else
				highestMipOffset = this->GetFileOffset(asset, asset.RawDataIndex, asset.RawDataOffset) + CalculateHighestMipOffset(txtrHdr, txtrHdr.permanentMipCount);
		}
		else
		{
			g_Logger.Warning("Asset 0x%llx has no valid data.\n", asset.NameHash);
			return;
		}
	}

	if (decompStarpakStream)
	{
		decompStarpakStream->Read(texture->GetPixels(), 0, blockSize);
		decompStarpakStream->Close();
	}
	else if (starpakStream)
	{
		starpakStream->SetPosition(highestMipOffset);
		starpakStream->Read(texture->GetPixels(), 0, blockSize);
	}
	else 
	{
		rpakStream->SetPosition(highestMipOffset);
		rpakStream->Read(texture->GetPixels(), 0, blockSize);
	}

	// unswizzle ps4 textures
	if (txtrHdr.unk == 8)
	{
		auto uTexture = std::make_unique<Assets::Texture>(txtrHdr.width, txtrHdr.height, ddsFormat.Format);

		uint8_t bpp = texture->GetBpp();
		int vp = (bpp * 2);

		int pixbl = texture->Pixbl();
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

					std::memcpy(tempArray, texture->GetPixels() + tmp, vp);
					tmp += vp;

					if (j * 8 + v1 < blocksX && i * 8 + v0 < blocksY)
					{
						int dstIdx = (vp) * ((i * 8 + v0) * blocksX + j * 8 + v1);
						std::memcpy(uTexture->GetPixels() + dstIdx, tempArray, vp);
					}
				}
			}
		}

		texture = std::move(uTexture);
	}

	// unswizzle switch textures
	/*else if (TexHeader.compressionType == 9)
	{
		// stub for now because there's other issues
	}*/
}