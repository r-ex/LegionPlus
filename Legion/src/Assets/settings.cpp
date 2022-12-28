#include "pch.h"
#include "RpakLib.h"
#include <Path.h>
#include <Directory.h>

bool operator<(const SettingsLayoutItem& lhs, const SettingsLayoutItem& rhs)
{
	return lhs.valueOffset < rhs.valueOffset;
}

void RpakLib::BuildSettingsInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
	SettingsHeader Header = Reader.Read<SettingsHeader>();

	string Name = string::Format("stgs_%llx", Asset.NameHash);

	if (Header.Name.Index || Header.Name.Offset)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, Header.Name.Index, Header.Name.Offset));

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

void RpakLib::BuildSettingsLayoutInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	auto Layout = this->ExtractSettingsLayout(Asset);

	if (ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = Layout.name;
	else
		Info.Name = IO::Path::GetFileNameWithoutExtension(Layout.name).ToLower();

	Info.Type = ApexAssetType::SettingsLayout;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = "N/A";
}

void RpakLib::ExportSettingsLayout(const RpakLoadAsset& Asset, const string& Path)
{
	SettingsLayout Layout = this->ExtractSettingsLayout(Asset);

	string dirpath = IO::Path::Combine(Path, IO::Path::GetDirectoryName(Layout.name));

	IO::Directory::CreateDirectory(dirpath);

	string DestinationPath = IO::Path::Combine(Path, IO::Path::ChangeExtension(Layout.name, "setl"));

	if (!Utils::ShouldWriteFile(DestinationPath))
		return;

	std::ofstream out(DestinationPath, std::ios::out);

	out << "\"" << Layout.name << "\"\n{";

	Layout.items.Sort();

	for (auto& it : Layout.items)
	{
		switch (it.type)
		{
		case SettingsFieldType::ST_Bool:    out << "\n\t[ " << it.valueOffset << " ] [Bool] " << it.name << " "; break;
		case SettingsFieldType::ST_Int:     out << "\n\t[ " << it.valueOffset << " ] [Int] " << it.name << " "; break;
		case SettingsFieldType::ST_Float:   out << "\n\t[ " << it.valueOffset << " ] [Float] " << it.name << " "; break;
		case SettingsFieldType::ST_Float2:  out << "\n\t[ " << it.valueOffset << " ] [Vector2] " << it.name << " "; break;
		case SettingsFieldType::ST_Float3:  out << "\n\t[ " << it.valueOffset << " ] [Vector3] " << it.name << " "; break;
		case SettingsFieldType::ST_String:  out << "\n\t[ " << it.valueOffset << " ] [String] " << it.name << " "; break;
		case SettingsFieldType::ST_Asset:   out << "\n\t[ " << it.valueOffset << " ] [Asset] " << it.name << " "; break;
		case SettingsFieldType::ST_Asset_2: out << "\n\t[ " << it.valueOffset << " ] [Asset2] " << it.name << " "; break;
		case SettingsFieldType::ST_Array:   out << "\n\t[ " << it.valueOffset << " ] [Array] " << it.name << " "; break;
		case SettingsFieldType::ST_Array_2: out << "\n\t[ " << it.valueOffset << " ] [Array2] " << it.name << " "; break;
		default: out << "\n\t [" << it.valueOffset << " ] [" << (int)it.type << "] " << it.name << " "; break;
		}
	}

	out << "\n}";
	//this->ExtractSettings(Asset, DestinationPath, Name, Header);
}

void RpakLib::ExportSettings(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
	SettingsHeader Header = Reader.Read<SettingsHeader>();

	string Name = string::Format("stgs_%llx", Asset.NameHash);

	if (Header.Name.Index || Header.Name.Offset)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, Header.Name.Index, Header.Name.Offset));
		Name = Reader.ReadCString();
	}

	string dirpath = IO::Path::Combine(Path, IO::Path::GetDirectoryName(Name));

	IO::Directory::CreateDirectory(dirpath);

	string DestinationPath = IO::Path::Combine(Path, IO::Path::ChangeExtension(Name, "set"));

	if (!Utils::ShouldWriteFile(DestinationPath))
		return;

	this->ExtractSettings(Asset, DestinationPath, Name, Header);
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

	out << "\"" << Name << "\" -> \"" << Layout.name << "\"" << "\n{";

	Layout.items.Sort();

	for (auto& it : Layout.items)
	{
		out << "\n\t" << it.name << " ";

		RpakStream->SetPosition(this->GetFileOffset(Asset, Header.Values.Index, Header.Values.Offset + it.valueOffset));

		switch (it.type)
		{
		case SettingsFieldType::ST_Bool:
		{
			bool bValue = Reader.Read<bool>();
			out << std::boolalpha << bValue;
			break;
		}
		case SettingsFieldType::ST_Int:
		{
			int nValue = Reader.Read<int>();
			out << nValue;
			break;
		}
		case SettingsFieldType::ST_Float:
		{
			float fValue = Reader.Read<float>();
			out << fValue;
			break;
		}
		case SettingsFieldType::ST_Float2:
		{
			Math::Vector2 vec = Reader.Read<Math::Vector2>();
			out << "\"" << vec.X << " " << vec.Y << "\"";
			break;
		}
		case SettingsFieldType::ST_Float3:
		{
			Math::Vector3 vec = Reader.Read<Math::Vector3>();
			out << "\"" << vec.X << " " << vec.Y << " " << vec.Z << "\"";
			break;
		}
		case SettingsFieldType::ST_String:
		{
			RPakPtr pValue = Reader.Read<RPakPtr>();
			string sValue = this->ReadStringFromPointer(Asset, pValue);
			out << "\"" << sValue.ToString() << "\"";
			break;
		}
		case SettingsFieldType::ST_Asset:
		{
			RPakPtr pValue = Reader.Read<RPakPtr>();
			string sValue = this->ReadStringFromPointer(Asset, pValue);
			out << "$\"" << sValue.ToString() << "\"";
			break;
		}
		case SettingsFieldType::ST_Asset_2:
		{
			RPakPtr pValue = Reader.Read<RPakPtr>();
			string sValue = this->ReadStringFromPointer(Asset, pValue);
			out << "$\"" << sValue.ToString() << "\"";
			break;
		}
		case SettingsFieldType::ST_Array_2:
		{
			try 
			{
				out << "\n\t{";
				RpakStream->SetPosition(this->GetFileOffset(Asset, Header.Values.Index, Header.Values.Offset + (it.valueOffset & 0xFFFFFF)));

				// First read the array size and offset..
				int nArraySize = Reader.Read<int>(); // YOU ARE WRONG BUT ALSO RIGHT, EITHER YOU ARE TOO SMALL SO I HAVE TO TAKE YOU * 2 OR TOO SMALL SO YOU MAKE ME CRASH.
				int nArrayPtrOffset = Reader.Read<int>();

				if (nArraySize != 0)
				{
					// Now we the the pointer that points at the start of the array.
					RpakStream->SetPosition(this->GetFileOffset(Asset, Header.Values.Index, Header.Values.Offset + (nArrayPtrOffset)));

					for (int i = 1; i <= nArraySize; i++)
					{
						int nEntryOffset = nArrayPtrOffset + ((i - 1) * sizeof(RPakPtr));
						RpakStream->SetPosition(this->GetFileOffset(Asset, Header.Values.Index, Header.Values.Offset + nEntryOffset));

						RPakPtr pValue = Reader.Read<RPakPtr>();
						if (pValue.Index && pValue.Offset)
						{
							RpakStream->SetPosition(this->GetFileOffset(Asset, pValue.Index, pValue.Offset));
							string sEntry = Reader.ReadCString();

							out << "\n\t\t\"" << sEntry.ToString() << "\" ";
						}
					}
				}

				out << "\n\t}";

				break;
			}
			catch (const char e)
			{
			}
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

	RpakStream->SetPosition(this->GetFileOffset(Asset, header.pItems.Index, header.pItems.Offset));
	for (uint32_t i = 0; i < layout.itemsCount; ++i)
	{
		SettingsFieldType itemType = Reader.Read<SettingsFieldType>();
		uint16_t nameOffset = Reader.Read<uint16_t>();
		uint32_t valueOffset = Reader.Read<uint32_t>();

		// ignore null items
		if (static_cast<int>(itemType) == 0 && valueOffset == 0 && nameOffset == 0)
			continue;

		string itemName = this->ReadStringFromPointer(Asset, header.pStringBuf.Index, header.pStringBuf.Offset + nameOffset);

		SettingsLayoutItem item{ itemType, itemName, valueOffset };
		layout.items.EmplaceBack(item);
	};

	return layout;
}