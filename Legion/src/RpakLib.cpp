#include "pch.h"

#include "RpakLib.h"
#include "Path.h"
#include "File.h"
#include "Directory.h"
#include "DDS.h"
#include "Half.h"
#include "Texture.h"
#include "Model.h"
#include "BinaryReader.h"

// Asset export formats
#include "CoDXAssetExport.h"
#include "XNALaraBinary.h"
#include "XNALaraAscii.h"
#include "WavefrontOBJ.h"
#include "KaydaraFBX.h"
#include "CastAsset.h"
#include "ValveSMD.h"
#include "AutodeskMaya.h"
#include "SEAsset.h"

// Rpak asset formats
#include "rtech.h"
#include "RpakImageTiles.h"

RpakFile::RpakFile()
	: SegmentData(nullptr), StartSegmentIndex(0), SegmentDataSize(0), PatchData(nullptr), PatchDataSize(0), Version(RpakGameVersion::Apex), EmbeddedStarpakOffset(0), EmbeddedStarpakSize(0)
{
}

RpakLib::RpakLib()
	: LoadedFileIndex(0), ImageExtension(".dds"), ImageSaveType(Assets::SaveFileType::Dds)
{
}

void RpakLib::LoadRpaks(const List<string>& Paths)
{
	for (auto& Rpak : Paths)
	{
		// Ignore duplicate files triggered by loading multiple rpaks at once.
		if (this->LoadedFilePaths.Contains(Rpak))
			continue;

		this->LoadRpak(Rpak, false);

		// Copy over to loaded, clear for next file
		for (auto& Loaded : this->LoadFileQueue)
		{
			this->LoadedFilePaths.EmplaceBack(Loaded);
		}
		this->LoadFileQueue.Clear();
	}
}

void RpakLib::LoadRpak(const string& Path, bool Dump)
{
	this->LoadFileQueue.EmplaceBack(Path);
	uint32_t LoadFileQueue = 0;

	while (LoadFileQueue < this->LoadFileQueue.Count() && this->MountRpak(this->LoadFileQueue[LoadFileQueue++], Dump));
}

void RpakLib::PatchAssets()
{
	// This is a dictionary of failed stream assets
	Dictionary<uint64_t, RpakLoadAsset> PatchedStreamAssets;

	// We must load this way...
	for (uint32_t i = 0; i < this->LoadedFileIndex; i++)
	{
		RpakFile& LoadedFile = this->LoadedFiles[i];

		for (auto& Kvp : LoadedFile.AssetHashmap)
		{
			if (!this->Assets.ContainsKey(Kvp.first))
			{
				RpakLoadAsset Asset = RpakLoadAsset(
					Kvp.second.NameHash,
					i,
					Kvp.second.Magic,
					Kvp.second.SubHeaderDataBlockIndex,
					Kvp.second.SubHeaderDataBlockOffset,
					Kvp.second.SubHeaderSize,
					Kvp.second.RawDataBlockIndex,
					Kvp.second.RawDataBlockOffset,
					Kvp.second.StarpakOffset,
					Kvp.second.OptimalStarpakOffset,
					LoadedFile.Version,
					Kvp.second.Version,
					&this->LoadedFiles[i]
				);

				// All assets must follow this patch sequence
				if (this->ValidateAssetPatchStatus(Asset))
				{
					// If we have a model or texture, it must also pass the stream test
					if (Asset.AssetType == (uint32_t)AssetType_t::Model
						|| Asset.AssetType == (uint32_t)AssetType_t::Texture
						|| Asset.AssetType == (uint32_t)AssetType_t::UIIA)
					{
						if (this->ValidateAssetStreamStatus(Asset))
						{
							this->Assets.Add(Kvp.first, Asset);
						}
						else if (!PatchedStreamAssets.ContainsKey(Kvp.first))
						{
							PatchedStreamAssets.Add(Kvp.first, Asset);
						}
					}
					else
					{
						this->Assets.Add(Kvp.first, Asset);
					}
				}
			}
		}
	}

	// We will attempt to brute force patch assets...
	for (auto& Kvp : PatchedStreamAssets)
	{
		for (uint32_t i = 0; i < this->LoadedFileIndex; i++)
		{
			if (i == Kvp.second.FileIndex)
				continue;

			RpakFile& File = this->LoadedFiles[i];

			if (File.AssetHashmap.ContainsKey(Kvp.first))
			{
				RpakApexAssetEntry& Asset = File.AssetHashmap[Kvp.first];

				Kvp.second.RpakFileIndex = i;
				Kvp.second.StarpakOffset = Asset.StarpakOffset;
				Kvp.second.OptimalStarpakOffset = Asset.OptimalStarpakOffset;

				if (this->ValidateAssetStreamStatus(Kvp.second))
				{
					this->Assets.Add(Kvp.first, Kvp.second);
					break;
				}
			}
		}
	}

	// Clean up the old cache of assets
	for (uint32_t i = 0; i < this->LoadedFileIndex; i++)
	{
		this->LoadedFiles[i].AssetHashmap.Clear();
	}
}

//std::unique_ptr<List<ApexAsset>> RpakLib::BuildAssetList(bool Models, bool Anims, bool Images, bool Materials, bool UIImages, bool DataTables)
std::unique_ptr<List<ApexAsset>> RpakLib::BuildAssetList(const std::array<bool, 11> &arrAssets)
{
	auto Result = std::make_unique<List<ApexAsset>>();

	for (auto& AssetKvp : Assets)
	{
		RpakLoadAsset& Asset = AssetKvp.Value();

		ApexAsset NewAsset;
		NewAsset.Hash = AssetKvp.first;
		NewAsset.FileCreatedTime = this->LoadedFiles[Asset.RpakFileIndex].CreatedTime;

		switch (Asset.AssetType)
		{
		case (uint32_t)AssetType_t::Model:
			if (!arrAssets[0])
				continue;
			BuildModelInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::AnimationRig:
			if (!arrAssets[1])
				continue;
			BuildAnimInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::Animation:
			if (!arrAssets[2])
				continue;
			BuildRawAnimInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::Texture:
			if (!arrAssets[3])
				continue;
			BuildTextureInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::Material:
			if (!arrAssets[4])
				continue;
			BuildMaterialInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::UIIA:
			if (!arrAssets[5])
				continue;
			BuildUIIAInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::DataTable:
			if (!arrAssets[6])
				continue;
			BuildDataTableInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::ShaderSet:
			if (!arrAssets[7])
				continue;
			BuildShaderSetInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::Settings:
			if (!arrAssets[8])
				continue;
			BuildSettingsInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::SettingsLayout:
			if (!arrAssets[8])
				continue;
			BuildSettingsLayoutInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::RSON:
			if (!arrAssets[9])
				continue;
			BuildRSONInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::RUI:
			//if (!arrAssets[9])
			//	continue;
			//BuildRUIInfo(Asset, NewAsset);
			continue;
			break;
		case (uint32_t)AssetType_t::Effect:
			if (!arrAssets[10])
				continue;
			BuildEffectInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::UIImageAtlas: // TODO ARRAY
			BuildUIImageAtlasInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::Subtitles:
			BuildSubtitleInfo(Asset, NewAsset);
			break;
		case (uint32_t)AssetType_t::Map:
			BuildMapInfo(Asset, NewAsset);
			break;
		default:
			continue;
		}

		NewAsset.Version = Asset.AssetVersion;

		Result->EmplaceBack(std::move(NewAsset));
	}

	return std::move(Result);
}

void RpakLib::InitializeModelExporter(ModelExportFormat_t Format)
{
	switch (Format)
	{
	case ModelExportFormat_t::Maya:
		ModelExporter = std::make_unique<Assets::Exporters::AutodeskMaya>();
		break;
	case ModelExportFormat_t::OBJ:
		ModelExporter = std::make_unique<Assets::Exporters::WavefrontOBJ>();
		break;
	case ModelExportFormat_t::XNALaraText:
		ModelExporter = std::make_unique<Assets::Exporters::XNALaraAscii>();
		break;
	case ModelExportFormat_t::XNALaraBinary:
		ModelExporter = std::make_unique<Assets::Exporters::XNALaraBinary>();
		break;
	case ModelExportFormat_t::SMD:
		ModelExporter = std::make_unique<Assets::Exporters::ValveSMD>();
		break;
	case ModelExportFormat_t::XModel:
		ModelExporter = std::make_unique<Assets::Exporters::CoDXAssetExport>();
		break;
	case ModelExportFormat_t::FBX:
		ModelExporter = std::make_unique<Assets::Exporters::KaydaraFBX>();
		break;
	case ModelExportFormat_t::Cast:
		ModelExporter = std::make_unique<Assets::Exporters::CastAsset>();
		break;
	// no custom exporter for RMDL as it will be handled directly in the export func
	default:
		ModelExporter = std::make_unique<Assets::Exporters::SEAsset>();
		break;
	}

	m_bModelExporterInitialized = true;
}

void RpakLib::InitializeAnimExporter(AnimExportFormat_t Format)
{
	switch (Format)
	{
	case AnimExportFormat_t::Cast:
		AnimExporter = std::make_unique<Assets::Exporters::CastAsset>();
		break;
	default:
		AnimExporter = std::make_unique<Assets::Exporters::SEAsset>();
		break;
	}

	m_bAnimExporterInitialized = true;
}

void RpakLib::InitializeImageExporter(ImageExportFormat_t Format)
{
	switch (Format)
	{
	case ImageExportFormat_t::Dds:
		ImageSaveType = Assets::SaveFileType::Dds;
		ImageExtension = Assets::Texture::GetExtensionForType(ImageSaveType);
		break;
	case ImageExportFormat_t::Png:
		ImageSaveType = Assets::SaveFileType::Png;
		ImageExtension = Assets::Texture::GetExtensionForType(ImageSaveType);
		break;
	case ImageExportFormat_t::Tiff:
		ImageSaveType = Assets::SaveFileType::Tiff;
		ImageExtension = Assets::Texture::GetExtensionForType(ImageSaveType);
		break;
	case ImageExportFormat_t::Tga:
		ImageSaveType = Assets::SaveFileType::Tga;
		ImageExtension = Assets::Texture::GetExtensionForType(ImageSaveType);
		break;
	}

	m_bImageExporterInitialized = true;
}

std::unique_ptr<IO::MemoryStream> RpakLib::GetFileStream(const RpakLoadAsset& Asset)
{
	RpakFile& File = this->LoadedFiles[Asset.FileIndex];

	return std::move(std::make_unique<IO::MemoryStream>(File.SegmentData.get(), 0, File.SegmentDataSize, false, true));
}

uint64_t RpakLib::GetFileOffset(const RpakLoadAsset& Asset, uint32_t SegmentIndex, uint32_t SegmentOffset)
{
	if (SegmentIndex < 0) return 0;

	return (Asset.PakFile->SegmentBlocks[SegmentIndex - Asset.PakFile->StartSegmentIndex].Offset + SegmentOffset);
}

uint64_t RpakLib::GetFileOffset(const RpakLoadAsset& Asset, RPakPtr& ptr)
{
	if (ptr.Index < 0) return 0;

	return (Asset.PakFile->SegmentBlocks[ptr.Index - Asset.PakFile->StartSegmentIndex].Offset + ptr.Offset);
}

uint64_t RpakLib::GetEmbeddedStarpakOffset(const RpakLoadAsset& Asset)
{
	return Asset.PakFile->EmbeddedStarpakOffset;
}

std::unique_ptr<IO::FileStream> RpakLib::GetStarpakStream(const RpakLoadAsset& Asset, bool Optimal)
{
	if (Optimal)
	{
		uint64_t OptStarpakIndex = Asset.OptimalStarpakOffset & 0xFF;
#if _DEBUG
		//g_Logger.Info("Load starpak: %s\n", this->LoadedFiles[Asset.RpakFileIndex].OptimalStarpakReferences[OptStarpakIndex].ToCString());
#endif
		if (!IO::File::Exists(this->LoadedFiles[Asset.RpakFileIndex].OptimalStarpakReferences[OptStarpakIndex]))
			return nullptr;

		return std::move(IO::File::OpenRead(this->LoadedFiles[Asset.RpakFileIndex].OptimalStarpakReferences[OptStarpakIndex]));
	}
	else
	{
		uint64_t StarpakPatchIndex = Asset.StarpakOffset & 0xFF;
#if _DEBUG
		//g_Logger.Info("Load starpak: %s\n", this->LoadedFiles[Asset.RpakFileIndex].StarpakReferences[StarpakPatchIndex].ToCString());
#endif
		if (!IO::File::Exists(this->LoadedFiles[Asset.RpakFileIndex].StarpakReferences[StarpakPatchIndex]))
			return nullptr;
		return std::move(IO::File::OpenRead(this->LoadedFiles[Asset.RpakFileIndex].StarpakReferences[StarpakPatchIndex]));
	}
}


// CalcBonePosition - 0x1401C97B0 - CL456479
void RpakLib::CalcBonePosition(const mstudio_rle_anim_t& pAnim, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	uint16_t* TranslationDataPtr = *BoneTrackData;

	if (!pAnim.bAnimPosition)
	{
		List<Assets::Curve>& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

		// TranslateX/Y/Z
		Curves[1].Keyframes.EmplaceBack(FrameIndex, Math::Half(TranslationDataPtr[0]).ToFloat());
		Curves[2].Keyframes.EmplaceBack(FrameIndex, Math::Half(TranslationDataPtr[1]).ToFloat());
		Curves[3].Keyframes.EmplaceBack(FrameIndex, Math::Half(TranslationDataPtr[2]).ToFloat());

		*BoneTrackData += 3;	// Advance over the size of the data
	}
	else
	{
		uint16_t TranslationFlags = TranslationDataPtr[2];
		float TranslationScale = *(float*)TranslationDataPtr;

		uint8_t* TranslationDataX = (uint8_t*)TranslationDataPtr + (TranslationFlags & 0x1FFF) + 4;	// Data for x

		uint64_t DataYOffset = *((uint8_t*)TranslationDataPtr + 6);
		uint64_t DataZOffset = *((uint8_t*)TranslationDataPtr + 7);

		uint8_t* TranslationDataY = &TranslationDataX[2 * DataYOffset];	// Data for y
		uint8_t* TranslationDataZ = &TranslationDataX[2 * DataZOffset];	// Data for z

		const Math::Vector3& Bone = Anim->Bones[BoneIndex].LocalPosition();

		float Result[3]{ Bone.X, Bone.Y, Bone.Z };

		uint32_t TranslationIndex = 0;
		uint32_t v32 = 0xF;	

		uint8_t* dataPtrs[] = { TranslationDataX,TranslationDataY,TranslationDataZ };

		float TranslationFinal = 0, TimeScale = 0; // might not be TimeScale
		float Time = 0;	// time but doesn't matter
		do
		{
			// 0x1401C9AA4 - CL456479
			if (_bittest((const long*)&TranslationFlags, v32))
			{
				RTech::ExtractAnimValue(Frame, dataPtrs[TranslationIndex], TranslationScale, &TranslationFinal, &TimeScale);

				if (pAnim.bAdditiveCustom)
					Result[TranslationIndex] = (float)((float)((float)(1.0 - Time) * TranslationFinal) + (float)(TimeScale * Time));
				else
					Result[TranslationIndex] = (float)((float)((float)(1.0 - Time) * TranslationFinal) + (float)(TimeScale * Time)) + Result[TranslationIndex];
			}

			--v32;
			++TranslationIndex;
		} while (TranslationIndex < 3);

		List<Assets::Curve>& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

		// TranslateX/Y/Z
		Curves[1].Keyframes.EmplaceBack(FrameIndex, Result[0]);
		Curves[2].Keyframes.EmplaceBack(FrameIndex, Result[1]);
		Curves[3].Keyframes.EmplaceBack(FrameIndex, Result[2]);

		*BoneTrackData += 4;	// Advance over the size of the data
	}
}

void RpakLib::CalcBoneQuaternion(const mstudio_rle_anim_t& pAnim, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	uint16_t* RotationDataPtr = *BoneTrackData;

	struct Quat64
	{
		uint64_t X : 21;
		uint64_t Y : 21;
		uint64_t Z : 21;
		uint64_t WNeg : 1;
	};

	if (!pAnim.bAnimRotation)
	{
		Quat64 PackedQuat = *(Quat64*)RotationDataPtr;

		Math::Quaternion Quat;

		Quat.X = ((int)PackedQuat.X - 0x100000) * (1 / 1048576.5f);
		Quat.Y = ((int)PackedQuat.Y - 0x100000) * (1 / 1048576.5f);
		Quat.Z = ((int)PackedQuat.Z - 0x100000) * (1 / 1048576.5f);
		Quat.W = std::sqrt(1 - Quat.X * Quat.X - Quat.Y * Quat.Y - Quat.Z * Quat.Z);

		if (PackedQuat.WNeg)
			Quat.W = -Quat.W;

		// RotateQuaternion
		Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name())[0].Keyframes.Emplace(FrameIndex, Math::Quaternion(Quat[0], Quat[1], Quat[2], Quat[3]));

		*BoneTrackData += 4; // Advance over the size of the data
	}
	else
	{
		mstudioanim_valueptr_t* animValuePtr = *reinterpret_cast<mstudioanim_valueptr_t**>(BoneTrackData);

		mstudioanimvalue_t* pAnimValues = reinterpret_cast<mstudioanimvalue_t*>((uint8_t*)RotationDataPtr + animValuePtr->offset); // Data for x

		// index into anim values array for the second and third axes. these axes are not necessarily y,z because:
		// if only x,z flags are set, offset will point to data for X, axisIndex1 will point to data for Z
		uint8_t axisIndex1 = animValuePtr->axisIdx1;
		uint8_t axisIndex2 = animValuePtr->axisIdx2;

		// get actual animvalue pointers from index
		mstudioanimvalue_t* pAnimValues_Axis1 = &pAnimValues[axisIndex1];
		mstudioanimvalue_t* pAnimValues_Axis2 = &pAnimValues[axisIndex2];

		Math::Vector3 BoneRotation = Anim->Bones[BoneIndex].LocalRotation().ToEulerAngles();

		Vector3 EulerResult = { Math::MathHelper::DegreesToRadians(BoneRotation.X),Math::MathHelper::DegreesToRadians(BoneRotation.Y),Math::MathHelper::DegreesToRadians(BoneRotation.Z) };

		uint8_t* dataPtrs[] = { (uint8_t*)pAnimValues,(uint8_t*)pAnimValues_Axis1,(uint8_t*)pAnimValues_Axis2 };

		// this loop is weird. the game does this slightly differently, so i'm not sure how this even functions
		float v1 = 0, v2 = 0;
		for(int i = 0; i < 3; ++i)
		{
			if (_bittest((const long*)animValuePtr, 15 - i))
			{
				RTech::ExtractAnimValue(Frame, dataPtrs[i], 0.00019175345f, &v1, &v2);
				EulerResult[i] = v1;
			}
		};

		Math::Quaternion Result;

		RTech::AngleQuaternion(EulerResult, Result);

		Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name())[0].Keyframes.Emplace(FrameIndex, Result);

		*BoneTrackData += 2; // Advance over the size of the data
	}
}

void RpakLib::CalcBoneScale(const mstudio_rle_anim_t& pAnim, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	uint16_t* ScaleDataPtr = *BoneTrackData;

	if (!pAnim.bAnimScale)
	{
		List<Assets::Curve>& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

		// ScaleX/Y/Z
		Curves[4].Keyframes.EmplaceBack(FrameIndex, Math::Half(ScaleDataPtr[0]).ToFloat());
		Curves[5].Keyframes.EmplaceBack(FrameIndex, Math::Half(ScaleDataPtr[1]).ToFloat());
		Curves[6].Keyframes.EmplaceBack(FrameIndex, Math::Half(ScaleDataPtr[2]).ToFloat());

		*BoneTrackData += 3; // Advance over the size of the data
	}
	else
	{
		mstudioanim_valueptr_t* animValuePtr = *reinterpret_cast<mstudioanim_valueptr_t**>(BoneTrackData);

		mstudioanimvalue_t* pAnimValues = reinterpret_cast<mstudioanimvalue_t*>((uint8_t*)ScaleDataPtr + animValuePtr->offset); // Data for x

		// index into anim values array for the second and third axes. these axes are not necessarily y,z because:
		// if only x,z flags are set, offset will point to data for X, axisIndex1 will point to data for Z
		uint8_t axisIndex1 = animValuePtr->axisIdx1;
		uint8_t axisIndex2 = animValuePtr->axisIdx2;

		// get actual animvalue pointers from index
		mstudioanimvalue_t* pAnimValues_Axis1 = &pAnimValues[axisIndex1];
		mstudioanimvalue_t* pAnimValues_Axis2 = &pAnimValues[axisIndex2];

		const Math::Vector3& BoneScale = Anim->Bones[BoneIndex].Scale();

		float Result[3]{ BoneScale.X, BoneScale.Y, BoneScale.Z };

		uint8_t* dataPtrs[] = { (uint8_t*)pAnimValues,(uint8_t*)pAnimValues_Axis1,(uint8_t*)pAnimValues_Axis2 };

		// this loop is weird. the game does this slightly differently, so i'm not sure how this even functions
		float v1 = 0, v2 = 0;
		for (int i = 0; i < 3; ++i)
		{
			if (_bittest((const long*)animValuePtr, 15 - i))
			{
				RTech::ExtractAnimValue(Frame, dataPtrs[i], 0.0030518509f, &v1, &v2);
				Result[i] = (float)((float)((float)(1.0 - 0) * v1) + (float)(v2 * 0)) + Result[i];
			}
		};

		List<Assets::Curve>& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

		// Scale X/Y/Z
		Curves[4].Keyframes.EmplaceBack(FrameIndex, Result[0]);
		Curves[5].Keyframes.EmplaceBack(FrameIndex, Result[1]);
		Curves[6].Keyframes.EmplaceBack(FrameIndex, Result[2]);

		*BoneTrackData += 2; // Advance over the size of the data
	}
}

bool RpakLib::ValidateAssetPatchStatus(const RpakLoadAsset& Asset)
{
	auto& LoadedFile = this->LoadedFiles[Asset.FileIndex];

	if (Asset.SubHeaderIndex >= LoadedFile.StartSegmentIndex)
	{
		// The sub header and data blocks are within the packages...
		// We now need to check if the asset subheader blocks are within the package...
		auto RpakStream = this->GetFileStream(Asset);
		IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

		switch (Asset.AssetType)
		{
		case (uint32_t)AssetType_t::Model:
		{
			ModelHeader mdlHdr;
			mdlHdr.ReadFromAssetStream(&RpakStream, Asset.SubHeaderSize, Asset.AssetVersion);

			return (mdlHdr.studioData.Index >= LoadedFile.StartSegmentIndex && mdlHdr.pName.Index >= LoadedFile.StartSegmentIndex);
		}
		case (uint32_t)AssetType_t::Texture:
		{
			TextureHeader txtrHdr;
			txtrHdr.ReadFromAssetStream(&RpakStream, Asset.AssetVersion);
			return (txtrHdr.width > 0 && txtrHdr.height > 0);
		}
		case (uint32_t)AssetType_t::UIIA:
		{
			UIIAHeader SubHeader = Reader.Read<UIIAHeader>();
			return (SubHeader.Width > 0 && SubHeader.Height > 0);
		}
		case (uint32_t)AssetType_t::Material:
		{
			MaterialHeader hdr;

			if (Asset.Version == RpakGameVersion::Apex)
				hdr = Reader.Read<MaterialHeader>();
			else
			{
				MaterialHeaderV12 temp = Reader.Read<MaterialHeaderV12>();

				hdr.pName = temp.pName;
				hdr.textureHandles = temp.textureHandles;
			}
			
			return (hdr.pName.Index >= LoadedFile.StartSegmentIndex && hdr.textureHandles.Index >= LoadedFile.StartSegmentIndex);
		}
		case (uint32_t)AssetType_t::AnimationRig:
		{
			AnimRigHeader RigHeader{};
			RigHeader.ReadFromAssetStream(&RpakStream, Asset.AssetVersion);

			return (RigHeader.name.Index >= LoadedFile.StartSegmentIndex && RigHeader.studioData.Index >= LoadedFile.StartSegmentIndex && RigHeader.animSeqs.Index >= LoadedFile.StartSegmentIndex);
		}
		case (uint32_t)AssetType_t::Animation:
		{
			ASeqHeader SubHeader = Reader.Read<ASeqHeader>();
			return (SubHeader.pAnimation.Index >= LoadedFile.StartSegmentIndex);
		}
		case (uint32_t)AssetType_t::DataTable:
		{
			DataTableHeader SubHeader = Reader.Read<DataTableHeader>();
			return (SubHeader.ColumnCount != 0 && SubHeader.RowCount != 0);
		}
		case (uint32_t)AssetType_t::UIImageAtlas:
		{ // finally an actual check for this
			UIAtlasHeader Header = Reader.Read<UIAtlasHeader>();
			return Header.TexturesCount != 0;
		}
		case (uint32_t)AssetType_t::Effect: // Ok, V4 will get semi support.
		case (uint32_t)AssetType_t::Shader:
		case (uint32_t)AssetType_t::ShaderSet:
		case (uint32_t)AssetType_t::Subtitles:
		case (uint32_t)AssetType_t::Settings:
		case (uint32_t)AssetType_t::SettingsLayout:
		case (uint32_t)AssetType_t::RSON:
		case (uint32_t)AssetType_t::RUI:
		case (uint32_t)AssetType_t::Map:
			return true;
		default:
			return false;
		}
	}

	// Outside bounds of app
	return false;
}

bool RpakLib::ValidateAssetStreamStatus(const RpakLoadAsset& Asset)
{
	uint64_t ActualStarpakOffset = Asset.StarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t ActualOptStarpakOffset = Asset.OptimalStarpakOffset & 0xFFFFFFFFFFFFFF00;
	uint64_t StarpakPatchIndex = Asset.StarpakOffset & 0xFF;
	uint64_t OptStarpakIndex = Asset.OptimalStarpakOffset & 0xFF;

	if (Asset.OptimalStarpakOffset != -1 && Asset.OptimalStarpakOffset != 0)
	{
		if (!this->LoadedFiles[Asset.RpakFileIndex].OptimalStarpakMap.ContainsKey(Asset.OptimalStarpakOffset))
			return false;
	}

	if (Asset.StarpakOffset != -1 && Asset.StarpakOffset != 0)
	{
		if (!this->LoadedFiles[Asset.RpakFileIndex].StarpakMap.ContainsKey(Asset.StarpakOffset))
			return false;
	}

	return true;
}

bool RpakLib::MountRpak(const string& Path, bool Dump)
{
	IO::BinaryReader Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	RpakBaseHeader BaseHeader = Reader.Read<RpakBaseHeader>();

	if (BaseHeader.Magic != 0x6B615052)
		return false;

	switch (BaseHeader.Version)
	{
	case (uint32_t)RpakGameVersion::Apex:
		return this->MountApexRpak(Path, Dump);
	case (uint32_t)RpakGameVersion::Titanfall:
		return this->MountTitanfallRpak(Path, Dump);
	case (uint32_t)RpakGameVersion::R2TT:
		return this->MountR2TTRpak(Path, Dump);
	default:
		return false;
	}
}

bool RpakLib::ParseApexRpak(const string& RpakPath, std::unique_ptr<IO::MemoryStream>& ParseStream)
{
	IO::BinaryReader Reader = IO::BinaryReader(ParseStream.get(), true);
	string RpakRoot = IO::Path::GetDirectoryName(RpakPath);
	RpakApexHeader Header = Reader.Read<RpakApexHeader>();
	RpakFile* File = &this->LoadedFiles[this->LoadedFileIndex++];

	File->CreatedTime = Header.CreatedFileTime;
	File->Hash = Header.Hash;

	RpakPatchHeader PatchHeader{};
	RpakPatchCompressPair PatchCompressPairs[16]{};
	uint16_t PatchIndicesToFile[16]{};

	if (Header.PatchIndex)
	{
		ParseStream->Read((uint8_t*)&PatchHeader, 0, sizeof(RpakPatchHeader));

		// we should never actually go above 16 patch files i hope :clueless:
		// but it'd be pretty bad if we did, so min(index, 16)
		ParseStream->Read((uint8_t*)&PatchCompressPairs, 0, sizeof(RpakPatchCompressPair) * min(Header.PatchIndex, 16));
		ParseStream->Read((uint8_t*)&PatchIndicesToFile, 0, sizeof(uint16_t) * min(Header.PatchIndex, 16));
	}

	uint32_t StarpakLen = Header.StarpakReferenceSize;
	while (StarpakLen > 0)
	{
		string Starpak = Reader.ReadCString();

		if (Starpak.Length() > 0)
		{
			string Path = IO::Path::Combine(RpakRoot, IO::Path::GetFileName(Starpak));
			this->MountStarpak(Path, this->LoadedFileIndex - 1, File->StarpakReferences.Count(), false);
			File->StarpakReferences.EmplaceBack(Path);
		}

		StarpakLen -= Starpak.Length() + sizeof(char);
	}
	StarpakLen = Header.StarpakOptReferenceSize;
	while (StarpakLen > 0)
	{
		string Starpak = Reader.ReadCString();

		if (Starpak.Length() > 0)
		{
			string Path = IO::Path::Combine(RpakRoot, IO::Path::GetFileName(Starpak));
			this->MountStarpak(Path, this->LoadedFileIndex - 1, File->OptimalStarpakReferences.Count(), true);
			File->OptimalStarpakReferences.EmplaceBack(Path);
		}

		StarpakLen -= Starpak.Length() + sizeof(char);
	}

	// We need to load the rest of the data before applying a patch stream
	List<RpakVirtualSegment> VirtualSegments(Header.VirtualSegmentCount, true);
	List<RpakVirtualSegmentBlock> MemPages(Header.MemPageCount, true); // mem pages

	// each of these points to a descriptor/pointer within rpak mem pages
	// they are used to convert the raw data into an actual pointer when the pak is loaded
	List<RpakDescriptor> Descriptors(Header.DescriptorCount, true);

	List<RpakApexAssetEntry> AssetEntries(Header.AssetEntryCount, true);

	// Faster loading here by reading to the buffers directly
	ParseStream->Read((uint8_t*)&VirtualSegments[0], 0, sizeof(RpakVirtualSegment) * Header.VirtualSegmentCount);
	ParseStream->Read((uint8_t*)&MemPages[0], 0, sizeof(RpakVirtualSegmentBlock) * Header.MemPageCount);
	ParseStream->Seek(Header.DescriptorCount * sizeof(RpakDescriptor), IO::SeekOrigin::Current);
	ParseStream->Read((uint8_t*)&AssetEntries[0], 0, sizeof(RpakApexAssetEntry) * Header.AssetEntryCount);

	ParseStream->Seek(sizeof(RpakDescriptor) * Header.GuidDescriptorCount, IO::SeekOrigin::Current);
	ParseStream->Seek(sizeof(RpakFileRelation) * Header.RelationsCount, IO::SeekOrigin::Current);

	// do we have patch info
	if (Header.PatchIndex)
	{
		File->PatchData = std::make_unique<uint8_t[]>(PatchHeader.PatchDataSize);
		File->PatchDataSize = PatchHeader.PatchDataSize;

		ParseStream->Read(File->PatchData.get(), 0, PatchHeader.PatchDataSize);

		// used to index an array of functions for patching data
		char patch_funcs[64];
		char some_buffer_1[64];

		char unk_buffer_2[256];
		char some_buffer_2[256];

		int new_index = RTech::PakPatch_DecodeData((char*)File->PatchData.get(), 6, nullptr, patch_funcs, some_buffer_1);

		new_index = RTech::PakPatch_DecodeData((char*)File->PatchData.get() + new_index, 8, nullptr, unk_buffer_2, some_buffer_2);
	}

	uint64_t BufferRemaining = ParseStream->GetLength() - ParseStream->GetPosition();

	uint64_t Offset = Header.PageOffset;
	for (uint32_t i = PatchHeader.PatchSegmentIndex; i < Header.MemPageCount; i++)
	{
		File->SegmentBlocks.EmplaceBack(Offset, MemPages[i].DataSize);
		Offset += MemPages[i].DataSize;
	}

	for (auto& Asset : AssetEntries)
	{
		File->AssetHashmap.Add(Asset.NameHash, Asset);
	}

	File->StartSegmentIndex = PatchHeader.PatchSegmentIndex;
	File->SegmentData = std::make_unique<uint8_t[]>(BufferRemaining);
	File->SegmentDataSize = BufferRemaining;
	File->EmbeddedStarpakOffset = Header.EmbeddedStarpakOffset - ParseStream->GetPosition();
	File->EmbeddedStarpakSize = Header.EmbeddedStarpakSize;

	ParseStream->Read(File->SegmentData.get(), 0, BufferRemaining);


	string BasePath = IO::Path::GetDirectoryName(RpakPath);
	string FileNameNoExt = IO::Path::GetFileNameWithoutExtension(RpakPath);

		// Trim off the () if exists
	if (FileNameNoExt.Contains("("))
		FileNameNoExt = FileNameNoExt.Substring(0, FileNameNoExt.IndexOf("("));

	string FinalPath = IO::Path::Combine(BasePath, FileNameNoExt);

	for (uint32_t i = 0; i < Header.PatchIndex; i++)
	{
		uint16_t PatchIndexToFile = PatchIndicesToFile[i];
		string AdditionalRpakToLoad = string::Format(PatchIndexToFile == 0 ? "%s.rpak" : "%s(%02d).rpak", FinalPath.ToCString(), PatchIndexToFile);

		if (this->LoadedFilePaths.Contains(AdditionalRpakToLoad) || this->LoadFileQueue.Contains(AdditionalRpakToLoad))
			continue;
		
		this->LoadFileQueue.EmplaceBack(AdditionalRpakToLoad);
	}

	return true;
}

bool RpakLib::ParseTitanfallRpak(const string& RpakPath, std::unique_ptr<IO::MemoryStream>& ParseStream)
{
	IO::BinaryReader Reader = IO::BinaryReader(ParseStream.get(), true);
	string RpakRoot = IO::Path::GetDirectoryName(RpakPath);
	RpakTitanfallHeader Header = Reader.Read<RpakTitanfallHeader>();
	RpakFile* File = &this->LoadedFiles[this->LoadedFileIndex++];

	File->CreatedTime = Header.CreatedFileTime;
	File->Hash = Header.Hash;

	// Default version is apex, 0x8, must make sure this is set.
	File->Version = RpakGameVersion::Titanfall;

	RpakPatchHeader PatchHeader{};
	RpakPatchCompressPair PatchCompressPairs[16]{};
	uint16_t PatchIndicesToFile[16]{};

	if (Header.PatchIndex)
	{
		ParseStream->Read((uint8_t*)&PatchHeader, 0, sizeof(RpakPatchHeader));
		ParseStream->Read((uint8_t*)&PatchCompressPairs, 0, sizeof(RpakPatchCompressPair) * min(Header.PatchIndex, 16));
		ParseStream->Read((uint8_t*)&PatchIndicesToFile, 0, sizeof(uint16_t) * min(Header.PatchIndex, 16));
	}

	uint32_t StarpakLen = Header.StarpakReferenceSize;
	while (StarpakLen > 0)
	{
		string Starpak = Reader.ReadCString();

		if (Starpak.Length() > 0)
		{
			string Path = IO::Path::Combine(RpakRoot, IO::Path::GetFileName(Starpak));
			this->MountStarpak(Path, this->LoadedFileIndex - 1, File->StarpakReferences.Count(), false);
			File->StarpakReferences.EmplaceBack(Path);
		}

		StarpakLen -= Starpak.Length() + sizeof(char);
	}

	// We need to load the rest of the data before applying a patch stream
	List<RpakVirtualSegment> VirtualSegments;
	List<RpakVirtualSegmentBlock> MemPages;
	List<RpakTitanfallAssetEntry> AssetEntries;

	for (uint32_t i = 0; i < Header.VirtualSegmentCount; i++)
	{
		VirtualSegments.EmplaceBack(Reader.Read<RpakVirtualSegment>());
	}
	for (uint32_t i = 0; i < Header.MemPageCount; i++)
	{
		MemPages.EmplaceBack(Reader.Read<RpakVirtualSegmentBlock>());
	}
	for (uint32_t i = 0; i < Header.DescriptorCount; i++)
	{
		RpakDescriptor Descriptor = Reader.Read<RpakDescriptor>();
	}
	for (uint32_t i = 0; i < Header.AssetEntryCount; i++)
	{
		AssetEntries.EmplaceBack(Reader.Read<RpakTitanfallAssetEntry>());
	}

	// number of guid references in the pakfile
	ParseStream->Seek(sizeof(RpakDescriptor) * Header.GuidDescriptorCount, IO::SeekOrigin::Current);
	ParseStream->Seek(sizeof(RpakFileRelation) * Header.UnknownSixedBlockCount, IO::SeekOrigin::Current);

	// 7th and 8th blocks are weird and useless
	ParseStream->Seek(sizeof(uint32_t) * Header.UnknownSeventhBlockCount, IO::SeekOrigin::Current);
	ParseStream->Seek(Header.UnknownEighthBlockCount, IO::SeekOrigin::Current);

	// At this point, we need to check if we have to switch to a patch edit stream
	if (Header.PatchIndex)
	{
		File->PatchData = std::make_unique<uint8_t[]>(PatchHeader.PatchDataSize);
		File->PatchDataSize = PatchHeader.PatchDataSize;

		ParseStream->Read(File->PatchData.get(), 0, PatchHeader.PatchDataSize);
	}

	uint64_t BufferRemaining = ParseStream->GetLength() - ParseStream->GetPosition();

	uint64_t Offset = 0;
	for (uint32_t i = PatchHeader.PatchSegmentIndex; i < Header.MemPageCount; i++)
	{
		File->SegmentBlocks.EmplaceBack(Offset, MemPages[i].DataSize);
		Offset += MemPages[i].DataSize;
	}

	for (auto& Asset : AssetEntries)
	{
		RpakApexAssetEntry NewAsset{};
		NewAsset.OptimalStarpakOffset = (uint64_t)(-1);

		std::memcpy(&NewAsset, &Asset, 40);
		std::memcpy(((uint8_t*)&NewAsset) + 48, ((uint8_t*)&Asset) + 40, 32);

		File->AssetHashmap.Add(Asset.NameHash, NewAsset);
	}
	File->StartSegmentIndex = PatchHeader.PatchSegmentIndex;
	File->SegmentData = std::make_unique<uint8_t[]>(BufferRemaining);
	File->SegmentDataSize = BufferRemaining;

	ParseStream->Read(File->SegmentData.get(), 0, BufferRemaining);

	if (this->LoadedFileIndex == 1)
	{
		string BasePath = IO::Path::GetDirectoryName(RpakPath);
		string FileNameNoExt = IO::Path::GetFileNameWithoutExtension(RpakPath);

		// Trim off the () if exists
		if (FileNameNoExt.Contains("("))
			FileNameNoExt = FileNameNoExt.Substring(0, FileNameNoExt.IndexOf("("));

		string FinalPath = IO::Path::Combine(BasePath, FileNameNoExt);

		for (uint32_t i = 0; i < Header.PatchIndex; i++)
		{
			uint16_t PatchIndexToFile = PatchIndicesToFile[i];
			if (PatchIndexToFile == 0)
				this->LoadFileQueue.EmplaceBack(string::Format("%s.rpak", FinalPath.ToCString()));
			else
				this->LoadFileQueue.EmplaceBack(string::Format("%s(%02d).rpak", FinalPath.ToCString(), PatchIndexToFile));
		}
	}

	return true;
}

bool RpakLib::ParseR2TTRpak(const string& RpakPath, std::unique_ptr<IO::MemoryStream>& ParseStream)
{
	IO::BinaryReader Reader = IO::BinaryReader(ParseStream.get(), true);
	string RpakRoot = IO::Path::GetDirectoryName(RpakPath);
	RpakHeaderV6 Header = Reader.Read<RpakHeaderV6>();
	RpakFile* File = &this->LoadedFiles[this->LoadedFileIndex++];

	File->CreatedTime = Header.CreatedFileTime;
	File->Hash = Header.Hash;

	// Default version is apex, 0x8, must make sure this is set.
	File->Version = RpakGameVersion::R2TT;

	uint32_t StarpakLen = Header.StarpakReferenceSize;
	while (StarpakLen > 0)
	{
		string Starpak = Reader.ReadCString();

		if (Starpak.Length() > 0)
		{
			string Path = IO::Path::Combine(RpakRoot, IO::Path::GetFileName(Starpak));
			this->MountStarpak(Path, this->LoadedFileIndex - 1, File->StarpakReferences.Count(), false);
			File->StarpakReferences.EmplaceBack(Path);
		}

		StarpakLen -= Starpak.Length() + sizeof(char);
	}

	List<RpakVirtualSegment> VirtualSegments;
	List<RpakVirtualSegmentBlock> MemPages;
	List<RpakTitanfallAssetEntry> AssetEntries;

	for (uint32_t i = 0; i < Header.VirtualSegmentCount; i++)
	{
		VirtualSegments.EmplaceBack(Reader.Read<RpakVirtualSegment>());
	}
	for (uint32_t i = 0; i < Header.MemPageCount; i++)
	{
		MemPages.EmplaceBack(Reader.Read<RpakVirtualSegmentBlock>());
	}
	for (uint32_t i = 0; i < Header.DescriptorCount; i++)
	{
		RpakDescriptor Descriptor = Reader.Read<RpakDescriptor>();
	}
	for (uint32_t i = 0; i < Header.AssetEntryCount; i++)
	{
		AssetEntries.EmplaceBack(Reader.Read<RpakTitanfallAssetEntry>());
	}

	ParseStream->Seek(sizeof(RpakDescriptor) * Header.GuidDescriptorCount, IO::SeekOrigin::Current);
	ParseStream->Seek(sizeof(RpakFileRelation) * Header.UnknownSixthBlockCount, IO::SeekOrigin::Current);

	// 7th and 8th blocks are weird and useless
	ParseStream->Seek(sizeof(uint32_t) * Header.UnknownSeventhBlockCount, IO::SeekOrigin::Current);
	ParseStream->Seek(Header.UnknownEighthBlockCount, IO::SeekOrigin::Current);

	uint64_t BufferRemaining = ParseStream->GetLength() - ParseStream->GetPosition();

	uint64_t Offset = 0;
	for (uint32_t i = 0; i < Header.MemPageCount; i++)
	{
		File->SegmentBlocks.EmplaceBack(Offset, MemPages[i].DataSize);
		Offset += MemPages[i].DataSize;
	}

	for (auto& Asset : AssetEntries)
	{
		RpakApexAssetEntry NewAsset{};
		NewAsset.OptimalStarpakOffset = (uint64_t)(-1);

		std::memcpy(&NewAsset, &Asset, 40);
		std::memcpy(((uint8_t*)&NewAsset) + 48, ((uint8_t*)&Asset) + 40, 32);

		File->AssetHashmap.Add(Asset.NameHash, NewAsset);
	}
	File->StartSegmentIndex = 0;
	File->SegmentData = std::make_unique<uint8_t[]>(BufferRemaining);
	File->SegmentDataSize = BufferRemaining;

	ParseStream->Read(File->SegmentData.get(), 0, BufferRemaining);

	if (this->LoadedFileIndex == 1)
	{
		string BasePath = IO::Path::GetDirectoryName(RpakPath);
		string FileNameNoExt = IO::Path::GetFileNameWithoutExtension(RpakPath);

		// Trim off the () if exists
		if (FileNameNoExt.Contains("("))
			FileNameNoExt = FileNameNoExt.Substring(0, FileNameNoExt.IndexOf("("));

		string FinalPath = IO::Path::Combine(BasePath, FileNameNoExt);

		this->LoadFileQueue.EmplaceBack(string::Format("%s.rpak", FinalPath.ToCString()));
	}

	return true;
}

void RpakLib::MountStarpak(const string& Path, uint32_t FileIndex, uint32_t StarpakIndex, bool Optimal)
{
	RpakFile& File = this->LoadedFiles[FileIndex];

	if (!IO::File::Exists(Path))
	{
		g_Logger.Warning("Missing streaming file %s\n", Path.ToCString());
		return;
	}

	IO::BinaryReader Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	IO::Stream* StarpakStream = Reader.GetBaseStream();

	StarpakStream->SetPosition(StarpakStream->GetLength() - sizeof(uint64_t));

	uint64_t EntryCount = Reader.Read<uint64_t>();

	StarpakStream->SetPosition(StarpakStream->GetLength() - sizeof(uint64_t) - (sizeof(StarpakStreamEntry) * EntryCount));

	for (uint32_t i = 0; i < EntryCount; i++)
	{
		StarpakStreamEntry Entry = Reader.Read<StarpakStreamEntry>();

		if (Optimal)
		{
			File.OptimalStarpakMap.Add(Entry.Offset + StarpakIndex, Entry.Size);
		}
		else
		{
			File.StarpakMap.Add(Entry.Offset + StarpakIndex, Entry.Size);
		}
	}
}

bool RpakLib::MountApexRpak(const string& Path, bool Dump)
{
	IO::BinaryReader Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	RpakApexHeader Header = Reader.Read<RpakApexHeader>();

	if (!Header.IsCompressed && Header.CompressedSize == Header.DecompressedSize)
	{
		auto Stream = std::make_unique<IO::MemoryStream>();

		Reader.GetBaseStream()->SetPosition(0);
		Reader.GetBaseStream()->CopyTo(Stream.get());
		Stream->SetPosition(0);

		return ParseApexRpak(Path, Stream);
	}

	auto CompressedBuffer = std::make_unique<uint8_t[]>(Header.CompressedSize);

	Reader.Read(CompressedBuffer.get() + sizeof(RpakApexHeader), 0, Header.CompressedSize - sizeof(RpakApexHeader));

	rpak_decomp_state state;

	uint64_t dSize = RTech::DecompressPakfileInit(&state, CompressedBuffer.get(), Header.CompressedSize, 0, sizeof(RpakApexHeader));

	std::vector<std::uint8_t> pakbuf(dSize, 0);

	state.out_mask = UINT64_MAX;
	state.out = uint64_t(pakbuf.data());

	std::uint8_t decomp_result = RTech::DecompressPakFile(&state, dSize, pakbuf.size());

	Header.CompressedSize = Header.DecompressedSize;
	std::memcpy(pakbuf.data(), &Header, sizeof(RpakApexHeader));

	auto ResultStream = std::make_unique<IO::MemoryStream>(pakbuf.data(), 0, Header.DecompressedSize, true, true, true);

#if _DEBUG
	if (Dump)
	{
		auto OutStream = IO::File::Create(IO::Path::Combine("D:\\", IO::Path::GetFileName(Path)));
		ResultStream->CopyTo(OutStream.get());
		ResultStream->SetPosition(0);
	}
#endif

	return ParseApexRpak(Path, ResultStream);
}

bool RpakLib::MountTitanfallRpak(const string& Path, bool Dump)
{
	IO::BinaryReader Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	RpakTitanfallHeader Header = Reader.Read<RpakTitanfallHeader>();

	if (Header.CompressedSize == Header.DecompressedSize)
	{
		auto Stream = std::make_unique<IO::MemoryStream>();

		Reader.GetBaseStream()->SetPosition(0);
		Reader.GetBaseStream()->CopyTo(Stream.get());
		Stream->SetPosition(0);

		return ParseTitanfallRpak(Path, Stream);
	}

	auto CompressedBuffer = std::make_unique<uint8_t[]>(Header.CompressedSize);

	Reader.Read(CompressedBuffer.get() + sizeof(RpakTitanfallHeader), 0, Header.CompressedSize - sizeof(RpakTitanfallHeader));

	rpak_decomp_state state;

	uint64_t dSize = RTech::DecompressPakfileInit(&state, CompressedBuffer.get(), Header.CompressedSize, 0, sizeof(RpakTitanfallHeader));

	std::vector<std::uint8_t> pakbuf(dSize, 0);

	state.out_mask = UINT64_MAX;
	state.out = uint64_t(pakbuf.data());

	std::uint8_t decomp_result = RTech::DecompressPakFile(&state, dSize, pakbuf.size());

	Header.CompressedSize = Header.DecompressedSize;
	std::memcpy(pakbuf.data(), &Header, sizeof(RpakTitanfallHeader));

	auto ResultStream = std::make_unique<IO::MemoryStream>(pakbuf.data(), 0, Header.DecompressedSize, true, true, true);

#if _DEBUG
	if (Dump)
	{
		auto OutStream = IO::File::Create(IO::Path::Combine("D:\\", IO::Path::GetFileName(Path)));
		ResultStream->CopyTo(OutStream.get());
		ResultStream->SetPosition(0);
	}
#endif
	return ParseTitanfallRpak(Path, ResultStream);
}

bool RpakLib::MountR2TTRpak(const string& Path, bool Dump)
{
	IO::BinaryReader Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	RpakHeaderV6 Header = Reader.Read<RpakHeaderV6>();

	// rpak v6 doesn't seem to support compression

	auto Stream = std::make_unique<IO::MemoryStream>();

	Reader.GetBaseStream()->SetPosition(0);
	Reader.GetBaseStream()->CopyTo(Stream.get());

	Stream.get()->SetPosition(0);

	return ParseR2TTRpak(Path, Stream);
}

string RpakLib::ReadStringFromPointer(const RpakLoadAsset& Asset, const RPakPtr& ptr)
{
	// this might be bad but it works for now
	if (!ptr.Index && !ptr.Offset)
		return "";

	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, ptr.Index, ptr.Offset));

	string result = Reader.ReadCString();

	return result;
}

string RpakLib::ReadStringFromPointer(const RpakLoadAsset& Asset, uint32_t index, uint32_t offset)
{
	// this might be bad but it works for now
	if (!index && !offset)
		return "";

	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, index, offset));

	string result = Reader.ReadCString();

	return result;
}

RpakLoadAsset::RpakLoadAsset(uint64_t NameHash, uint32_t FileIndex, uint32_t AssetType, uint32_t SubHeaderIndex, uint32_t SubHeaderOffset, uint32_t SubHeaderSize, uint32_t RawDataIndex, uint32_t RawDataOffset, uint64_t StarpakOffset, uint64_t OptimalStarpakOffset, RpakGameVersion Version, uint32_t AssetVersion, RpakFile* PakFile)
	: NameHash(NameHash), FileIndex(FileIndex), RpakFileIndex(FileIndex), AssetType(AssetType), SubHeaderIndex(SubHeaderIndex), SubHeaderOffset(SubHeaderOffset), SubHeaderSize(SubHeaderSize), RawDataIndex(RawDataIndex), RawDataOffset(RawDataOffset), StarpakOffset(StarpakOffset), OptimalStarpakOffset(OptimalStarpakOffset), Version(Version), AssetVersion(AssetVersion), PakFile(PakFile)
{
}

RpakSegmentBlock::RpakSegmentBlock(uint64_t Offset, uint64_t Size)
	: Offset(Offset), Size(Size)
{
}
