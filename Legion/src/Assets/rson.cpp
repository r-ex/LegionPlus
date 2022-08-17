#include "pch.h"
#include "RpakLib.h"
#include <Path.h>
#include <Directory.h>

void RpakLib::BuildRSONInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	Info.Name = string::Format("rson_0x%llx", Asset.NameHash);
	Info.Type = ApexAssetType::RSON;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = "N/A";
}

void RpakLib::ExportRSON(const RpakLoadAsset& Asset, const string& Path)
{
	string Name = string::Format("0x%llx.rson", Asset.NameHash);

	IO::Directory::CreateDirectory(Path);

	string DestinationPath = IO::Path::Combine(Path, Name);

	if (!Utils::ShouldWriteFile(DestinationPath))
		return;

	this->ExtractRSON(Asset, DestinationPath);
}

std::string GetIndentation(int level)
{
	std::string ret = "";

	for (int i = 0; i < level; ++i)
		ret += '\t';
	return ret;
}


void RpakLib::R_WriteRSONFile(const RpakLoadAsset& Asset, std::ofstream& out, IO::BinaryReader& Reader, RSONNode node, int level)
{
	auto RpakStream = Reader.GetBaseStream();
	string name = this->ReadStringFromPointer(Asset, node.pName);
	out << GetIndentation(level) << name.ToString() << ":";

	switch (node.type)
	{
	case 2: // single string value
	{
		string value = this->ReadStringFromPointer(Asset, node.pValues);
		out << " \"" + value.ToString() + "\"\n";
		break;
	}
	case 8: // object
	{
		for (int i = 0; i < node.valueCount; ++i)
		{
			// i hate this so much
			RpakStream->SetPosition(this->GetFileOffset(Asset, node.pValues.Index, node.pValues.Offset + (i*sizeof(RSONNode))));

			out << "\n" << GetIndentation(level) << "{\n";

			while (true)
			{
				RSONNode newNode = Reader.Read<RSONNode>();

				uint64_t startPos = RpakStream->GetPosition();

				this->R_WriteRSONFile(Asset, out, Reader, newNode, level + 1);

				RpakStream->SetPosition(startPos);

				// after the node, there is a slot for a pointer to another node, check if it is present
				RPakPtr nextPtr = Reader.Read<RPakPtr>();

				// check if the pointer is actually a pointer
				if (nextPtr.Index == 0 && nextPtr.Offset == 0)
					break;

				RpakStream->SetPosition(this->GetFileOffset(Asset, nextPtr.Index, nextPtr.Offset));
			}

			out << GetIndentation(level) << "}\n";
		}
		break;
	}
	case 4098: // list of strings
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, node.pValues.Index, node.pValues.Offset));
		out << "\n" << GetIndentation(level) << "[\n";
		for (int i = 0; i < node.valueCount; ++i)
		{
			string value = this->ReadStringFromPointer(Asset, Reader.Read<RPakPtr>());

			out << GetIndentation(level + 1) << value.ToString() << "\n";
		}
		out << GetIndentation(level) << "]\n";
		break;
	}
	default:
		out << "!!! NOT IMPLEMENTED !!! nodeType " << node.type << "\n";
		g_Logger.Info("!!! rson nodeType %i not implemented !!!\n", node.type);
		break;
	}
}

void RpakLib::ExtractRSON(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	RSONHeader header = Reader.Read<RSONHeader>();

	std::ofstream out_stream(Path.ToString(), std::ios::out);



	switch (header.type)
	{
	case 0x1002:
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, header.pNodes.Index, header.pNodes.Offset));
		
		out_stream << "[\n";
		for (int i = 0; i < header.nodeCount; ++i)
		{
			RPakPtr ptr = Reader.Read<RPakPtr>();
			string value = this->ReadStringFromPointer(Asset, ptr);
			out_stream << GetIndentation(1) << value << "\n";
		}
		out_stream << "]";
		break;
	}
	case 0x1008:
	{
		for (int i = 0; i < header.nodeCount; ++i)
		{
			RpakStream->SetPosition(this->GetFileOffset(Asset, header.pNodes.Index, header.pNodes.Offset + (i * sizeof(RPakPtr))));

			RPakPtr ptr = Reader.Read<RPakPtr>();

			if (ptr.Index == 0 && ptr.Offset == 0)
				continue;

			RpakStream->SetPosition(this->GetFileOffset(Asset, ptr.Index, ptr.Offset));

			RSONNode node = Reader.Read<RSONNode>();

			this->R_WriteRSONFile(Asset, out_stream, Reader, node, 0);
		}
	}
	}


	out_stream.close();
}