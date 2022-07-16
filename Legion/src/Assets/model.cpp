#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"

void RpakLib::BuildModelInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	ModelHeaderS68 ModHeader{};
	if (Asset.SubHeaderSize <= 0x68)
	{
		if (Asset.AssetVersion > 8)
		{
			ModHeader = Reader.Read<ModelHeaderS68>();
		}
		else
		{
			ModelHeaderS50 ModHeaderTmp = Reader.Read<ModelHeaderS50>();
			ModHeader.SkeletonIndex = ModHeaderTmp.SkeletonIndex;
			ModHeader.SkeletonOffset = ModHeaderTmp.SkeletonOffset;

			ModHeader.NameIndex = ModHeaderTmp.NameIndex;
			ModHeader.NameOffset = ModHeaderTmp.NameOffset;
			// we don't need the rest here
		}
	}
	else
	{
		ModelHeaderS80 ModHeaderTmp = Reader.Read<ModelHeaderS80>();
		std::memcpy(&ModHeader, &ModHeaderTmp, offsetof(ModelHeaderS80, DataFlags));
		std::memcpy(&ModHeader.AnimSequenceCount, &ModHeaderTmp.AnimSequenceCount, sizeof(uint32_t) * 3);
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, ModHeader.NameIndex, ModHeader.NameOffset));

	string ModelName = Reader.ReadCString();

	if (ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = ModelName;
	else
		Info.Name = IO::Path::GetFileNameWithoutExtension(ModelName).ToLower();

	Info.Type = ApexAssetType::Model;

	RpakStream->SetPosition(this->GetFileOffset(Asset, ModHeader.SkeletonIndex, ModHeader.SkeletonOffset));

	studiohdr_t SkeletonHeader = Reader.Read<studiohdr_t>();

	if (ModHeader.AnimSequenceCount > 0)
	{
		Info.Info = string::Format("Bones: %d, Meshes: %d, Animations: %d", SkeletonHeader.BoneCount, SkeletonHeader.BodyPartCount, ModHeader.AnimSequenceCount);
	}
	else
	{
		Info.Info = string::Format("Bones: %d, Meshes: %d", SkeletonHeader.BoneCount, SkeletonHeader.BodyPartCount);
	}
}

void RpakLib::ExportModel(const RpakLoadAsset& Asset, const string& Path, const string& AnimPath)
{
	auto Model = this->ExtractModel(Asset, Path, AnimPath, true, true);

	if (Model && this->ModelExporter)
	{
		string DestinationPath = IO::Path::Combine(IO::Path::Combine(Path, Model->Name), Model->Name + "_LOD0" + (const char*)ModelExporter->ModelExtension());

		if (Utils::ShouldWriteFile(DestinationPath))
			this->ModelExporter->ExportModel(*Model.get(), DestinationPath);
	}
}

std::unique_ptr<Assets::Model> RpakLib::ExtractModel(const RpakLoadAsset& Asset, const string& Path, const string& AnimPath, bool IncludeMaterials, bool IncludeAnimations)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);
	auto Model = std::make_unique<Assets::Model>(0, 0);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	ModelHeaderS68 ModHeader{};
	if (Asset.SubHeaderSize <= 0x68)
	{
		if (Asset.AssetVersion > 8)
		{
			ModHeader = Reader.Read<ModelHeaderS68>();
		}
		else
		{
			ModelHeaderS50 mht = Reader.Read<ModelHeaderS50>();
			ModHeader.SkeletonIndex = mht.SkeletonIndex;
			ModHeader.SkeletonOffset = mht.SkeletonOffset;

			ModHeader.NameIndex = mht.NameIndex;
			ModHeader.NameOffset = mht.NameOffset;
			ModHeader.PhyIndex = mht.PhyIndex;
			ModHeader.PhyOffset = mht.PhyOffset;
			ModHeader.StreamedDataSize = mht.StreamedDataSize;
		}
	}
	else
	{
		ModelHeaderS80 ModHeaderTmp = Reader.Read<ModelHeaderS80>();
		std::memcpy(&ModHeader, &ModHeaderTmp, offsetof(ModelHeaderS80, DataFlags));
		std::memcpy(&ModHeader.AnimSequenceCount, &ModHeaderTmp.AnimSequenceCount, sizeof(uint32_t) * 3);
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, ModHeader.NameIndex, ModHeader.NameOffset));

	string ModelName = IO::Path::GetFileNameWithoutExtension(Reader.ReadCString());
	string ModelPath = IO::Path::Combine(Path, ModelName);
	string TexturePath = IO::Path::Combine(ModelPath, "_images");
	string AnimationPath = IO::Path::Combine(AnimPath, ModelName);

	Model->Name = ModelName;

	if (IncludeMaterials)
	{
		IO::Directory::CreateDirectory(ModelPath);
		IO::Directory::CreateDirectory(TexturePath);
	}

	auto ModelFormat = (ModelExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("ModelFormat");

	const uint64_t SkeletonOffset = this->GetFileOffset(Asset, ModHeader.SkeletonIndex, ModHeader.SkeletonOffset);

	bool bExportingRawRMdl = false;

	string BaseFileName = IO::Path::Combine(ModelPath, ModelName);

	if (Path != "" && AnimPath != "" && ModelFormat == ModelExportFormat_t::RMDL)
	{
		// set this here so we don't have to do this check every time
		bExportingRawRMdl = true;

		RpakStream->SetPosition(SkeletonOffset);

		studiohdr_t SkeletonHeader = Reader.Read<studiohdr_t>();

		RpakStream->SetPosition(SkeletonOffset);

		char* skelBuf = new char[SkeletonHeader.DataSize];

		Reader.Read(skelBuf, 0, SkeletonHeader.DataSize);

		uint64_t PhyOffset = 0;

		if (ModHeader.PhyIndex != 0 || ModHeader.PhyOffset != 0)
			this->GetFileOffset(Asset, ModHeader.PhyIndex, ModHeader.PhyOffset);

		// check if this model has a phy segment
		if (PhyOffset)
		{
			RpakStream->SetPosition(PhyOffset);

			auto PhyHeader = Reader.Read<RMdlPhyHeader>();

			RpakStream->SetPosition(PhyOffset + PhyHeader.TextOffset);

			string Text = Reader.ReadCString();

			uint64_t PhySize = PhyHeader.TextOffset + Text.Length();

			RpakStream->SetPosition(PhyOffset);

			char* phyBuf = new char[PhySize];

			Reader.Read(phyBuf, 0, PhySize);

			std::ofstream phyOut(BaseFileName + ".phy", std::ios::out | std::ios::binary);
			phyOut.write(phyBuf, PhySize);
			phyOut.close();
		}

		std::ofstream rmdlOut(BaseFileName + ".rmdl", std::ios::out | std::ios::binary);

		rmdlOut.write(skelBuf, SkeletonHeader.DataSize);
		rmdlOut.close();
	}

	Model->Bones = std::move(ExtractSkeleton(Reader, SkeletonOffset));

	if (!bExportingRawRMdl)
		Model->GenerateGlobalTransforms(true, true); // We need global transforms

	if (IncludeAnimations && ModHeader.AnimSequenceCount > 0 && Asset.AssetVersion > 9)
	{
		IO::Directory::CreateDirectory(AnimationPath);

		RpakStream->SetPosition(this->GetFileOffset(Asset, ModHeader.AnimSequenceIndex, ModHeader.AnimSequenceOffset));

		for (uint32_t i = 0; i < ModHeader.AnimSequenceCount; i++)
		{
			uint64_t AnimHash = Reader.Read<uint64_t>();

			if (!Assets.ContainsKey(AnimHash))
				continue;	// Should never happen

			// We need to make sure the skeleton is kept alive (copied) here...
			this->ExtractAnimation(Assets[AnimHash], Model->Bones, AnimationPath);
		}
	}

	RpakStream->SetPosition(SkeletonOffset);

	studiohdr_t SkeletonHeader{};

	if (Asset.SubHeaderSize != 120)
	{
		SkeletonHeader = Reader.Read<studiohdr_t>();
	}
	else {
		s3studiohdr_t TempSkeletonHeader = Reader.Read<s3studiohdr_t>();

		memcpy(&SkeletonHeader, &TempSkeletonHeader, 0x110);

		SkeletonHeader.SubmeshLodsOffset = TempSkeletonHeader.SubmeshLodsOffset;
		SkeletonHeader.BoneRemapCount = TempSkeletonHeader.BoneRemapCount;
	}

	uint32_t SubmeshLodsOffset = SkeletonHeader.SubmeshLodsOffset;
	uint32_t TexturesOffset = SkeletonHeader.TextureOffset;
	uint32_t TexturesCount = SkeletonHeader.TextureCount;
	uint32_t BoneRemapCount = SkeletonHeader.BoneRemapCount;
	uint32_t BoneRemapOffset = SkeletonHeader.OffsetToBoneRemapInfo;

	if (Asset.AssetVersion >= 14)
	{
		SubmeshLodsOffset = SkeletonHeader.SubmeshLodsOffset_V14;
		TexturesOffset = SkeletonHeader.TextureDirOffset;
		TexturesCount = SkeletonHeader.TextureDirCount;
		BoneRemapCount = SkeletonHeader.BoneRemapCount_V14;
	}

	RpakStream->SetPosition(SkeletonOffset + TexturesOffset);

	List<RMdlTexture> MaterialBuffer(TexturesCount, true);
	RpakStream->Read((uint8_t*)&MaterialBuffer[0], 0, TexturesCount * sizeof(RMdlTexture));

	List<uint8_t> BoneRemapTable(BoneRemapCount, true);

	if (BoneRemapCount == 0)
	{
		for (uint32_t i = 0; i < 0xFF; i++)
		{
			BoneRemapTable.EmplaceBack(i);
		}
	}
	else
	{
		RpakStream->SetPosition(SkeletonOffset + (SkeletonHeader.DataSize - BoneRemapCount));
		RpakStream->Read((uint8_t*)&BoneRemapTable[0], 0, BoneRemapCount);
	}

	RpakStream->SetPosition(SkeletonOffset + SubmeshLodsOffset);

	RMdlFixupPatches Fixups{};
	Fixups.MaterialPath = TexturePath;
	Fixups.Materials = &MaterialBuffer;
	Fixups.BoneRemaps = &BoneRemapTable;
	Fixups.FixupTableOffset = (SkeletonOffset + SubmeshLodsOffset);

	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t StarpakPatchIndex = Asset.StarpakOffset & 0xFF;
	uint64_t OptStarpakIndex = Asset.OptimalStarpakOffset & 0xFF;

	uint64_t Offset = 0;
	std::unique_ptr<IO::FileStream> StarpakStream = nullptr;

	if (Asset.OptimalStarpakOffset != -1)
	{
		Offset = ActualOptStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, true);
	}
	else if (Asset.StarpakOffset != -1)
	{
		Offset = ActualStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, false);
	}
	else
	{
		// An error occured while parsing
		return nullptr;
	}

	IO::BinaryReader StarpakReader = IO::BinaryReader(StarpakStream.get(), true);

	if (bExportingRawRMdl)
	{
		IO::Stream* StarpakStream = StarpakReader.GetBaseStream();


		if (Asset.AssetVersion <= 8) // s1
		{
			StarpakStream->SetPosition(Offset);
			char* vvBuf = new char[ModHeader.StreamedDataSize];

			StarpakReader.Read(vvBuf, 0, ModHeader.StreamedDataSize);

			std::ofstream vvOut(BaseFileName + ".vv", std::ios::out | std::ios::binary);

			vvOut.write(vvBuf, ModHeader.StreamedDataSize);
			vvOut.close();
		}
		else if (Asset.AssetVersion >= 9 && Asset.AssetVersion <= 11) // s2/3
		{

			StarpakStream->SetPosition(Offset);
			auto VGHeader = StarpakReader.Read<RMdlVGHeaderOld>();

			StarpakStream->SetPosition(Offset);
			char* vgBuf = new char[VGHeader.DataSize];

			StarpakReader.Read(vgBuf, 0, VGHeader.DataSize);

			std::ofstream vgOut(BaseFileName + ".vg", std::ios::out | std::ios::binary);

			vgOut.write(vgBuf, VGHeader.DataSize);
			vgOut.close();
		}
		else if (Asset.AssetVersion == 13)
		{

		}
		return nullptr;
	}

	if(Asset.AssetVersion >= 14)
		this->ExtractModelLod_V14(StarpakReader, RpakStream, ModelName, Offset, Model, Fixups, Asset.AssetVersion, IncludeMaterials);
	else
		this->ExtractModelLod(StarpakReader, RpakStream, ModelName, Offset, Model, Fixups, Asset.AssetVersion, IncludeMaterials);

	return std::move(Model);
}

void RpakLib::ExtractModelLod_V14(IO::BinaryReader& Reader, const std::unique_ptr<IO::MemoryStream>& RpakStream, string Name, uint64_t Offset, const std::unique_ptr<Assets::Model>& Model, RMdlFixupPatches& Fixup, uint32_t Version, bool IncludeMaterials)
{
	IO::Stream* BaseStream = Reader.GetBaseStream();

	if (!BaseStream)
	{
		g_Logger.Warning("!!! - Failed to extract Model LOD for %s. BaseStream was NULL (you probably don't have the required starpak)\n", Name.ToCString());
		return;
	}

	BaseStream->SetPosition(Offset);

	RMdlVGHeader VGHeader{};
	if (Version >= 12)
		VGHeader = Reader.Read<RMdlVGHeader>();
	else {
		RMdlVGHeaderOld TempVGHeader = Reader.Read<RMdlVGHeaderOld>();

		memcpy(&VGHeader, &TempVGHeader, 0xc);

		VGHeader.LodCount = TempVGHeader.LodCount;
		VGHeader.DataSize = TempVGHeader.DataSize;
		VGHeader.SubmeshCount = TempVGHeader.SubmeshCount;
		VGHeader.StreamFlags = 0x10;
	}

	uint32_t MainStreamFlags = VGHeader.StreamFlags;
	switch (VGHeader.StreamFlags)
	{
	case 0x10:	// submeshes follow direct
		break;
	case 0x60:	// vg header
	case 0x80:	// vg header
	case 0x90:	// vg header
	case 0xa0:
	case 0xb0:	// vg header
	case 0xc0:	// vg header
	{
		uint32_t skip = 0;
		while (true)
		{
			BaseStream->SetPosition(Offset + 0x10 * skip + sizeof(RMdlVGHeader));
			if (Reader.Read<uint32_t>() == 0x47567430)
			{
				break;
			}
			skip++;
		}
		BaseStream->SetPosition(Offset + 0x10 * skip + sizeof(RMdlVGHeader));
		VGHeader = Reader.Read<RMdlVGHeader>();

		if (MainStreamFlags == 0xa0) // idk
		{
			BaseStream->SetPosition(BaseStream->GetPosition() + 0x10);
		}
	}
	break;
	case 0x40:	// skip 0x10 * lodcount
	case 0x50:	// skip 0x10 * lodcount
		BaseStream->SetPosition(Offset + (0x10 * VGHeader.LodCount) + sizeof(RMdlVGHeader));
		break;
	case 0x20:	// skip 0x10
		BaseStream->SetPosition(Offset + 0x10 + sizeof(RMdlVGHeader));
		break;
	default:
#if _DEBUG
		printf("Unknown mesh command: 0x%x\n", VGHeader.StreamFlags);
#endif
		break;
	}

	// Offsets in submesh are relative to the submesh we're reading...
	const uint64_t SubmeshPointer = BaseStream->GetPosition();

	// We need to read the submeshes
	List<RMdlVGSubmesh_V14> SubmeshBuffer(VGHeader.SubmeshCount, true);
	Reader.Read((uint8_t*)&SubmeshBuffer[0], 0, VGHeader.SubmeshCount * sizeof(RMdlVGSubmesh_V14));

	// Loop and read submeshes
	for (uint32_t s = 0; s < VGHeader.SubmeshCount; s++)
	{
		RMdlVGSubmesh_V14& Submesh = SubmeshBuffer[s];

		// We have buffers per submesh now thank god
		List<uint8_t> VertexBuffer(Submesh.VertexCountBytes, true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh_V14)) + offsetof(RMdlVGSubmesh_V14, VertexOffset) + Submesh.VertexOffset);
		Reader.Read((uint8_t*)&VertexBuffer[0], 0, Submesh.VertexCountBytes);

		List<uint16_t> IndexBuffer(Submesh.IndexPacked.Count, true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh_V14)) + offsetof(RMdlVGSubmesh_V14, IndexOffset) + Submesh.IndexOffset);
		Reader.Read((uint8_t*)&IndexBuffer[0], 0, Submesh.IndexPacked.Count * sizeof(uint16_t));

		List<RMdlExtendedWeight> ExtendedWeights(Submesh.ExtendedWeightsCount / sizeof(RMdlExtendedWeight), true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh_V14)) + offsetof(RMdlVGSubmesh_V14, ExtendedWeightsOffset) + Submesh.ExtendedWeightsOffset);
		Reader.Read((uint8_t*)&ExtendedWeights[0], 0, Submesh.ExtendedWeightsCount);

		List<RMdlVGExternalWeights> ExternalWeightsBuffer(Submesh.ExternalWeightsCount, true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh_V14)) + offsetof(RMdlVGSubmesh_V14, ExternalWeightsOffset) + Submesh.ExternalWeightsOffset);
		Reader.Read((uint8_t*)&ExternalWeightsBuffer[0], 0, Submesh.ExternalWeightsCount * sizeof(RMdlVGExternalWeights));

		List<RMdlVGStrip> StripBuffer(Submesh.StripsCount, true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh_V14)) + offsetof(RMdlVGSubmesh_V14, StripsOffset) + Submesh.StripsOffset);
		Reader.Read((uint8_t*)&StripBuffer[0], 0, Submesh.StripsCount * sizeof(RMdlVGStrip));

		// Ignore a submesh that has no strips, otherwise there is no mesh.
		// This is likely also determined by flags == 0x0, but this is a good check.
		if (Submesh.StripsCount == 0)
		{
			continue;
		}

		List<uint8_t>& BoneRemapBuffer = *Fixup.BoneRemaps;

		Assets::Mesh& Mesh = Model->Meshes.Emplace(0x10, (((Submesh.Flags2 & 0x2) == 0x2) ? 2 : 1));	// max weights / max uvs
		RMdlVGStrip& Strip = StripBuffer[0];

		uint8_t* VertexBufferPtr = (uint8_t*)&VertexBuffer[0];
		uint16_t* FaceBufferPtr = (uint16_t*)&IndexBuffer[0];

		// Cache these here, flags in the submesh dictate what to use
		Math::Vector3 Position{};
		Math::Vector3 Normal{};
		Math::Vector2 UVs{};
		Assets::VertexColor Color{};

		for (uint32_t v = 0; v < Submesh.VertexCount; v++)
		{
			uint32_t Shift = 0;

			if ((Submesh.Flags1 & 0x1) == 0x1)
			{
				Position = *(Math::Vector3*)(VertexBufferPtr + Shift);
				Shift += sizeof(Math::Vector3);
			}
			else if ((Submesh.Flags1 & 0x2) == 0x2)
			{
				Position = (*(RMdlPackedVertexPosition*)(VertexBufferPtr + Shift)).Unpack();
				Shift += sizeof(RMdlPackedVertexPosition);
			}

			RMdlPackedVertexWeights Weights{};

			if ((Submesh.Flags1 & 0x5000) == 0x5000)
			{
				Weights = *(RMdlPackedVertexWeights*)(VertexBufferPtr + Shift);
				Shift += sizeof(RMdlPackedVertexWeights);
			}

			Normal = (*(RMdlPackedVertexNormal*)(VertexBufferPtr + Shift)).Unpack();
			Shift += sizeof(RMdlPackedVertexNormal);

			if ((Submesh.Flags1 & 0x10) == 0x10)
			{
				Color = *(Assets::VertexColor*)(VertexBufferPtr + Shift);
				Shift += sizeof(Assets::VertexColor);
			}

			UVs = *(Math::Vector2*)(VertexBufferPtr + Shift);
			Shift += sizeof(Math::Vector2);

			Assets::Vertex Vertex = Mesh.Vertices.Emplace(Position, Normal, Color, UVs);

			if ((Submesh.Flags2 & 0x2) == 0x2)
			{
				Vertex.SetUVLayer(*(Math::Vector2*)(VertexBufferPtr + Shift), 1);
				Shift += sizeof(Math::Vector2);
			}

			if ((Submesh.Flags1 & 0x5000) == 0x5000)
			{
				auto& ExternalWeights = ExternalWeightsBuffer[v];

				if (ExtendedWeights.Count() > 0)
				{
					//
					// These models have complex extended weights
					//

					uint32_t ExtendedWeightsIndex = (uint32_t)Weights.BlendIds[2] << 16;
					ExtendedWeightsIndex |= (uint32_t)Weights.BlendWeights[1];

					float CurrentWeightTotal = (float)(Weights.BlendWeights[0] + 1) / (float)0x8000;
					uint32_t WeightsIndex = 0;

					Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], CurrentWeightTotal }, WeightsIndex++);

					uint32_t ExtendedCounter = 1;
					uint32_t ExtendedComparer = 0;
					uint32_t ExtendedIndexShift = 0;
					while (true)
					{
						ExtendedComparer = (ExtendedCounter >= Weights.BlendIds[3]);
						if (ExtendedComparer != 0)
							break;

						ExtendedIndexShift = ExtendedWeightsIndex + ExtendedCounter;
						ExtendedIndexShift = ExtendedIndexShift + -1;

						RMdlExtendedWeight& ExtendedWeight = ExtendedWeights[ExtendedIndexShift];

						float ExtendedValue = (float)(ExtendedWeight.Weight + 1) / (float)0x8000;
						uint32_t ExtendedIndex = BoneRemapBuffer[ExtendedWeight.BoneId];

						Vertex.SetWeight({ ExtendedIndex, ExtendedValue }, WeightsIndex++);

						CurrentWeightTotal += ExtendedValue;
						ExtendedCounter = ExtendedCounter + 1;
					}

					if (Weights.BlendIds[0] != Weights.BlendIds[1] && CurrentWeightTotal < 1.0f)
					{
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], 1.0f - CurrentWeightTotal }, WeightsIndex);
					}
				}
				else
				{
					auto externalWeightsBufferCount = ExternalWeightsBuffer.Count();

					if (externalWeightsBufferCount > v)// || Version < 14) already checked in ExportModel()
					{
						//
						// These models have 3 or less weights
						//

						if (ExternalWeights.NumWeights == 0x1)
						{
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], 1.0f }, 0);
						}
						else if (ExternalWeights.NumWeights == 0x2)
						{
							float CurrentWeightTotal = (float)(Weights.BlendWeights[0] + 1) / (float)0x8000;

							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], CurrentWeightTotal }, 0);
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], 1.0f - CurrentWeightTotal }, 1);
						}
						else if (ExternalWeights.NumWeights == 0x3)
						{
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], ExternalWeights.SimpleWeights[0] }, 0);
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], ExternalWeights.SimpleWeights[1] }, 1);
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[2]], ExternalWeights.SimpleWeights[2] }, 2);
						}
					}
					else if (externalWeightsBufferCount == 0) // simple 'else' could be enough?
					{
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], 1.0f }, 0);
					}
				}
			}
			else if (Model->Bones.Count() > 0)
			{
				// Only a default weight is needed
				Vertex.SetWeight({ 0, 1.f }, 0);
			}

			VertexBufferPtr += Submesh.VertexBufferStride;
		}

		for (uint32_t f = 0; f < (Strip.IndexCount / 3); f++)
		{
			uint16_t i1 = *(uint16_t*)FaceBufferPtr;
			uint16_t i2 = *(uint16_t*)(FaceBufferPtr + 1);
			uint16_t i3 = *(uint16_t*)(FaceBufferPtr + 2);

			Mesh.Faces.EmplaceBack(i1, i2, i3);

			FaceBufferPtr += 3;
		}

		RpakStream->SetPosition(Fixup.FixupTableOffset);

		IO::BinaryReader SubmeshLodReader = IO::BinaryReader(RpakStream.get(), true);
		RMdlLodSubmesh SubmeshLod = SubmeshLodReader.Read<RMdlLodSubmesh>();
		RMdlTexture& Material = (*Fixup.Materials)[SubmeshLod.Index];

		if (SubmeshLod.Index < Fixup.Materials->Count() && Assets.ContainsKey(Material.MaterialHash))
		{
			RpakLoadAsset& MaterialAsset = Assets[Material.MaterialHash];

			RMdlMaterial ParsedMaterial = this->ExtractMaterial(MaterialAsset, Fixup.MaterialPath, IncludeMaterials, false);
			uint32_t MaterialIndex = Model->AddMaterial(ParsedMaterial.MaterialName, ParsedMaterial.AlbedoHash);

			Assets::Material& MaterialInstance = Model->Materials[MaterialIndex];

			if (ParsedMaterial.AlbedoMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::Albedo, { "_images\\" + ParsedMaterial.AlbedoMapName, ParsedMaterial.AlbedoHash });
			if (ParsedMaterial.NormalMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::Normal, { "_images\\" + ParsedMaterial.NormalMapName, ParsedMaterial.NormalHash });
			if (ParsedMaterial.GlossMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::Gloss, { "_images\\" + ParsedMaterial.GlossMapName, ParsedMaterial.GlossHash });
			if (ParsedMaterial.SpecularMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::Specular, { "_images\\" + ParsedMaterial.SpecularMapName, ParsedMaterial.SpecularHash });
			if (ParsedMaterial.EmissiveMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::Emissive, { "_images\\" + ParsedMaterial.EmissiveMapName, ParsedMaterial.EmissiveHash });
			if (ParsedMaterial.AmbientOcclusionMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::AmbientOcclusion, { "_images\\" + ParsedMaterial.AmbientOcclusionMapName, ParsedMaterial.AmbientOcclusionHash });
			if (ParsedMaterial.CavityMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::Cavity, { "_images\\" + ParsedMaterial.CavityMapName, ParsedMaterial.CavityHash });

			Mesh.MaterialIndices.EmplaceBack(MaterialIndex);
		}
		else
		{
			Mesh.MaterialIndices.EmplaceBack(-1);
		}

		// Add an extra slot for the extra UV Layer if present
		if ((Submesh.Flags2 & 0x2) == 0x2)
			Mesh.MaterialIndices.EmplaceBack(-1);

		Fixup.FixupTableOffset += sizeof(RMdlLodSubmesh);

		// these fixes just keep getting worse
		// (go back 4 bytes to account for the final int in the struct which only exists on v14)
		if (Version < 14)
			Fixup.FixupTableOffset -= 4;
	}
}

void RpakLib::ExtractModelLod(IO::BinaryReader& Reader, const std::unique_ptr<IO::MemoryStream>& RpakStream, string Name, uint64_t Offset, const std::unique_ptr<Assets::Model>& Model, RMdlFixupPatches& Fixup, uint32_t Version, bool IncludeMaterials)
{
	IO::Stream* BaseStream = Reader.GetBaseStream();

	if (!BaseStream)
	{
		g_Logger.Warning("!!! - Failed to extract Model LOD for %s. BaseStream was NULL (you probably don't have the required starpak)\n", Name.ToCString());
		return;
	}

	BaseStream->SetPosition(Offset);

	RMdlVGHeader VGHeader{};
	if (Version >= 12)
		VGHeader = Reader.Read<RMdlVGHeader>();
	else {
		RMdlVGHeaderOld TempVGHeader = Reader.Read<RMdlVGHeaderOld>();

		memcpy(&VGHeader, &TempVGHeader, 0xc);

		VGHeader.LodCount = TempVGHeader.LodCount;
		VGHeader.DataSize = TempVGHeader.DataSize;
		VGHeader.SubmeshCount = TempVGHeader.SubmeshCount;
		VGHeader.StreamFlags = 0x10;
	}

	uint32_t MainStreamFlags = VGHeader.StreamFlags;
	switch (VGHeader.StreamFlags)
	{
	case 0x10:	// submeshes follow direct
		break;
	case 0x60:	// vg header
	case 0x80:	// vg header
	case 0x90:	// vg header
	case 0xa0:
	case 0xb0:	// vg header
	case 0xc0:	// vg header
	{
		uint32_t skip = 0;
		while (true)
		{
			BaseStream->SetPosition(Offset + 0x10 * skip + sizeof(RMdlVGHeader));
			if (Reader.Read<uint32_t>() == 0x47567430)
			{
				break;
			}
			skip++;
		}
		BaseStream->SetPosition(Offset + 0x10 * skip + sizeof(RMdlVGHeader));
		VGHeader = Reader.Read<RMdlVGHeader>();

		if (MainStreamFlags == 0xa0) // idk
		{
			BaseStream->SetPosition(BaseStream->GetPosition() + 0x10);
		}
	}
	break;
	case 0x40:	// skip 0x10 * lodcount
	case 0x50:	// skip 0x10 * lodcount
		BaseStream->SetPosition(Offset + (0x10 * VGHeader.LodCount) + sizeof(RMdlVGHeader));
		break;
	case 0x20:	// skip 0x10
		BaseStream->SetPosition(Offset + 0x10 + sizeof(RMdlVGHeader));
		break;
	default:
#if _DEBUG
		printf("Unknown mesh command: 0x%x\n", VGHeader.StreamFlags);
#endif
		break;
	}

	// Offsets in submesh are relative to the submesh we're reading...
	const uint64_t SubmeshPointer = BaseStream->GetPosition();

	// We need to read the submeshes
	List<RMdlVGSubmesh> SubmeshBuffer(VGHeader.SubmeshCount, true);
	Reader.Read((uint8_t*)&SubmeshBuffer[0], 0, VGHeader.SubmeshCount * sizeof(RMdlVGSubmesh));

	// Loop and read submeshes
	for (uint32_t s = 0; s < VGHeader.SubmeshCount; s++)
	{
		RMdlVGSubmesh& Submesh = SubmeshBuffer[s];

		// We have buffers per submesh now thank god
		List<uint8_t> VertexBuffer(Submesh.VertexCountBytes, true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh)) + offsetof(RMdlVGSubmesh, VertexOffset) + Submesh.VertexOffset);
		Reader.Read((uint8_t*)&VertexBuffer[0], 0, Submesh.VertexCountBytes);

		List<uint16_t> IndexBuffer(Submesh.IndexPacked.Count, true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh)) + offsetof(RMdlVGSubmesh, IndexOffset) + Submesh.IndexOffset);
		Reader.Read((uint8_t*)&IndexBuffer[0], 0, Submesh.IndexPacked.Count * sizeof(uint16_t));

		List<RMdlExtendedWeight> ExtendedWeights(Submesh.ExtendedWeightsCount / sizeof(RMdlExtendedWeight), true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh)) + offsetof(RMdlVGSubmesh, ExtendedWeightsOffset) + Submesh.ExtendedWeightsOffset);
		Reader.Read((uint8_t*)&ExtendedWeights[0], 0, Submesh.ExtendedWeightsCount);

		List<RMdlVGExternalWeights> ExternalWeightsBuffer(Submesh.ExternalWeightsCount, true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh)) + offsetof(RMdlVGSubmesh, ExternalWeightsOffset) + Submesh.ExternalWeightsOffset);
		Reader.Read((uint8_t*)&ExternalWeightsBuffer[0], 0, Submesh.ExternalWeightsCount * sizeof(RMdlVGExternalWeights));

		List<RMdlVGStrip> StripBuffer(Submesh.StripsCount, true);
		BaseStream->SetPosition(SubmeshPointer + (s * sizeof(RMdlVGSubmesh)) + offsetof(RMdlVGSubmesh, StripsOffset) + Submesh.StripsOffset);
		Reader.Read((uint8_t*)&StripBuffer[0], 0, Submesh.StripsCount * sizeof(RMdlVGStrip));

		// Ignore a submesh that has no strips, otherwise there is no mesh.
		// This is likely also determined by flags == 0x0, but this is a good check.
		if (Submesh.StripsCount == 0)
		{
			continue;
		}

		List<uint8_t>& BoneRemapBuffer = *Fixup.BoneRemaps;

		Assets::Mesh& Mesh = Model->Meshes.Emplace(0x10, (((Submesh.Flags2 & 0x2) == 0x2) ? 2 : 1));	// max weights / max uvs
		RMdlVGStrip& Strip = StripBuffer[0];

		uint8_t* VertexBufferPtr = (uint8_t*)&VertexBuffer[0];
		uint16_t* FaceBufferPtr = (uint16_t*)&IndexBuffer[0];

		// Cache these here, flags in the submesh dictate what to use
		Math::Vector3 Position{};
		Math::Vector3 Normal{};
		Math::Vector2 UVs{};
		Assets::VertexColor Color{};

		for (uint32_t v = 0; v < Submesh.VertexCount; v++)
		{
			uint32_t Shift = 0;

			if ((Submesh.Flags1 & 0x1) == 0x1)
			{
				Position = *(Math::Vector3*)(VertexBufferPtr + Shift);
				Shift += sizeof(Math::Vector3);
			}
			else if ((Submesh.Flags1 & 0x2) == 0x2)
			{
				Position = (*(RMdlPackedVertexPosition*)(VertexBufferPtr + Shift)).Unpack();
				Shift += sizeof(RMdlPackedVertexPosition);
			}

			RMdlPackedVertexWeights Weights{};

			if ((Submesh.Flags1 & 0x5000) == 0x5000)
			{
				Weights = *(RMdlPackedVertexWeights*)(VertexBufferPtr + Shift);
				Shift += sizeof(RMdlPackedVertexWeights);
			}

			Normal = (*(RMdlPackedVertexNormal*)(VertexBufferPtr + Shift)).Unpack();
			Shift += sizeof(RMdlPackedVertexNormal);

			if ((Submesh.Flags1 & 0x10) == 0x10)
			{
				Color = *(Assets::VertexColor*)(VertexBufferPtr + Shift);
				Shift += sizeof(Assets::VertexColor);
			}

			UVs = *(Math::Vector2*)(VertexBufferPtr + Shift);
			Shift += sizeof(Math::Vector2);

			Assets::Vertex Vertex = Mesh.Vertices.Emplace(Position, Normal, Color, UVs);

			if ((Submesh.Flags2 & 0x2) == 0x2)
			{
				Vertex.SetUVLayer(*(Math::Vector2*)(VertexBufferPtr + Shift), 1);
				Shift += sizeof(Math::Vector2);
			}

			if ((Submesh.Flags1 & 0x5000) == 0x5000)
			{
				auto& ExternalWeights = ExternalWeightsBuffer[v];

				if (ExtendedWeights.Count() > 0)
				{
					//
					// These models have complex extended weights
					//

					uint32_t ExtendedWeightsIndex = (uint32_t)Weights.BlendIds[2] << 16;
					ExtendedWeightsIndex |= (uint32_t)Weights.BlendWeights[1];

					float CurrentWeightTotal = (float)(Weights.BlendWeights[0] + 1) / (float)0x8000;
					uint32_t WeightsIndex = 0;

					Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], CurrentWeightTotal }, WeightsIndex++);

					uint32_t ExtendedCounter = 1;
					uint32_t ExtendedComparer = 0;
					uint32_t ExtendedIndexShift = 0;
					while (true)
					{
						ExtendedComparer = (ExtendedCounter >= Weights.BlendIds[3]);
						if (ExtendedComparer != 0)
							break;

						ExtendedIndexShift = ExtendedWeightsIndex + ExtendedCounter;
						ExtendedIndexShift = ExtendedIndexShift + -1;

						RMdlExtendedWeight& ExtendedWeight = ExtendedWeights[ExtendedIndexShift];

						float ExtendedValue = (float)(ExtendedWeight.Weight + 1) / (float)0x8000;
						uint32_t ExtendedIndex = BoneRemapBuffer[ExtendedWeight.BoneId];

						Vertex.SetWeight({ ExtendedIndex, ExtendedValue }, WeightsIndex++);

						CurrentWeightTotal += ExtendedValue;
						ExtendedCounter = ExtendedCounter + 1;
					}

					if (Weights.BlendIds[0] != Weights.BlendIds[1] && CurrentWeightTotal < 1.0f)
					{
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], 1.0f - CurrentWeightTotal }, WeightsIndex);
					}
				}
				else
				{
					//
					// These models have 3 or less weights
					//

					if (ExternalWeights.NumWeights == 0x1)
					{
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], 1.0f }, 0);
					}
					else if (ExternalWeights.NumWeights == 0x2)
					{
						float CurrentWeightTotal = (float)(Weights.BlendWeights[0] + 1) / (float)0x8000;

						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], CurrentWeightTotal }, 0);
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], 1.0f - CurrentWeightTotal }, 1);
					}
					else if (ExternalWeights.NumWeights == 0x3)
					{
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], ExternalWeights.SimpleWeights[0] }, 0);
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], ExternalWeights.SimpleWeights[1] }, 1);
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[2]], ExternalWeights.SimpleWeights[2] }, 2);
					}
				}
			}
			else if (Model->Bones.Count() > 0)
			{
				// Only a default weight is needed
				Vertex.SetWeight({ 0, 1.f }, 0);
			}

			VertexBufferPtr += Submesh.VertexBufferStride;
		}

		for (uint32_t f = 0; f < (Strip.IndexCount / 3); f++)
		{
			uint16_t i1 = *(uint16_t*)FaceBufferPtr;
			uint16_t i2 = *(uint16_t*)(FaceBufferPtr + 1);
			uint16_t i3 = *(uint16_t*)(FaceBufferPtr + 2);

			Mesh.Faces.EmplaceBack(i1, i2, i3);

			FaceBufferPtr += 3;
		}

		RpakStream->SetPosition(Fixup.FixupTableOffset);

		IO::BinaryReader SubmeshLodReader = IO::BinaryReader(RpakStream.get(), true);
		RMdlLodSubmesh SubmeshLod = SubmeshLodReader.Read<RMdlLodSubmesh>();
		RMdlTexture& Material = (*Fixup.Materials)[SubmeshLod.Index];

		if (SubmeshLod.Index < Fixup.Materials->Count() && Assets.ContainsKey(Material.MaterialHash))
		{
			RpakLoadAsset& MaterialAsset = Assets[Material.MaterialHash];

			RMdlMaterial ParsedMaterial = this->ExtractMaterial(MaterialAsset, Fixup.MaterialPath, IncludeMaterials, false);
			uint32_t MaterialIndex = Model->AddMaterial(ParsedMaterial.MaterialName, ParsedMaterial.AlbedoHash);

			Assets::Material& MaterialInstance = Model->Materials[MaterialIndex];

			if (ParsedMaterial.AlbedoMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::Albedo, { "_images\\" + ParsedMaterial.AlbedoMapName, ParsedMaterial.AlbedoHash });
			if (ParsedMaterial.NormalMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::Normal, { "_images\\" + ParsedMaterial.NormalMapName, ParsedMaterial.NormalHash });
			if (ParsedMaterial.GlossMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::Gloss, { "_images\\" + ParsedMaterial.GlossMapName, ParsedMaterial.GlossHash });
			if (ParsedMaterial.SpecularMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::Specular, { "_images\\" + ParsedMaterial.SpecularMapName, ParsedMaterial.SpecularHash });
			if (ParsedMaterial.EmissiveMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::Emissive, { "_images\\" + ParsedMaterial.EmissiveMapName, ParsedMaterial.EmissiveHash });
			if (ParsedMaterial.AmbientOcclusionMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::AmbientOcclusion, { "_images\\" + ParsedMaterial.AmbientOcclusionMapName, ParsedMaterial.AmbientOcclusionHash });
			if (ParsedMaterial.CavityMapName != "")
				MaterialInstance.Slots.Add(Assets::MaterialSlotType::Cavity, { "_images\\" + ParsedMaterial.CavityMapName, ParsedMaterial.CavityHash });

			Mesh.MaterialIndices.EmplaceBack(MaterialIndex);
		}
		else
		{
			Mesh.MaterialIndices.EmplaceBack(-1);
		}

		// Add an extra slot for the extra UV Layer if present
		if ((Submesh.Flags2 & 0x2) == 0x2)
			Mesh.MaterialIndices.EmplaceBack(-1);

		Fixup.FixupTableOffset += sizeof(RMdlLodSubmesh);
	}
}

List<Assets::Bone> RpakLib::ExtractSkeleton(IO::BinaryReader& Reader, uint64_t SkeletonOffset)
{
	IO::Stream* RpakStream = Reader.GetBaseStream();

	RpakStream->SetPosition(SkeletonOffset);

	studiohdr_t SkeletonHeader = Reader.Read<studiohdr_t>();

	List<Assets::Bone> Result = List<Assets::Bone>(SkeletonHeader.BoneCount);

	for (uint32_t i = 0; i < SkeletonHeader.BoneCount; i++)
	{
		uint64_t Position = SkeletonOffset + SkeletonHeader.BoneDataOffset + (i * sizeof(mstudiobone_t));

		RpakStream->SetPosition(Position);
		mstudiobone_t Bone = Reader.Read<mstudiobone_t>();
		RpakStream->SetPosition(Position + Bone.NameOffset);

		string TagName = Reader.ReadCString();

		Result.EmplaceBack(TagName, Bone.ParentIndex, Bone.Position, Bone.Rotation);
	}

	return Result;
}

std::unique_ptr<Assets::Model> RpakLib::BuildPreviewModel(uint64_t Hash)
{
	if (!this->Assets.ContainsKey(Hash))
		return nullptr;

	auto& Asset = this->Assets[Hash];

	if (Asset.AssetType != (uint32_t)AssetType_t::Model)
		return nullptr;

	return std::move(this->ExtractModel(Asset, "", "", false, false));
}