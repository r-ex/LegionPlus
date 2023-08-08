#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include <rtech.h>

void RpakLib::BuildWrapInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
	WrapAssetHeader_v7_t Header = Reader.Read<WrapAssetHeader_v7_t>();

	if (Header.name.Index || Header.name.Offset)
		Info.Name = this->ReadStringFromPointer(Asset, Header.name);
	else
		Info.Name = string::Format("wrap_0x%llx", Asset.NameHash);

	Info.Type = ApexAssetType::Wrap;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = "";
}

void RpakLib::ExportWrappedFile(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
	WrapAssetHeader_v7_t Header = Reader.Read<WrapAssetHeader_v7_t>();

	string name = string::Format("0x%llx.bin", Asset.NameHash);

	if(Header.name.Index || Header.name.Offset)
		name = this->ReadStringFromPointer(Asset, Header.name);

	string exportPath = IO::Path::Combine(Path, name);

	IO::Directory::CreateDirectory(IO::Path::GetDirectoryName(exportPath));

	std::ofstream ofs(exportPath.ToCString(), std::ios::out | std::ios::binary);


	if ((Header.flags & 0x10) == 0)
	{
		size_t dataOffset = this->GetFileOffset(Asset, Header.data);
		RpakStream->SetPosition(dataOffset);

		std::unique_ptr<IO::MemoryStream> stream;
		if (Header.flags & 1) // compressed
		{
			auto compressedBuffer = new uint8_t[Header.cmpSize];
			Reader.Read(compressedBuffer, 0, Header.cmpSize);
			uint64_t bufferSize = Header.dcmpSize;

			std::unique_ptr<IO::MemoryStream> stream = RTech::DecompressStreamedBuffer(compressedBuffer, bufferSize, (uint8_t)CompressionType::OODLE);
			
			char* decompressedBuffer = new char[Header.dcmpSize];
			stream->Read((uint8_t*)decompressedBuffer, 0, Header.dcmpSize);
			stream->Close();
			ofs.write(decompressedBuffer, Header.dcmpSize - 1);
		}
		else
		{
			char* buffer = new char[Header.dcmpSize];

			Reader.Read(buffer, 0, Header.dcmpSize);

			ofs.write(buffer, Header.dcmpSize);

			delete[] buffer;
		}
	}


	ofs.close();
}
