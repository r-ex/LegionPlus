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

	Info.Name = string::Format("Wrap_0x%llX", Asset.NameHash);

	// nam
	if (Header.name.Index || Header.name.Offset)
	{
		Info.Name = this->ReadStringFromPointer(Asset, Header.name);
		if (Header.nameLength)
			Info.Name = Info.Name.Substring(0, Header.nameLength);
	}

	if (!ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = IO::Path::GetFileNameWithoutExtension(Info.Name).ToLower();

	bool IsCompressed = Header.flags & 1;

	Info.Type = ApexAssetType::Wrap;
	Info.Status = ApexAssetStatus::Loaded;
	Info.DebugInfo = string::Format("0x%02X | 0x%llX", Header.flags, Asset.NameHash);
}

void RpakLib::ExportWrappedFile(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
	WrapAssetHeader_v7_t Header = Reader.Read<WrapAssetHeader_v7_t>();

	string name = string::Format("Wrap_0x%llX", Asset.NameHash);

	// nam
	if (Header.name.Index || Header.name.Offset)
	{
		name = this->ReadStringFromPointer(Asset, Header.name);
		if (Header.nameLength)
			name = name.Substring(0, Header.nameLength);
	}

	string dirpath = IO::Path::Combine(Path, IO::Path::GetDirectoryName(name));

	IO::Directory::CreateDirectory(dirpath);

	string DestinationPath = IO::Path::Combine(Path, name);

	if (!Utils::ShouldWriteFile(DestinationPath))
		return;

	bool IsCompressedBigger = Header.cmpSize > Header.dcmpSize;
	bool IsCompressed = Header.flags & 1 && !IsCompressedBigger;
	bool ContainsNullByte = Header.flags & 3;
	bool IsStreamed = Asset.OptimalStarpakOffset != -1 || Asset.StarpakOffset != -1;

	uint64_t Size = Header.dcmpSize;

	if (!name.Contains("bsp"))
		Size = ContainsNullByte ? Size : Size - 1;

	std::ofstream out(DestinationPath, std::ios::out | std::ios::binary);

	uint8_t* tmpBuf = new uint8_t[Size];

	if (!IsStreamed)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, Header.data.Index, Header.data.Offset));
		Reader.Read(tmpBuf, 0, Size);
		Reader.Close();
	}
	else
	{
		std::unique_ptr<IO::FileStream> StarpakStream = nullptr;
		uint64_t StreamOffset = 0;

		if (Asset.OptimalStarpakOffset != -1)
		{
			StreamOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;
			StarpakStream = this->GetStarpakStream(Asset, true);
		}
		else if (Asset.StarpakOffset != -1)
		{
			StreamOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
			StarpakStream = this->GetStarpakStream(Asset, false);
		}

		uint64_t OutputOffset = IsCompressedBigger ? (StreamOffset - (Header.dcmpSize - Header.cmpSize)) : StreamOffset;

		StarpakStream->SetPosition(OutputOffset);
		IO::BinaryReader StarReader = IO::BinaryReader(StarpakStream.get(), true);

		StarReader.Read(tmpBuf, 0, Size);
		StarReader.Close();
	}

	if (IsCompressed)
	{
		std::unique_ptr<IO::MemoryStream> DecompStream = RTech::DecompressStreamedBuffer(tmpBuf, Size, (uint8_t)CompressionType::OODLE);

		uint8_t* outtmpBuf = new uint8_t[Size];

		DecompStream->Read(outtmpBuf, 0, Size);

		out.write((char*)outtmpBuf, Size);

		DecompStream.release();

		delete[] outtmpBuf;
	}
	else
	{
		out.write((char*)tmpBuf, Size);
		delete[] tmpBuf;
	}

	out.close();
};