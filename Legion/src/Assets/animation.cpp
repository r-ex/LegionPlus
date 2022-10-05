#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include <rtech.h>
#include <animtypes.h>

void RpakLib::BuildAnimInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	AnimRigHeader RigHeader = Reader.Read<AnimRigHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, RigHeader.NameIndex, RigHeader.NameOffset));

	string RigName = Reader.ReadCString();

	if (ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = RigName;
	else
		Info.Name = IO::Path::GetFileNameWithoutExtension(RigName).ToLower();

	Info.Type = ApexAssetType::AnimationSet;
	Info.Status = ApexAssetStatus::Loaded;

	RpakStream->SetPosition(this->GetFileOffset(Asset, RigHeader.SkeletonIndex, RigHeader.SkeletonOffset));

	studiohdr_t SkeletonHeader = Reader.Read<studiohdr_t>();

	Info.Info = string::Format("Animations: %d, Bones: %d", RigHeader.AnimationReferenceCount, SkeletonHeader.BoneCount);
}

void RpakLib::BuildRawAnimInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	AnimHeader AnHeader = Reader.Read<AnimHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, AnHeader.NameIndex, AnHeader.NameOffset));

	string AnimName = Reader.ReadCString();

	if (ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = AnimName;
	else
		Info.Name = IO::Path::GetFileNameWithoutExtension(AnimName).ToLower();

	Info.Type = ApexAssetType::AnimationSet;
	Info.Status = ApexAssetStatus::Loaded;
}

void RpakLib::ExportAnimationRig(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	AnimRigHeader RigHeader = Reader.Read<AnimRigHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, RigHeader.NameIndex, RigHeader.NameOffset));

	string FullAnimSetName = Reader.ReadCString();
	string AnimSetName = IO::Path::GetFileNameWithoutExtension(FullAnimSetName);
	string AnimSetPath{};
	if (ExportManager::Config.GetBool("UseFullPaths"))
		AnimSetPath = IO::Path::Combine(Path, IO::Path::Combine(IO::Path::GetDirectoryName(FullAnimSetName), AnimSetName));
	else
		AnimSetPath = IO::Path::Combine(Path, AnimSetName);

	IO::Directory::CreateDirectory(AnimSetPath);

	AnimExportFormat_t AnimFormat = (AnimExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("AnimFormat");

	if (AnimFormat == AnimExportFormat_t::RAnim)
	{
		uint64_t SkeletonOffset = this->GetFileOffset(Asset, RigHeader.SkeletonIndex, RigHeader.SkeletonOffset);
		RpakStream->SetPosition(SkeletonOffset);

		studiohdr_t SkeletonHeader = Reader.Read<studiohdr_t>();

		RpakStream->SetPosition(SkeletonOffset);

		char* skelBuf = new char[SkeletonHeader.DataSize];
		Reader.Read(skelBuf, 0, SkeletonHeader.DataSize);

		std::ofstream skelOut(IO::Path::Combine(AnimSetPath, AnimSetName + ".rrig"), std::ios::out | std::ios::binary);
		skelOut.write(skelBuf, SkeletonHeader.DataSize);
		skelOut.close();

		// todo: rseq
		return;
	}

	// version is 99 because it's supposed to only check model version, not arig version
	const List<Assets::Bone> Skeleton = this->ExtractSkeleton(Reader, this->GetFileOffset(Asset, RigHeader.SkeletonIndex, RigHeader.SkeletonOffset), 99);

	const uint64_t ReferenceOffset = this->GetFileOffset(Asset, RigHeader.AnimationReferenceIndex, RigHeader.AnimationReferenceOffset);

	for (uint32_t i = 0; i < RigHeader.AnimationReferenceCount; i++)
	{
		RpakStream->SetPosition(ReferenceOffset + ((uint64_t)i * 0x8));

		uint64_t AnimHash = Reader.Read<uint64_t>();

		// excluded by DFS
		if (AnimHash == 0xdf5)
		{
			g_Logger.Warning("unable to export anim idx %i for '%s' because it is self-excluded\n", i, FullAnimSetName.ToCString());
			continue;
		}

		if (!Assets.ContainsKey(AnimHash))
		{
			g_Logger.Warning("missing anim 0x%llx for '%s'\n", AnimHash, FullAnimSetName.ToCString());
			continue;
		}

		// We need to make sure the skeleton is kept alive (copied) here...
		this->ExtractAnimation(Assets[AnimHash], Skeleton, AnimSetPath);
	}
}

void RpakLib::ExtractAnimation(const RpakLoadAsset& Asset, const List<Assets::Bone>& Skeleton, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	AnimHeader animHeader = Reader.Read<AnimHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, animHeader.NameIndex, animHeader.NameOffset));

	string AnimRawStream = Reader.ReadCString();

	string AnimName = IO::Path::GetFileNameWithoutExtension(AnimRawStream);
	string AnimPath = IO::Path::GetDirectoryName(AnimRawStream);

	const uint64_t seqOffset = this->GetFileOffset(Asset, animHeader.AnimationIndex, animHeader.AnimationOffset);

	RpakStream->SetPosition(seqOffset);

	mstudioseqdesc_t seqdesc = Reader.Read<mstudioseqdesc_t>();

	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;

	uint64_t starpakDataOffset = 0;
	std::unique_ptr<IO::FileStream> StarpakStream = nullptr;

	if (Asset.OptimalStarpakOffset != -1)
	{
		starpakDataOffset = ActualOptStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, true);
	}
	else if (Asset.StarpakOffset != -1)
	{
		starpakDataOffset = ActualStarpakOffset;
		StarpakStream = this->GetStarpakStream(Asset, false);
	}

	IO::BinaryReader StarpakReader = IO::BinaryReader(StarpakStream.get(), true);

	for (uint32_t i = 0; i < seqdesc.numblends; i++)
	{
		RpakStream->SetPosition(seqOffset + seqdesc.animindexindex + ((uint64_t)i * sizeof(uint32_t)));

		int animindex = Reader.Read<int>();

		RpakStream->SetPosition(seqOffset + animindex);

		mstudioanimdescv54_t animdesc = Reader.Read<mstudioanimdescv54_t>();

		// unsure what this flag is
		if (!(animdesc.flags & 0x20000))
			continue;

		std::unique_ptr<Assets::Animation> Anim = std::make_unique<Assets::Animation>(Skeleton.Count());

		Assets::AnimationCurveMode AnimCurveType = Assets::AnimationCurveMode::Absolute;

		// anim is delta
		if (animdesc.flags & STUDIO_DELTA)
			AnimCurveType = Assets::AnimationCurveMode::Additive;

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

		const uint64_t AnimHeaderPointer = seqOffset + animindex;

		for (uint32_t Frame = 0; Frame < animdesc.numframes; Frame++)
		{
			uint32_t ChunkTableIndex = 0;
			uint32_t ChunkFrame = Frame;
			uint32_t FrameCountOneLess = 0;
			uint32_t FirstChunk = animdesc.animindex;
			uint64_t ChunkDataOffset = 0;
			uint32_t IsChunkInStarpak = 0;
			uint64_t ResultDataPtr = 0;

			if (!animdesc.FrameMedianCount)
			{
				// Nothing here
				goto nomedian;
			}
			else if (ChunkFrame >= animdesc.FrameSplitCount)
			{
				uint32_t FrameCount = animdesc.numframes;
				uint32_t ChunkFrameMinusSplitCount = ChunkFrame - animdesc.FrameSplitCount;
				if (FrameCount <= animdesc.FrameMedianCount || ChunkFrame != FrameCount - 1)
				{
					ChunkTableIndex = ChunkFrameMinusSplitCount / animdesc.FrameMedianCount + 1;
					ChunkFrame = ChunkFrame - (animdesc.FrameMedianCount * (ChunkFrameMinusSplitCount / animdesc.FrameMedianCount)) - animdesc.FrameSplitCount;
				}
				else
				{
					ChunkFrame = 0;
					ChunkTableIndex = (FrameCount - animdesc.FrameSplitCount - 1) / animdesc.FrameMedianCount + 2;
				}
			}

			ChunkDataOffset = animdesc.sectionindex + 8 * (uint64_t)ChunkTableIndex;

			RpakStream->SetPosition(AnimHeaderPointer + ChunkDataOffset);
			FirstChunk = Reader.Read<uint32_t>();

			RpakStream->SetPosition(AnimHeaderPointer + ChunkDataOffset + 4);
			IsChunkInStarpak = Reader.Read<uint32_t>();

			if (IsChunkInStarpak)
			{
				uint64_t v13 = animdesc.SomeDataOffset;
				if (v13)
				{
					ResultDataPtr = v13 + FirstChunk;
				}
				else
				{
					RpakStream->SetPosition(AnimHeaderPointer + ChunkDataOffset);
					uint32_t v14 = Reader.Read<uint32_t>();
					ResultDataPtr = starpakDataOffset + v14;
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

		string DestinationPath = IO::Path::Combine(Path, animName + string::Format("_%d", i) + (const char*)this->AnimExporter->AnimationExtension());

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