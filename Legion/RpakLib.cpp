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
	List<string> LoadedFiles;

	for (auto& Rpak : Paths)
	{
		// Ignore duplicate files triggered by loading multiple rpaks at once.
		if (LoadedFiles.Contains(Rpak))
			continue;

		this->LoadRpak(Rpak, false);

		// Copy over to loaded, clear for next file
		for (auto& Loaded : this->LoadFileQueue)
		{
			LoadedFiles.EmplaceBack(Loaded);
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
		auto& LoadedFile = this->LoadedFiles[i];

		for (auto& Kvp : LoadedFile.AssetHashmap)
		{
			if (!this->Assets.ContainsKey(Kvp.first))
			{
				auto Asset = RpakLoadAsset(
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
					Kvp.second.Version
				);

				// All assets must follow this patch sequence
				if (this->ValidateAssetPatchStatus(Asset))
				{
					// If we have a model or texture, it must also pass the stream test
					if (Asset.AssetType == (uint32_t)RpakAssetType::Model
						|| Asset.AssetType == (uint32_t)RpakAssetType::Texture
						|| Asset.AssetType == (uint32_t)RpakAssetType::UIIA)
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

			auto& File = this->LoadedFiles[i];

			if (File.AssetHashmap.ContainsKey(Kvp.first))
			{
				auto& Asset = File.AssetHashmap[Kvp.first];

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

std::unique_ptr<List<ApexAsset>> RpakLib::BuildAssetList(bool Models, bool Anims, bool Images, bool Materials, bool UIImages, bool DataTables)
{
	auto Result = std::make_unique<List<ApexAsset>>();

	for (auto& AssetKvp : Assets)
	{
		auto& Asset = AssetKvp.Value();

		ApexAsset NewAsset;
		NewAsset.Hash = AssetKvp.first;

		switch (Asset.AssetType)
		{
		case (uint32_t)RpakAssetType::Model:
			if (!Models)
				continue;
			BuildModelInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::AnimationRig:
			if (!Anims)
				continue;
			BuildAnimInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::Material:
			if (!Materials)
				continue;
			BuildMaterialInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::Texture:
			if (!Images)
				continue;
			BuildTextureInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::UIIA:
			if (!UIImages)
				continue;
			BuildUIIAInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::DataTable:
			if (!DataTables)
				continue;
			BuildDataTableInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::Subtitles:
			// todo: subtitle loading setting
			BuildSubtitleInfo(Asset, NewAsset);
			break;
		case (uint32_t)RpakAssetType::ShaderSet:
			if (!ExportManager::Config.Get<System::SettingType::Boolean>("LoadShaderSets"))
				continue;
			BuildShaderSetInfo(Asset, NewAsset);
			break;
		default:
			continue;
		}

		Result->EmplaceBack(std::move(NewAsset));
	}

	return std::move(Result);
}

void RpakLib::InitializeModelExporter(RpakModelExportFormat Format)
{
	switch (Format)
	{
	case RpakModelExportFormat::Maya:
		ModelExporter = std::make_unique<Assets::Exporters::AutodeskMaya>();
		break;
	case RpakModelExportFormat::OBJ:
		ModelExporter = std::make_unique<Assets::Exporters::WavefrontOBJ>();
		break;
	case RpakModelExportFormat::XNALaraText:
		ModelExporter = std::make_unique<Assets::Exporters::XNALaraAscii>();
		break;
	case RpakModelExportFormat::XNALaraBinary:
		ModelExporter = std::make_unique<Assets::Exporters::XNALaraBinary>();
		break;
	case RpakModelExportFormat::SMD:
		ModelExporter = std::make_unique<Assets::Exporters::ValveSMD>();
		break;
	case RpakModelExportFormat::XModel:
		ModelExporter = std::make_unique<Assets::Exporters::CoDXAssetExport>();
		break;
	case RpakModelExportFormat::FBX:
		ModelExporter = std::make_unique<Assets::Exporters::KaydaraFBX>();
		break;
	case RpakModelExportFormat::Cast:
		ModelExporter = std::make_unique<Assets::Exporters::CastAsset>();
		break;
	// no custom exporter for RMDL as it will be handled directly in the export func
	default:
		ModelExporter = std::make_unique<Assets::Exporters::SEAsset>();
		break;
	}

	m_bModelExporterInitialized = true;
}

void RpakLib::InitializeAnimExporter(RpakAnimExportFormat Format)
{
	switch (Format)
	{
	case RpakAnimExportFormat::Cast:
		AnimExporter = std::make_unique<Assets::Exporters::CastAsset>();
		break;
	default:
		AnimExporter = std::make_unique<Assets::Exporters::SEAsset>();
		break;
	}

	m_bAnimExporterInitialized = true;
}

void RpakLib::InitializeImageExporter(RpakImageExportFormat Format)
{
	switch (Format)
	{
	case RpakImageExportFormat::Dds:
		ImageSaveType = Assets::SaveFileType::Dds;
		ImageExtension = Assets::Texture::GetExtensionForType(ImageSaveType);
		break;
	case RpakImageExportFormat::Png:
		ImageSaveType = Assets::SaveFileType::Png;
		ImageExtension = Assets::Texture::GetExtensionForType(ImageSaveType);
		break;
	case RpakImageExportFormat::Tiff:
		ImageSaveType = Assets::SaveFileType::Tiff;
		ImageExtension = Assets::Texture::GetExtensionForType(ImageSaveType);
		break;
	}

	m_bImageExporterInitialized = true;
}

std::unique_ptr<IO::MemoryStream> RpakLib::GetFileStream(const RpakLoadAsset& Asset)
{
	auto& File = this->LoadedFiles[Asset.FileIndex];

	return std::move(std::make_unique<IO::MemoryStream>(File.SegmentData.get(), 0, File.SegmentDataSize, false, true));
}

// this is really slow
bool RpakLib::IsValidDescriptor(const RpakLoadAsset& Asset, uint32_t SegmentIndex, uint32_t SegmentOffset)
{
	uint64_t Temp = static_cast<uint64_t>(SegmentIndex) << 32 | SegmentOffset;
	return (this->LoadedFiles[Asset.FileIndex].DescriptorList.Contains(Temp));
}

uint64_t RpakLib::GetFileOffset(const RpakLoadAsset& Asset, uint32_t SegmentIndex, uint32_t SegmentOffset)
{
	if (SegmentIndex < 0) return 0;

	return (this->LoadedFiles[Asset.FileIndex].SegmentBlocks[SegmentIndex - this->LoadedFiles[Asset.FileIndex].StartSegmentIndex].Offset + SegmentOffset);
}

uint64_t RpakLib::GetEmbeddedStarpakOffset(const RpakLoadAsset& Asset)
{
	return this->LoadedFiles[Asset.FileIndex].EmbeddedStarpakOffset;
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

string RpakLib::GetSubtitlesNameFromHash(uint64_t Hash)
{
	if (SubtitleLanguageMap.count((SubtitleLanguageHash)Hash))
		return "subtitles_" + SubtitleLanguageMap[(SubtitleLanguageHash)Hash];

	return string::Format("subt_0x%llx", Hash);
}

void RpakLib::ParseRAnimBoneTranslationTrack(const RAnimBoneFlag& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	uint16_t* TranslationDataPtr = *BoneTrackData;

	if (!BoneFlags.bDynamicTranslation)
	{
		auto& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

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

		auto& Bone = Anim->Bones[BoneIndex].LocalPosition();

		float Result[3]{ Bone.X, Bone.Y, Bone.Z };

		uint32_t TranslationIndex = 0;
		uint32_t v32 = 0xF;

		uint8_t* dataPtrs[] = { TranslationDataX,TranslationDataY,TranslationDataZ };

		float TranslationFinal = 0, TimeScale = 0 /*Might not be 'TimeScale'*/;
		float Time = 0;	// time but doesn't matter
		do
		{
			if (_bittest((const long*)&TranslationFlags, v32))
			{
				g_pRtech->DecompressDynamicTrack(Frame, dataPtrs[TranslationIndex], TranslationScale, &TranslationFinal, &TimeScale);

				if (BoneFlags.bAdditiveCustom)
					Result[TranslationIndex] = (float)((float)((float)(1.0 - Time) * TranslationFinal) + (float)(TimeScale * Time));
				else
					Result[TranslationIndex] = (float)((float)((float)(1.0 - Time) * TranslationFinal) + (float)(TimeScale * Time)) + Result[TranslationIndex];
			}

			--v32;
			++TranslationIndex;
		} while (TranslationIndex < 3);

		auto& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

		// TranslateX/Y/Z
		Curves[1].Keyframes.EmplaceBack(FrameIndex, Result[0]);
		Curves[2].Keyframes.EmplaceBack(FrameIndex, Result[1]);
		Curves[3].Keyframes.EmplaceBack(FrameIndex, Result[2]);

		*BoneTrackData += 4;	// Advance over the size of the data
	}
}

void RpakLib::ParseRAnimBoneRotationTrack(const RAnimBoneFlag& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	uint16_t* RotationDataPtr = *BoneTrackData;

	struct Quat64
	{
		uint64_t X : 21;
		uint64_t Y : 21;
		uint64_t Z : 21;
		uint64_t WNeg : 1;
	};

	if (!BoneFlags.bDynamicRotation)
	{
		auto PackedQuat = *(Quat64*)RotationDataPtr;

		Math::Quaternion Quat;

		Quat.X = ((int)PackedQuat.X - 1048576) * (1 / 1048576.5f);
		Quat.Y = ((int)PackedQuat.Y - 1048576) * (1 / 1048576.5f);
		Quat.Z = ((int)PackedQuat.Z - 1048576) * (1 / 1048576.5f);
		Quat.W = std::sqrt(1 - Quat.X * Quat.X - Quat.Y * Quat.Y - Quat.Z * Quat.Z);

		if (PackedQuat.WNeg)
			Quat.W = -Quat.W;

		// RotateQuaternion
		Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name())[0].Keyframes.Emplace(FrameIndex, Math::Quaternion(Quat[0], Quat[1], Quat[2], Quat[3]));

		*BoneTrackData += 4;	// Advance over the size of the data
	}
	else
	{
		uint16_t RotationFlags = RotationDataPtr[0];

		uint8_t* TranslationDataX = (uint8_t*)RotationDataPtr + (RotationFlags & 0x1FFF);	// Data for x

		uint64_t DataYOffset = *((uint8_t*)RotationDataPtr + 2);
		uint64_t DataZOffset = *((uint8_t*)RotationDataPtr + 3);

		uint8_t* TranslationDataY = &TranslationDataX[2 * DataYOffset];	// Data for y
		uint8_t* TranslationDataZ = &TranslationDataX[2 * DataZOffset];	// Data for z


		auto BoneRotation = Anim->Bones[BoneIndex].LocalRotation().ToEulerAngles();

		float EulerResult[4]{ Math::MathHelper::DegreesToRadians(BoneRotation.X),Math::MathHelper::DegreesToRadians(BoneRotation.Y),Math::MathHelper::DegreesToRadians(BoneRotation.Z),0 };

		uint32_t v31 = 0;
		uint32_t v32 = 0xF;

		uint8_t* dataPtrs[] = { TranslationDataX,TranslationDataY,TranslationDataZ };

		float TranslationFinal = 0, TimeScale = 0 /*Might not be 'TimeScale'*/;
		float a2 = 0;	// time?
		do
		{
			if (_bittest((const long*)&RotationFlags, v32))
			{
				g_pRtech->DecompressDynamicTrack(Frame, dataPtrs[v31], 0.00019175345f, &TranslationFinal, &TimeScale);
				EulerResult[v31] = TranslationFinal;
			}

			--v32;
			++v31;
		} while (v31 < 3);

		Math::Quaternion Result;
		g_pRtech->DecompressConvertRotation((const __m128i*) & EulerResult[0], (float*)&Result);

		Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name())[0].Keyframes.Emplace(FrameIndex, Result);

		*BoneTrackData += 2;	// Advance over the size of the data
	}
}

void RpakLib::ParseRAnimBoneScaleTrack(const RAnimBoneFlag& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	uint16_t* ScaleDataPtr = *BoneTrackData;

	if (!BoneFlags.bDynamicScale)
	{
		auto& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

		// ScaleX/Y/Z
		Curves[4].Keyframes.EmplaceBack(FrameIndex, Math::Half(ScaleDataPtr[0]).ToFloat());
		Curves[5].Keyframes.EmplaceBack(FrameIndex, Math::Half(ScaleDataPtr[1]).ToFloat());
		Curves[6].Keyframes.EmplaceBack(FrameIndex, Math::Half(ScaleDataPtr[2]).ToFloat());

		*BoneTrackData += 3;	// Advance over the size of the data
	}
	else
	{
		uint32_t ScaleFlags = ScaleDataPtr[0];

		uint8_t* TranslationDataX = (uint8_t*)ScaleDataPtr + (ScaleFlags & 0x1FFF);	// Data for x

		uint64_t DataYOffset = *((uint8_t*)ScaleDataPtr + 2);
		uint64_t DataZOffset = *((uint8_t*)ScaleDataPtr + 3);

		uint8_t* TranslationDataY = &TranslationDataX[2 * DataYOffset];	// Data for y
		uint8_t* TranslationDataZ = &TranslationDataX[2 * DataZOffset];	// Data for z

		auto& BoneScale = Anim->Bones[BoneIndex].Scale();

		float Result[3]{ BoneScale.X, BoneScale.Y, BoneScale.Z };

		uint32_t v31 = 0;
		uint32_t v32 = 0xF;

		uint8_t* dataPtrs[] = { TranslationDataX,TranslationDataY,TranslationDataZ };

		float TranslationFinal = 0, TimeScale = 0 /*Might not be 'TimeScale'*/;
		float a2 = 0;	// time but doesn't matter
		do
		{
			if (_bittest((const long*)&ScaleFlags, v32))
			{
				g_pRtech->DecompressDynamicTrack(Frame, dataPtrs[v31], 0.0030518509f, &TranslationFinal, &TimeScale);
				Result[v31] = (float)((float)((float)(1.0 - a2) * TranslationFinal) + (float)(TimeScale * a2)) + Result[v31];
			}

			--v32;
			++v31;
		} while (v31 < 3);

		auto& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

		// ScaleX/Y/Z
		Curves[4].Keyframes.EmplaceBack(FrameIndex, Result[0]);
		Curves[5].Keyframes.EmplaceBack(FrameIndex, Result[1]);
		Curves[6].Keyframes.EmplaceBack(FrameIndex, Result[2]);

		*BoneTrackData += 2;	// Advance over the size of the data
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
		auto Reader = IO::BinaryReader(RpakStream.get(), true);

		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

		switch (Asset.AssetType)
		{
		case (uint32_t)RpakAssetType::Model:
		{
			if (Asset.SubHeaderSize <= 0x68)
			{
				auto SubHeader = Reader.Read<ModelHeaderS68>();
				return (SubHeader.NameIndex >= LoadedFile.StartSegmentIndex && SubHeader.SkeletonIndex >= LoadedFile.StartSegmentIndex);
			}
			else
			{
				auto SubHeader = Reader.Read<ModelHeaderS80>();
				return (SubHeader.NameIndex >= LoadedFile.StartSegmentIndex && SubHeader.SkeletonIndex >= LoadedFile.StartSegmentIndex);
			}
		}
		case (uint32_t)RpakAssetType::Texture:
		{
			auto SubHeader = Reader.Read<TextureHeader>();
			return (SubHeader.DataSize > 0);
		}
		case (uint32_t)RpakAssetType::UIIA:
		{
			auto SubHeader = Reader.Read<UIIAHeader>();
			return (SubHeader.Width > 0 && SubHeader.Height > 0);
		}
		case (uint32_t)RpakAssetType::Material:
		{
			auto SubHeader = Reader.Read<MaterialHeader>();
			if (Asset.Version == RpakGameVersion::Apex)
				return (SubHeader.NameIndex >= LoadedFile.StartSegmentIndex && SubHeader.TexturesIndex >= LoadedFile.StartSegmentIndex);
			else
				return (SubHeader.NameIndex >= LoadedFile.StartSegmentIndex && SubHeader.TexturesTFIndex >= LoadedFile.StartSegmentIndex);
		}
		case (uint32_t)RpakAssetType::AnimationRig:
		{
			auto SubHeader = Reader.Read<AnimRigHeader>();
			return (SubHeader.NameIndex >= LoadedFile.StartSegmentIndex && SubHeader.SkeletonIndex >= LoadedFile.StartSegmentIndex && SubHeader.AnimationReferenceIndex >= LoadedFile.StartSegmentIndex);
		}
		case (uint32_t)RpakAssetType::Animation:
		{
			auto SubHeader = Reader.Read<AnimHeader>();
			return (SubHeader.AnimationIndex >= LoadedFile.StartSegmentIndex);
		}
		case (uint32_t)RpakAssetType::DataTable:
		{
			auto SubHeader = Reader.Read<DataTableHeader>();
			return (SubHeader.ColumnCount != 0 && SubHeader.RowCount != 0);
		}
		case (uint32_t)RpakAssetType::Subtitles:
		{
			return true;
		}
		case (uint32_t)RpakAssetType::ShaderSet:
		{
			return true;
		}
		case (uint32_t)RpakAssetType::Shader:
		{
			return true;
		}
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
	auto Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	auto BaseHeader = Reader.Read<RpakBaseHeader>();

	if (BaseHeader.Magic != 0x6B615052)
	{
		return false;
	}

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
	auto Reader = IO::BinaryReader(ParseStream.get(), true);
	auto RpakRoot = IO::Path::GetDirectoryName(RpakPath);
	auto Header = Reader.Read<RpakApexHeader>();
	auto File = &this->LoadedFiles[this->LoadedFileIndex++];

	RpakPatchHeader PatchHeader{};
	RpakPatchCompressPair PatchCompressPairs[16]{};
	uint16_t PatchIndicesToFile[16]{};

	if (Header.PatchIndex)
	{
		ParseStream->Read((uint8_t*)&PatchHeader, 0, sizeof(RpakPatchHeader));
		ParseStream->Read((uint8_t*)&PatchCompressPairs, 0, sizeof(RpakPatchCompressPair) * Header.PatchIndex);
		ParseStream->Read((uint8_t*)&PatchIndicesToFile, 0, sizeof(uint16_t) * Header.PatchIndex);
	}

	uint32_t StarpakLen = Header.StarpakReferenceSize;
	while (StarpakLen > 0)
	{
		auto Starpak = Reader.ReadCString();

		if (Starpak.Length() > 0)
		{
			auto Path = IO::Path::Combine(RpakRoot, IO::Path::GetFileName(Starpak));
			this->MountStarpak(Path, this->LoadedFileIndex - 1, File->StarpakReferences.Count(), false);
			File->StarpakReferences.EmplaceBack(Path);
		}

		StarpakLen -= Starpak.Length() + sizeof(char);
	}
	StarpakLen = Header.StarpakOptReferenceSize;
	while (StarpakLen > 0)
	{
		auto Starpak = Reader.ReadCString();

		if (Starpak.Length() > 0)
		{
			auto Path = IO::Path::Combine(RpakRoot, IO::Path::GetFileName(Starpak));
			this->MountStarpak(Path, this->LoadedFileIndex - 1, File->OptimalStarpakReferences.Count(), true);
			File->OptimalStarpakReferences.EmplaceBack(Path);
		}

		StarpakLen -= Starpak.Length() + sizeof(char);
	}

	// We need to load the rest of the data before applying a patch stream
	List<RpakVirtualSegment> VirtualSegments(Header.VirtualSegmentCount, true);
	List<RpakVirtualSegmentBlock> VirtualSegmentBlocks(Header.VirtualSegmentBlockCount, true); // mem pages

	// each of these points to a descriptor/pointer within rpak mem pages
	// they are used to convert the raw data into an actual pointer when the pak is loaded
	List<RpakDescriptor> Descriptors(Header.DescriptorCount, true);

	List<RpakApexAssetEntry> AssetEntries(Header.AssetEntryCount, true);

	// Faster loading here by reading to the buffers directly
	ParseStream->Read((uint8_t*)&VirtualSegments[0], 0, sizeof(RpakVirtualSegment) * Header.VirtualSegmentCount);
	ParseStream->Read((uint8_t*)&VirtualSegmentBlocks[0], 0, sizeof(RpakVirtualSegmentBlock) * Header.VirtualSegmentBlockCount);

	for (uint32_t i = 0; i < Header.DescriptorCount; i++)
	{
		auto Descriptor = Reader.Read<RpakDescriptor>();
		File->DescriptorList.EmplaceBack(static_cast<uint64_t>(Descriptor.PageIdx) << 32 | Descriptor.PageOffset);
	}

	ParseStream->Read((uint8_t*)&AssetEntries[0], 0, sizeof(RpakApexAssetEntry) * Header.AssetEntryCount);

	// The fifth and sixth blocks appear to only
	// be used for streaming images / starpak stuff, not always there
	ParseStream->Seek(sizeof(RpakDescriptor) * Header.GuidDescriptorCount, IO::SeekOrigin::Current);
	ParseStream->Seek(sizeof(RpakFileRelation) * Header.RelationsCount, IO::SeekOrigin::Current);

	// At this point, we need to check if we have to switch to a patch edit stream
	if (Header.PatchIndex)
	{
		File->PatchData = std::make_unique<uint8_t[]>(PatchHeader.PatchDataSize);
		File->PatchDataSize = PatchHeader.PatchDataSize;

		ParseStream->Read(File->PatchData.get(), 0, PatchHeader.PatchDataSize);
	}

	auto BufferRemaining = ParseStream->GetLength() - ParseStream->GetPosition();

	uint64_t Offset = 0;
	for (uint32_t i = PatchHeader.PatchSegmentIndex; i < Header.VirtualSegmentBlockCount; i++)
	{
		File->SegmentBlocks.EmplaceBack(Offset, VirtualSegmentBlocks[i].DataSize);
		Offset += VirtualSegmentBlocks[i].DataSize;
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

	if (this->LoadedFileIndex == 1)
	{
		auto BasePath = IO::Path::GetDirectoryName(RpakPath);
		auto FileNameNoExt = IO::Path::GetFileNameWithoutExtension(RpakPath);

		// Trim off the () if exists
		if (FileNameNoExt.Contains("("))
			FileNameNoExt = FileNameNoExt.Substring(0, FileNameNoExt.IndexOf("("));

		auto FinalPath = IO::Path::Combine(BasePath, FileNameNoExt);

		for (uint32_t i = 0; i < Header.PatchIndex; i++)
		{
			auto PatchIndexToFile = PatchIndicesToFile[i];

			if (PatchIndexToFile == 0)
				this->LoadFileQueue.EmplaceBack(string::Format("%s.rpak", FinalPath.ToCString()));
			else
				this->LoadFileQueue.EmplaceBack(string::Format("%s(%02d).rpak", FinalPath.ToCString(), PatchIndexToFile));
		}
	}

	return true;
}

bool RpakLib::ParseTitanfallRpak(const string& RpakPath, std::unique_ptr<IO::MemoryStream>& ParseStream)
{
	auto Reader = IO::BinaryReader(ParseStream.get(), true);
	auto RpakRoot = IO::Path::GetDirectoryName(RpakPath);
	auto Header = Reader.Read<RpakTitanfallHeader>();
	auto File = &this->LoadedFiles[this->LoadedFileIndex++];

	// Default version is apex, 0x8, must make sure this is set.
	File->Version = RpakGameVersion::Titanfall;

	RpakPatchHeader PatchHeader{};
	RpakPatchCompressPair PatchCompressPairs[16]{};
	uint16_t PatchIndicesToFile[16]{};

	if (Header.PatchIndex)
	{
		ParseStream->Read((uint8_t*)&PatchHeader, 0, sizeof(RpakPatchHeader));
		ParseStream->Read((uint8_t*)&PatchCompressPairs, 0, sizeof(RpakPatchCompressPair) * Header.PatchIndex);
		ParseStream->Read((uint8_t*)&PatchIndicesToFile, 0, sizeof(uint16_t) * Header.PatchIndex);
	}

	uint32_t StarpakLen = Header.StarpakReferenceSize;
	while (StarpakLen > 0)
	{
		auto Starpak = Reader.ReadCString();

		if (Starpak.Length() > 0)
		{
			auto Path = IO::Path::Combine(RpakRoot, IO::Path::GetFileName(Starpak));
			this->MountStarpak(Path, this->LoadedFileIndex - 1, File->StarpakReferences.Count(), false);
			File->StarpakReferences.EmplaceBack(Path);
		}

		StarpakLen -= Starpak.Length() + sizeof(char);
	}

	// We need to load the rest of the data before applying a patch stream
	List<RpakVirtualSegment> VirtualSegments;
	List<RpakVirtualSegmentBlock> VirtualSegmentBlocks;
	List<RpakTitanfallAssetEntry> AssetEntries;

	for (uint32_t i = 0; i < Header.VirtualSegmentCount; i++)
	{
		VirtualSegments.EmplaceBack(Reader.Read<RpakVirtualSegment>());
	}
	for (uint32_t i = 0; i < Header.VirtualSegmentBlockCount; i++)
	{
		VirtualSegmentBlocks.EmplaceBack(Reader.Read<RpakVirtualSegmentBlock>());
	}
	for (uint32_t i = 0; i < Header.DescriptorCount; i++)
	{
		auto Descriptor = Reader.Read<RpakDescriptor>();
		File->DescriptorList.EmplaceBack(static_cast<uint64_t>(Descriptor.PageIdx) << 32 | Descriptor.PageOffset);
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

	auto BufferRemaining = ParseStream->GetLength() - ParseStream->GetPosition();

	uint64_t Offset = 0;
	for (uint32_t i = PatchHeader.PatchSegmentIndex; i < Header.VirtualSegmentBlockCount; i++)
	{
		File->SegmentBlocks.EmplaceBack(Offset, VirtualSegmentBlocks[i].DataSize);
		Offset += VirtualSegmentBlocks[i].DataSize;
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
		auto BasePath = IO::Path::GetDirectoryName(RpakPath);
		auto FileNameNoExt = IO::Path::GetFileNameWithoutExtension(RpakPath);

		// Trim off the () if exists
		if (FileNameNoExt.Contains("("))
			FileNameNoExt = FileNameNoExt.Substring(0, FileNameNoExt.IndexOf("("));

		auto FinalPath = IO::Path::Combine(BasePath, FileNameNoExt);

		for (uint32_t i = 0; i < Header.PatchIndex; i++)
		{
			auto PatchIndexToFile = PatchIndicesToFile[i];
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
	auto Reader = IO::BinaryReader(ParseStream.get(), true);
	auto RpakRoot = IO::Path::GetDirectoryName(RpakPath);
	auto Header = Reader.Read<RpakHeaderV6>();
	auto File = &this->LoadedFiles[this->LoadedFileIndex++];

	// Default version is apex, 0x8, must make sure this is set.
	File->Version = RpakGameVersion::R2TT;

	uint32_t StarpakLen = Header.StarpakReferenceSize;
	while (StarpakLen > 0)
	{
		auto Starpak = Reader.ReadCString();

		if (Starpak.Length() > 0)
		{
			auto Path = IO::Path::Combine(RpakRoot, IO::Path::GetFileName(Starpak));
			this->MountStarpak(Path, this->LoadedFileIndex - 1, File->StarpakReferences.Count(), false);
			File->StarpakReferences.EmplaceBack(Path);
		}

		StarpakLen -= Starpak.Length() + sizeof(char);
	}

	List<RpakVirtualSegment> VirtualSegments;
	List<RpakVirtualSegmentBlock> Pages;
	List<RpakTitanfallAssetEntry> AssetEntries;

	for (uint32_t i = 0; i < Header.VirtualSegmentCount; i++)
	{
		VirtualSegments.EmplaceBack(Reader.Read<RpakVirtualSegment>());
	}
	for (uint32_t i = 0; i < Header.PageCount; i++)
	{
		Pages.EmplaceBack(Reader.Read<RpakVirtualSegmentBlock>());
	}
	for (uint32_t i = 0; i < Header.DescriptorCount; i++)
	{
		auto Descriptor = Reader.Read<RpakDescriptor>();
		File->DescriptorList.EmplaceBack(static_cast<uint64_t>(Descriptor.PageIdx) << 32 | Descriptor.PageOffset);
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

	auto BufferRemaining = ParseStream->GetLength() - ParseStream->GetPosition();

	uint64_t Offset = 0;
	for (uint32_t i = 0; i < Header.PageCount; i++)
	{
		File->SegmentBlocks.EmplaceBack(Offset, Pages[i].DataSize);
		Offset += Pages[i].DataSize;
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
		auto BasePath = IO::Path::GetDirectoryName(RpakPath);
		auto FileNameNoExt = IO::Path::GetFileNameWithoutExtension(RpakPath);

		// Trim off the () if exists
		if (FileNameNoExt.Contains("("))
			FileNameNoExt = FileNameNoExt.Substring(0, FileNameNoExt.IndexOf("("));

		auto FinalPath = IO::Path::Combine(BasePath, FileNameNoExt);

		this->LoadFileQueue.EmplaceBack(string::Format("%s.rpak", FinalPath.ToCString()));
	}

	return true;
}

void RpakLib::MountStarpak(const string& Path, uint32_t FileIndex, uint32_t StarpakIndex, bool Optimal)
{
	auto& File = this->LoadedFiles[FileIndex];

	if (!IO::File::Exists(Path))
	{
		g_Logger.Warning("Missing streaming file %s\n", Path.ToCString());
		return;
	}

	auto Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	auto StarpakStream = Reader.GetBaseStream();

	StarpakStream->SetPosition(StarpakStream->GetLength() - sizeof(uint64_t));

	auto EntryCount = Reader.Read<uint64_t>();

	StarpakStream->SetPosition(StarpakStream->GetLength() - sizeof(uint64_t) - (sizeof(StarpakStreamEntry) * EntryCount));

	for (uint32_t i = 0; i < EntryCount; i++)
	{
		auto Entry = Reader.Read<StarpakStreamEntry>();

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
	auto Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	auto Header = Reader.Read<RpakApexHeader>();

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

	//std::int64_t params[18];
	rpak_decomp_state state;

	uint32_t dSize = g_pRtech->DecompressPakfileInit(&state, CompressedBuffer.get(), Header.CompressedSize, 0, sizeof(RpakApexHeader));

	std::vector<std::uint8_t> pakbuf(dSize, 0);

	state.out_mask = UINT64_MAX;
	state.out = uint64_t(pakbuf.data());

	std::uint8_t decomp_result = g_pRtech->DecompressPakFile(&state, dSize, pakbuf.size());

	std::memcpy(pakbuf.data(), &Header, sizeof(RpakApexHeader));

	auto ResultStream = std::make_unique<IO::MemoryStream>(pakbuf.data(), 0, Header.DecompressedSize, true, false, true);

#if _DEBUG
	if (Dump)
	{
		auto OutStream = IO::File::Create(IO::Path::Combine("D:\\", IO::Path::GetFileName(Path)));
		ResultStream->CopyTo(OutStream.get());
		ResultStream->SetPosition(0);
	}
#endif
	bool r = ParseApexRpak(Path, ResultStream);

	g_Logger.Info("Parse: %i\n", r);

	return r;
}

bool RpakLib::MountTitanfallRpak(const string& Path, bool Dump)
{
	auto Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	auto Header = Reader.Read<RpakTitanfallHeader>();

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

	//std::int64_t params[18];
	rpak_decomp_state state;

	uint32_t dSize = g_pRtech->DecompressPakfileInit(&state, CompressedBuffer.get(), Header.CompressedSize, 0, sizeof(RpakTitanfallHeader));

	std::vector<std::uint8_t> pakbuf(dSize, 0);

	//params[1] = std::int64_t(pakbuf.data());
	//params[3] = -1i64;
	state.out_mask = UINT64_MAX;
	state.out = uint64_t(pakbuf.data());

	std::uint8_t decomp_result = g_pRtech->DecompressPakFile(&state, dSize, pakbuf.size());

	std::memcpy(pakbuf.data(), &Header, sizeof(RpakTitanfallHeader));

	auto ResultStream = std::make_unique<IO::MemoryStream>(pakbuf.data(), 0, Header.DecompressedSize, true, false, true);

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
	auto Reader = IO::BinaryReader(IO::File::OpenRead(Path));
	auto Header = Reader.Read<RpakHeaderV6>();

	// rpak v6 doesn't seem to support compression

	auto Stream = std::make_unique<IO::MemoryStream>();

	Reader.GetBaseStream()->SetPosition(0);
	Reader.GetBaseStream()->CopyTo(Stream.get());

	Stream.get()->SetPosition(0);

	return ParseR2TTRpak(Path, Stream);
}

RpakLoadAsset::RpakLoadAsset(uint64_t NameHash, uint32_t FileIndex, uint32_t AssetType, uint32_t SubHeaderIndex, uint32_t SubHeaderOffset, uint32_t SubHeaderSize, uint32_t RawDataIndex, uint32_t RawDataOffset, uint64_t StarpakOffset, uint64_t OptimalStarpakOffset, RpakGameVersion Version, uint32_t AssetVersion)
	: NameHash(NameHash), FileIndex(FileIndex), RpakFileIndex(FileIndex), AssetType(AssetType), SubHeaderIndex(SubHeaderIndex), SubHeaderOffset(SubHeaderOffset), SubHeaderSize(SubHeaderSize), RawDataIndex(RawDataIndex), RawDataOffset(RawDataOffset), StarpakOffset(StarpakOffset), OptimalStarpakOffset(OptimalStarpakOffset), Version(Version), AssetVersion(AssetVersion)
{
}

RpakSegmentBlock::RpakSegmentBlock(uint64_t Offset, uint64_t Size)
	: Offset(Offset), Size(Size)
{
}
