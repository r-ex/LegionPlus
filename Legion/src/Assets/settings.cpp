#include "pch.h"
#include "RpakLib.h"
#include <Path.h>
#include <Directory.h>

void RpakLib::BuildSettingsInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
	SettingsHeader Header = Reader.Read<SettingsHeader>();

	string Name = string::Format("stgs_%llx", Asset.NameHash);

	if (Header.Name.index || Header.Name.offset)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, Header.Name.index, Header.Name.offset));

		Name = Reader.ReadCString();
	}

	if (ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = Name;
	else
		Info.Name = IO::Path::GetFileNameWithoutExtension(Name).ToLower();

	Info.Type = ApexAssetType::Settings;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = "N/A";
}

void RpakLib::ExportSettings(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
	SettingsHeader Header = Reader.Read<SettingsHeader>();

	string Name = string::Format("stgs_%llx", Asset.NameHash);

	if (Header.Name.index || Header.Name.offset)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, Header.Name.index, Header.Name.offset));

		Name = Reader.ReadCString();
	}

	string dirpath = IO::Path::Combine(Path, IO::Path::GetDirectoryName(Name));

	IO::Directory::CreateDirectory(dirpath);

	string DestinationPath = IO::Path::Combine(Path, IO::Path::ChangeExtension(Name, "set"));

	if (!Utils::ShouldWriteFile(DestinationPath))
		return;

	this->ExtractSettings(Asset, DestinationPath, Name, Header);
}

// why is this here
bool operator<(const SettingsLayoutItem& lhs, const SettingsLayoutItem& rhs)
{
	return lhs.valueOffset < rhs.valueOffset;
}

void RpakLib::ExtractSettings(const RpakLoadAsset& Asset, const string& Path, const string& Name, const SettingsHeader& Header)
{
	if (!Assets.ContainsKey(Header.LayoutGUID))
	{
		g_Logger.Info("Unable to export settings asset '%s'. Settings Layout asset with GUID '%llx' was not loaded.\n", Name.ToCString(), Header.LayoutGUID);
		return;
	}

	SettingsLayout Layout = this->ExtractSettingsLayout(Assets[Header.LayoutGUID]);

	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	std::ofstream out(Path.ToCString(), std::ios::out);

	out << "\"" << Name << "\"\n{";

	Layout.items.Sort();

	for (auto& it : Layout.items)
	{
		out << "\n\t" << it.name << " ";

		RpakStream->SetPosition(this->GetFileOffset(Asset, Header.Values.index, Header.Values.offset + it.valueOffset));
		
		switch (it.type)
		{
		case ST_Bool:
		{
			bool bValue = Reader.Read<bool>();
			out << std::boolalpha << bValue;
			break;
		}
		case ST_Int:
		{
			int nValue = Reader.Read<int>();
			out << nValue;
			break;
		}
		case ST_Float:
		{
			float fValue = Reader.Read<float>();
			out << fValue;
			break;
		}
		case ST_Float2:
		{
			Math::Vector2 vec = Reader.Read<Math::Vector2>();
			out << "\"" << vec.X << " " << vec.Y << "\"";
			break;
		}
		case ST_Float3:
		{
			Math::Vector3 vec = Reader.Read<Math::Vector3>();
			out << "\"" << vec.X << " " << vec.Y << " " << vec.Z << "\"";
			break;
		}
		case ST_String:
		{
			RPakPtr pValue = Reader.Read<RPakPtr>();
			string sValue = this->ReadStringFromPointer(Asset, pValue);
			out << "\"" << sValue.ToString() << "\"";
			break;
		}
		case ST_Asset:
		{
			RPakPtr pValue = Reader.Read<RPakPtr>();
			string sValue = this->ReadStringFromPointer(Asset, pValue);
			out << "$\"" << sValue.ToString() << "\"";
			break;
		}
		case ST_Asset_2:
		{
			RPakPtr pValue = Reader.Read<RPakPtr>();
			string sValue = this->ReadStringFromPointer(Asset, pValue);
			out << "$\"" << sValue.ToString() << "\"";
			break;
		}
		default:
			printf("NOT IMPLEMENTED: SettingType %i\n", it.type);
			break;
		}
	}

	out << "\n}";
	out.close();
}

SettingsLayout RpakLib::ExtractSettingsLayout(const RpakLoadAsset& Asset)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
	SettingsLayoutHeader header = Reader.Read<SettingsLayoutHeader>();

	SettingsLayout layout;

	layout.name = this->ReadStringFromPointer(Asset, header.pName);
	layout.itemsCount = header.itemsCount;

	RpakStream->SetPosition(this->GetFileOffset(Asset, header.pItems.index, header.pItems.offset));
	for (uint32_t i = 0; i < layout.itemsCount; ++i)
	{
		SettingsFieldType itemType = Reader.Read<SettingsFieldType>();
		uint16_t nameOffset = Reader.Read<uint16_t>();
		uint32_t unk = Reader.Read<uint32_t>();

		// ignore null items
		if (itemType == 0 && unk == 0 && nameOffset == 0)
			continue;

		string itemName = this->ReadStringFromPointer(Asset, header.pStringBuf.index, header.pStringBuf.offset + nameOffset);

		SettingsLayoutItem item{ itemType, itemName, unk };
		layout.items.EmplaceBack(item);
	};

	return layout;
}