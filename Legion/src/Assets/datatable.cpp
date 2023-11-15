#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"

void RpakLib::BuildDataTableInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
	DataTableHeader DtblHeader = Reader.Read<DataTableHeader>();

	Info.Name = string::Format("datatable_0x%llx", Asset.NameHash);
	Info.Type = ApexAssetType::DataTable;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = string::Format("Columns: %d Rows: %d", DtblHeader.ColumnCount, DtblHeader.RowCount);
}

void RpakLib::ExportDataTable(const RpakLoadAsset& Asset, const string& Path)
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

	string DestinationPath = IO::Path::Combine(Path, string::Format("0x%llx", Asset.NameHash) + sExtension);

	if (!Utils::ShouldWriteFile(DestinationPath))
		return;

	List<List<DataTableColumnData>> DataTable = this->ExtractDataTable(Asset);

	std::ofstream dtbl_out(DestinationPath.ToCString(), std::ios::out);

	for (uint32_t i = 0; i < DataTable.Count(); ++i)
	{
		List<DataTableColumnData> Row = DataTable[i];

		for (uint32_t c = 0; c < Row.Count(); ++c)
		{
			DataTableColumnData cd = Row[c];

			switch (cd.Type)
			{
			case DataTableColumnDataType::Bool:
				dtbl_out << cd.bValue;
				break;
			case DataTableColumnDataType::Int:
				dtbl_out << cd.iValue;
				break;
			case DataTableColumnDataType::Float:
				dtbl_out << cd.fValue;
				break;
			case DataTableColumnDataType::Vector:
			{
				dtbl_out << "\"<" << cd.vValue.X << "," << cd.vValue.Y << "," << cd.vValue.Z << ">\"";
				break;
			}
			case DataTableColumnDataType::Asset:
			{
				dtbl_out << "\"" + cd.assetValue + "\"";
				break;
			}
			case DataTableColumnDataType::AssetNoPrecache:
			{
				dtbl_out << "\"" + cd.assetNPValue + "\"";
				break;
			}
			case DataTableColumnDataType::StringT:
			{
				dtbl_out << "\"" + cd.stringValue + "\"";
				break;
			}
			}
			if (c != Row.Count() - 1)
			{
				dtbl_out << ",";
			}
			else {
				dtbl_out << "\n";
			}
		}
	}

	auto LastExport = DataTable[DataTable.Count() - 1];

	for (uint32_t c = 0; c < LastExport.Count(); ++c)
	{
		DataTableColumnData cd = LastExport[c];

		switch (cd.Type)
		{
		case DataTableColumnDataType::Bool:
			dtbl_out << "\"bool\"";
			break;
		case DataTableColumnDataType::Int:
			dtbl_out << "\"int\"";
			break;
		case DataTableColumnDataType::Float:
			dtbl_out << "\"float\"";
			break;
		case DataTableColumnDataType::Vector:
		{
			dtbl_out << "\"vector\"";
			break;
		}
		case DataTableColumnDataType::Asset:
		{
			dtbl_out << "\"asset\"";
			break;
		}
		case DataTableColumnDataType::AssetNoPrecache:
		{
			dtbl_out << "\"assetnoprecache\"";
			break;
		}
		case DataTableColumnDataType::StringT:
		{
			dtbl_out << "\"string\"";
			break;
		}

		}

		if (c != LastExport.Count() - 1)
			dtbl_out << ",";
		else
			dtbl_out << "\n";
	}

	dtbl_out.close();
}

List<List<DataTableColumnData>> RpakLib::ExtractDataTable(const RpakLoadAsset& Asset)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	DataTableHeader DtblHeader = Reader.Read<DataTableHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, DtblHeader.ColumnHeaderBlock, DtblHeader.ColumnHeaderOffset));

	List<DataTableColumn> Columns;
	List<List<DataTableColumnData>> Data;

	for (uint32_t i = 0; i < DtblHeader.ColumnCount; ++i)
	{
		DataTableColumn col{};

		uint32_t id = Reader.Read<uint32_t>();
		uint32_t offset = Reader.Read<uint32_t>();

		col.Unk0Seek = this->GetFileOffset(Asset, id, offset);

		// titanfall 2 uses version 0 and does not have this member
		// all of apex uses version 1, but only later game versions have this member
		// in order to make sure that the struct reads correctly, we must check the pak's creation time
		if (Asset.AssetVersion != 0 && this->LoadedFiles[Asset.FileIndex].Hash != 0 && this->LoadedFiles[Asset.FileIndex].CreatedTime > 0x1d692d897275335) // 25/09/2020 01:10:00
			col.Unk8 = Reader.Read<uint64_t>();

		col.Type = Reader.Read<uint32_t>();
		col.RowOffset = Reader.Read<uint32_t>();

		Columns.EmplaceBack(col);
	}

	List<DataTableColumnData> ColumnNameData;

	for (uint32_t i = 0; i < DtblHeader.ColumnCount; ++i)
	{
		DataTableColumn col = Columns[i];

		RpakStream->SetPosition(col.Unk0Seek);
		string name = Reader.ReadCString();

		DataTableColumnData cd;

		cd.stringValue = name;
		cd.Type = DataTableColumnDataType::StringT;

		ColumnNameData.EmplaceBack(cd);
	}

	Data.EmplaceBack(ColumnNameData);

	uint64_t rows_seek = this->GetFileOffset(Asset, DtblHeader.RowHeaderBlock, DtblHeader.RowHeaderOffset);

	for (uint32_t i = 0; i < DtblHeader.RowCount; ++i)
	{
		List<DataTableColumnData> RowData;

		for (uint32_t c = 0; c < DtblHeader.ColumnCount; ++c)
		{
			DataTableColumn col = Columns[c];

			// this part is the same across all versions
			uint64_t seek_pos = rows_seek + col.RowOffset;

			// titanfall 2
			if (Asset.AssetVersion == 0)
				seek_pos += (i * DtblHeader.UnkHash);
			else
				seek_pos += (i * DtblHeader.RowStride);

			RpakStream->SetPosition(seek_pos);

			DataTableColumnData d;

			d.Type = (DataTableColumnDataType)col.Type;

			switch (col.Type)
			{
			case DataTableColumnDataType::Bool:
				d.bValue = Reader.Read<uint32_t>() != 0;
				break;
			case DataTableColumnDataType::Int:
				d.iValue = Reader.Read<int32_t>();
				break;
			case DataTableColumnDataType::Float:
				d.fValue = Reader.Read<float>();
				break;
			case DataTableColumnDataType::Vector:
				d.vValue = Reader.Read<Math::Vector3>();
				break;
			case DataTableColumnDataType::Asset:
			{
				uint32_t id = Reader.Read<uint32_t>();
				uint32_t off = Reader.Read<uint32_t>();

				uint64_t pos = this->GetFileOffset(Asset, id, off);
				RpakStream->SetPosition(pos);

				d.assetValue = Reader.ReadCString();
				break;
			}
			case DataTableColumnDataType::AssetNoPrecache:
			{
				uint32_t id = Reader.Read<uint32_t>();
				uint32_t off = Reader.Read<uint32_t>();

				uint64_t pos = this->GetFileOffset(Asset, id, off);
				RpakStream->SetPosition(pos);

				d.assetNPValue = Reader.ReadCString();
				break;
			}
			case DataTableColumnDataType::StringT:
			{
				uint32_t id = Reader.Read<uint32_t>();
				uint32_t off = Reader.Read<uint32_t>();

				uint64_t pos = this->GetFileOffset(Asset, id, off);
				RpakStream->SetPosition(pos);

				d.stringValue = Reader.ReadCString();
				break;
			}
			}
			RowData.EmplaceBack(d);
		}
		Data.EmplaceBack(RowData);
	}
	return Data;
}