#include "stdafx.h"
#include "Texture.h"
#include "DDS.h"
#include "MathHelper.h"
#include <wincodec.h>

#include "..\cppkore_incl\DirectXTex\DirectXTex.h"

#if _WIN64
#if _DEBUG
#pragma comment(lib, "..\\cppkore_libs\\DirectXTex\\DirectXTex_x64d.lib")
#else
#pragma comment(lib, "..\\cppkore_libs\\DirectXTex\\DirectXTex_x64r.lib")
#endif
#else
#error DirectXTex doesn't support non x64 builds yet
#endif

#define InternalScratchImage ((DirectX::ScratchImage*)this->DirectXImage)

namespace Assets
{
	Texture::Texture(void* ScratchImage)
		: DirectXImage(ScratchImage)
	{
	}

	Texture::Texture(uint32_t Width, uint32_t Height)
		: Texture(Width, Height, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM)
	{
	}

	Texture::Texture(uint32_t Width, uint32_t Height, DXGI_FORMAT Format)
		: DirectXImage(nullptr)
	{
		this->DirectXImage = new DirectX::ScratchImage();

		DirectX::TexMetadata Data{};
		Data.width = Width;
		Data.height = Height;
		Data.depth = 1;
		Data.arraySize = 1;
		Data.mipLevels = 1;
		Data.dimension = DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE2D;
		Data.format = Format;
		InternalScratchImage->Initialize(Data);
	}

	Texture::~Texture()
	{
		if (DirectXImage)
			delete InternalScratchImage;
		DirectXImage = nullptr;
	}

	Texture::Texture(Texture&& Rhs) noexcept
	{
		this->DirectXImage = Rhs.DirectXImage;
		Rhs.DirectXImage = nullptr;
	}

	const uint32_t Texture::Width() const
	{
		return (uint32_t)InternalScratchImage->GetMetadata().width;
	}

	const uint32_t Texture::Height() const
	{
		return (uint32_t)InternalScratchImage->GetMetadata().height;
	}

	const uint32_t Texture::Pitch() const
	{
		return (uint32_t)InternalScratchImage->GetImages()->rowPitch;
	}

	const uint32_t Texture::MipCount() const
	{
		return (uint32_t)InternalScratchImage->GetMetadata().mipLevels;
	}

	const uint32_t Texture::BlockSize() const
	{
		return (uint32_t)InternalScratchImage->GetPixelsSize();
	}

	uint8_t* Texture::GetPixels()
	{
		return InternalScratchImage->GetImages()->pixels;
	}

	const uint8_t* Texture::GetPixels() const
	{
		return InternalScratchImage->GetImages()->pixels;
	}

	const DXGI_FORMAT Texture::Format() const
	{
		return InternalScratchImage->GetMetadata().format;
	}

	const uint8_t Texture::GetBpp() const
	{
		DXGI_FORMAT format = this->Format();
		switch (format)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 128;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 96;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_Y416:
		case DXGI_FORMAT_Y210:
		case DXGI_FORMAT_Y216:
			return 64;

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_AYUV:
		case DXGI_FORMAT_Y410:
		case DXGI_FORMAT_YUY2:
			return 32;

		case DXGI_FORMAT_P010:
		case DXGI_FORMAT_P016:
		case DXGI_FORMAT_V408:
			return 24;

		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_A8P8:
		case DXGI_FORMAT_B4G4R4A4_UNORM:
		case DXGI_FORMAT_P208:
		case DXGI_FORMAT_V208:
			return 16;

		case DXGI_FORMAT_NV12:
		case DXGI_FORMAT_420_OPAQUE:
		case DXGI_FORMAT_NV11:
			return 12;

		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
		case DXGI_FORMAT_AI44:
		case DXGI_FORMAT_IA44:
		case DXGI_FORMAT_P8:
			return 8;

		case DXGI_FORMAT_R1_UNORM:
			return 1;

		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return 4;

		default:
			return 0;
		}
	}

	void Texture::ConvertToFormat(DXGI_FORMAT Format)
	{
		if (InternalScratchImage->GetMetadata().format == Format)
			return;

		// If we need to decompress, and our target is not compressed, we can skip a loop here by directly going to the target
		DXGI_FORMAT DecompressFmt = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		if (!DirectX::IsCompressed(Format))
			DecompressFmt = Format;

		// First, check if we are currently compressed...
		if (DirectX::IsCompressed(InternalScratchImage->GetMetadata().format))
		{
			auto TemporaryImage = std::make_unique<DirectX::ScratchImage>();
			auto Result = DirectX::Decompress(InternalScratchImage->GetImages(), InternalScratchImage->GetImageCount(), InternalScratchImage->GetMetadata(), DecompressFmt, *TemporaryImage);

			if (SUCCEEDED(Result))
			{
				delete InternalScratchImage;
				DirectXImage = TemporaryImage.release();
			}
			else
			{
				throw std::exception("Failed to decompress the image format");
			}
		}

		// If the target is compressed, compress the new data
		if (DirectX::IsCompressed(Format))
		{
			auto TemporaryImage = std::make_unique<DirectX::ScratchImage>();
			auto Result = DirectX::Compress(InternalScratchImage->GetImages(), InternalScratchImage->GetImageCount(), InternalScratchImage->GetMetadata(), Format, DirectX::TEX_COMPRESS_FLAGS::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *TemporaryImage);

			if (SUCCEEDED(Result))
			{
				delete InternalScratchImage;
				DirectXImage = TemporaryImage.release();
			}
			else
			{
				throw std::exception("Failed to compress the image format");
			}
		}
		else if (InternalScratchImage->GetMetadata().format != Format)
		{
			auto TemporaryImage = std::make_unique<DirectX::ScratchImage>();
			auto Result = DirectX::Convert(InternalScratchImage->GetImages(), InternalScratchImage->GetImageCount(), InternalScratchImage->GetMetadata(), Format, DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *TemporaryImage);

			if (SUCCEEDED(Result))
			{
				delete InternalScratchImage;
				DirectXImage = TemporaryImage.release();
			}
			else
			{
				throw std::exception("Failed to convert the image format");
			}
		}
	}

	void Texture::Transcode(TranscodeType Type)
	{
		// Depending on the transcode type, we need to ensure a format...
		switch (Type)
		{
		case TranscodeType::NormalMapBC5:
			this->Transcoder_NormalMapBC5();
			break;
		case TranscodeType::NormalMapBC5OpenGl:
			this->Transcoder_NormalMapBC5OpenGl();
			break;
		}
	}

	void Texture::Save(const string& File)
	{
		if (File.EndsWith(".dds"))
			this->Save(File, SaveFileType::Dds);
		else if (File.EndsWith(".tga"))
			this->Save(File, SaveFileType::Tga);
		else if (File.EndsWith(".tiff"))
			this->Save(File, SaveFileType::Tiff);
		else if (File.EndsWith(".hdr"))
			this->Save(File, SaveFileType::Hdr);
		else if (File.EndsWith(".png"))
			this->Save(File, SaveFileType::Png);
		else if (File.EndsWith(".bmp"))
			this->Save(File, SaveFileType::Bmp);
		else if (File.EndsWith(".jpg"))
			this->Save(File, SaveFileType::Jpeg);
		else if (File.EndsWith(".jxr"))
			this->Save(File, SaveFileType::Jxr);
		else if (File.EndsWith(".gif"))
			this->Save(File, SaveFileType::Gif);
		else
			throw std::exception("Unknown image file extension");
	}

	void Texture::Save(const string& File, SaveFileType Type)
	{
		this->EnsureFormatForType(Type);
		HRESULT SaveResult = 0;

		auto OutputWide = File.ToWString();

		switch (Type)
		{
		case SaveFileType::Dds:
			SaveResult = DirectX::SaveToDDSFile(InternalScratchImage->GetImages(), InternalScratchImage->GetImageCount(), InternalScratchImage->GetMetadata(), DirectX::DDS_FLAGS::DDS_FLAGS_NONE, (const wchar_t*)OutputWide);
			break;
		case SaveFileType::Tga:
			SaveResult = DirectX::SaveToTGAFile(*InternalScratchImage->GetImages(), (const wchar_t*)OutputWide);
			break;
		case SaveFileType::Hdr:
			SaveResult = DirectX::SaveToHDRFile(*InternalScratchImage->GetImages(), (const wchar_t*)OutputWide);
			break;
		default:
		{
			// This handles all WIC-Codec based formats
			auto Wc = DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_BMP);
			std::function<void __cdecl(IPropertyBag2*)> PropertyWriter = nullptr;

			switch (Type)
			{
			case SaveFileType::Gif:
				Wc = DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_GIF);
				break;
			case SaveFileType::Jpeg:
				Wc = DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_JPEG);
				PropertyWriter = [](IPropertyBag2* props)
				{
					PROPBAG2 options{};
					VARIANT varValues{};
					options.pstrName = (LPOLESTR)L"ImageQuality";
					varValues.vt = VT_R4;
					varValues.fltVal = 1.f;

					(void)props->Write(1, &options, &varValues);
				};
				break;
			case SaveFileType::Jxr:
				Wc = DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_WMP);
				break;
			case SaveFileType::Png:
				Wc = DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_PNG);
				break;
			case SaveFileType::Tiff:
				Wc = DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_TIFF);
				PropertyWriter = [](IPropertyBag2* props)
				{
					PROPBAG2 options{};
					VARIANT varValues{};
					options.pstrName = (LPOLESTR)L"TiffCompressionMethod";
					varValues.vt = VT_UI1;
					varValues.bVal = WICTiffCompressionOption::WICTiffCompressionDontCare;

					(void)props->Write(1, &options, &varValues);
				};
				break;
			}

			SaveResult = DirectX::SaveToWICFile(*InternalScratchImage->GetImages(), DirectX::WIC_FLAGS::WIC_FLAGS_FORCE_SRGB, Wc, (const wchar_t*)OutputWide, nullptr, PropertyWriter);
		}
		break;
		}

		if (FAILED(SaveResult))
			throw std::exception("An error occured while saving the image");
	}

	void Texture::Save(IO::Stream& Stream, SaveFileType Type)
	{
		this->EnsureFormatForType(Type);
		DirectX::Blob TemporaryBuffer;

		SaveToMemoryBlob(&TemporaryBuffer, Type);

		Stream.Write((uint8_t*)TemporaryBuffer.GetBufferPointer(), 0, (uint64_t)TemporaryBuffer.GetBufferSize());
	}

	void Texture::Save(uint8_t* Buffer, uint64_t BufferLength, SaveFileType Type)
	{
		this->EnsureFormatForType(Type);
		DirectX::Blob TemporaryBuffer;

		SaveToMemoryBlob(&TemporaryBuffer, Type);

		std::memcpy(Buffer, TemporaryBuffer.GetBufferPointer(), min(TemporaryBuffer.GetBufferSize(), BufferLength));
	}

	Texture Texture::FromFile(const string& File)
	{
		Texture Result;
		DirectX::TexMetadata MetaData;
		HRESULT LoadResult = 0;

		Result.DirectXImage = new DirectX::ScratchImage();

		auto WidePath = File.ToWString();
		if (File.EndsWith(".dds"))
			LoadResult = DirectX::LoadFromDDSFile((const wchar_t*)WidePath, DirectX::DDS_FLAGS::DDS_FLAGS_NONE, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
		else if (File.EndsWith(".tga"))
			LoadResult = DirectX::LoadFromTGAFile((const wchar_t*)WidePath, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
		else if (File.EndsWith(".hdr"))
			LoadResult = DirectX::LoadFromHDRFile((const wchar_t*)WidePath, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
		else
			LoadResult = DirectX::LoadFromWICFile((const wchar_t*)WidePath, DirectX::WIC_FLAGS::WIC_FLAGS_NONE, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);

		if (FAILED(LoadResult))
		{
			delete (DirectX::ScratchImage*)Result.DirectXImage;
			Result.DirectXImage = nullptr;

			throw std::exception("An error occured loading the image");
		}

		return std::move(Result);
	}

	Texture Texture::FromFile(const string& File, TextureType Type)
	{
		Texture Result;
		DirectX::TexMetadata MetaData;
		HRESULT LoadResult = 0;

		Result.DirectXImage = new DirectX::ScratchImage();

		auto WidePath = File.ToWString();
		switch (Type)
		{
		case TextureType::DDS:
			LoadResult = DirectX::LoadFromDDSFile((const wchar_t*)WidePath, DirectX::DDS_FLAGS::DDS_FLAGS_NONE, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
			break;
		case TextureType::TGA:
			LoadResult = DirectX::LoadFromTGAFile((const wchar_t*)WidePath, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
			break;
		case TextureType::HDR:
			LoadResult = DirectX::LoadFromHDRFile((const wchar_t*)WidePath, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
			break;
		case TextureType::WIC:
			LoadResult = DirectX::LoadFromWICFile((const wchar_t*)WidePath, DirectX::WIC_FLAGS::WIC_FLAGS_NONE, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
			break;
		}

		if (FAILED(LoadResult))
		{
			delete (DirectX::ScratchImage*)Result.DirectXImage;
			Result.DirectXImage = nullptr;

			throw std::exception("An error occured loading the image");
		}

		return std::move(Result);
	}

	Texture Texture::FromStream(IO::Stream& Stream, TextureType Type)
	{
		Texture Result;
		DirectX::TexMetadata MetaData;
		HRESULT LoadResult = 0;

		Result.DirectXImage = new DirectX::ScratchImage();

		auto ScratchBuffer = std::make_unique<uint8_t[]>(Stream.GetLength());
		Stream.Read(ScratchBuffer.get(), 0, Stream.GetLength());

		switch (Type)
		{
		case TextureType::DDS:
			LoadResult = DirectX::LoadFromDDSMemory(ScratchBuffer.get(), Stream.GetLength(), DirectX::DDS_FLAGS::DDS_FLAGS_NONE, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
			break;
		case TextureType::TGA:
			LoadResult = DirectX::LoadFromTGAMemory(ScratchBuffer.get(), Stream.GetLength(), &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
			break;
		case TextureType::HDR:
			LoadResult = DirectX::LoadFromHDRMemory(ScratchBuffer.get(), Stream.GetLength(), &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
			break;
		case TextureType::WIC:
			LoadResult = DirectX::LoadFromWICMemory(ScratchBuffer.get(), Stream.GetLength(), DirectX::WIC_FLAGS::WIC_FLAGS_NONE, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
			break;
		}
		
		if (FAILED(LoadResult))
		{
			delete (DirectX::ScratchImage*)Result.DirectXImage;
			Result.DirectXImage = nullptr;

			throw std::exception("An error occured loading the image");
		}

		return std::move(Result);
	}

	Texture Texture::FromBuffer(uint8_t* Buffer, uint64_t BufferLength, TextureType Type)
	{
		Texture Result;
		DirectX::TexMetadata MetaData;
		HRESULT LoadResult = 0;

		Result.DirectXImage = new DirectX::ScratchImage();

		switch (Type)
		{
		case TextureType::DDS:
			LoadResult = DirectX::LoadFromDDSMemory(Buffer, BufferLength, DirectX::DDS_FLAGS::DDS_FLAGS_NONE, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
			break;
		case TextureType::TGA:
			LoadResult = DirectX::LoadFromTGAMemory(Buffer, BufferLength, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
			break;
		case TextureType::HDR:
			LoadResult = DirectX::LoadFromHDRMemory(Buffer, BufferLength, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
			break;
		case TextureType::WIC:
			LoadResult = DirectX::LoadFromWICMemory(Buffer, BufferLength, DirectX::WIC_FLAGS::WIC_FLAGS_NONE, &MetaData, *(DirectX::ScratchImage*)Result.DirectXImage);
			break;
		}

		if (FAILED(LoadResult))
		{
			delete (DirectX::ScratchImage*)Result.DirectXImage;
			Result.DirectXImage = nullptr;

			throw std::exception("An error occured loading the image");
		}

		return std::move(Result);
	}

	Texture Texture::FromRawBlock(uint8_t* Buffer, uint64_t BufferLength, uint32_t Width, uint32_t Height, DXGI_FORMAT Format)
	{
		DDSFormat BlockFormat{};
		BlockFormat.Format = Format;

		auto ExpectedBlockSize = Assets::DDS::CalculateBlockSize(Width, Height, BlockFormat);

		if (BufferLength < ExpectedBlockSize)
			throw std::exception("Invalid buffer length specified");

		auto Result = Texture(Width, Height, Format);

		std::memcpy(((DirectX::ScratchImage*)Result.DirectXImage)->GetImages()->pixels, Buffer, ExpectedBlockSize);

		return std::move(Result);
	}

	int Texture::Morton(uint32_t i, uint32_t sx, uint32_t sy)
	{
		int v0 = 1;
		int v1 = 1;
		int v2 = i;
		int v3 = sx;
		int v4 = sy;
		int v5 = 0;
		int v6 = 0;
		while (v3 > 1 || v4 > 1)
		{
			if (v3 > 1)
			{
				v5 += v1 * (v2 & 1);
				v2 >>= 1;
				v1 *= 2;
				v3 >>= 1;
			}
			if (v4 > 1)
			{
				v6 += v0 * (v2 & 1);
				v2 >>= 1;
				v0 *= 2;
				v4 >>= 1;
			}
		}
		return v6 * sx + v5;
	}


	Texture::Texture()
		: DirectXImage(nullptr)
	{
	}

	void Texture::EnsureFormatForType(SaveFileType Type)
	{
		switch (Type)
		{
			// These formats require a 32bpp, so B8G8R8A8_UNORM - like format to encode (Only transcode if not 32bpp)
		case SaveFileType::Png:
		case SaveFileType::Bmp:
		case SaveFileType::Jpeg:
		case SaveFileType::Tiff:
		case SaveFileType::Gif:
		case SaveFileType::Jxr:
		case SaveFileType::Tga:
			if (!IsValid32bppFormat(InternalScratchImage->GetMetadata().format))
				this->ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM);
			break;
			// This format requires either R32G32B32A32_FLOAT or R32G32B32_FLOAT
		case SaveFileType::Hdr:
			this->ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT);
			break;
		}
	}

	void Texture::SaveToMemoryBlob(void* Blob, SaveFileType Type)
	{
		HRESULT SaveResult = 0;

		switch (Type)
		{
		case SaveFileType::Dds:
			SaveResult = DirectX::SaveToDDSMemory(InternalScratchImage->GetImages(), InternalScratchImage->GetImageCount(), InternalScratchImage->GetMetadata(), DirectX::DDS_FLAGS::DDS_FLAGS_NONE, *(DirectX::Blob*)Blob);
			break;
		case SaveFileType::Tga:
			SaveResult = DirectX::SaveToTGAMemory(*InternalScratchImage->GetImages(), *(DirectX::Blob*)Blob);
			break;
		case SaveFileType::Hdr:
			SaveResult = DirectX::SaveToHDRMemory(*InternalScratchImage->GetImages(), *(DirectX::Blob*)Blob);
			break;
		default:
		{
			// This handles all WIC-Codec based formats
			auto Wc = DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_BMP);
			std::function<void __cdecl(IPropertyBag2*)> PropertyWriter = nullptr;

			switch (Type)
			{
			case SaveFileType::Gif:
				Wc = DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_GIF);
				break;
			case SaveFileType::Jpeg:
				Wc = DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_JPEG);
				PropertyWriter = [](IPropertyBag2 * props)
				{
					PROPBAG2 options{};
					VARIANT varValues{};
					options.pstrName = (LPOLESTR)L"ImageQuality";
					varValues.vt = VT_R4;
					varValues.fltVal = 1.f;

					(void)props->Write(1, &options, &varValues);
				};
				break;
			case SaveFileType::Jxr:
				Wc = DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_WMP);
				break;
			case SaveFileType::Png:
				Wc = DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_PNG);
				break;
			case SaveFileType::Tiff:
				Wc = DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_TIFF);
				PropertyWriter = [](IPropertyBag2 * props)
				{
					PROPBAG2 options{};
					VARIANT varValues{};
					options.pstrName = (LPOLESTR)L"TiffCompressionMethod";
					varValues.vt = VT_UI1;
					varValues.bVal = WICTiffCompressionOption::WICTiffCompressionDontCare;

					(void)props->Write(1, &options, &varValues);
				};
				break;
			}

			SaveResult = DirectX::SaveToWICMemory(*InternalScratchImage->GetImages(), DirectX::WIC_FLAGS::WIC_FLAGS_NONE, Wc, *(DirectX::Blob*)Blob, nullptr, PropertyWriter);
		}
		break;
		}

		if (FAILED(SaveResult))
			throw std::exception("An error occured while saving the image");
	}

	void Texture::Transcoder_NormalMapBC5()
	{
		// Ensure R8G8B8A8 value and ordering...
		if (this->Format() != DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM)
			this->ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);

		// Create a temporary image
		auto TemporaryResult = std::make_unique<DirectX::ScratchImage>();
		auto CurrentResult = (DirectX::ScratchImage*)this->DirectXImage;

		auto TranscodeResult = DirectX::TransformImage(CurrentResult->GetImages(), CurrentResult->GetImageCount(), CurrentResult->GetMetadata(),
			[](DirectX::XMVECTOR* OutPixels, const DirectX::XMVECTOR* InPixels, size_t Width, size_t Slice)
			{
				for (size_t i = 0; i < Width; i++)
				{
					auto Scanline = InPixels[i];

					auto Red = DirectX::XMVectorGetX(Scanline);
					auto Green = DirectX::XMVectorGetY(Scanline);

					// Calculate the blue channel
					float NormalX = 2 * Red - 1;
					float NormalY = 2 * Green - 1;
					float NormalZ = 0.0f;

					// Check if we can average it
					if (1 - NormalX * NormalX - NormalY * NormalY > 0)
					{
						NormalZ = std::sqrtf(1 - NormalX * NormalX - NormalY * NormalY);
					}

					// Calculate the final blue value and clamp it between 0 and 1
					float ResultBlueVal = Math::MathHelper::Clamp<float>(((NormalZ + 1) / 2.0f), 0, 1.0);

					OutPixels[i] = DirectX::XMVectorSetZ(Scanline, ResultBlueVal);
				}
			}, *TemporaryResult);

		// If it succeeds, we must then
		if (SUCCEEDED(TranscodeResult))
		{
			delete (DirectX::ScratchImage*)this->DirectXImage;
			this->DirectXImage = TemporaryResult.release();
		}
	}

	void Texture::Transcoder_NormalMapBC5OpenGl()
	{
		// Ensure R8G8B8A8 value and ordering...
		if (this->Format() != DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM)
			this->ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);

		// Create a temporary image
		auto TemporaryResult = std::make_unique<DirectX::ScratchImage>();
		auto CurrentResult = (DirectX::ScratchImage*)this->DirectXImage;

		auto TranscodeResult = DirectX::TransformImage(CurrentResult->GetImages(), CurrentResult->GetImageCount(), CurrentResult->GetMetadata(),
			[](DirectX::XMVECTOR* OutPixels, const DirectX::XMVECTOR* InPixels, size_t Width, size_t Slice)
			{
				for (size_t i = 0; i < Width; i++)
				{
					auto Scanline = InPixels[i];

					auto Red = DirectX::XMVectorGetX(Scanline);
					auto Green = DirectX::XMVectorGetY(Scanline);

					// Invert G (Y) to match DirectX
					Green = 1.0f - Green;
					Scanline = DirectX::XMVectorSetY(Scanline, Green);

					// Calculate the blue channel
					float NormalX = 2 * Red - 1;
					float NormalY = 2 * Green - 1;
					float NormalZ = 0.0f;

					// Check if we can average it
					if (1 - NormalX * NormalX - NormalY * NormalY > 0)
					{
						NormalZ = std::sqrtf(1 - NormalX * NormalX - NormalY * NormalY);
					}

					// Calculate the final blue value and clamp it between 0 and 1
					float ResultBlueVal = Math::MathHelper::Clamp<float>(((NormalZ + 1) / 2.0f), 0, 1.0);

					OutPixels[i] = DirectX::XMVectorSetZ(Scanline, ResultBlueVal);
				}
			}, *TemporaryResult);

		// If it succeeds, we must then
		if (SUCCEEDED(TranscodeResult))
		{
			delete (DirectX::ScratchImage*)this->DirectXImage;
			this->DirectXImage = TemporaryResult.release();
		}
	}

	bool Texture::IsValid32bppFormat(DXGI_FORMAT Format)
	{
		switch (Format)
		{
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM:
			return true;
		default:
			return false;
		}
	}
}