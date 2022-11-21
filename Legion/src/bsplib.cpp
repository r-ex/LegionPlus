#include "pch.h"
#include "MdlLib.h"
#include "File.h"
#include "Directory.h"
#include "Path.h"
#include "Half.h"

#include "SEAsset.h"
#include "AutodeskMaya.h"
#include "WavefrontOBJ.h"
#include "XNALaraAscii.h"
#include "XNALaraBinary.h"
#include "ValveSMD.h"
#include "KaydaraFBX.h"
#include "CoDXAssetExport.h"

#include "bsplib.h"
#include "CastAsset.h"

void InitializeBSPModelExporter(ModelExportFormat_t Format)
{
	switch (Format)
	{
	case ModelExportFormat_t::Maya:
		s_BSPModelExporter = std::make_unique<Assets::Exporters::AutodeskMaya>();
		break;
	case ModelExportFormat_t::OBJ:
		s_BSPModelExporter = std::make_unique<Assets::Exporters::WavefrontOBJ>();
		break;
	case ModelExportFormat_t::XNALaraText:
		s_BSPModelExporter = std::make_unique<Assets::Exporters::XNALaraAscii>();
		break;
	case ModelExportFormat_t::XNALaraBinary:
		s_BSPModelExporter = std::make_unique<Assets::Exporters::XNALaraBinary>();
		break;
	case ModelExportFormat_t::SMD:
		s_BSPModelExporter = std::make_unique<Assets::Exporters::ValveSMD>();
		break;
	case ModelExportFormat_t::XModel:
		s_BSPModelExporter = std::make_unique<Assets::Exporters::CoDXAssetExport>();
		break;
	case ModelExportFormat_t::FBX:
		s_BSPModelExporter = std::make_unique<Assets::Exporters::KaydaraFBX>();
		break;
	case ModelExportFormat_t::Cast:
		s_BSPModelExporter = std::make_unique<Assets::Exporters::CastAsset>();
		break;
	default:
		s_BSPModelExporter = std::make_unique<Assets::Exporters::SEAsset>();
		break;
	}
}

List<string> ExportPropContainer(std::unique_ptr<IO::MemoryStream>& Stream, const string& Name, const string& Path)
{
	auto Reader = IO::BinaryReader(Stream.get(), true);
	auto Output = IO::BinaryWriter(IO::File::Create(IO::Path::Combine(Path, Name + ".mprt")));

	dgamelumpheader_t gamelumphdr = Reader.Read<dgamelumpheader_t>();

	// this is technically bad because the first gamelump may not be static props
	// however: i believe r2/r5 only use sprp, and we don't currently support r1 so this is fine
	dgamelump_t gamelump = Reader.Read<dgamelump_t>();
	int modelCount = Reader.Read<int>();

	List<string> Names;

	for (uint32_t i = 0; i < modelCount; i++)
	{
		char Buffer[0x80]{};
		Reader.Read(Buffer, 0, 0x80);

		Names.EmplaceBack(string(Buffer));
	}

	int propCount = Reader.Read<int>();
	Stream->Seek(0x8, IO::SeekOrigin::Current);

	Output.Write<uint32_t>(0x7472706D);
	Output.Write<uint32_t>(0x3);
	Output.Write<uint32_t>(propCount);


	List<string> propNames;
	for (uint32_t i = 0; i < propCount; i++)
	{
		auto Prop = Reader.Read<StaticPropLump_t>();
		auto Name = IO::Path::GetFileNameWithoutExtension(Names[Prop.nameIdx]).ToLower();

		if (!propNames.Contains(Name))
			propNames.EmplaceBack(Name);

		Output.WriteCString(Name);
		Output.Write<Math::Vector3>(Prop.origin);
		Output.Write<Math::Vector3>(Math::Vector3(Prop.angles.Z, Prop.angles.X, Prop.angles.Y));
		Output.Write<float>(Prop.scale);
	}

	return propNames;
}

// generic export func. decides which version to export as
void ExportBsp(const std::unique_ptr<RpakLib>& RpakFileSystem, const string& Asset, const string& Path)
{
	std::unique_ptr<IO::FileStream> stream = IO::File::OpenRead(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(stream.get(), true);
	BSPHeader_t header = Reader.Read<BSPHeader_t>();

	for (auto& it : s_BSPGameFormats)
	{
		// if magic is matched
		if (it.ident == header.ident)
		{
			// if version range is met
			if (header.version >= it.minVersion && header.version <= it.maxVersion)
			{
				// call defined export func for this version
				g_Logger.Info("Exporting bsp for '%s'\n", it.name);
				it.exportFunc(RpakFileSystem, stream, header, Asset, Path);
				return;
			}
		}
	}
}