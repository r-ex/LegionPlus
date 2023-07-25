#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include "RpakImageTiles.h"
#include <DDS.h>
#include <rtech.h>
// this file could probably be renamed to be more generic ui stuff in the future

void RpakLib::BuildUIIAInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	UIIAHeader TexHeader = Reader.Read<UIIAHeader>();

	string CompressionType = "";

	switch (TexHeader.Flags.CompressionType)
	{
	case 0:
		CompressionType = "NONE";
		break;
	case 1:
		CompressionType = "DEFAULT";
		break;
	case 2:
		CompressionType = "SNOWFLAKE"; // idk why it's called this tbh
		break;
	case 3: // pretty sure this isn't used by ui images rn
		CompressionType = "OODLE";
		break;
	default:
		CompressionType = "UNKNOWN";
		break;
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

	RUIImage ri = Reader.Read<RUIImage>();

	string name = string::Format("uiimage_0x%llx", Asset.NameHash);

	if (ri.NameIndex || ri.NameOffset)
		name = this->ReadStringFromPointer(Asset, { ri.NameIndex, ri.NameOffset });

	Info.Name = name;
	Info.Type = ApexAssetType::UIImage;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = string::Format("Width: %d Height %d", TexHeader.Width, TexHeader.Height);
	Info.DebugInfo = string::Format("Mode: %s (%i)", CompressionType.ToCString(), TexHeader.Flags.CompressionType);
}

void RpakLib::ExportUIIA(const RpakLoadAsset& Asset, const string& Path)
{
	string DestinationPath = IO::Path::Combine(Path, string::Format("0x%llx%s", Asset.NameHash, (const char*)ImageExtension));

	if (!Utils::ShouldWriteFile(DestinationPath)) // Ignore existing assets...
		return;

	std::unique_ptr<Assets::Texture> Texture = nullptr;

	this->ExtractUIIA(Asset, Texture);

	try
	{
		if (Texture != nullptr)
			Texture->Save(DestinationPath, ImageSaveType);
	}
	catch (...)
	{
		// Nothing, the thread attempted to export an image that already exists...
	}
}

void RpakLib::ExtractUIIA(const RpakLoadAsset& Asset, std::unique_ptr<Assets::Texture>& Texture)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	UIIAHeader TexHeader = Reader.Read<UIIAHeader>();
	int UnpackedShiftWidth = (~(unsigned __int8)((*(unsigned __int16*)&TexHeader.Flags) >> 5) & 2);
	int UnpackedShiftHeight = (~(unsigned __int8)((*(unsigned __int16*)&TexHeader.Flags) >> 6) & 2);

	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;

	std::unique_ptr<IO::MemoryStream> StarpakStream = nullptr;

	if (Asset.OptimalStarpakOffset != -1 && Asset.OptimalStarpakOffset != 0)
	{
		auto TempStream = this->GetStarpakStream(Asset, true);

		if (this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap.ContainsKey(Asset.OptimalStarpakOffset))
		{
			auto BufferSize = this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap[Asset.OptimalStarpakOffset];
			auto CompressedBuffer = std::make_unique<uint8_t[]>(BufferSize);

			TempStream->SetPosition(ActualOptStarpakOffset);
			TempStream->Read(CompressedBuffer.get(), 0, BufferSize);

			StarpakStream = RTech::DecompressStreamedBuffer(CompressedBuffer.get(), BufferSize, TexHeader.Flags.CompressionType);
		}
		else
		{
			return;
		}
	}
	else if (Asset.StarpakOffset != -1 && Asset.StarpakOffset != 0)
	{
		auto TempStream = this->GetStarpakStream(Asset, false);

		if (this->LoadedFiles[Asset.FileIndex].StarpakMap.ContainsKey(Asset.StarpakOffset))
		{
			uint64_t BufferSize = this->LoadedFiles[Asset.FileIndex].StarpakMap[Asset.StarpakOffset];
			auto CompressedBuffer = std::make_unique<uint8_t[]>(BufferSize);

			TempStream->SetPosition(ActualStarpakOffset);
			TempStream->Read(CompressedBuffer.get(), 0, BufferSize);

			StarpakStream = RTech::DecompressStreamedBuffer(CompressedBuffer.get(), BufferSize, TexHeader.Flags.CompressionType);
		}
		else
		{
			return;
		}
	}
	else if (Asset.StarpakOffset == 0)
	{
		RpakStream->SetPosition(this->LoadedFiles[Asset.FileIndex].EmbeddedStarpakOffset);

		uint64_t BufferSize = this->LoadedFiles[Asset.FileIndex].EmbeddedStarpakSize;
		auto CompressedBuffer = std::make_unique<uint8_t[]>(BufferSize);

		RpakStream->Read(CompressedBuffer.get(), 0, BufferSize);

		StarpakStream = RTech::DecompressStreamedBuffer(CompressedBuffer.get(), BufferSize, TexHeader.Flags.CompressionType);
	}

	if (Asset.RawDataIndex != -1 && Asset.RawDataIndex >= this->LoadedFiles[Asset.FileIndex].StartSegmentIndex)
	{
		//
		// All texture data is inline in rpak, we can calculate without anything else
		//

		RUIImage RImage{};
		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));
		RpakStream->Read((uint8_t*)&RImage, 0, sizeof(RUIImage));

		bool UseHighResolution = StarpakStream != nullptr;

		uint32_t WidthBlocks = ((UseHighResolution ? RImage.HighResolutionWidth : RImage.LowResolutionWidth) + UnpackedShiftWidth + 29) / 31;
		uint32_t HeightBlocks = ((UseHighResolution ? RImage.HighResolutionHeight : RImage.LowResolutionHeight) + UnpackedShiftHeight + 29) / 31;
		uint32_t TotalBlocks = WidthBlocks * HeightBlocks;
		uint32_t Width = WidthBlocks * 32;
		uint32_t Height = HeightBlocks * 32;

		uint64_t Offset = UseHighResolution ? 0 : this->GetFileOffset(Asset, RImage.BufferIndex, RImage.BufferOffset);

		if (UseHighResolution)
		{
			RpakStream = std::move(StarpakStream);
		}
		RpakStream->SetPosition(Offset);

		auto CodePoints = std::make_unique<RUIImageTile[]>(TotalBlocks);
		uint64_t CodePointsSize = TotalBlocks * sizeof(RUIImageTile);

		// Check if we have tables for the tiles, if not, generate the opcodes directly.
		if (!UseHighResolution && (!TexHeader.Flags.HasLowTable || !TexHeader.Flags.LowTableBc7))
		{
			for (uint32_t i = 0; i < TotalBlocks; i++)
			{
				CodePoints[i].Opcode = TexHeader.Flags.LowTableBc7 ? 0x41 : 0x40;
				CodePoints[i].Offset = i * (TexHeader.Flags.LowTableBc7 ? 1024 : 512);
			}
			CodePointsSize = 0;
		}
		else if (UseHighResolution && (!TexHeader.Flags.HasHighTable || !TexHeader.Flags.HighTableBc7))
		{
			for (uint32_t i = 0; i < TotalBlocks; i++)
			{
				CodePoints[i].Opcode = TexHeader.Flags.HighTableBc7 ? 0x41 : 0x40;
				CodePoints[i].Offset = i * (TexHeader.Flags.HighTableBc7 ? 1024 : 512);
			}
			CodePointsSize = 0;
		}
		else
		{
			RpakStream->Read((uint8_t*)CodePoints.get(), 0, CodePointsSize);
		}

		uint32_t NumBc1Blocks = 0;
		uint32_t NumBc7Blocks = 0;

		for (uint32_t i = 0; i < TotalBlocks; i++)
		{
			switch (CodePoints[i].Opcode)
			{
			case 0x40: // bc1 block
				NumBc1Blocks++;
				break;
			case 0x41: // bc7 block
				NumBc7Blocks++;
				break;
			case 0xC0: // This opcode requires us to copy an existing opcode.
				CodePoints[i] = CodePoints[CodePoints[i].Offset];
				break;
			default:
				break;
			}
		}

		if (NumBc1Blocks)
		{
			//
			// We are now at the Bc1 blocks.
			//

			auto Bc1Destination = std::make_unique<uint8_t[]>(TotalBlocks * 512);
			auto Bc1Texture = std::make_unique<Assets::Texture>(Width, Height, DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM);

			//
			// Copy over the blocks
			//

			for (uint32_t y = 0; y < HeightBlocks; y++)
			{
				for (uint32_t x = 0; x < WidthBlocks; x++)
				{
					RUIImageTile& Point = CodePoints[x + (y * WidthBlocks)];
					uint32_t BlockOffset = (x * 512) + ((y * WidthBlocks) * 512);

					if (Point.Opcode != 0x40)
					{
						std::memcpy(Bc1Destination.get() + BlockOffset, RUIImageTileBc1, sizeof(RUIImageTileBc1));
						continue;
					}

					RpakStream->SetPosition(Point.Offset + Offset);

					// Bc1Destination contains 32x32 BC1 512 bytes swizzled, copy to texture location.
					RpakStream->Read(Bc1Destination.get() + BlockOffset, 0, 512);
				}
			}

			//
			// Unswizzle Bc1 blocks.
			//

			uint32_t Num5 = Height / 4;
			uint32_t Num6 = Width / 4;
			constexpr uint32_t Bc1Bpp2x = 4 * 2;

			uint64_t Bc1Offset = 0;

			for (uint32_t y = 0; y < Num5; y++)
			{
				for (uint32_t x = 0; x < Num6; x++)
				{
					uint32_t mx = x;
					uint32_t my = y;

					//g_pRtech->UnswizzleBlock(x, y, Num6, 2, mx, my);
					//g_pRtech->UnswizzleBlock(mx, my, Num6, 4, mx, my);
					//g_pRtech->UnswizzleBlock(mx, my, Num6, 8, mx, my);

					// please don't mess with this code. it only just works and idk why it's being such a pain

					// --- 2 ---
					int power = 2;
					int b_2 = (mx / 2 + my * (Num6 / power)) % (2 * (Num6 / power)) /
						2 + (Num6 / power) * ((mx / 2 + my * (Num6 / power)) % (2 * (Num6 / power)) % 2) +
						2 * (Num6 / power) * ((mx / 2 + my * (Num6 / power)) / (2 * (Num6 / power)));

					int c_2 = mx % 2 + 2 * (b_2 % (Num6 / power));
					mx = b_2 / (Num6 / power);
					my = c_2 / 4;
					c_2 %= 4;

					// --- 4 ---
					power = 4;
					int b_4 = (my + mx / 2 * (Num6 / power)) % (2 * (Num6 / power)) /
						2 + (Num6 / power) * ((my + mx / 2 * (Num6 / power)) % (2 * (Num6 / power)) % 2) +
						2 * (Num6 / power) * ((my + mx / 2 * (Num6 / power)) / (2 * (Num6 / power)));

					int c_4 = mx % 2 + 2 * (b_4 / (Num6 / power));
					mx = b_4 / (Num6 / power);
					my = c_4 / 4;
					c_4 %= 4;

					// --- 8 ---
					power = 8;
					int b_8 = ((c_2 + 4 * (b_4 % (Num6 / 4))) / 8 + my * (Num6 / power)) % (2 * (Num6 / power)) /
						2 + (Num6 / power) * (((c_2 + 4 * (b_4 % (Num6 / 4))) / 8 + my * (Num6 / power)) % (2 * (Num6 / power)) % 2) +
						2 * (Num6 / power) * (((c_2 + 4 * (b_4 % (Num6 / 4))) / 8 + my * (Num6 / power)) / (2 * (Num6 / power)));

					int v85 = b_8 / (Num6 / 8);

					my = (c_4 + 4 * ((int)b_8 / (Num6 / power)));
					mx = (c_2 + 4 * (b_4 % (Num6 / 4))) % 8 + 8 * (unsigned int)(b_8 % (Num6 / power));

					uint64_t destination = Bc1Bpp2x * (my * Num6 + mx);

					std::memcpy(Bc1Texture->GetPixels() + destination, Bc1Destination.get() + Bc1Offset, Bc1Bpp2x);
					Bc1Offset += Bc1Bpp2x;
				}
			}

			Bc1Texture->ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
			Texture = std::move(Bc1Texture);
		}
		else
		{
			// We didn't have an initial texture to use, to initialize the resulting texture
			Texture = std::make_unique<Assets::Texture>(Width, Height, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
		}

		if (NumBc7Blocks)
		{
			//
			// We are now at the Bc7 blocks.
			//

			auto Bc7Destination = std::make_unique<uint8_t[]>(TotalBlocks * 1024);
			auto Bc7Texture = std::make_unique<Assets::Texture>(Width, Height, DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM);

			//
			// Copy over the blocks
			//

			for (uint32_t y = 0; y < HeightBlocks; y++)
			{
				for (uint32_t x = 0; x < WidthBlocks; x++)
				{
					RUIImageTile& Point = CodePoints[x + (y * WidthBlocks)];
					uint32_t BlockOffset = (x * 1024) + ((y * WidthBlocks) * 1024);

					if (Point.Opcode != 0x41)
					{
						std::memcpy(Bc7Destination.get() + BlockOffset, RUIImageTileBc7, sizeof(RUIImageTileBc7));
						continue;
					}

					RpakStream->SetPosition(Point.Offset + Offset);

					// Bc1Destination contains 32x32 BC7 1024 bytes swizzled, copy to texture location.
					RpakStream->Read(Bc7Destination.get() + BlockOffset, 0, 1024);
				}
			}

			//
			// Unswizzle Bc7 blocks.
			//

			uint32_t blocksH = Height / 4;
			uint32_t blocksW = Width / 4;
			constexpr uint32_t Bc7Bpp2x = 8 * 2;

			uint64_t Bc7Offset = 0;

			for (uint32_t blockI = 0; blockI < blocksH; blockI++)
			{
				int v77 = blocksW / 2;
				int v147 = blocksW / 2;
				int v78 = blockI * (blocksW / 2);
				int v79 = blocksW / 4;
				int v80 = blocksW / 8;
				for (uint32_t blockJ = 0; blockJ < blocksW; blockJ++)
				{
					uint32_t mx = blockJ;
					uint32_t my = blockI;

					/*g_pRtech->UnswizzleBlock(blockJ, blockI, blocksW, 2, mx, my);
					g_pRtech->UnswizzleBlock(mx, my, blocksW, 4, mx, my);
					g_pRtech->UnswizzleBlock(mx, my, blocksW, 8, mx, my);*/

					// this is ugly and bad, should get put back into the function calls eventually
					int v81 = (blockJ / 2 + v78) % (2 * (blocksW / 2)) / 2
						+ v77 * ((blockJ / 2 + v78) % (2 * (blocksW / 2)) % 2)
						+ 2 * (blocksW / 2) * ((blockJ / 2 + v78) / (2 * (blocksW / 2)));
					int v82 = blockJ % 2 + 2 * (v81 % v77);
					int v83 = v81 / v77;
					int v84 = v82 / 4;
					v82 %= 4;
					int v85 = (v84 + v83 / 2 * v79) % (2 * (blocksW / 4)) / 2
						+ v79 * ((v84 + v83 / 2 * v79) % (2 * (blocksW / 4)) % 2)
						+ 2 * (blocksW / 4) * ((v84 + v83 / 2 * v79) / (2 * (blocksW / 4)));
					int v86 = v83 % 2 + 2 * (v85 / v79);
					int v87 = ((v82 + 4 * (v85 % v79)) / 8 + v86 / 4 * v80) % (2 * v80) / 2
						+ v80 * (((v82 + 4 * (v85 % v79)) / 8 + v86 / 4 * v80) % (2 * v80) % 2)
						+ 2 * v80 * (((v82 + 4 * (v85 % v79)) / 8 + v86 / 4 * v80) / (2 * v80));
					int v88 = Bc7Bpp2x
						* ((v82 + 4 * (v85 % v79)) % 8
							+ 8 * (unsigned int)(v87 % v80)
							+ blocksW * (v86 % 4 + 4 * ((int)v87 / v80)));

					//uint64_t destination = Bc7Bpp2x * (my * blocksW + mx);

					std::memcpy(Bc7Texture->GetPixels() + v88, Bc7Destination.get() + Bc7Offset, Bc7Bpp2x);
					Bc7Offset += Bc7Bpp2x;
				}
			}

			Bc7Texture->ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);

			if (NumBc1Blocks)
			{
				// Stitch the 32x32 uncompressed tiles to the final image.
				for (uint32_t y = 0; y < HeightBlocks; y++)
				{
					for (uint32_t x = 0; x < WidthBlocks; x++)
					{
						RUIImageTile& Point = CodePoints[x + (y * WidthBlocks)];

						if (Point.Opcode == 0x41)
						{
							Texture->CopyTextureSlice(Bc7Texture, { (x * 32), (y * 32), 32, 32 }, (x * 32), (y * 32));
						}
					}
				}
			}
			else
			{
				// We are the final image
				Texture = std::move(Bc7Texture);
			}
		}

		uint16_t OutWidth = UseHighResolution ? RImage.HighResolutionWidth : RImage.LowResolutionWidth;
		uint16_t OutHeight = UseHighResolution ? RImage.HighResolutionHeight : RImage.LowResolutionHeight;
		auto FinalTexture = std::make_unique<Assets::Texture>(OutWidth, OutHeight, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
		
		for (uint32_t y = 0; y < HeightBlocks; y++)
		{
			uint32_t SizeY = y * 31 + 30 < OutHeight ? 31 : OutHeight - y * 31;
			for (uint32_t x = 0; x < WidthBlocks; x++)
			{
				uint32_t SizeX = x * 31 + 30 < OutWidth ? 31 : OutWidth - x * 31;
				FinalTexture->CopyTextureSlice(Texture, { (x * 32), (y * 32), SizeX, SizeY }, (x * 31), (y * 31));
			}
		}
		Texture = std::move(FinalTexture);
	}
}