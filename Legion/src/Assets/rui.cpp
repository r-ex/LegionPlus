#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include <rtech.h>

void RpakLib::BuildRUIInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	RUIHeader hdr = Reader.Read<RUIHeader>();

	string AssetName = this->ReadStringFromPointer(Asset, hdr.name);

	Info.Name = AssetName;
	Info.Type = ApexAssetType::RUI;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = string::Format("Width: %.0f, Height: %.0f", hdr.elementWidth, hdr.elementHeight);
}
void RpakLib::ExportRUI(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	RUIHeader hdr = Reader.Read<RUIHeader>();
	string Name = string::Format("%s.rui", this->ReadStringFromPointer(Asset, hdr.name).ToCString());

	IO::Directory::CreateDirectory(Path);

	string DestinationPath = IO::Path::Combine(Path, Name);

	if (!Utils::ShouldWriteFile(DestinationPath))
		return;

	this->ExtractRUI(Asset, DestinationPath);
}

struct Vector4
{
	float X,Y,Z,W;
};

void RpakLib::ExtractRUI(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	RUIHeader hdr = Reader.Read<RUIHeader>();

	std::ofstream out_stream(Path.ToString(), std::ios::out);

	RpakStream->SetPosition(this->GetFileOffset(Asset, hdr.args.Index, hdr.args.Offset));

	size_t basePosition = RpakStream->GetPosition();

	out_stream << "\"ui/" << this->ReadStringFromPointer(Asset, hdr.name).ToCString() << ".rui\"\n{\n";

	// i am definitely not supposed to do it like this. oh well
	for (int i = 0; i < hdr.argCount; ++i)
	{
		RpakStream->SetPosition(basePosition + (i * sizeof(RuiArg)));
		RuiArg arg = Reader.Read<RuiArg>();

		if (arg.shortHash == 0) // if empty slot
			continue;
		string name = string::Format("%s_0x%x", s_RuiArgTypes[(uint8_t)arg.type], arg.shortHash & 0xFFFF);

		bool hasArgNames = hdr.argNames.Index || hdr.argNames.Offset;

		if(hasArgNames)
			name = this->ReadStringFromPointer(Asset, { hdr.argNames.Index, hdr.argNames.Offset + arg.nameOffset });

		out_stream << Utils::GetIndentation(1) << "\"" << name.ToCString() << "\" ";

		RpakStream->SetPosition(this->GetFileOffset(Asset, hdr.values.Index, hdr.values.Offset + arg.valueOffset));

		switch (arg.type)
		{
		case RuiArgumentType_t::TYPE_BOOL:
		{
			out_stream << std::boolalpha << Reader.Read<bool>();
			break;
		}
		case RuiArgumentType_t::TYPE_INT:
		{
			out_stream << Reader.Read<int>();
			break;
		}
		case RuiArgumentType_t::TYPE_FLOAT:
		{
			out_stream << Reader.Read<float>();
			break;
		}
		case RuiArgumentType_t::TYPE_FLOAT2:
		{
			Vector2 vec_val = Reader.Read<Vector2>();
			out_stream << "\"" << vec_val.X << " " << vec_val.Y << "\"";
			break;
		}
		case RuiArgumentType_t::TYPE_FLOAT3:
		{
			Vector3 vec_val = Reader.Read<Vector3>();
			out_stream << "\"" << vec_val.X << " " << vec_val.Y << " " << vec_val.Z << "\"";
			break;
		}
		case RuiArgumentType_t::TYPE_ASSET:
		case RuiArgumentType_t::TYPE_IMAGE:
			out_stream << "$";
		case RuiArgumentType_t::TYPE_STRING:
		{
			string val = this->ReadStringFromPointer(Asset, Reader.Read<RPakPtr>());
			out_stream << "\"" << val.ToCString() << "\"";
			break;
		}
		case RuiArgumentType_t::TYPE_GAMETIME:
		{
			out_stream << Reader.Read<float>();
			break;
		}
		case RuiArgumentType_t::TYPE_WALLTIME:
		{
			out_stream << Reader.Read<__int64>();
			break;
		}
		case RuiArgumentType_t::TYPE_COLOR_ALPHA:
		{
			Vector4 vec_val = Reader.Read<Vector4>();
			out_stream << "\"" << vec_val.X * 255 << " " << vec_val.Y * 255 << " " << vec_val.Z * 255 << " " << vec_val.W * 255 << "\"";
			break;
		}
		case RuiArgumentType_t::TYPE_UIHANDLE:
		{
			string val = "uihandle";// 

			if(Asset.AssetVersion <= 30)
				val = this->ReadStringFromPointer(Asset, Reader.Read<RPakPtr>());
			out_stream << "$\"" << val.ToCString() << "\"";
			break;
		}
		default:
		{
			out_stream << "0 // !! UNIMPLEMENTED TYPE " << (int)arg.type << " !!";
			break;
		}
		}
		out_stream << "\n";
	}

	out_stream << "}";

	out_stream.close();
}