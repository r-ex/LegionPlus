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

	string animName = Reader.ReadCString();

	const uint64_t AnimationOffset = this->GetFileOffset(Asset, AnHeader.AnimationIndex, AnHeader.AnimationOffset);

	RpakStream->SetPosition(AnimationOffset);

	mstudioseqdesc_t AnimSequenceHeader = Reader.Read<mstudioseqdesc_t>();

	RpakStream->SetPosition(AnimationOffset + AnimSequenceHeader.szactivitynameindex);

	string ActivityName = Reader.ReadCString();

	RpakStream->SetPosition(AnimationOffset + AnimSequenceHeader.szlabelindex);
	auto eventid = Reader.Read<uint64_t>();

	if (ExportManager::Config.GetBool("UseFullPaths"))
		Info.Name = animName;
	else
		Info.Name = IO::Path::GetFileNameWithoutExtension(animName).ToLower();

	Info.Type = ApexAssetType::AnimationSeq;
	Info.Status = ApexAssetStatus::Loaded;

	if (ActivityName != "")
		Info.Info = string::Format("%s", ActivityName.ToCString());
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

		const uint64_t ReferenceOffset = this->GetFileOffset(Asset, RigHeader.AnimationReferenceIndex, RigHeader.AnimationReferenceOffset);

		for (uint32_t i = 0; i < RigHeader.AnimationReferenceCount; i++)
		{
			RpakStream->SetPosition(ReferenceOffset + ((uint64_t)i * 0x8));

			uint64_t AnimHash = Reader.Read<uint64_t>();

			auto SeqAsset = Assets[AnimHash];

			auto AnimStream = this->GetFileStream(SeqAsset);

			IO::BinaryReader AnimReader = IO::BinaryReader(AnimStream.get(), true);

			AnimStream->SetPosition(this->GetFileOffset(SeqAsset, SeqAsset.SubHeaderIndex, SeqAsset.SubHeaderOffset));

			AnimHeader AnHeader = AnimReader.Read<AnimHeader>();

			AnimStream->SetPosition(this->GetFileOffset(SeqAsset, AnHeader.NameIndex, AnHeader.NameOffset));

			string SeqName = AnimReader.ReadCString();
			string SeqSetName = IO::Path::GetFileNameWithoutExtension(SeqName);
			string SeqSetPath = IO::Path::Combine(Path, AnimSetName);

			g_Logger.Info("-> %s\n", SeqName.ToCString());

			const uint64_t AnimationOffset = this->GetFileOffset(SeqAsset, AnHeader.AnimationIndex, AnHeader.AnimationOffset);

			this->ExportAnimationSeq(Assets[AnimHash], AnimSetPath);

			AnimStream.release();

			continue;
		}

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


struct FilterOffset
{
	int offset;
	int count = 1;
};

bool FilterByOffset(FilterOffset const& x, FilterOffset const& y)
{
	return x.offset > y.offset;
}

void RpakLib::ExportAnimationSeq(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	AnimExportFormat_t AnimFormat = (AnimExportFormat_t)ExportManager::Config.Get<System::SettingType::Integer>("AnimFormat");
	if (AnimFormat != AnimExportFormat_t::RAnim)
		return;

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	AnimHeader AnHeader = Reader.Read<AnimHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, AnHeader.NameIndex, AnHeader.NameOffset));

	string FullAnimSetName = Reader.ReadCString();
	string AnimSetPath = IO::Path::Combine(Path, FullAnimSetName);
	string FolderPath = IO::Path::GetDirectoryName(AnimSetPath);

	IO::Directory::CreateDirectory(FolderPath);

	const uint64_t AnimationOffset = this->GetFileOffset(Asset, AnHeader.AnimationIndex, AnHeader.AnimationOffset);

	RpakStream->SetPosition(AnimationOffset);

	mstudioseqdesc_t seqdesc = Reader.Read<mstudioseqdesc_t>();

	RpakStream->SetPosition(AnimationOffset);

	uint64_t OffsetOfStarpakData = 0;
	std::unique_ptr<IO::FileStream> StarpakStream = nullptr;

	if (Asset.OptimalStarpakOffset != -1)
	{
		OffsetOfStarpakData = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
		StarpakStream = this->GetStarpakStream(Asset, true);
	}
	else if (Asset.StarpakOffset != -1)
	{
		OffsetOfStarpakData = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;
		StarpakStream = this->GetStarpakStream(Asset, false);
	}

	int numbones = (seqdesc.activitymodifierindex - seqdesc.weightlistindex) / 4;
	auto numanims = seqdesc.groupsize[0] * seqdesc.groupsize[1];
	auto numkeys = seqdesc.groupsize[0] + seqdesc.groupsize[1];

	std::vector<int> Blends;
	RpakStream->SetPosition(AnimationOffset + seqdesc.animindexindex);
	for (int j = 0; j < numanims; j++)
		Blends.push_back(Reader.Read<int>());

	size_t RSeqSize = 0;//sizeof(mstudioseqdesc_t);

	std::vector<FilterOffset> Filter{};

	for (int i = 0; i < seqdesc.numblends; i++)
	{
		RpakStream->SetPosition(AnimationOffset + Blends[i]);

		const uint64_t AnimHeaderPointer = AnimationOffset + Blends[i];

		mstudioanimdescv54_t animdesc = Reader.Read<mstudioanimdescv54_t>();

		if (seqdesc.numevents > 0)
		{
			RpakStream->SetPosition(AnimationOffset + seqdesc.eventindex);
			for (int i = 0; i < seqdesc.numevents; i++)
			{
				auto Data = Reader.Read<mstudioeventv54_t>();

				FilterOffset InputData = { sizeof(mstudioseqdesc_t) + Data.szeventindex , seqdesc.numevents };
				Filter.push_back(InputData);
			}
		}

		if (seqdesc.numactivitymodifiers > 0)
		{
			RpakStream->SetPosition(AnimationOffset + seqdesc.activitymodifierindex);
			for (int i = 0; i < seqdesc.numactivitymodifiers; i++)
			{
				auto Data = Reader.Read<mstudioactivitymodifierv53_t>();

				FilterOffset InputData = { Data.sznameindex , seqdesc.numactivitymodifiers };
				Filter.push_back(InputData);
			}
		}

		FilterOffset InputData = { Blends[i] + animdesc.sznameindex , 1 };
		Filter.push_back(InputData);

		FilterOffset InputData2 = { seqdesc.szactivitynameindex , 1 };
		Filter.push_back(InputData2);

		FilterOffset InputData3 = { seqdesc.szlabelindex , 1 };
		Filter.push_back(InputData3);

		std::sort(Filter.begin(), Filter.end(), FilterByOffset);
	}

	RpakStream->SetPosition(AnimationOffset + Filter[0].offset);
	RSeqSize += Filter[0].offset;

	for (int i = 0; i < Filter[0].count; i++)
	{
		string label = Reader.ReadCString();

		if (label.Length() > 0)
			RSeqSize += label.Length() + 1;
	}

	RpakStream->SetPosition(AnimationOffset);
	char* rseqBuf = new char[RSeqSize];
	Reader.Read(rseqBuf, 0, RSeqSize);

	std::ofstream rseqOut(AnimSetPath, std::ios::out | std::ios::binary);
	rseqOut.write(rseqBuf, RSeqSize);
	rseqOut.close();
}

void RpakLib::ExtractAnimation(const RpakLoadAsset& Asset, const List<Assets::Bone>& Skeleton, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	AnimHeader animHeader = Reader.Read<AnimHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, animHeader.NameIndex, animHeader.NameOffset));

	string AnimRawStream = Reader.ReadCString();

	string animName = IO::Path::GetFileNameWithoutExtension(AnimRawStream);
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

			if (!animdesc.mediancount)
			{
				// Nothing here
				goto nomedian;
			}
			else if (ChunkFrame >= animdesc.sectionframes)
			{
				uint32_t FrameCount = animdesc.numframes;
				uint32_t ChunkFrameMinusSplitCount = ChunkFrame - animdesc.sectionframes;
				if (FrameCount <= animdesc.sectionframes || ChunkFrame != FrameCount - 1)
				{
					ChunkTableIndex = ChunkFrameMinusSplitCount / animdesc.mediancount + 1;
					ChunkFrame = ChunkFrame - (animdesc.mediancount * (ChunkFrameMinusSplitCount / animdesc.mediancount)) - animdesc.sectionframes;
				}
				else
				{
					ChunkFrame = 0;
					ChunkTableIndex = (FrameCount - animdesc.sectionframes - 1) / animdesc.mediancount + 2;
				}
			}

			ChunkDataOffset = animdesc.sectionindex + 8 * (uint64_t)ChunkTableIndex;

			RpakStream->SetPosition(AnimHeaderPointer + ChunkDataOffset);
			FirstChunk = Reader.Read<uint32_t>();

			RpakStream->SetPosition(AnimHeaderPointer + ChunkDataOffset + 4);
			IsChunkInStarpak = Reader.Read<uint32_t>();

			if (IsChunkInStarpak)
			{
				uint64_t v13 = animdesc.somedataoffset;
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