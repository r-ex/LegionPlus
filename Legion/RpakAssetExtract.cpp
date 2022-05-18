#include "pch.h"

#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include "DDS.h"
#include "rtech.h"
#include "RpakImageTiles.h"
#include "..\cppkore_incl\OODLE\oodle2.h"

std::unique_ptr<IO::MemoryStream> DecompressStreamedBuffer(const uint8_t* Data, uint64_t& DataSize, uint8_t Format)
{
	switch ((CompressionType)Format)
	{
	case CompressionType::PAKFILE:
	{
		rpak_decomp_state state;

		uint32_t dSize = g_pRtech->DecompressPakfileInit(&state, (uint8_t*)Data, DataSize, 0, 0);

		uint8_t* Result = new uint8_t[state.decompressed_size]{};
		state.out_mask = UINT64_MAX;
		state.out = (uint64_t)Result;

		uint8_t decomp_result = g_pRtech->DecompressPakFile(&state, DataSize, dSize); // porter uses 0x400000, but using decompsize should be enough.

		DataSize = state.decompressed_size;

		return std::make_unique<IO::MemoryStream>(Result, 0, state.decompressed_size);
	}
	case CompressionType::SNOWFLAKE:
	{
		auto State = std::make_unique<uint8_t[]>(0x25000);
		g_pRtech->DecompressSnowflakeInit((long long)&State.get()[0], (int64_t)Data, DataSize);

		__int64* EditState = (__int64*)&State.get()[0];
		__int64 DecompressedSize = EditState[0x48D3];

		unsigned int v15 = *((unsigned int*)EditState + 0x91A4);
		__int64 v16 = DecompressedSize;
		*((uint32_t*)EditState + 0x91A2) = 0;
		if (v15 < DecompressedSize)
			v16 = v15;

		uint8_t* Result = new uint8_t[DecompressedSize]{};

		EditState[0x48D4] = (__int64)v16;
		EditState[0x48DA] = (__int64)Result;
		EditState[0x48DB] = 0;

		g_pRtech->DecompressSnowflake((long long)&State.get()[0], DataSize, DecompressedSize);

		DataSize = EditState[0x48db];

		return std::make_unique<IO::MemoryStream>(Result, 0, EditState[0x48db]);
	}
	case CompressionType::OODLE:
	{
		int SizeNeeded = OodleLZDecoder_MemorySizeNeeded(OodleLZ_Compressor_Invalid, -1);

		uint8_t* Decoder = new uint8_t[SizeNeeded]{};
		uint8_t* OutBuf = new uint8_t[DataSize]{};

		OodleLZDecoder_Create(OodleLZ_Compressor::OodleLZ_Compressor_Invalid, DataSize, Decoder, SizeNeeded);

		int DecPos = 0;
		int DataPos = 0;

		OodleLZ_DecodeSome_Out out{};
		if (!OodleLZDecoder_DecodeSome((OodleLZDecoder*)Decoder, &out, OutBuf, DecPos, DataSize, DataSize - DecPos, Data + DataPos, DataSize - DataPos, OodleLZ_FuzzSafe_No, OodleLZ_CheckCRC_No, OodleLZ_Verbosity::OodleLZ_Verbosity_None, OodleLZ_Decode_ThreadPhaseAll))
		{
			// If it fails it shouldn't be compressed?
			delete[] Decoder;
			delete[] OutBuf;

			return std::make_unique<IO::MemoryStream>(const_cast<uint8_t*>(Data), 0, DataSize, true, true);
		}

		while (true)
		{
			DecPos += out.decodedCount;
			DataPos += out.compBufUsed;

			if (out.compBufUsed + out.decodedCount == 0)
				break;

			if (DecPos >= DataSize)
				break;

			bool DecodeResult = OodleLZDecoder_DecodeSome((OodleLZDecoder*)Decoder, &out, OutBuf, DecPos, DataSize, DataSize - DecPos, Data + DataPos, DataSize - DataPos, OodleLZ_FuzzSafe_No, OodleLZ_CheckCRC_No, OodleLZ_Verbosity::OodleLZ_Verbosity_None, OodleLZ_Decode_ThreadPhaseAll);
		}

		delete[] Decoder;
		return std::make_unique<IO::MemoryStream>(OutBuf, 0, DataSize);
	}
	default:
	{
		DataSize = 0;
		return nullptr;
	}
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

		auto SkeletonHeader = Reader.Read<RMdlSkeletonHeader>();

		RpakStream->SetPosition(SkeletonOffset);

		char* skelBuf = new char[SkeletonHeader.DataSize];

		Reader.Read(skelBuf, 0, SkeletonHeader.DataSize);

		uint64_t PhyOffset = 0;

		if (ModHeader.PhyIndex != 0 || ModHeader.PhyOffset != 0)
			this->GetFileOffset(Asset, ModHeader.PhyIndex, ModHeader.PhyOffset);

		// check if this model has a phy segment
		if(PhyOffset)
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

	if(!bExportingRawRMdl)
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

	RMdlSkeletonHeader SkeletonHeader{};
	
	if (Asset.SubHeaderSize != 120)
	{
		SkeletonHeader = Reader.Read<RMdlSkeletonHeader>();
	}
	else {
		auto TempSkeletonHeader = Reader.Read<RMdlSkeletonHeader_S3>();

		memcpy(&SkeletonHeader, &TempSkeletonHeader, 0x110);

		SkeletonHeader.SubmeshLodsOffset = TempSkeletonHeader.SubmeshLodsOffset;
		SkeletonHeader.BoneRemapCount = TempSkeletonHeader.BoneRemapCount;
	}

	RpakStream->SetPosition(SkeletonOffset + SkeletonHeader.TextureOffset);

	List<RMdlTexture> MaterialBuffer(SkeletonHeader.TextureCount, true);
	RpakStream->Read((uint8_t*)&MaterialBuffer[0], 0, SkeletonHeader.TextureCount * sizeof(RMdlTexture));

	List<uint8_t> BoneRemapTable(SkeletonHeader.BoneRemapCount, true);

	if (SkeletonHeader.BoneRemapCount == 0)
	{
		for (uint32_t i = 0; i < 0xFF; i++)
		{
			BoneRemapTable.EmplaceBack(i);
		}
	}
	else
	{
		RpakStream->SetPosition(SkeletonOffset + (SkeletonHeader.DataSize - SkeletonHeader.BoneRemapCount));
		RpakStream->Read((uint8_t*)&BoneRemapTable[0], 0, SkeletonHeader.BoneRemapCount);
	}

	RpakStream->SetPosition(SkeletonOffset + SkeletonHeader.SubmeshLodsOffset);

	RMdlFixupPatches Fixups{};
	Fixups.MaterialPath = TexturePath;
	Fixups.Materials = &MaterialBuffer;
	Fixups.BoneRemaps = &BoneRemapTable;
	Fixups.FixupTableOffset = (SkeletonOffset + SkeletonHeader.SubmeshLodsOffset);

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

	this->ExtractModelLod(StarpakReader, RpakStream, ModelName, Offset, Model, Fixups, Asset.AssetVersion, IncludeMaterials);

	return std::move(Model);
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
	if(Version >= 12)
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

RMdlMaterial RpakLib::ExtractMaterial(const RpakLoadAsset& Asset, const string& Path, bool IncludeImages, bool IncludeImageNames)
{
	RMdlMaterial Result;

	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	MaterialHeader MatHeader = Reader.Read<MaterialHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, MatHeader.NameIndex, MatHeader.NameOffset));

	Result.MaterialName = IO::Path::GetFileNameWithoutExtension(Reader.ReadCString());

	List<ShaderResBinding> PixelShaderResBindings;

	bool shadersetLoaded = Assets.ContainsKey(MatHeader.ShaderSetHash);

	if (shadersetLoaded)
	{
		RpakLoadAsset ShaderSetAsset = Assets[MatHeader.ShaderSetHash];
		ShaderSetHeader ShaderSetHeader = ExtractShaderSet(ShaderSetAsset);

		uint64_t PixelShaderGuid = ShaderSetHeader.PixelShaderHash;

		if (ShaderSetAsset.AssetVersion <= 11)
		{
			PixelShaderGuid = ShaderSetHeader.OldPixelShaderHash;
		}

		if (Assets.ContainsKey(PixelShaderGuid))
		{
			PixelShaderResBindings = ExtractShaderResourceBindings(Assets[PixelShaderGuid], D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE);
		}
		else {
			g_Logger.Warning("Shaderset for material '%s' referenced a pixel shader that is not currently loaded. Unable to associate texture types.\n", Result.MaterialName.ToCString());
		}
	}

	g_Logger.Info("\nMaterial Info for '%s'\n", Result.MaterialName.ToCString());
	g_Logger.Info("> ShaderSet: %llx (%s)\n", MatHeader.ShaderSetHash, shadersetLoaded ? "LOADED" : "NOT LOADED");

	const uint64_t TextureTable = (Asset.Version == RpakGameVersion::Apex) ? this->GetFileOffset(Asset, MatHeader.TexturesIndex, MatHeader.TexturesOffset) : this->GetFileOffset(Asset, MatHeader.TexturesTFIndex, MatHeader.TexturesTFOffset);
	uint32_t TexturesCount = (Asset.Version == RpakGameVersion::Apex) ? 0x10 : 0x11;

	// we're actually gonna ignore the hardcoded value for apex materials
	if (Asset.Version == RpakGameVersion::Apex)
	{
		TexturesCount = (MatHeader.UnknownOffset - MatHeader.TexturesOffset) / 8;
		g_Logger.Info("> %i textures:\n", TexturesCount);
	}

	// These textures have named slots
	for (uint32_t i = 0; i < TexturesCount; i++)
	{
		RpakStream->SetPosition(TextureTable + ((uint64_t)i * 8));

		uint64_t TextureHash = Reader.Read<uint64_t>();
		string TextureName = "";
		bool bOverridden = false;
		bool bNormalRecalculate = false;

		if (TextureHash != 0)
		{

			TextureName = string::Format("0x%llx%s", TextureHash, (const char*)ImageExtension);

			if (PixelShaderResBindings.Count() > 0 && i < PixelShaderResBindings.Count())
			{
				string ResName = PixelShaderResBindings[i].Name;
				if (!ExportManager::Config.GetBool("UseTxtrGuids"))
				{
					TextureName = string::Format("%s_%s%s", Result.MaterialName.ToCString(), ResName.ToCString(), (const char*)ImageExtension);
				}
				bOverridden = true;

				if (ResName == "normalTexture")
					bNormalRecalculate = true;
			}

			if(Asset.Version == RpakGameVersion::Apex)
				g_Logger.Info(">> %i: 0x%llx - %s\n", i, TextureHash, bOverridden ? TextureName.ToCString() : "(no assigned name)");

			switch (i)
			{
			case 0:
				Result.AlbedoHash = TextureHash;
				Result.AlbedoMapName = TextureName;
				break;
			case 1:
				Result.NormalHash = TextureHash;
				Result.NormalMapName = TextureName;
				break;
			case 2:
				Result.GlossHash = TextureHash;
				Result.GlossMapName = TextureName;
				break;
			case 3:
				Result.SpecularHash = TextureHash;
				Result.SpecularMapName = TextureName;
				break;
			case 4:
				Result.EmissiveHash = TextureHash;
				Result.EmissiveMapName = TextureName;
				break;
			case 5:
				Result.AmbientOcclusionHash = TextureHash;
				Result.AmbientOcclusionMapName = TextureName;
				break;
			case 6:
				Result.CavityHash = TextureHash;
				Result.CavityMapName = TextureName;
				break;
			}
		}

		// Extract to disk if need be
		if (IncludeImages && Assets.ContainsKey(TextureHash))
		{
			RpakLoadAsset& Asset = Assets[TextureHash];
			// Make sure the data we got to is a proper texture
			if (Asset.AssetType == (uint32_t)AssetType_t::Texture)
			{
				ExportTexture(Asset, Path, IncludeImageNames, bOverridden ? TextureName : string(), bNormalRecalculate);
			}
		}
	}

	g_Logger.Info("\n");

	return Result;
}

void RpakLib::ExtractTexture(const RpakLoadAsset& Asset, std::unique_ptr<Assets::Texture>& Texture, string& Name)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	TextureHeader TexHeader = Reader.Read<TextureHeader>();

	if (Asset.AssetVersion >= 9)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
		auto TexHeaderV9 = Reader.Read<TextureHeaderV9>();

		TexHeader.NameIndex = TexHeaderV9.NameIndex;
		TexHeader.NameOffset = TexHeaderV9.NameOffset;
		TexHeader.Width = TexHeaderV9.Width;
		TexHeader.Height = TexHeaderV9.Height;
		TexHeader.Format = TexHeaderV9.Format;
		TexHeader.DataSize = TexHeaderV9.DataSize;
	}

	Assets::DDSFormat Fmt;

	Fmt.Format = TxtrFormatToDXGI[TexHeader.Format];

	if (TexHeader.NameIndex || TexHeader.NameOffset)
	{
		uint64_t NameOffset = this->GetFileOffset(Asset, TexHeader.NameIndex, TexHeader.NameOffset);

		RpakStream->SetPosition(NameOffset);

		Name = Reader.ReadCString();
	}
	else {
		Name = "";
	}

	Texture = std::make_unique<Assets::Texture>(TexHeader.Width, TexHeader.Height, Fmt.Format);

	uint64_t BlockSize = Texture->BlockSize();

	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;

	uint64_t Offset = 0;
	std::unique_ptr<IO::FileStream> StarpakStream = nullptr;
	bool bStreamed = false;

	if (Asset.OptimalStarpakOffset != -1)
	{
		Offset = ActualOptStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, true);

		if (this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap.ContainsKey(Asset.OptimalStarpakOffset))
		{
			if (Asset.AssetVersion != 9)
			{
				Offset += (this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap[Asset.OptimalStarpakOffset] - BlockSize);
				bStreamed = true;
			}
			else
			{
				auto BufferSize = this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap[Asset.OptimalStarpakOffset];
				auto Buffer = std::make_unique<uint8_t[]>(BufferSize);

				// Get location of compress starpakstream buffer.
				auto TempStream = this->GetStarpakStream(Asset, true);
				TempStream->SetPosition(ActualOptStarpakOffset);
				TempStream->Read(Buffer.get(), 0, BufferSize);
				
				// Decompress starpak texture.
				auto BufferResult = DecompressStreamedBuffer(Buffer.get(), BlockSize, (uint8_t)CompressionType::OODLE);
				BufferResult->Read(Texture->GetPixels(), 0, BlockSize);
				BufferResult.get()->Close();

				return;
			}
		}
		else
		{
			///
			// Support for finding the next highest quality mip that has a valid data source (i.e. no missing starpak)
			// This should be relatively easy
			///
			g_Logger.Warning("OptStarpak for asset 0x%llx is not loaded. Output may be incorrect/weird\n", Asset.NameHash);

			///
			// Use non-streamed data instead to make up for the missing starpak this data WILL NOT fit the intended higher quality image size
			// so the resulting image will be totally messed up
			//
			// ???: why didnt this originally just check if it also had non-opt starpak offsets and use that for the image?
			//      then at least the image would be higher quality than the highest permanent mip
			///
			if (Asset.AssetVersion != 9)
				Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset) + (TexHeader.DataSize - BlockSize);
			else
				Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset);
		}
	}
	else if (Asset.StarpakOffset != -1)
	{
		Offset = ActualStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, false);

		if (this->LoadedFiles[Asset.FileIndex].StarpakMap.ContainsKey(Asset.StarpakOffset))
		{
			if (Asset.AssetVersion != 9)
			{
				Offset += (this->LoadedFiles[Asset.FileIndex].StarpakMap[Asset.StarpakOffset] - BlockSize);
				bStreamed = true;
			}
			else
			{
				auto BufferSize = this->LoadedFiles[Asset.FileIndex].StarpakMap[Asset.StarpakOffset];
				auto Buffer = std::make_unique<uint8_t[]>(BufferSize);

				// Get location of compress starpakstream buffer.
				auto TempStream = this->GetStarpakStream(Asset, false);
				TempStream->SetPosition(ActualStarpakOffset);
				TempStream->Read(Buffer.get(), 0, BufferSize);

				// Decompress starpak texture.
				auto BufferResult = DecompressStreamedBuffer(Buffer.get(), BlockSize, (uint8_t)CompressionType::OODLE);
				BufferResult->Read(Texture->GetPixels(), 0, BlockSize);
				BufferResult.get()->Close();

				return;
			}
		}
		else
		{
			g_Logger.Warning("Starpak for asset 0x%llx is not loaded. Output may be incorrect/weird\n", Asset.NameHash);

			if (Asset.AssetVersion != 9)
				Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset) + (TexHeader.DataSize - BlockSize);
			else
				this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset);
		}
	}
	else if (Asset.RawDataIndex != -1 && Asset.RawDataIndex >= this->LoadedFiles[Asset.FileIndex].StartSegmentIndex)
	{
		//
		// All texture data is inline in rpak, we can calculate without anything else
		//

		if (Asset.AssetVersion != 9)
			Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset) + (TexHeader.DataSize - BlockSize);
		else
			Offset = this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset);
	}
	else
	{
		// No bank found
		return;
	}

	if (bStreamed)
	{
		StarpakStream->SetPosition(Offset);
		StarpakStream->Read(Texture->GetPixels(), 0, BlockSize);
	}
	else {
		RpakStream->SetPosition(Offset);
		RpakStream->Read(Texture->GetPixels(), 0, BlockSize);
	}

	// this is a kinda dumb check but i'm assuming we'll never see rpak v6 on anything other than ps4
	if (Asset.Version == RpakGameVersion::R2TT)
	{
		auto UTexture = std::make_unique<Assets::Texture>(TexHeader.Width, TexHeader.Height, Fmt.Format);

		uint8_t bpp = Texture->GetBpp();
		int vp = (bpp * 2);

		int pixbl = Texture->Pixbl();
		if (pixbl == 1)
			vp = bpp / 8;

		int blocksY = TexHeader.Height / pixbl;
		int blocksX = TexHeader.Width / pixbl;

		uint8_t tempArray[16]{};
		int tmp = 0;

		for (int i = 0; i < (blocksY + 7) / 8; i++)
		{
			for (int j = 0; j < (blocksX + 7) / 8; j++)
			{
				for (int k = 0; k < 64; k++)
				{
					int mr = Assets::Texture::Morton(k, 8, 8);
					int v0 = mr / 8;
					int v1 = mr % 8;

					std::memcpy(tempArray, Texture->GetPixels() + tmp, vp);
					tmp += vp;

					if (j * 8 + v1 < blocksX && i * 8 + v0 < blocksY)
					{
						int dstIdx = (vp) * ((i * 8 + v0) * blocksX + j * 8 + v1);
						std::memcpy(UTexture->GetPixels() + dstIdx, tempArray, vp);
					}
				}
			}
		}

		Texture = std::move(UTexture);
	}
}

void RpakLib::ExtractUIIA(const RpakLoadAsset& Asset, std::unique_ptr<Assets::Texture>& Texture)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	UIIAHeader TexHeader = Reader.Read<UIIAHeader>();
	int UnpackedShift = (~(unsigned __int8)((*(unsigned __int16*)&TexHeader.Flags) >> 5) & 2);

	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;

	std::unique_ptr<IO::MemoryStream> StarpakStream = nullptr;

	if (Asset.OptimalStarpakOffset != -1 && Asset.OptimalStarpakOffset != 0)
	{
		auto TempStream = this->GetStarpakStream(Asset, true);

		if (this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap.ContainsKey(Asset.OptimalStarpakOffset))
		{
			auto BufferSize = this->LoadedFiles[Asset.FileIndex].OptimalStarpakMap[Asset.OptimalStarpakOffset];
			auto CompressedBuffer = std::make_unique<uint8_t[]>(BufferSize);

			TempStream->SetPosition(ActualOptStarpakOffset);
			TempStream->Read(CompressedBuffer.get(), 0, BufferSize);

			StarpakStream = DecompressStreamedBuffer(CompressedBuffer.get(), BufferSize, TexHeader.Flags.CompressionType);
		}
		else
		{
			return;
		}
	}
	else if (Asset.StarpakOffset != -1 && Asset.StarpakOffset != 0)
	{
		auto TempStream = this->GetStarpakStream(Asset, false);

		if (this->LoadedFiles[Asset.FileIndex].StarpakMap.ContainsKey(Asset.StarpakOffset))
		{
			uint64_t BufferSize = this->LoadedFiles[Asset.FileIndex].StarpakMap[Asset.StarpakOffset];
			auto CompressedBuffer = std::make_unique<uint8_t[]>(BufferSize);

			TempStream->SetPosition(ActualStarpakOffset);
			TempStream->Read(CompressedBuffer.get(), 0, BufferSize);

			StarpakStream = DecompressStreamedBuffer(CompressedBuffer.get(), BufferSize, TexHeader.Flags.CompressionType);
		}
		else
		{
			return;
		}
	}
	else if (Asset.StarpakOffset == 0)
	{
		RpakStream->SetPosition(this->LoadedFiles[Asset.FileIndex].EmbeddedStarpakOffset);

		uint64_t BufferSize = this->LoadedFiles[Asset.FileIndex].EmbeddedStarpakSize;
		auto CompressedBuffer = std::make_unique<uint8_t[]>(BufferSize);

		RpakStream->Read(CompressedBuffer.get(), 0, BufferSize);

		StarpakStream = DecompressStreamedBuffer(CompressedBuffer.get(), BufferSize, TexHeader.Flags.CompressionType);
	}

	if (Asset.RawDataIndex != -1 && Asset.RawDataIndex >= this->LoadedFiles[Asset.FileIndex].StartSegmentIndex)
	{
		//
		// All texture data is inline in rpak, we can calculate without anything else
		//

		RUIImage RImage{};
		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));
		RpakStream->Read((uint8_t*)&RImage, 0, sizeof(RUIImage));

		if (RImage.HighResolutionWidth + UnpackedShift > 2)
			RImage.HighResolutionWidth += UnpackedShift;
		if (RImage.HighResolutionHeight + UnpackedShift > 2)
			RImage.HighResolutionHeight += UnpackedShift;
		if (RImage.LowResolutionWidth + UnpackedShift > 2)
			RImage.LowResolutionWidth += UnpackedShift;
		if (RImage.LowResolutionHeight + UnpackedShift > 2)
			RImage.LowResolutionHeight += UnpackedShift;

		bool UseHighResolution = StarpakStream != nullptr;

		uint32_t WidthBlocks = UseHighResolution ? (RImage.HighResolutionWidth + 29) / 31 : (RImage.LowResolutionWidth + 29) / 31;
		uint32_t HeightBlocks = UseHighResolution ? (RImage.HighResolutionHeight + 29) / 31 : (RImage.LowResolutionHeight + 29) / 31;
		uint32_t TotalBlocks = WidthBlocks * HeightBlocks;
		uint32_t Width = WidthBlocks * 32;
		uint32_t Height = HeightBlocks * 32;

		uint64_t Offset = UseHighResolution ? 0 : this->GetFileOffset(Asset, RImage.BufferIndex, RImage.BufferOffset);

		if (UseHighResolution)
		{
			RpakStream = std::move(StarpakStream);
		}
		RpakStream->SetPosition(Offset);

		auto CodePoints = std::make_unique<RUIImageTile[]>(TotalBlocks);
		uint64_t CodePointsSize = TotalBlocks * sizeof(RUIImageTile);

		// Check if we have tables for the tiles, if not, generate the opcodes directly.
		if (!UseHighResolution && (!TexHeader.Flags.HasLowTable || !TexHeader.Flags.LowTableBc7))
		{
			for (uint32_t i = 0; i < TotalBlocks; i++)
			{
				CodePoints[i].Opcode = TexHeader.Flags.LowTableBc7 ? 0x41 : 0x40;
				CodePoints[i].Offset = i * (TexHeader.Flags.LowTableBc7 ? 1024 : 512);
			}
			CodePointsSize = 0;
		}
		else if (UseHighResolution && (!TexHeader.Flags.HasHighTable || !TexHeader.Flags.HighTableBc7))
		{
			for (uint32_t i = 0; i < TotalBlocks; i++)
			{
				CodePoints[i].Opcode = TexHeader.Flags.HighTableBc7 ? 0x41 : 0x40;
				CodePoints[i].Offset = i * (TexHeader.Flags.HighTableBc7 ? 1024 : 512);
			}
			CodePointsSize = 0;
		}
		else
		{
			RpakStream->Read((uint8_t*)CodePoints.get(), 0, CodePointsSize);
		}

		uint32_t NumBc1Blocks = 0;
		uint32_t NumBc7Blocks = 0;

		for (uint32_t i = 0; i < TotalBlocks; i++)
		{
			switch (CodePoints[i].Opcode)
			{
			case 0x40: // bc1 block
				NumBc1Blocks++;
				break;
			case 0x41: // bc7 block
				NumBc7Blocks++;
				break;
			case 0xC0: // This opcode requires us to copy an existing opcode.
				CodePoints[i] = CodePoints[CodePoints[i].Offset];
				break;
			default:
				break;
			}
		}

		if (NumBc1Blocks)
		{
			//
			// We are now at the Bc1 blocks.
			//

			auto Bc1Destination = std::make_unique<uint8_t[]>(TotalBlocks * 512);
			auto Bc1Texture = std::make_unique<Assets::Texture>(Width, Height, DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM);

			//
			// Copy over the blocks
			//

			for (uint32_t y = 0; y < HeightBlocks; y++)
			{
				for (uint32_t x = 0; x < WidthBlocks; x++)
				{
					RUIImageTile& Point = CodePoints[x + (y * WidthBlocks)];
					uint32_t BlockOffset = (x * 512) + ((y * WidthBlocks) * 512);

					if (Point.Opcode != 0x40)
					{
						std::memcpy(Bc1Destination.get() + BlockOffset, RUIImageTileBc1, sizeof(RUIImageTileBc1));
						continue;
					}

					RpakStream->SetPosition(Point.Offset + Offset);

					// Bc1Destination contains 32x32 BC1 512 bytes swizzled, copy to texture location.
					RpakStream->Read(Bc1Destination.get() + BlockOffset, 0, 512);
				}
			}

			//
			// Unswizzle Bc1 blocks.
			//

			uint32_t Num5 = Height / 4;
			uint32_t Num6 = Width / 4;
			constexpr uint32_t Bc1Bpp2x = 4 * 2;

			uint64_t Bc1Offset = 0;

			for (uint32_t y = 0; y < Num5; y++)
			{
				for (uint32_t x = 0; x < Num6; x++)
				{
					uint32_t mx = x;
					uint32_t my = y;

					//g_pRtech->UnswizzleBlock(x, y, Num6, 2, mx, my);
					//g_pRtech->UnswizzleBlock(mx, my, Num6, 4, mx, my);
					//g_pRtech->UnswizzleBlock(mx, my, Num6, 8, mx, my);

					// please don't mess with this code. it only just works and idk why it's being such a pain
					
					// --- 2 ---
					int power = 2;
					int b_2 = (mx / 2 + my * (Num6 / power)) % (2 * (Num6 / power)) /
					2 + (Num6 / power) * ((mx / 2 + my * (Num6 / power)) % (2 * (Num6 / power)) % 2) +
					2 * (Num6 / power) * ((mx / 2 + my * (Num6 / power)) / (2 * (Num6 / power)));

					int c_2 = mx % 2 + 2 * (b_2 % (Num6 / power));
					mx = b_2 / (Num6 / power);
					my = c_2 / 4;
					c_2 %= 4;

					// --- 4 ---
					power = 4;
					int b_4 = (my + mx / 2 * (Num6 / power)) % (2 * (Num6 / power)) /
					2 + (Num6 / power) * ((my + mx / 2 * (Num6 / power)) % (2 * (Num6 / power)) % 2) +
					2 * (Num6 / power) * ((my + mx / 2 * (Num6 / power)) / (2 * (Num6 / power)));

					int c_4 = mx % 2 + 2 * (b_4 / (Num6 / power));
					mx = b_4 / (Num6 / power);
					my = c_4 / 4;
					c_4 %= 4;

					// --- 8 ---
					power = 8;
					int b_8 = ((c_2 + 4 * (b_4 % (Num6 / 4))) / 8 + my * (Num6 / power)) % (2 * (Num6 / power)) /
					2 + (Num6 / power) * (((c_2 + 4 * (b_4 % (Num6 / 4))) / 8 + my * (Num6 / power)) % (2 * (Num6 / power)) % 2) +
					2 * (Num6 / power) * (((c_2 + 4 * (b_4 % (Num6 / 4))) / 8 + my * (Num6 / power)) / (2 * (Num6 / power)));
					
					int v85 = b_8 / (Num6 / 8);

					my = (c_4 + 4 * ((int)b_8 / (Num6 / power)));
					mx = (c_2 + 4 * (b_4 % (Num6 / 4))) % 8 + 8 * (unsigned int)(b_8 % (Num6 / power));

					uint64_t destination = Bc1Bpp2x * (my * Num6 + mx);

					std::memcpy(Bc1Texture->GetPixels() + destination, Bc1Destination.get() + Bc1Offset, Bc1Bpp2x);
					Bc1Offset += Bc1Bpp2x;
				}
			}

			Bc1Texture->ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
			Texture = std::move(Bc1Texture);
		}
		else
		{
			// We didn't have an initial texture to use, to initialize the resulting texture
			Texture = std::make_unique<Assets::Texture>(Width, Height, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
		}

		if (NumBc7Blocks)
		{
			//
			// We are now at the Bc7 blocks.
			//

			auto Bc7Destination = std::make_unique<uint8_t[]>(TotalBlocks * 1024);
			auto Bc7Texture = std::make_unique<Assets::Texture>(Width, Height, DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM);

			//
			// Copy over the blocks
			//

			for (uint32_t y = 0; y < HeightBlocks; y++)
			{
				for (uint32_t x = 0; x < WidthBlocks; x++)
				{
					RUIImageTile& Point = CodePoints[x + (y * WidthBlocks)];
					uint32_t BlockOffset = (x * 1024) + ((y * WidthBlocks) * 1024);

					if (Point.Opcode != 0x41)
					{
						std::memcpy(Bc7Destination.get() + BlockOffset, RUIImageTileBc7, sizeof(RUIImageTileBc7));
						continue;
					}

					RpakStream->SetPosition(Point.Offset + Offset);

					// Bc1Destination contains 32x32 BC7 1024 bytes swizzled, copy to texture location.
					RpakStream->Read(Bc7Destination.get() + BlockOffset, 0, 1024);
				}
			}

			//
			// Unswizzle Bc7 blocks.
			//

			uint32_t blocksH = Height / 4;
			uint32_t blocksW = Width / 4;
			constexpr uint32_t Bc7Bpp2x = 8 * 2;

			uint64_t Bc7Offset = 0;

			for (uint32_t blockI = 0; blockI < blocksH; blockI++)
			{
				int v77 = blocksW / 2;
				int v147 = blocksW / 2;
				int v78 = blockI * (blocksW / 2);
				int v79 = blocksW / 4;
				int v80 = blocksW / 8;
				for (uint32_t blockJ = 0; blockJ < blocksW; blockJ++)
				{
					uint32_t mx = blockJ;
					uint32_t my = blockI;

					/*g_pRtech->UnswizzleBlock(blockJ, blockI, blocksW, 2, mx, my);
					g_pRtech->UnswizzleBlock(mx, my, blocksW, 4, mx, my);
					g_pRtech->UnswizzleBlock(mx, my, blocksW, 8, mx, my);*/

					// this is ugly and bad, should get put back into the function calls eventually
					int v81 = (blockJ / 2 + v78) % (2 * (blocksW / 2)) / 2
						+ v77 * ((blockJ / 2 + v78) % (2 * (blocksW / 2)) % 2)
						+ 2 * (blocksW / 2) * ((blockJ / 2 + v78) / (2 * (blocksW / 2)));
					int v82 = blockJ % 2 + 2 * (v81 % v77);
					int v83 = v81 / v77;
					int v84 = v82 / 4;
					v82 %= 4;
					int v85 = (v84 + v83 / 2 * v79) % (2 * (blocksW / 4)) / 2
						+ v79 * ((v84 + v83 / 2 * v79) % (2 * (blocksW / 4)) % 2)
						+ 2 * (blocksW / 4) * ((v84 + v83 / 2 * v79) / (2 * (blocksW / 4)));
					int v86 = v83 % 2 + 2 * (v85 / v79);
					int v87 = ((v82 + 4 * (v85 % v79)) / 8 + v86 / 4 * v80) % (2 * v80) / 2
						+ v80 * (((v82 + 4 * (v85 % v79)) / 8 + v86 / 4 * v80) % (2 * v80) % 2)
						+ 2 * v80 * (((v82 + 4 * (v85 % v79)) / 8 + v86 / 4 * v80) / (2 * v80));
					int v88 = Bc7Bpp2x
						* ((v82 + 4 * (v85 % v79)) % 8
							+ 8 * (unsigned int)(v87 % v80)
							+ blocksW * (v86 % 4 + 4 * ((int)v87 / v80)));

					//uint64_t destination = Bc7Bpp2x * (my * blocksW + mx);

					std::memcpy(Bc7Texture->GetPixels() + v88, Bc7Destination.get() + Bc7Offset, Bc7Bpp2x);
					Bc7Offset += Bc7Bpp2x;
				}
			}

			Bc7Texture->ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);

			if (NumBc1Blocks)
			{
				// Stitch the 32x32 uncompressed tiles to the final image.
				for (uint32_t y = 0; y < HeightBlocks; y++)
				{
					for (uint32_t x = 0; x < WidthBlocks; x++)
					{
						RUIImageTile& Point = CodePoints[x + (y * WidthBlocks)];

						if (Point.Opcode == 0x41)
						{
							Texture->CopyTextureSlice(Bc7Texture, {(x * 32), (y * 32), 32, 32}, (x * 32), (y * 32));
						}
					}
				}
			}
			else
			{
				// We are the final image
				Texture = std::move(Bc7Texture);
			}
		}
	}
}

void RpakLib::ExtractAnimation(const RpakLoadAsset& Asset, const List<Assets::Bone>& Skeleton, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	AnimHeader AnHeader = Reader.Read<AnimHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, AnHeader.NameIndex, AnHeader.NameOffset));

	string AnimName = IO::Path::GetFileNameWithoutExtension(Reader.ReadCString());

	const uint64_t AnimationOffset = this->GetFileOffset(Asset, AnHeader.AnimationIndex, AnHeader.AnimationOffset);

	RpakStream->SetPosition(AnimationOffset);

	RAnimHeader AnimSequenceHeader = Reader.Read<RAnimHeader>();

	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t StarpakPatchIndex = Asset.StarpakOffset & 0xFF;
	uint64_t OptStarpakIndex = Asset.OptimalStarpakOffset & 0xFF;

	uint64_t OffsetOfStarpakData = 0;
	std::unique_ptr<IO::FileStream> StarpakStream = nullptr;

	if (Asset.OptimalStarpakOffset != -1)
	{
		OffsetOfStarpakData = ActualOptStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, true);
	}
	else if (Asset.StarpakOffset != -1)
	{
		OffsetOfStarpakData = ActualStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, false);
	}

	IO::BinaryReader StarpakReader = IO::BinaryReader(StarpakStream.get(), true);

	for (uint32_t i = 0; i < AnimSequenceHeader.AnimationCount; i++)
	{
		RpakStream->SetPosition(AnimationOffset + AnimSequenceHeader.AnimationOffset + ((uint64_t)i * 0x4));

		uint32_t AnimDataOffset = Reader.Read<uint32_t>();

		RpakStream->SetPosition(AnimationOffset + AnimDataOffset);

		uint64_t offseq = RpakStream->GetPosition();

		RAnimSequenceHeader AnimDataHeader = Reader.Read<RAnimSequenceHeader>();

		if (!(AnimDataHeader.Flags & 0x20000))
			continue;

		auto Anim = std::make_unique<Assets::Animation>(Skeleton.Count());
		auto AnimCurveType = Assets::AnimationCurveMode::Absolute;

		if (AnimDataHeader.Flags & 0x4)
		{
			AnimCurveType = Assets::AnimationCurveMode::Additive;
		}

		for (auto& Bone : Skeleton)
		{
			Anim->Bones.EmplaceBack(Bone.Name(), Bone.Parent(), Bone.LocalPosition(), Bone.LocalRotation());

			List<Assets::Curve>& CurveNodes = Anim->GetNodeCurves(Bone.Name());

			// Inject curve nodes here, we can use the purge empty later to remove them
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::RotateQuaternion, AnimCurveType);
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::TranslateX, AnimCurveType);
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::TranslateY, AnimCurveType);
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::TranslateZ, AnimCurveType);
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::ScaleX, AnimCurveType);
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::ScaleY, AnimCurveType);
			CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::ScaleZ, AnimCurveType);
		}

		const uint64_t AnimHeaderPointer = AnimationOffset + AnimDataOffset;

		for (uint32_t Frame = 0; Frame < AnimDataHeader.FrameCount; Frame++)
		{
			uint32_t ChunkTableIndex = 0;
			uint32_t ChunkFrame = Frame;
			uint32_t FrameCountOneLess = 0;
			uint32_t FirstChunk = AnimDataHeader.FirstChunkOffset;
			uint64_t ChunkDataOffset = 0;
			uint32_t IsChunkInStarpak = 0;
			uint64_t ResultDataPtr = 0;

			if (!AnimDataHeader.FrameMedianCount)
			{
				// Nothing here
				goto nomedian;
			}
			else if (ChunkFrame >= AnimDataHeader.FrameSplitCount)
			{
				uint32_t FrameCount = AnimDataHeader.FrameCount;
				uint32_t ChunkFrameMinusSplitCount = ChunkFrame - AnimDataHeader.FrameSplitCount;
				if (FrameCount <= AnimDataHeader.FrameMedianCount || ChunkFrame != FrameCount - 1)
				{
					ChunkTableIndex = ChunkFrameMinusSplitCount / AnimDataHeader.FrameMedianCount + 1;
					ChunkFrame = ChunkFrame - (AnimDataHeader.FrameMedianCount * (ChunkFrameMinusSplitCount / AnimDataHeader.FrameMedianCount)) - AnimDataHeader.FrameSplitCount;
				}
				else
				{
					ChunkFrame = 0;
					ChunkTableIndex = (FrameCount - AnimDataHeader.FrameSplitCount - 1) / AnimDataHeader.FrameMedianCount + 2;
				}
			}

			ChunkDataOffset = AnimDataHeader.OffsetToChunkOffsetsTable + 8 * (uint64_t)ChunkTableIndex;

			RpakStream->SetPosition(AnimHeaderPointer + ChunkDataOffset);
			FirstChunk = Reader.Read<uint32_t>();

			RpakStream->SetPosition(AnimHeaderPointer + ChunkDataOffset + 4);
			IsChunkInStarpak = Reader.Read<uint32_t>();

			if (IsChunkInStarpak)
			{
				uint64_t v13 = AnimDataHeader.SomeDataOffset;
				if (v13)
				{
					ResultDataPtr = v13 + FirstChunk;
				}
				else
				{
					RpakStream->SetPosition(AnimHeaderPointer + ChunkDataOffset);
					uint32_t v14 = Reader.Read<uint32_t>();
					ResultDataPtr = OffsetOfStarpakData + v14;
				}
			}
			else
			{
			nomedian:
				ResultDataPtr = AnimHeaderPointer + FirstChunk;
			}

			char BoneFlags[256]{};

			if (IsChunkInStarpak && Asset.AssetVersion > 7)
			{
				StarpakStream->SetPosition(ResultDataPtr);
				StarpakStream->Read((uint8_t*)BoneFlags, 0, ((4 * (uint64_t)Skeleton.Count() + 7) / 8 + 1) & 0xFFFFFFFFFFFFFFFE);
			}
			else
			{
				RpakStream->SetPosition(ResultDataPtr);
				RpakStream->Read((uint8_t*)BoneFlags, 0, ((4 * (uint64_t)Skeleton.Count() + 7) / 8 + 1) & 0xFFFFFFFFFFFFFFFE);
			}

			for (uint32_t b = 0; b < Skeleton.Count(); b++)
			{
				uint32_t Shift = 4 * (b % 2);
				char BoneTrackFlags = BoneFlags[b / 2] >> Shift;

				if (BoneTrackFlags & 0x7)
				{
					uint64_t TrackDataRead = 0;

					RAnimBoneFlag BoneDataFlags = IsChunkInStarpak ? StarpakReader.Read<RAnimBoneFlag>() : Reader.Read<RAnimBoneFlag>();
					auto BoneTrackData = IsChunkInStarpak ? StarpakReader.Read((BoneDataFlags.Size > 0) ? BoneDataFlags.Size - sizeof(uint16_t) : 0, TrackDataRead) : Reader.Read((BoneDataFlags.Size > 0) ? BoneDataFlags.Size - sizeof(uint16_t) : 0, TrackDataRead);

					uint16_t* BoneTrackDataPtr = (uint16_t*)BoneTrackData.get();

					// Set this so when we do translations we will know whether or not to add the rest position onto it...
					BoneDataFlags.bAdditiveCustom = (AnimCurveType == Assets::AnimationCurveMode::Additive);

					if (BoneTrackFlags & 0x1)
						ParseRAnimBoneTranslationTrack(BoneDataFlags, &BoneTrackDataPtr, Anim, b, ChunkFrame, Frame);
					if (BoneTrackFlags & 0x2)
						ParseRAnimBoneRotationTrack(BoneDataFlags, &BoneTrackDataPtr, Anim, b, ChunkFrame, Frame);
					if (BoneTrackFlags & 0x4)
						ParseRAnimBoneScaleTrack(BoneDataFlags, &BoneTrackDataPtr, Anim, b, ChunkFrame, Frame);
				}
			}
		}

		Anim->RemoveEmptyNodes();

		string DestinationPath = IO::Path::Combine(Path, AnimName + string::Format("_%d", i) + (const char*)this->AnimExporter->AnimationExtension());

		if (!Utils::ShouldWriteFile(DestinationPath))
			continue;

		try
		{
			this->AnimExporter->ExportAnimation(*Anim.get(), DestinationPath);
		}
		catch (...)
		{
		}
	}
}

List<Assets::Bone> RpakLib::ExtractSkeleton(IO::BinaryReader& Reader, uint64_t SkeletonOffset)
{
	IO::Stream* RpakStream = Reader.GetBaseStream();

	RpakStream->SetPosition(SkeletonOffset);

	RMdlSkeletonHeader SkeletonHeader = Reader.Read<RMdlSkeletonHeader>();

	List<Assets::Bone> Result = List<Assets::Bone>(SkeletonHeader.BoneCount);

	for (uint32_t i = 0; i < SkeletonHeader.BoneCount; i++)
	{
		uint64_t Position = SkeletonOffset + SkeletonHeader.BoneDataOffset + (i * sizeof(RMdlBone));

		RpakStream->SetPosition(Position);
		RMdlBone Bone = Reader.Read<RMdlBone>();
		RpakStream->SetPosition(Position + Bone.NameOffset);

		string TagName = Reader.ReadCString();

		Result.EmplaceBack(TagName, Bone.ParentIndex, Bone.Position, Bone.Rotation);
	}

	return Result;
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

		// todo: season 3 and earlier will not have "Unk8", but all later builds do,
		// in order to maintain compatibility, something must be used to determine which version of dtbl this is
		if(Asset.Version == RpakGameVersion::Apex)
			col.Unk8 = Reader.Read<uint64_t>(); // old apex datatables do not have this

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

			uint64_t base_pos = rows_seek + (DtblHeader.RowStride * i);
			RpakStream->SetPosition(base_pos + col.RowOffset);

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

void RpakLib::ExtractShader(const RpakLoadAsset& Asset, const string& Path)
{
	if (!Utils::ShouldWriteFile(Path))
		return;

	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

	ShaderDataHeader DataHeader = Reader.Read<ShaderDataHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, DataHeader.ByteCodeIndex, DataHeader.ByteCodeOffset));

	char* bcBuf = new char[DataHeader.DataSize];

	Reader.Read(bcBuf, 0, DataHeader.DataSize);

	std::ofstream shaderOut(Path.ToCString(), std::ios::binary | std::ios::out);
	shaderOut.write(bcBuf, DataHeader.DataSize);
	shaderOut.close();
}

ShaderSetHeader RpakLib::ExtractShaderSet(const RpakLoadAsset& Asset)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	ShaderSetHeader Header = Reader.Read<ShaderSetHeader>();

	return Header;
}

List<ShaderVar> RpakLib::ExtractShaderVars(const RpakLoadAsset& Asset, D3D_SHADER_VARIABLE_TYPE VarsType)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	List<ShaderVar> Vars;

	if (Asset.RawDataIndex == -1)
		return Vars;

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

	ShaderDataHeader DataHeader = Reader.Read<ShaderDataHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, DataHeader.ByteCodeIndex, DataHeader.ByteCodeOffset));

	const uint64_t BasePos = RpakStream->GetPosition();
	DXBCHeader hdr = Reader.Read<DXBCHeader>();

	List<uint32_t> ChunkOffsets(hdr.ChunkCount, true);
	Reader.Read((uint8_t*)&ChunkOffsets[0], 0, hdr.ChunkCount*sizeof(uint32_t));

	for (uint32_t i = 0; i < hdr.ChunkCount; ++i)
		ChunkOffsets[i] += BasePos;

	for (auto& ChunkOffset : ChunkOffsets)
	{
		RpakStream->SetPosition(ChunkOffset);

		if (Reader.Read<uint32_t>() == 'FEDR') // Resource Definitions
		{
			RpakStream->SetPosition(ChunkOffset);

			RDefHeader RDefHdr = Reader.Read<RDefHeader>();

			if (RDefHdr.MajorVersion >= 5)
			{
				RpakStream->Seek(32, IO::SeekOrigin::Current); // skip RD11 header
			}
			
			uint64_t ConstBufferPos = (ChunkOffset + 8) + RDefHdr.ConstBufferOffset;

			for (uint32_t i = 0; i < RDefHdr.ConstBufferCount; ++i)
			{
				RpakStream->SetPosition(ConstBufferPos + (i * sizeof(RDefConstBuffer)));
				RDefConstBuffer ConstBuffer = Reader.Read<RDefConstBuffer>();

				for (uint32_t j = 0; j < ConstBuffer.VariableCount; ++j)
				{
					RpakStream->SetPosition((ChunkOffset + 8) + ConstBuffer.VariableOffset + (j*sizeof(RDefCBufVar)));

					RDefCBufVar CBufVar = Reader.Read<RDefCBufVar>();

					uint64_t NameOffset = ChunkOffset + 8 + CBufVar.NameOffset;
					uint64_t TypeOffset = ChunkOffset + 8 + CBufVar.TypeOffset;

					RpakStream->SetPosition(NameOffset);
					string Name = Reader.ReadCString();

					RpakStream->SetPosition(TypeOffset);
					RDefCBufVarType Type = Reader.Read<RDefCBufVarType>();

					ShaderVar Var;

					Var.Name = Name;
					Var.Type = (D3D_SHADER_VARIABLE_TYPE)Type.Type;
					
					// make sure that the VarsType arg is actually specified and then check if this var matches that type
					if (Var.Type != D3D_SVT_FORCE_DWORD && Var.Type == VarsType)
						Vars.EmplaceBack(Var);
				}
			}
			break;
		}
	}
	return Vars;
}

List<ShaderResBinding> RpakLib::ExtractShaderResourceBindings(const RpakLoadAsset& Asset, D3D_SHADER_INPUT_TYPE InputType)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	List<ShaderResBinding> ResBindings;

	if (Asset.RawDataIndex == -1)
		return ResBindings;

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

	ShaderDataHeader DataHeader = Reader.Read<ShaderDataHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, DataHeader.ByteCodeIndex, DataHeader.ByteCodeOffset));

	const uint64_t BasePos = RpakStream->GetPosition();
	auto hdr = Reader.Read<DXBCHeader>();

	List<uint32_t> ChunkOffsets(hdr.ChunkCount, true);
	Reader.Read((uint8_t*)&ChunkOffsets[0], 0, hdr.ChunkCount * sizeof(uint32_t));

	for (uint32_t i = 0; i < hdr.ChunkCount; ++i)
		ChunkOffsets[i] += BasePos;

	for (auto& ChunkOffset : ChunkOffsets)
	{
		RpakStream->SetPosition(ChunkOffset);

		if (Reader.Read<uint32_t>() == 'FEDR') // Resource Definitions
		{
			RpakStream->SetPosition(ChunkOffset);

			RDefHeader RDefHdr = Reader.Read<RDefHeader>();

			if (RDefHdr.MajorVersion >= 5)
			{
				RpakStream->Seek(32, IO::SeekOrigin::Current); // skip RD11 header
			}

			uint64_t ResBindingPos = (ChunkOffset + 8) + RDefHdr.ResBindingOffset;

			for (uint32_t i = 0; i < RDefHdr.ResBindingCount; ++i)
			{
				RpakStream->SetPosition(ResBindingPos + (i * sizeof(RDefResBinding)));
				RDefResBinding ResBinding = Reader.Read<RDefResBinding>();

				uint64_t NameOffset = ChunkOffset + 8 + ResBinding.NameOffset;

				RpakStream->SetPosition(NameOffset);
				string Name = Reader.ReadCString();

				ShaderResBinding Res;
				Res.Name = Name;
				Res.Type = ResBinding.InputType;

				if (Res.Type == InputType)
				{
					ResBindings.EmplaceBack(Res);
				}
			}
			break;
		}
	}
	return ResBindings;
}

void RpakLib::ExtractUIImageAtlas(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	if (Asset.RawDataIndex == -1) // no uvs - we shouldn't be able to get to this point
		return;

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
	
	UIAtlasHeader Header = Reader.Read<UIAtlasHeader>();

	if (!Assets.ContainsKey(Header.TextureGuid)) // can't get the images without texture data so let's head out
		return;

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

	List<UIAtlasImage> UIAtlasImages(Header.TexturesCount, true);

	for (int i = 0; i < Header.TexturesCount; ++i)
	{
		UIAtlasUV uvs = Reader.Read<UIAtlasUV>();
		UIAtlasImages[i].uvs = uvs;
		UIAtlasImages[i].PosX = uvs.uv0x * Header.Width;
		UIAtlasImages[i].PosY = uvs.uv0y * Header.Height;
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, Header.TextureOffsetsIndex, Header.TextureOffsetsOffset));

	for (int i = 0; i < Header.TexturesCount; ++i)
	{
		UIAtlasOffset offset = Reader.Read<UIAtlasOffset>();
		UIAtlasImages[i].offsets = offset;
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, Header.TextureDimsIndex, Header.TextureDimsOffset));

	for (int i = 0; i < Header.TexturesCount; ++i)
	{
		UIAtlasOffset offsets = UIAtlasImages[i].offsets;
		UIAtlasUV uvs = UIAtlasImages[i].uvs;

		UIAtlasImages[i].Width = Reader.Read<uint16_t>();
		UIAtlasImages[i].Height = Reader.Read<uint16_t>();

		// ya
		if (offsets.endX < 1)
			UIAtlasImages[i].Width = Header.Width * uvs.uv1x;
		if (offsets.endY < 1)
			UIAtlasImages[i].Height = Header.Height * uvs.uv1y;
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, Header.TextureHashesIndex, Header.TextureHashesOffset));

	for (int i = 0; i < Header.TexturesCount; ++i)
	{
		UIAtlasImages[i].Hash = Reader.Read<uint32_t>();
		UIAtlasImages[i].PathTableOffset = Reader.Read<uint64_t>();
	}

	if (Header.TextureNamesIndex != 0 || Header.TextureNamesOffset != 0)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, Header.TextureNamesIndex, Header.TextureNamesOffset));
		for (int i = 0; i < Header.TexturesCount; ++i)
		{
			UIAtlasImages[i].Path = Reader.ReadCString();
		}
	}

	std::unique_ptr<Assets::Texture> Texture = nullptr;
	string Name;

	this->ExtractTexture(Assets[Header.TextureGuid], Texture, Name);
	Texture->ConvertToFormat(DXGI_FORMAT_R8G8B8A8_UNORM);

	for (auto& img : UIAtlasImages)
	{
		std::unique_ptr<Assets::Texture> tmp = std::make_unique<Assets::Texture>(img.Width, img.Height, Texture.get()->Format());
		uint32_t BytesPerPixel = tmp->GetBpp() / 8;

		DirectX::Rect srcRect{ img.PosX, img.PosY, img.Width, img.Height };

		tmp->CopyTextureSlice(Texture, srcRect, 0, 0);

		string ImageName = string::Format("0x%x%s", img.Hash, (const char*)ImageExtension);

		if (img.Path.Length() > 0)
			ImageName = string::Format("%s%s", IO::Path::GetFileNameWithoutExtension(img.Path).ToCString(), (const char*)ImageExtension);

		tmp->Save(IO::Path::Combine(Path, ImageName), ImageSaveType);
	}
}