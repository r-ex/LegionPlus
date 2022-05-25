#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"

string RpakLib::GetSubtitlesNameFromHash(uint64_t Hash)
{
	if (SubtitleLanguageMap.count((SubtitleLanguageHash)Hash))
		return "subtitles_" + SubtitleLanguageMap[(SubtitleLanguageHash)Hash];

	return string::Format("subt_0x%llx", Hash);
}

void RpakLib::BuildSubtitleInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	Info.Name = GetSubtitlesNameFromHash(Asset.NameHash);
	Info.Type = ApexAssetType::Subtitles;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = "N/A";
}

void RpakLib::ExportSubtitles(const RpakLoadAsset& Asset, const string& Path)
{
	TextExportFormat_t Format = (TextExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("TextFormat");

	string sExtension = "";

	switch (Format)
	{
	case TextExportFormat_t::CSV:
		sExtension = ".csv";
		break;
	case TextExportFormat_t::TXT:
		sExtension = ".txt";
		break;
	}

	string DestinationPath = IO::Path::Combine(Path, GetSubtitlesNameFromHash(Asset.NameHash) + sExtension);

	if (!Utils::ShouldWriteFile(DestinationPath))
		return;

	List<SubtitleEntry> Subtitles = this->ExtractSubtitles(Asset);

	std::ofstream subt_out(DestinationPath.ToCString(), std::ios::out);

	switch (Format)
	{
	case TextExportFormat_t::CSV:
	{
		subt_out << "color,text\n";
		for (auto& Entry : Subtitles)
		{
			subt_out << "\"#" << Utils::Vector3ToHexColor(Entry.Color) << "\",\"" << Entry.SubtitleText << "\"\n";
		}
		break;
	}
	case TextExportFormat_t::TXT:
	{
		for (auto& Entry : Subtitles)
		{
			subt_out << Entry.SubtitleText << "\n";
		}
		break;
	}
	default:
		g_Logger.Warning("Attempted to export Subtitles asset with an invalid format (%i)\n", Format);
		break;
	}

	subt_out.close();
}

List<SubtitleEntry> RpakLib::ExtractSubtitles(const RpakLoadAsset& Asset)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	SubtitleHeader SubtHdr = Reader.Read<SubtitleHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, SubtHdr.EntriesIndex, SubtHdr.EntriesOffset));

	List<SubtitleEntry> Subtitles;

	std::regex ClrRegex("<clr:([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3})>");

	while (!RpakStream->GetIsEndOfFile())
	{
		SubtitleEntry se;

		string temp_string = Reader.ReadCString();

		std::smatch sm;

		std::string s(temp_string);

		std::regex_search(s, sm, ClrRegex);

		if (sm.size() == 4)
			se.Color = Math::Vector3(atof(sm[1].str().c_str()), atof(sm[2].str().c_str()), atof(sm[3].str().c_str()));
		else
			se.Color = Math::Vector3(255, 255, 255);

		se.SubtitleText = temp_string.Substring(sm[0].str().length());

		Subtitles.EmplaceBack(se);
	}
	return Subtitles;
}
