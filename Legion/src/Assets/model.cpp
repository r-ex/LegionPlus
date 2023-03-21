#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include <rtech.h>

void RpakLib::BuildModelInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	ModelHeader mdlHdr;
	mdlHdr.ReadFromAssetStream(&RpakStream, Asset.SubHeaderSize, Asset.AssetVersion);

	mdlHdr.name = this->ReadStringFromPointer(Asset, mdlHdr.pName);

	if (ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = mdlHdr.name;
	else
		Info.Name = IO::Path::GetFileNameWithoutExtension(mdlHdr.name).ToLower();

	Info.Type = ApexAssetType::Model;

	RpakStream->SetPosition(this->GetFileOffset(Asset, mdlHdr.studioData.Index, mdlHdr.studioData.Offset));

	if (Asset.AssetVersion < 16)
	{
		studiohdr_t studiohdr = Reader.Read<studiohdr_t>();

		Info.Info = string::Format("Bones: %d, Parts: %d", studiohdr.numbones, studiohdr.numbodyparts);

		if (mdlHdr.animSeqCount > 0)
			Info.Info += string::Format(", Animations: %d", mdlHdr.animSeqCount);
	}
	else
	{
		studiohdr_t_v16 studiohdr = Reader.Read<studiohdr_t_v16>();

		Info.Info = string::Format("Bones: %d, Parts: %d", studiohdr.numbones, studiohdr.numbodyparts);

		if (mdlHdr.animSeqCount > 0)
			Info.Info += string::Format(", Animations: %d", mdlHdr.animSeqCount);

		if (studiohdr.numskinfamilies > 1)
			Info.Info += string::Format(", Skins: %d", studiohdr.numskinfamilies);
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

#define FIX_OFFSET(offset) ((offset & 0xFFFE) << (4 * (offset & 1)))

std::unique_ptr<Assets::Model> RpakLib::ExtractModel_V16(const RpakLoadAsset& Asset, const string& Path, const string& AnimPath, bool IncludeMaterials, bool IncludeAnimations)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);
	auto Model = std::make_unique<Assets::Model>(0, 0);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	ModelHeader mdlHdr{};
	mdlHdr.ReadFromAssetStream(&RpakStream, Asset.SubHeaderSize, Asset.AssetVersion);

	ModelCPU cpuData{};
	if (Asset.RawDataIndex || Asset.RawDataOffset)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));
		cpuData = Reader.Read<ModelCPU>();
	}

	mdlHdr.name = this->ReadStringFromPointer(Asset, mdlHdr.pName);

#if _DEBUG
	if (mdlHdr.animRigCount > 0)
	{
		g_Logger.Info("====================== RIGS\n");
		for (int i = 0; i < mdlHdr.animRigCount; i++)
		{
			RpakStream->SetPosition(this->GetFileOffset(Asset, mdlHdr.animRigs.Index, mdlHdr.animRigs.Offset + (sizeof(uint64_t) * i)));
			uint64_t RigGuid = Reader.Read<uint64_t>();

			if (Assets.ContainsKey(RigGuid))
				g_Logger.Info("Rig %d -> %s\n", i, this->ExtractAnimationRig(Assets[RigGuid]).ToCString());
			else
				g_Logger.Info("Rig %d -> %llx\n", i, RigGuid);
		}
	}

	if (mdlHdr.animSeqCount > 0)
	{
		g_Logger.Info("====================== RSEQS\n");
		for (int i = 0; i < mdlHdr.animSeqCount; i++)
		{
			RpakStream->SetPosition(this->GetFileOffset(Asset, mdlHdr.animSeqs.Index, mdlHdr.animSeqs.Offset + (sizeof(uint64_t) * i)));
			uint64_t SeqGuid = Reader.Read<uint64_t>();

			if (Assets.ContainsKey(SeqGuid))
				g_Logger.Info("Seq %d -> %s\n", i, this->ExtractAnimationSeq(Assets[SeqGuid]).ToCString());
			else
				g_Logger.Info("Seq %d -> %llx\n", i, SeqGuid);

		}
		g_Logger.Info("======================\n");
	}
#endif

	string RawModelName = mdlHdr.name;
	string ModelName = IO::Path::GetFileNameWithoutExtension(RawModelName);
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

	const uint64_t StudioOffset = this->GetFileOffset(Asset, mdlHdr.studioData.Index, mdlHdr.studioData.Offset);

	bool bExportingRawRMdl = false;

	string BaseFileName = IO::Path::Combine(ModelPath, ModelName);

	RpakStream->SetPosition(StudioOffset);

	studiohdr_t_v16 studiohdr = Reader.Read<studiohdr_t_v16>();

	RpakStream->SetPosition(StudioOffset);

	std::unique_ptr<char[]> studioBuf(new char[cpuData.modelLength]);

	Reader.Read(studioBuf.get(), 0, cpuData.modelLength);

	// write QC file when exporting as SMD
	if (Path != "" && AnimPath != "" && ModelFormat == ModelExportFormat_t::SMD)
	{
		this->ExportQC(Asset.AssetVersion, BaseFileName + ".qc", RawModelName, studioBuf.get(), nullptr);
	}

	if (Path != "" && AnimPath != "" && ModelFormat == ModelExportFormat_t::RMDL)
	{
		// set this here so we don't have to do this check every time
		bExportingRawRMdl = true;

		uint64_t PhyOffset = 0;

		// phy is in cpu now, no ptr in header as far as I know
		if (cpuData.phyDataSize)
			PhyOffset = this->GetFileOffset(Asset, cpuData.phyData.Index, cpuData.phyData.Offset);

		// check if this model has a phy segment
		if (PhyOffset)
		{
			int PhySize = cpuData.phyDataSize; // handily provided size

			RpakStream->SetPosition(PhyOffset);

			char* phyBuf = new char[PhySize];

			Reader.Read(phyBuf, 0, PhySize);

			std::ofstream phyOut(BaseFileName + ".phy", std::ios::out | std::ios::binary);
			phyOut.write(phyBuf, PhySize);
			phyOut.close();
		}

		std::ofstream rmdlOut(BaseFileName + ".rmdl", std::ios::out | std::ios::binary);

		rmdlOut.write(studioBuf.get(), cpuData.modelLength);
		rmdlOut.close();
	}

	Model->Bones = std::move(ExtractSkeleton_V16(Reader, StudioOffset, Asset.AssetVersion, Asset.SubHeaderSize));

	if (!bExportingRawRMdl)
		Model->GenerateGlobalTransforms(true, true); // We need global transforms

	if (IncludeAnimations && mdlHdr.animSeqCount > 0)
	{
		IO::Directory::CreateDirectory(AnimationPath);

		RpakStream->SetPosition(this->GetFileOffset(Asset, mdlHdr.animSeqs.Index, mdlHdr.animSeqs.Offset));

		for (uint32_t i = 0; i < mdlHdr.animSeqCount; i++)
		{
			uint64_t AnimHash = Reader.Read<uint64_t>();

			if (!Assets.ContainsKey(AnimHash))
				continue;	// Should never happen

			// We need to make sure the skeleton is kept alive (copied) here...
			if (!bExportingRawRMdl)
				this->ExtractAnimation_V11(Assets[AnimHash], Model->Bones, AnimationPath);
			else
				this->ExportAnimationSeq(Assets[AnimHash], AnimationPath);
		}
	}

	RpakStream->SetPosition(StudioOffset);

	uint32_t meshOffset = 0;// SkeletonHeader.SubmeshLodsOffset;

	if (studiohdr.numbodyparts > 0)
	{
		RpakStream->SetPosition(StudioOffset + studiohdr.bodypartindex);
		mstudiobodyparts_t_v16 bodyPart = Reader.Read<mstudiobodyparts_t_v16>();

		meshOffset = studiohdr.bodypartindex + bodyPart.meshindex;
	}

	uint32_t BoneRemapCount = studiohdr.numboneremaps;// SkeletonHeader.BoneRemapCount;
	uint32_t BoneRemapOffset = FIX_OFFSET(studiohdr.boneremapindex);

	RpakStream->SetPosition(StudioOffset + studiohdr.textureindex);

	List<uint64_t> MaterialBuffer(studiohdr.numtextures, true);
	RpakStream->Read((uint8_t*)&MaterialBuffer[0], 0, studiohdr.numtextures * sizeof(uint64_t));

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
		RpakStream->SetPosition(StudioOffset + offsetof(studiohdr_t_v16, boneremapindex) + BoneRemapOffset);
		RpakStream->Read((uint8_t*)&BoneRemapTable[0], 0, BoneRemapCount);
	}

	List<mstudiomaterial_t> materials;

	for (int i = 0; i < studiohdr.numtextures; ++i)
	{
		materials.EmplaceBack(mstudiomaterial_t{ MaterialBuffer[i], ""});
	}

	RMdlFixupPatches Fixups{};
	Fixups.MaterialPath = TexturePath;
	Fixups.Materials = &materials;
	Fixups.BoneRemaps = &BoneRemapTable;
	Fixups.MeshOffset = (StudioOffset + meshOffset);

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

	std::vector<vgloddata_t_v16> lods;
	std::unique_ptr<IO::MemoryStream> vgStream = nullptr;
	std::unique_ptr<uint8_t[]> dcmpBuf = nullptr;
	size_t lodSize = 0;
	size_t cmpSize = 0;

	size_t streamedDataSize = this->LoadedFiles[Asset.FileIndex].StarpakMap[Asset.StarpakOffset];

	if (this->LoadedFiles[Asset.FileIndex].StarpakMap.ContainsKey(Asset.StarpakOffset))
	{
		IO::Stream* StarpakStream = StarpakReader.GetBaseStream();

		if (streamedDataSize)
		{	
			// loop through all lods for full vg size
			for (int i = 0; i < studiohdr.numvgloddata; i++)
			{
				RpakStream->SetPosition(StudioOffset + offsetof(studiohdr_t_v16, vgloddataindex) + FIX_OFFSET(studiohdr.vgloddataindex) + (sizeof(vgloddata_t_v16) * i));
				vgloddata_t_v16 lod = Reader.Read<vgloddata_t_v16>();

				// full size of vg buffer
				lodSize += lod.vgsizedecompressed;

				lods.push_back(lod);
			}

			if (!lods.empty())
			{
				// load decompressed sections into this
				dcmpBuf = std::make_unique<uint8_t[]>(lodSize);

				// dcmp offset
				int decompOffset = 0;

				// this is purely for getting the full vg, has nothing to do with actual model export
				for (const auto& lod : lods)
				{
					cmpSize = lod.vgsizedecompressed;

					// DecompressStreamedBuffer deletes the buffer, no need to free it.
					uint8_t* tmpCmpBuf = new uint8_t[cmpSize];

					StarpakStream->SetPosition(Offset + lod.vgoffset);
					StarpakReader.Read(tmpCmpBuf, 0, lod.vgsizecompressed);

					// read into vg stream and decompress
					vgStream = RTech::DecompressStreamedBuffer(tmpCmpBuf, cmpSize, (uint8_t)CompressionType::OODLE);
					vgStream->Read(dcmpBuf.get(), decompOffset, cmpSize);

					// add size for an offset so we can write from the stream into the dcmpBuf at the right pos
					decompOffset += lod.vgsizedecompressed;
				}

				vgStream = std::move(std::make_unique<IO::MemoryStream>(dcmpBuf.get(), 0, lods.front().vgsizedecompressed, false, true));
			}
		}
	}
	else {
		return std::move(Model);
	}

	if (!vgStream && streamedDataSize)
	{
		g_Logger.Warning("VG is streamed but stream was invalid.\n");
		return nullptr;
	}

	if (bExportingRawRMdl)
	{
		if (!dcmpBuf)
		{
			g_Logger.Warning("Exporting raw .rmdl failed due to no LOD.\n");
			return nullptr;
		}

		std::ofstream vgOut(BaseFileName + ".vg", std::ios::out | std::ios::binary);
		vgOut.write((char*)dcmpBuf.get(), lodSize);
		vgOut.close();
		return nullptr;
	}

	RpakStream->SetPosition(StudioOffset + meshOffset);
	int maxMaterialLength = 0;

	for (int i = 0; i < studiohdr.numtextures; ++i)
	{
		mstudiomaterial_t& material = materials[i];

		if (Assets.ContainsKey(material.guid))
		{
			RpakLoadAsset& MaterialAsset = Assets[material.guid];
			bool bExportAllMaterials = ExportManager::Config.GetBool("SkinExport") ? IncludeMaterials : false;

			this->ExportMaterialCPU(MaterialAsset, TexturePath);
			RMdlMaterial ParsedMaterial = this->ExtractMaterial(MaterialAsset, TexturePath, bExportAllMaterials, false);
			uint32_t MaterialIndex = Model->AddMaterial(ParsedMaterial.MaterialName, ParsedMaterial.AlbedoHash);

			material.name = ParsedMaterial.MaterialName;

			if (material.name.Length() > maxMaterialLength)
				maxMaterialLength = material.name.Length();

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
		}
		else {
			Model->AddMaterial(string::Format("UNK_0x%llx"), 0);

			material.name = string::Format("0x%llx", material.guid);

			if (material.name.Length() > maxMaterialLength)
				maxMaterialLength = material.name.Length();
		}
	}


	if (studiohdr.numskinfamilies > 0)
	{
		int skinNameIndex = studiohdr.skinindex + (studiohdr.numtextures * studiohdr.numskinfamilies * sizeof(short));

		List<string> skinNames;
		skinNames.EmplaceBack("default");

		for (int i = 0; i < (studiohdr.numskinfamilies - 1); ++i)
		{
			RpakStream->SetPosition(StudioOffset + skinNameIndex + (i * sizeof(short)));
			short sznameindex = Reader.Read<short>();

			RpakStream->SetPosition(StudioOffset + sznameindex);
			skinNames.EmplaceBack(Reader.ReadCString());
		}
		Model->AddSkinMaterialNames(&skinNames);

		RpakStream->SetPosition(StudioOffset + studiohdr.skinindex);
		//printf("skins:\n");
		for (int i = 0; i < studiohdr.numskinfamilies; ++i)
		{
			//printf("- '%s':\n", skinNames[i].ToCString());
			List<int> skinMaterials;
			for (int j = 0; j < studiohdr.numskinref; ++j)
			{
				mstudiomaterial_t& origMat = materials[j];
				short newIdx = Reader.Read<short>();

				mstudiomaterial_t& newMat = materials[newIdx];
				skinMaterials.EmplaceBack((int)newIdx);

				//printf(string::Format("  %%-%is -> %%s\n", maxMaterialLength), origMat.name.ToCString(), newMat.name.ToCString());
			}
			Model->AddSkinMaterials(&skinMaterials);
			//printf("\n");
		}
	}

	IO::BinaryReader vgReader = IO::BinaryReader(vgStream.get(), true);
	if(lods.front().numMeshes > 0)
		this->ExtractModelLod_V16(vgReader, RpakStream, ModelName, vgStream->GetPosition(), Model, Fixups, Asset.AssetVersion, IncludeMaterials);

	vgStream->Close();

	return std::move(Model);
}

std::unique_ptr<Assets::Model> RpakLib::ExtractModel(const RpakLoadAsset& Asset, const string& Path, const string& AnimPath, bool IncludeMaterials, bool IncludeAnimations)
{
	if (Asset.AssetVersion >= 16)
		return this->ExtractModel_V16(Asset, Path, AnimPath, IncludeMaterials, IncludeAnimations);

	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);
	auto Model = std::make_unique<Assets::Model>(0, 0);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	ModelHeader mdlHdr;
	mdlHdr.ReadFromAssetStream(&RpakStream, Asset.SubHeaderSize, Asset.AssetVersion);

	mdlHdr.name = this->ReadStringFromPointer(Asset, mdlHdr.pName);

	if (mdlHdr.animRigCount > 0)
	{
		g_Logger.Info("====================== RIGS\n");
		for (int i = 0; i < mdlHdr.animRigCount; i++)
		{
			RpakStream->SetPosition(this->GetFileOffset(Asset, mdlHdr.animRigs.Index, mdlHdr.animRigs.Offset + (sizeof(uint64_t) * i)));
			uint64_t RigGuid = Reader.Read<uint64_t>();

			if (Assets.ContainsKey(RigGuid))
				g_Logger.Info("Rig %d -> %s\n", i, this->ExtractAnimationRig(Assets[RigGuid]).ToCString());
			else
				g_Logger.Info("Rig %d -> %llx\n", i, RigGuid);
		}
	}

	if (mdlHdr.animSeqCount > 0)
	{
		g_Logger.Info("====================== RSEQS\n");
		for (int i = 0; i < mdlHdr.animSeqCount; i++)
		{
			RpakStream->SetPosition(this->GetFileOffset(Asset, mdlHdr.animSeqs.Index, mdlHdr.animSeqs.Offset + (sizeof(uint64_t) * i)));
			uint64_t SeqGuid = Reader.Read<uint64_t>();

			if (Assets.ContainsKey(SeqGuid))
				g_Logger.Info("Seq %d -> %s\n", i, this->ExtractAnimationSeq(Assets[SeqGuid]).ToCString());
			else
				g_Logger.Info("Seq %d -> %llx\n", i, SeqGuid);

		}
		g_Logger.Info("======================\n");
	}

	string RawModelName = mdlHdr.name;
	string ModelName = IO::Path::GetFileNameWithoutExtension(RawModelName);
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

	const uint64_t StudioOffset = this->GetFileOffset(Asset, mdlHdr.studioData.Index, mdlHdr.studioData.Offset);

	bool bExportingRawRMdl = false;

	string BaseFileName = IO::Path::Combine(ModelPath, ModelName);

	RpakStream->SetPosition(StudioOffset);

	studiohdr_t studiohdr = Reader.Read<studiohdr_t>();

	RpakStream->SetPosition(StudioOffset);

	std::unique_ptr<char[]> studioBuf(new char[studiohdr.length]);

	Reader.Read(studioBuf.get(), 0, studiohdr.length);

	// write QC file when exporting as SMD
	if (Path != "" && AnimPath != "" && ModelFormat == ModelExportFormat_t::SMD)
	{
		this->ExportQC(Asset.AssetVersion, BaseFileName + ".qc", RawModelName, studioBuf.get(), nullptr);
	}

	if (Path != "" && AnimPath != "" && ModelFormat == ModelExportFormat_t::RMDL)
	{
		// set this here so we don't have to do this check every time
		bExportingRawRMdl = true;

		uint64_t PhyOffset = 0;

		if (mdlHdr.IsFlagSet(MODEL_HAS_PHYSICS))
			PhyOffset = this->GetFileOffset(Asset, mdlHdr.phyData.Index, mdlHdr.phyData.Offset);

		// check if this model has a phy segment
		if (PhyOffset)
		{
			RpakStream->SetPosition(PhyOffset);

			auto PhyHeader = Reader.Read<RMdlPhyHeader>();

			RpakStream->SetPosition(PhyOffset + PhyHeader.TextOffset);

			string Text = Reader.ReadCString();

			uint64_t PhySize = PhyHeader.TextOffset + Text.Length() + 1; // add null terminator because it's missing otherwise

			RpakStream->SetPosition(PhyOffset);

			char* phyBuf = new char[PhySize];

			Reader.Read(phyBuf, 0, PhySize);

			std::ofstream phyOut(BaseFileName + ".phy", std::ios::out | std::ios::binary);
			phyOut.write(phyBuf, PhySize);
			phyOut.close();
		}

		std::ofstream rmdlOut(BaseFileName + ".rmdl", std::ios::out | std::ios::binary);

		rmdlOut.write(studioBuf.get(), studiohdr.length);
		rmdlOut.close();
	}

	Model->Bones = std::move(ExtractSkeleton(Reader, StudioOffset, Asset.AssetVersion, Asset.SubHeaderSize));

	if (!bExportingRawRMdl)
		Model->GenerateGlobalTransforms(true, true); // We need global transforms

	if (IncludeAnimations && mdlHdr.animSeqCount > 0 && Asset.AssetVersion > 9)
	{
		IO::Directory::CreateDirectory(AnimationPath);

		RpakStream->SetPosition(this->GetFileOffset(Asset, mdlHdr.animSeqs.Index, mdlHdr.animSeqs.Offset));

		for (uint32_t i = 0; i < mdlHdr.animSeqCount; i++)
		{
			uint64_t AnimHash = Reader.Read<uint64_t>();

			if (!Assets.ContainsKey(AnimHash))
				continue;	// Should never happen

			// We need to make sure the skeleton is kept alive (copied) here...
			if (!bExportingRawRMdl)
				this->ExtractAnimation(Assets[AnimHash], Model->Bones, AnimationPath);
			else
				this->ExportAnimationSeq(Assets[AnimHash], AnimationPath);
		}
	}

	RpakStream->SetPosition(StudioOffset);

	studiohdr_t SkeletonHeader{};

	if (Asset.SubHeaderSize != 120)
	{
		SkeletonHeader = Reader.Read<studiohdr_t>();
	}
	else {
		s3studiohdr_t TempSkeletonHeader = Reader.Read<s3studiohdr_t>();

		memcpy(&SkeletonHeader, &TempSkeletonHeader, 0x110);

		SkeletonHeader.SubmeshLodsOffset = TempSkeletonHeader.meshindex;
		SkeletonHeader.BoneRemapCount = 0; // s3 doesnt use these from here
	}

	uint32_t SubmeshLodsOffset = SkeletonHeader.SubmeshLodsOffset;
	uint32_t TexturesOffset = SkeletonHeader.textureindex;
	uint32_t TexturesCount = SkeletonHeader.numtextures;
	uint32_t BoneRemapCount = SkeletonHeader.BoneRemapCount;
	uint32_t BoneRemapOffset = SkeletonHeader.OffsetToBoneRemapInfo;

	if (Asset.AssetVersion >= 14)
	{
		SubmeshLodsOffset = SkeletonHeader.SubmeshLodsOffset_V14;
		TexturesOffset = SkeletonHeader.cdtextureindex;
		TexturesCount = SkeletonHeader.numcdtextures;
		BoneRemapCount = SkeletonHeader.BoneRemapCount_V14;
	}

	RpakStream->SetPosition(StudioOffset + TexturesOffset);

	List<mstudiotexturev54_t> MaterialBuffer(TexturesCount, true);
	RpakStream->Read((uint8_t*)&MaterialBuffer[0], 0, TexturesCount * sizeof(mstudiotexturev54_t));

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
		RpakStream->SetPosition(StudioOffset + (SkeletonHeader.length - BoneRemapCount));
		RpakStream->Read((uint8_t*)&BoneRemapTable[0], 0, BoneRemapCount);
	}

	List<mstudiomaterial_t> materials;

	for (int i = 0; i < TexturesCount; ++i)
	{
		materials.EmplaceBack(mstudiomaterial_t{ MaterialBuffer[i].guid, "" });
	}

	RpakStream->SetPosition(StudioOffset + SubmeshLodsOffset);

	RMdlFixupPatches Fixups{};
	Fixups.MaterialPath = TexturePath;
	Fixups.Materials = &materials;
	Fixups.BoneRemaps = &BoneRemapTable;
	Fixups.MeshOffset = (StudioOffset + SubmeshLodsOffset);

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
			char* streamBuf = new char[mdlHdr.alignedStreamingSize];

			StarpakReader.Read(streamBuf, 0, mdlHdr.alignedStreamingSize);

			RpakStream->SetPosition(StudioOffset);

			s3studiohdr_t hdr = Reader.Read<s3studiohdr_t>();

			if (hdr.vtxsize != 0)
			{
				std::ofstream vtxOut(BaseFileName + ".vtx", std::ios::out | std::ios::binary);

				vtxOut.write(streamBuf, hdr.vtxsize);
				vtxOut.close();
			}

			if (hdr.vvdsize != 0 && hdr.vvdindex > 0)
			{
				std::ofstream vvdOut(BaseFileName + ".vvd", std::ios::out | std::ios::binary);

				vvdOut.write(streamBuf + hdr.vvdindex, hdr.vvdsize);
				vvdOut.close();
			}

			if (hdr.vvcsize != 0 && hdr.vvcindex > 0)
			{
				std::ofstream vvcOut(BaseFileName + ".vvc", std::ios::out | std::ios::binary);

				vvcOut.write(streamBuf + hdr.vvcindex, hdr.vvcsize);
				vvcOut.close();
			}

			// probably not the real file extension
			if (hdr.weightsize != 0 && hdr.weightindex > 0)
			{
				std::ofstream vvwOut(BaseFileName + ".vvw", std::ios::out | std::ios::binary);

				vvwOut.write(streamBuf + hdr.weightindex, hdr.weightsize);
				vvwOut.close();
			}
		}
		else if ((Asset.AssetVersion >= 9 && Asset.AssetVersion <= 11) || (Asset.AssetVersion == 12 && Asset.SubHeaderSize == 0x78)) // s2-s6
		{
			StarpakStream->SetPosition(Offset);
			auto VGHeader = StarpakReader.Read<RMdlVGHeaderOld>();

			StarpakStream->SetPosition(Offset);
			char* vgBuf = new char[VGHeader.DataSize];

			StarpakReader.Read(vgBuf, 0, VGHeader.DataSize);

			std::ofstream vgOut(BaseFileName + ".vg", std::ios::out | std::ios::binary);

			vgOut.write(vgBuf, VGHeader.DataSize);
			vgOut.close();

			RpakStream->SetPosition(StudioOffset);

			s3studiohdr_t hdr = Reader.Read<s3studiohdr_t>();

			for (int i = 0; i < hdr.numtextures; i++)
			{
				mstudiotexturev54_t* mat = reinterpret_cast<mstudiotexturev54_t*>(studioBuf.get() + hdr.textureindex + (i * sizeof(mstudiotexturev54_t)));

				//if(Assets.ContainsKey(mat->guid))
				//	this->ExtractMaterial(Assets[mat->guid], Fixups.MaterialPath, IncludeMaterials, true);
			}
		}
		else if (Asset.AssetVersion >= 12)
		{
			StarpakStream->SetPosition(Offset);
			auto VGHeader = StarpakReader.Read<RMdlVGHeaderOld>();

			uint64_t dataSize = VGHeader.DataSize;

			// if dataSize is not actually dataSize
			if (dataSize < 32)
			{
				// here we go again
				
				StarpakStream->SetPosition(Offset);
				auto vg = StarpakReader.Read<RMdlVGHeader>();

				// offset to array of lods
				size_t lodsOffset = offsetof(RMdlVGHeader, lodOffset) + vg.lodOffset;

				dataSize = lodsOffset + (vg.lodCount * sizeof(VGLod));

				dataSize += 16 - (dataSize % 16);

				for (int i = 0; i < vg.lodCount; ++i)
				{
					// offset within starpak data entry
					size_t relOffset = lodsOffset + (sizeof(VGLod) * i);
					size_t thisLodOffset = Offset + relOffset;

					StarpakStream->SetPosition(thisLodOffset);

					VGLod lod = StarpakReader.Read<VGLod>();
					dataSize += offsetof(RMdlVGHeader, unk1) + lod.dataSize;

					if (i + 1 != vg.lodCount)
						dataSize += 16 - (dataSize % 16);
				}
			}

			if (dataSize)
			{
				StarpakStream->SetPosition(Offset);
				char* vgBuf = new char[dataSize];

				StarpakReader.Read(vgBuf, 0, dataSize);

				std::ofstream vgOut(BaseFileName + ".vg", std::ios::out | std::ios::binary);

				vgOut.write(vgBuf, dataSize);
				vgOut.close();
			}
		}

		return nullptr;
	}

	// accept v9-v12.0 (v8 is vtx/vvd/vvc, v12.1+ is new VG)
	bool useOldVg = (mdlHdr.version().major > 8 && mdlHdr.version().major < 12) || (mdlHdr.version().major == 12 && mdlHdr.version().minor == 0);

	if(useOldVg)
		this->ExtractModelLodOld(StarpakReader, RpakStream, ModelName, Offset, Model, Fixups, Asset.AssetVersion, IncludeMaterials);
	else if (Asset.AssetVersion >= 14)
		this->ExtractModelLod_V14(StarpakReader, RpakStream, ModelName, Offset, Model, Fixups, Asset.AssetVersion, IncludeMaterials);
	else // v12.1-v13
		this->ExtractModelLod(StarpakReader, RpakStream, ModelName, Offset, Model, Fixups, Asset.AssetVersion, IncludeMaterials);

	return std::move(Model);
}

void RpakLib::ExtractModelLod_V16(IO::BinaryReader& Reader, const std::unique_ptr<IO::MemoryStream>& RpakStream, string Name, uint64_t Offset, const std::unique_ptr<Assets::Model>& Model, RMdlFixupPatches& Fixup, uint32_t Version, bool IncludeMaterials)
{
	IO::Stream* BaseStream = Reader.GetBaseStream();

	if (!BaseStream)
	{
		g_Logger.Warning("!!! - Failed to extract Model LOD for %s. BaseStream was NULL (you probably don't have the required starpak)\n", Name.ToCString());
		return;
	}
	VGHeader_t_v16 vg = Reader.Read<VGHeader_t_v16>();

	if (!vg.nummeshes)
		return;

	size_t meshOffset = Offset + offsetof(VGHeader_t_v16, meshindex) + vg.meshindex;

	BaseStream->SetPosition(meshOffset);

	// We need to read the meshes
	List<VGMesh_t_v16> MeshBuffer(vg.nummeshes, true);
	Reader.Read((uint8_t*)&MeshBuffer[0], 0, vg.nummeshes * sizeof(VGMesh_t_v16));

	// Loop and read meshes
	for (uint32_t s = 0; s < vg.nummeshes; s++)
	{
		VGMesh_t_v16& mesh = MeshBuffer[s];

		// We have buffers per mesh now thank god
		List<uint8_t> VertexBuffer(mesh.vertexBufferSize, true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(VGMesh_t_v16)) + offsetof(VGMesh_t_v16, vertexOffset) + mesh.vertexOffset);
		Reader.Read((uint8_t*)&VertexBuffer[0], 0, mesh.vertexBufferSize);

		List<uint16_t> IndexBuffer(mesh.indexPacked.Count, true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(VGMesh_t_v16)) + offsetof(VGMesh_t_v16, indexOffset) + mesh.indexOffset);
		Reader.Read((uint8_t*)&IndexBuffer[0], 0, mesh.indexPacked.Count * sizeof(uint16_t));

		List<vvw::mstudioboneweightextra_t> ExtendedWeights(mesh.weightsCount / sizeof(vvw::mstudioboneweightextra_t), true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(VGMesh_t_v16)) + offsetof(VGMesh_t_v16, weightsOffset) + mesh.weightsOffset);
		Reader.Read((uint8_t*)&ExtendedWeights[0], 0, mesh.weightsCount);

		List<uint8_t>& BoneRemapBuffer = *Fixup.BoneRemaps;

		Assets::Mesh& NewMesh = Model->Meshes.Emplace(0x10, (((mesh.flags & 0x200000000) == 0x200000000) ? 2 : 1));	// max weights / max uvs

		uint8_t* VertexBufferPtr = (uint8_t*)&VertexBuffer[0];
		uint16_t* FaceBufferPtr = (uint16_t*)&IndexBuffer[0];

		// Cache these here, flags in the mesh dictate what to use
		Math::Vector3 Position{};
		Math::Vector3 Normal{};
		Math::Vector2 UVs{};
		Assets::VertexColor Color{};

		for (uint32_t v = 0; v < mesh.vertexCount; v++)
		{
			uint32_t Shift = 0;

			if ((mesh.flags & 0x1) == 0x1)
			{
				Position = *(Math::Vector3*)(VertexBufferPtr + Shift);
				Shift += sizeof(Math::Vector3);
			}
			else if ((mesh.flags & 0x2) == 0x2)
			{
				Position = (*(RMdlPackedVertexPosition*)(VertexBufferPtr + Shift)).Unpack();
				Shift += sizeof(RMdlPackedVertexPosition);
			}

			RMdlPackedVertexWeights Weights{};

			if ((mesh.flags & 0x5000) == 0x5000)
			{
				Weights = *(RMdlPackedVertexWeights*)(VertexBufferPtr + Shift);
				Shift += sizeof(RMdlPackedVertexWeights);
			}

			Normal = (*(RMdlPackedVertexTBN*)(VertexBufferPtr + Shift)).UnpackNormal();
			Shift += sizeof(RMdlPackedVertexTBN);

			if ((mesh.flags & 0x10) == 0x10)
			{
				Color = *(Assets::VertexColor*)(VertexBufferPtr + Shift);
				Shift += sizeof(Assets::VertexColor);
			}

			UVs = *(Math::Vector2*)(VertexBufferPtr + Shift);
			Shift += sizeof(Math::Vector2);

			Assets::Vertex Vertex = NewMesh.Vertices.Emplace(Position, Normal, Color, UVs);

			if ((mesh.flags & 0x200000000) == 0x200000000)
			{
				Vertex.SetUVLayer(*(Math::Vector2*)(VertexBufferPtr + Shift), 1);
				Shift += sizeof(Math::Vector2);
			}

			if ((mesh.flags & 0x5000) == 0x5000)
			{
				float CurrentWeightTotal = (float)(Weights.BlendWeights[0] + 1) / (float)0x8000;
				uint32_t WeightsIndex = 0;

				Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], CurrentWeightTotal }, WeightsIndex++);

				if (ExtendedWeights.Count() > 0)
				{
					uint32_t ExtendedWeightsIndex = (uint32_t)Weights.BlendIds[2] << 16;
					ExtendedWeightsIndex |= (uint32_t)Weights.BlendWeights[1];

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

						vvw::mstudioboneweightextra_t& ExtendedWeight = ExtendedWeights[ExtendedIndexShift];

						float ExtendedValue = (float)(ExtendedWeight.weight + 1) / (float)0x8000;
						uint32_t ExtendedIndex = BoneRemapBuffer[ExtendedWeight.bone];

						Vertex.SetWeight({ ExtendedIndex, ExtendedValue }, WeightsIndex++);

						CurrentWeightTotal += ExtendedValue;
						ExtendedCounter = ExtendedCounter + 1;
					}
				}

				if (Weights.BlendIds[0] != Weights.BlendIds[1] && CurrentWeightTotal < 1.f)
				{
					Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], 1.f - CurrentWeightTotal }, WeightsIndex);
				}
			}
			else if (Model->Bones.Count() > 0)
			{
				// Only a default weight is needed
				Vertex.SetWeight({ 0, 1.f }, 0);
			}

			VertexBufferPtr += mesh.vertexSize;
		}

		for (uint32_t f = 0; f < (IndexBuffer.Count() / 3); f++)
		{
			uint16_t i1 = *(uint16_t*)FaceBufferPtr;
			uint16_t i2 = *(uint16_t*)(FaceBufferPtr + 1);
			uint16_t i3 = *(uint16_t*)(FaceBufferPtr + 2);

			NewMesh.Faces.EmplaceBack(i1, i2, i3);

			FaceBufferPtr += 3;
		}

		RpakStream->SetPosition(Fixup.MeshOffset);

		IO::BinaryReader meshreader = IO::BinaryReader(RpakStream.get(), true);
		mstudiomesh_t_v16 rmdlMesh = meshreader.Read<mstudiomesh_t_v16>();
		mstudiomaterial_t& Material = (*Fixup.Materials)[rmdlMesh.material];

		if (Assets.ContainsKey(Material.guid))
		{
			NewMesh.MaterialIndices.EmplaceBack(rmdlMesh.material);
			if (IncludeMaterials)
				this->ExtractMaterial(Assets[Material.guid], Fixup.MaterialPath, IncludeMaterials, false);
		}
		else
			NewMesh.MaterialIndices.EmplaceBack(-1);

		// Add an extra slot for the extra UV Layer if present
		if ((mesh.flags & 0x200000000) == 0x200000000)
			NewMesh.MaterialIndices.EmplaceBack(-1);

		Fixup.MeshOffset += sizeof(mstudiomesh_t_v16);
	}
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

	RMdlVGHeader vg = Reader.Read<RMdlVGHeader>();

	if (!vg.lodCount)
		return;

	size_t thisLodOffset = Offset + offsetof(RMdlVGHeader, lodOffset) + vg.lodOffset;

	BaseStream->SetPosition(thisLodOffset);

	VGLod lod = Reader.Read<VGLod>();

	size_t meshOffset = thisLodOffset + offsetof(VGLod, meshOffset) + lod.meshOffset;

	BaseStream->SetPosition(meshOffset);

	// We need to read the meshes
	List<RMdlVGMesh_V14> MeshBuffer(lod.meshCount, true);
	Reader.Read((uint8_t*)&MeshBuffer[0], 0, lod.meshCount * sizeof(RMdlVGMesh_V14));

	// Loop and read meshes
	for (uint32_t s = 0; s < lod.meshCount; s++)
	{
		RMdlVGMesh_V14& mesh = MeshBuffer[s];

		// We have buffers per mesh now thank god
		List<uint8_t> VertexBuffer(mesh.VertexCountBytes, true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(RMdlVGMesh_V14)) + offsetof(RMdlVGMesh_V14, VertexOffset) + mesh.VertexOffset);
		Reader.Read((uint8_t*)&VertexBuffer[0], 0, mesh.VertexCountBytes);

		List<uint16_t> IndexBuffer(mesh.IndexPacked.Count, true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(RMdlVGMesh_V14)) + offsetof(RMdlVGMesh_V14, IndexOffset) + mesh.IndexOffset);
		Reader.Read((uint8_t*)&IndexBuffer[0], 0, mesh.IndexPacked.Count * sizeof(uint16_t));

		List<vvw::mstudioboneweightextra_t> ExtendedWeights(mesh.ExtendedWeightsCount / sizeof(vvw::mstudioboneweightextra_t), true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(RMdlVGMesh_V14)) + offsetof(RMdlVGMesh_V14, ExtendedWeightsOffset) + mesh.ExtendedWeightsOffset);
		Reader.Read((uint8_t*)&ExtendedWeights[0], 0, mesh.ExtendedWeightsCount);

		List<vvd::mstudioboneweight_t> ExternalWeightsBuffer(mesh.ExternalWeightsCount, true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(RMdlVGMesh_V14)) + offsetof(RMdlVGMesh_V14, ExternalWeightsOffset) + mesh.ExternalWeightsOffset);
		Reader.Read((uint8_t*)&ExternalWeightsBuffer[0], 0, mesh.ExternalWeightsCount * sizeof(vvd::mstudioboneweight_t));

		List<vtx::StripHeader_t> StripBuffer(mesh.StripsCount, true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(RMdlVGMesh_V14)) + offsetof(RMdlVGMesh_V14, StripsOffset) + mesh.StripsOffset);
		Reader.Read((uint8_t*)&StripBuffer[0], 0, mesh.StripsCount * sizeof(vtx::StripHeader_t));

		// Ignore a mesh that has no strips, otherwise there is no mesh data.
		// This is likely also determined by flags == 0x0, but this is a good check.
		/*if (Submesh.StripsCount == 0)
		{
			continue;
		}*/

		List<uint8_t>& BoneRemapBuffer = *Fixup.BoneRemaps;

		Assets::Mesh& NewMesh = Model->Meshes.Emplace(0x10, (((mesh.Flags2 & 0x2) == 0x2) ? 2 : 1));	// max weights / max uvs
		vtx::StripHeader_t& Strip = StripBuffer[0];

		uint8_t* VertexBufferPtr = (uint8_t*)&VertexBuffer[0];
		uint16_t* FaceBufferPtr = (uint16_t*)&IndexBuffer[0];

		// Cache these here, flags in the mesh dictate what to use
		Math::Vector3 Position{};
		Math::Vector3 Normal{};
		Math::Vector2 UVs{};
		Assets::VertexColor Color{};

		for (uint32_t v = 0; v < mesh.VertexCount; v++)
		{
			uint32_t Shift = 0;

			if ((mesh.Flags1 & 0x1) == 0x1)
			{
				Position = *(Math::Vector3*)(VertexBufferPtr + Shift);
				Shift += sizeof(Math::Vector3);
			}
			else if ((mesh.Flags1 & 0x2) == 0x2)
			{
				Position = (*(RMdlPackedVertexPosition*)(VertexBufferPtr + Shift)).Unpack();
				Shift += sizeof(RMdlPackedVertexPosition);
			}

			RMdlPackedVertexWeights Weights{};

			if ((mesh.Flags1 & 0x5000) == 0x5000)
			{
				Weights = *(RMdlPackedVertexWeights*)(VertexBufferPtr + Shift);
				Shift += sizeof(RMdlPackedVertexWeights);
			}

			Normal = (*(RMdlPackedVertexTBN*)(VertexBufferPtr + Shift)).UnpackNormal();
			Shift += sizeof(RMdlPackedVertexTBN);

			if ((mesh.Flags1 & 0x10) == 0x10)
			{
				Color = *(Assets::VertexColor*)(VertexBufferPtr + Shift);
				Shift += sizeof(Assets::VertexColor);
			}

			UVs = *(Math::Vector2*)(VertexBufferPtr + Shift);
			Shift += sizeof(Math::Vector2);

			Assets::Vertex Vertex = NewMesh.Vertices.Emplace(Position, Normal, Color, UVs);

			if ((mesh.Flags2 & 0x2) == 0x2)
			{
				Vertex.SetUVLayer(*(Math::Vector2*)(VertexBufferPtr + Shift), 1);
				Shift += sizeof(Math::Vector2);
			}

			if ((mesh.Flags1 & 0x5000) == 0x5000)
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

						vvw::mstudioboneweightextra_t& ExtendedWeight = ExtendedWeights[ExtendedIndexShift];

						float ExtendedValue = (float)(ExtendedWeight.weight + 1) / (float)0x8000;
						uint32_t ExtendedIndex = BoneRemapBuffer[ExtendedWeight.bone];

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

						if (ExternalWeights.numbones == 0x1)
						{
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], 1.0f }, 0);
						}
						else if (ExternalWeights.numbones == 0x2)
						{
							float CurrentWeightTotal = (float)(Weights.BlendWeights[0] + 1) / (float)0x8000;

							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], CurrentWeightTotal }, 0);
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], 1.0f - CurrentWeightTotal }, 1);
						}
						else if (ExternalWeights.numbones == 0x3)
						{
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], ExternalWeights.weight[0] }, 0);
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], ExternalWeights.weight[1] }, 1);
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[2]], ExternalWeights.weight[2] }, 2);
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

			VertexBufferPtr += mesh.VertexBufferStride;
		}

		for (uint32_t f = 0; f < (IndexBuffer.Count() / 3); f++)
		{
			uint16_t i1 = *(uint16_t*)FaceBufferPtr;
			uint16_t i2 = *(uint16_t*)(FaceBufferPtr + 1);
			uint16_t i3 = *(uint16_t*)(FaceBufferPtr + 2);

			NewMesh.Faces.EmplaceBack(i1, i2, i3);

			FaceBufferPtr += 3;
		}

		RpakStream->SetPosition(Fixup.MeshOffset);

		IO::BinaryReader meshreader = IO::BinaryReader(RpakStream.get(), true);
		mstudiomesh_v121_t rmdlMesh = meshreader.Read<mstudiomesh_v121_t>();
		mstudiomaterial_t& Material = (*Fixup.Materials)[rmdlMesh.material];

		if (rmdlMesh.material < Fixup.Materials->Count() && Assets.ContainsKey(Material.guid))
		{
			RpakLoadAsset& MaterialAsset = Assets[Material.guid];

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

			NewMesh.MaterialIndices.EmplaceBack(MaterialIndex);
		}
		else
		{
			NewMesh.MaterialIndices.EmplaceBack(-1);
		}

		// Add an extra slot for the extra UV Layer if present
		if ((mesh.Flags2 & 0x2) == 0x2)
			NewMesh.MaterialIndices.EmplaceBack(-1);

		Fixup.MeshOffset += sizeof(mstudiomesh_v121_t);
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

	RMdlVGHeader vg = Reader.Read<RMdlVGHeader>();

	if (!vg.lodCount)
		return;

	size_t thisLodOffset = Offset + offsetof(RMdlVGHeader, lodOffset) + vg.lodOffset;

	BaseStream->SetPosition(thisLodOffset);

	VGLod lod = Reader.Read<VGLod>();

	size_t meshOffset = thisLodOffset + offsetof(VGLod, meshOffset) + lod.meshOffset;

	BaseStream->SetPosition(meshOffset);

	// We need to read the meshes
	List<RMdlVGMesh> MeshBuffer(lod.meshCount, true);
	Reader.Read((uint8_t*)&MeshBuffer[0], 0, lod.meshCount * sizeof(RMdlVGMesh));

	// Loop and read meshes
	for (uint32_t s = 0; s < lod.meshCount; s++)
	{
		RMdlVGMesh& mesh = MeshBuffer[s];

		// We have buffers per mesh now thank god
		List<uint8_t> VertexBuffer(mesh.VertexCountBytes, true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(RMdlVGMesh)) + offsetof(RMdlVGMesh, VertexOffset) + mesh.VertexOffset);
		Reader.Read((uint8_t*)&VertexBuffer[0], 0, mesh.VertexCountBytes);

		List<uint16_t> IndexBuffer(mesh.IndexPacked.Count, true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(RMdlVGMesh)) + offsetof(RMdlVGMesh, IndexOffset) + mesh.IndexOffset);
		Reader.Read((uint8_t*)&IndexBuffer[0], 0, mesh.IndexPacked.Count * sizeof(uint16_t));

		List<vvw::mstudioboneweightextra_t> ExtendedWeights(mesh.ExtendedWeightsCount / sizeof(vvw::mstudioboneweightextra_t), true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(RMdlVGMesh)) + offsetof(RMdlVGMesh, ExtendedWeightsOffset) + mesh.ExtendedWeightsOffset);
		Reader.Read((uint8_t*)&ExtendedWeights[0], 0, mesh.ExtendedWeightsCount);

		List<vvd::mstudioboneweight_t> ExternalWeightsBuffer(mesh.ExternalWeightsCount, true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(RMdlVGMesh)) + offsetof(RMdlVGMesh, ExternalWeightsOffset) + mesh.ExternalWeightsOffset);
		Reader.Read((uint8_t*)&ExternalWeightsBuffer[0], 0, mesh.ExternalWeightsCount * sizeof(vvd::mstudioboneweight_t));

		List<vtx::StripHeader_t> StripBuffer(mesh.StripsCount, true);
		BaseStream->SetPosition(meshOffset + (s * sizeof(RMdlVGMesh)) + offsetof(RMdlVGMesh, StripsOffset) + mesh.StripsOffset);
		Reader.Read((uint8_t*)&StripBuffer[0], 0, mesh.StripsCount * sizeof(vtx::StripHeader_t));

		// Ignore a mesh that has no strips, otherwise there is no mesh.
		// This is likely also determined by flags == 0x0, but this is a good check.
		if (mesh.StripsCount == 0)
		{
			continue;
		}

		List<uint8_t>& BoneRemapBuffer = *Fixup.BoneRemaps;

		Assets::Mesh& NewMesh = Model->Meshes.Emplace(0x10, (((mesh.Flags2 & 0x2) == 0x2) ? 2 : 1));	// max weights / max uvs
		vtx::StripHeader_t& Strip = StripBuffer[0];

		uint8_t* VertexBufferPtr = (uint8_t*)&VertexBuffer[0];
		uint16_t* FaceBufferPtr = (uint16_t*)&IndexBuffer[0];

		// Cache these here, flags in the submesh dictate what to use
		Math::Vector3 Position{};
		Math::Vector3 Normal{};
		Math::Vector2 UVs{};
		Assets::VertexColor Color{};

		for (uint32_t v = 0; v < mesh.VertexCount; v++)
		{
			uint32_t Shift = 0;

			if ((mesh.Flags1 & 0x1) == 0x1)
			{
				Position = *(Math::Vector3*)(VertexBufferPtr + Shift);
				Shift += sizeof(Math::Vector3);
			}
			else if ((mesh.Flags1 & 0x2) == 0x2)
			{
				Position = (*(RMdlPackedVertexPosition*)(VertexBufferPtr + Shift)).Unpack();
				Shift += sizeof(RMdlPackedVertexPosition);
			}

			RMdlPackedVertexWeights Weights{};

			if ((mesh.Flags1 & 0x5000) == 0x5000)
			{
				Weights = *(RMdlPackedVertexWeights*)(VertexBufferPtr + Shift);
				Shift += sizeof(RMdlPackedVertexWeights);
			}

			Normal = (*(RMdlPackedVertexTBN*)(VertexBufferPtr + Shift)).UnpackNormal();
			Shift += sizeof(RMdlPackedVertexTBN);

			if ((mesh.Flags1 & 0x10) == 0x10)
			{
				Color = *(Assets::VertexColor*)(VertexBufferPtr + Shift);
				Shift += sizeof(Assets::VertexColor);
			}

			UVs = *(Math::Vector2*)(VertexBufferPtr + Shift);
			Shift += sizeof(Math::Vector2);

			Assets::Vertex Vertex = NewMesh.Vertices.Emplace(Position, Normal, Color, UVs);

			if ((mesh.Flags2 & 0x2) == 0x2)
			{
				Vertex.SetUVLayer(*(Math::Vector2*)(VertexBufferPtr + Shift), 1);
				Shift += sizeof(Math::Vector2);
			}

			if ((mesh.Flags1 & 0x5000) == 0x5000)
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

						vvw::mstudioboneweightextra_t& ExtendedWeight = ExtendedWeights[ExtendedIndexShift];

						float ExtendedValue = (float)(ExtendedWeight.weight + 1) / (float)0x8000;
						uint32_t ExtendedIndex = BoneRemapBuffer[ExtendedWeight.bone];

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

					if (ExternalWeights.numbones == 0x1)
					{
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], 1.0f }, 0);
					}
					else if (ExternalWeights.numbones == 0x2)
					{
						float CurrentWeightTotal = (float)(Weights.BlendWeights[0] + 1) / (float)0x8000;

						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], CurrentWeightTotal }, 0);
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], 1.0f - CurrentWeightTotal }, 1);
					}
					else if (ExternalWeights.numbones == 0x3)
					{
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], ExternalWeights.weight[0] }, 0);
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], ExternalWeights.weight[1] }, 1);
						Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[2]], ExternalWeights.weight[2] }, 2);
					}
				}
			}
			else if (Model->Bones.Count() > 0)
			{
				// Only a default weight is needed
				Vertex.SetWeight({ 0, 1.f }, 0);
			}

			VertexBufferPtr += mesh.VertexBufferStride;
		}

		for (uint32_t f = 0; f < (Strip.numIndices / 3); f++)
		{
			uint16_t i1 = *(uint16_t*)FaceBufferPtr;
			uint16_t i2 = *(uint16_t*)(FaceBufferPtr + 1);
			uint16_t i3 = *(uint16_t*)(FaceBufferPtr + 2);

			NewMesh.Faces.EmplaceBack(i1, i2, i3);

			FaceBufferPtr += 3;
		}

		RpakStream->SetPosition(Fixup.MeshOffset);

		IO::BinaryReader meshreader = IO::BinaryReader(RpakStream.get(), true);
		mstudiomesh_v121_t rmdlMesh = meshreader.Read<mstudiomesh_v121_t>();
		mstudiomaterial_t& Material = (*Fixup.Materials)[rmdlMesh.material];

		if (rmdlMesh.material < Fixup.Materials->Count() && Assets.ContainsKey(Material.guid))
		{
			RpakLoadAsset& MaterialAsset = Assets[Material.guid];

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

			NewMesh.MaterialIndices.EmplaceBack(MaterialIndex);
		}
		else
		{
			NewMesh.MaterialIndices.EmplaceBack(-1);
		}

		// Add an extra slot for the extra UV Layer if present
		if ((mesh.Flags2 & 0x2) == 0x2)
			NewMesh.MaterialIndices.EmplaceBack(-1);

		Fixup.MeshOffset += sizeof(mstudiomesh_v121_t);
	}
}

void RpakLib::ExtractModelLodOld(IO::BinaryReader& Reader, const std::unique_ptr<IO::MemoryStream>& RpakStream, string Name, uint64_t Offset, const std::unique_ptr<Assets::Model>& Model, RMdlFixupPatches& Fixup, uint32_t Version, bool IncludeMaterials)
{
	auto BaseStream = Reader.GetBaseStream();

	if (!BaseStream)
	{
		g_Logger.Warning("!!! - Failed to extract Model LOD for %s. BaseStream was NULL (you probably don't have the required starpak)\n", Name.ToCString());
		return;
	}

	BaseStream->SetPosition(Offset);

	auto VGHeader = Reader.Read<RMdlVGHeaderOld>();

	// Offsets in submesh are relative to the submesh we're reading...
	const auto meshOffset = Offset + VGHeader.MeshOffset;

	BaseStream->SetPosition(meshOffset);
	// We need to read the submeshes
	List<RMdlVGMeshOld> SubmeshBuffer(VGHeader.MeshCount, true);
	Reader.Read((uint8_t*)&SubmeshBuffer[0], 0, VGHeader.MeshCount * sizeof(RMdlVGMeshOld));

	List<uint8_t> VertexBuffer(VGHeader.VertexBufferSize, true);
	BaseStream->SetPosition(Offset + VGHeader.VertexBufferOffset);
	Reader.Read((uint8_t*)&VertexBuffer[0], 0, VGHeader.VertexBufferSize);

	List<uint16_t> IndexBuffer(VGHeader.IndexCount, true);
	BaseStream->SetPosition(Offset + VGHeader.IndexOffset);
	Reader.Read((uint8_t*)&IndexBuffer[0], 0, VGHeader.IndexCount * sizeof(uint16_t));

	List<vvw::mstudioboneweightextra_t> ExtendedWeights(VGHeader.ExtendedWeightsCount / sizeof(vvw::mstudioboneweightextra_t), true);
	BaseStream->SetPosition(Offset + VGHeader.ExtendedWeightsOffset);
	Reader.Read((uint8_t*)&ExtendedWeights[0], 0, VGHeader.ExtendedWeightsCount);

	List<vvd::mstudioboneweight_t> ExternalWeightsBuffer(VGHeader.ExternalWeightsCount, true);
	BaseStream->SetPosition(Offset + VGHeader.ExternalWeightsOffset);
	Reader.Read((uint8_t*)&ExternalWeightsBuffer[0], 0, VGHeader.ExternalWeightsCount * sizeof(vvd::mstudioboneweight_t));

	List<vtx::StripHeader_t> StripBuffer(VGHeader.StripsCount, true);
	BaseStream->SetPosition(Offset + VGHeader.StripsOffset);
	Reader.Read((uint8_t*)&StripBuffer[0], 0, VGHeader.StripsCount * sizeof(vtx::StripHeader_t));

	List<vg::ModelLODHeader_t> lods(VGHeader.LodCount, true);
	BaseStream->SetPosition(Offset + VGHeader.LodOffset);
	Reader.Read((uint8_t*)&lods[0], 0, VGHeader.LodCount * sizeof(vg::ModelLODHeader_t));

	if (VGHeader.LodCount == 0)
		return;

	if (lods[0].numMeshes == 0)
		return;

	// Loop and read submeshes
	for (uint32_t s = lods[0].meshIndex; s < lods[0].numMeshes; s++)
	{
		RMdlVGMeshOld& Submesh = SubmeshBuffer[s];

		// Ignore a submesh that has no strips, otherwise there is no mesh.
		// This is likely also determined by flags == 0x0, but this is a good check.
		if (Submesh.StripsCount == 0)
			continue;

		auto& BoneRemapBuffer = *Fixup.BoneRemaps;

		auto& NewMesh = Model->Meshes.Emplace(0x10, (((Submesh.Flags2 & 0x2) == 0x2) ? 2 : 1));	// max weights / max uvs
		auto& Strip = StripBuffer[Submesh.StripsIndex];

		auto VertexBufferPtr = (uint8_t*)&VertexBuffer[Submesh.VertexOffsetBytes];
		auto FaceBufferPtr = (uint16_t*)&IndexBuffer[Submesh.IndexOffset];

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

			Normal = (*(RMdlPackedVertexTBN*)(VertexBufferPtr + Shift)).UnpackNormal();
			Shift += sizeof(RMdlPackedVertexTBN);

			if ((Submesh.Flags1 & 0x10) == 0x10)
			{
				Color = *(Assets::VertexColor*)(VertexBufferPtr + Shift);
				Shift += sizeof(Assets::VertexColor);
			}

			UVs = *(Math::Vector2*)(VertexBufferPtr + Shift);
			Shift += sizeof(Math::Vector2);

			auto Vertex = NewMesh.Vertices.Emplace(Position, Normal, Color, UVs);

			if ((Submesh.Flags2 & 0x2) == 0x2)
			{
				Vertex.SetUVLayer(*(Math::Vector2*)(VertexBufferPtr + Shift), 1);
				Shift += sizeof(Math::Vector2);
			}

			if ((Submesh.Flags1 & 0x5000) == 0x5000)
			{

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

						auto& ExtendedWeight = ExtendedWeights[ExtendedIndexShift];

						float ExtendedValue = (float)(ExtendedWeight.weight + 1) / (float)0x8000;
						uint32_t ExtendedIndex = BoneRemapBuffer[ExtendedWeight.bone];

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

					if (ExternalWeightsBuffer.Count() > 0)
					{
						auto& ExternalWeights = ExternalWeightsBuffer[v];

						if (ExternalWeights.numbones == 0x1)
						{
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], 1.0f }, 0);
						}
						else if (ExternalWeights.numbones == 0x2)
						{
							float CurrentWeightTotal = (float)(Weights.BlendWeights[0] + 1) / (float)0x8000;

							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], CurrentWeightTotal }, 0);
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], 1.0f - CurrentWeightTotal }, 1);
						}
						else if (ExternalWeights.numbones == 0x3)
						{
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[0]], ExternalWeights.weight[0] }, 0);
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[1]], ExternalWeights.weight[1] }, 1);
							Vertex.SetWeight({ BoneRemapBuffer[Weights.BlendIds[2]], ExternalWeights.weight[2] }, 2);
						}

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

		for (uint32_t f = 0; f < (Strip.numIndices / 3); f++)
		{
			auto i1 = *(uint16_t*)FaceBufferPtr;
			auto i2 = *(uint16_t*)(FaceBufferPtr + 1);
			auto i3 = *(uint16_t*)(FaceBufferPtr + 2);

			NewMesh.Faces.EmplaceBack(i1, i2, i3);

			FaceBufferPtr += 3;
		}

		RpakStream->SetPosition(Fixup.MeshOffset);

		auto meshreader = IO::BinaryReader(RpakStream.get(), true);
		mstudiomesh_s3_t rmdlMesh = meshreader.Read<mstudiomesh_s3_t>();
		mstudiomaterial_t& Material = (*Fixup.Materials)[rmdlMesh.material];

		if (rmdlMesh.material < Fixup.Materials->Count() && Assets.ContainsKey(Material.guid))
		{
			auto& MaterialAsset = Assets[Material.guid];

			auto ParsedMaterial = this->ExtractMaterial(MaterialAsset, Fixup.MaterialPath, IncludeMaterials, false);
			auto MaterialIndex = Model->AddMaterial(ParsedMaterial.MaterialName, ParsedMaterial.AlbedoHash);

			auto& MaterialInstance = Model->Materials[MaterialIndex];

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

			NewMesh.MaterialIndices.EmplaceBack(MaterialIndex);
		}
		else
		{
			NewMesh.MaterialIndices.EmplaceBack(-1);
		}

		// Add an extra slot for the extra UV Layer if present
		if ((Submesh.Flags2 & 0x2) == 0x2)
			NewMesh.MaterialIndices.EmplaceBack(-1);

		Fixup.MeshOffset += sizeof(mstudiomesh_s3_t);
	}
}

List<Assets::Bone> RpakLib::ExtractSkeleton(IO::BinaryReader& Reader, uint64_t SkeletonOffset, uint32_t Version, int mdlHeaderSize)
{
	IO::Stream* RpakStream = Reader.GetBaseStream();

	RpakStream->SetPosition(SkeletonOffset);

	studiohdr_t SkeletonHeader = Reader.Read<studiohdr_t>();

	List<Assets::Bone> Result = List<Assets::Bone>(SkeletonHeader.numbones);

	for (uint32_t i = 0; i < SkeletonHeader.numbones; i++)
	{
		bool bIsOldModel = (Version >= 8 && Version < 12) || (Version == 12 && mdlHeaderSize == 0x78);
		uint64_t Position = SkeletonOffset + SkeletonHeader.boneindex + (i * (sizeof(mstudiobone_t) + (bIsOldModel ? 4 : 0)));

		RpakStream->SetPosition(Position);
		mstudiobone_t Bone = Reader.Read<mstudiobone_t>();
		RpakStream->SetPosition(Position + Bone.NameOffset);

		string TagName = Reader.ReadCString();

		Result.EmplaceBack(TagName, Bone.ParentIndex, Bone.Position, Bone.Rotation);
	}

	if (SkeletonHeader.numbones == 1)
		Result[0].SetParent(-1);

	return Result;
}

List<Assets::Bone> RpakLib::ExtractSkeleton_V16(IO::BinaryReader& Reader, uint64_t baseOffset, uint32_t Version, int mdlHeaderSize)
{
	IO::Stream* RpakStream = Reader.GetBaseStream();

	RpakStream->SetPosition(baseOffset);

	studiohdr_t_v16 studiohdr = Reader.Read<studiohdr_t_v16>();

	List<Assets::Bone> Result = List<Assets::Bone>(studiohdr.numbones);

	for (uint32_t i = 0; i < studiohdr.numbones; i++)
	{
		uint64_t Position = baseOffset + studiohdr.boneindex + (i * (sizeof(mstudiobone_t_v16)));
		
		RpakStream->SetPosition(Position);
		mstudiobone_t_v16 bone = Reader.Read<mstudiobone_t_v16>();

		RpakStream->SetPosition(Position + bone.sznameindex);
		string boneName = Reader.ReadCString();

		RpakStream->SetPosition(baseOffset + studiohdr.bonedataindex + (i * sizeof(mstudiobonedata_t_v16)));
		mstudiobonedata_t_v16 boneData = Reader.Read<mstudiobonedata_t_v16>();

		//printf("%s %i %.3f %.3f %.3f\n", boneName.ToCString(), boneData.parent, boneData.pos.X, boneData.pos.Y, boneData.pos.Z);

		Result.EmplaceBack(boneName, boneData.parent, boneData.pos, boneData.quat);
	}

	if (studiohdr.numbones == 1)
		Result[0].SetParent(-1);

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