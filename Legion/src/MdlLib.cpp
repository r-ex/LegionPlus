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
#include "CastAsset.h"

#include "rtech.h"

MdlLib::MdlLib()
{
}

void MdlLib::InitializeModelExporter(ModelExportFormat_t Format)
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
	default:
		ModelExporter = std::make_unique<Assets::Exporters::SEAsset>();
		break;
	}
}

void MdlLib::InitializeAnimExporter(AnimExportFormat_t Format)
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
}

void MdlLib::ExportRMdl(const string& Asset, const string& Path)
{
	IO::BinaryReader Reader = IO::BinaryReader(IO::File::OpenRead(Asset));
	IO::Stream* Stream = Reader.GetBaseStream();
	r2studiohdr_t hdr = Reader.Read<r2studiohdr_t>();

	if (hdr.id != 0x54534449 || hdr.version != 0x35)
		return;

	auto Model = std::make_unique<Assets::Model>(0, 0);
	Model->Name = IO::Path::GetFileNameWithoutExtension(hdr.name);

	List<r2mstudiobone_t> BoneBuffer;

	for (int i = 0; i < hdr.numbones; i++)
	{
		uint64_t Position = hdr.boneindex + (i * sizeof(r2mstudiobone_t));

		Stream->SetPosition(Position);
		r2mstudiobone_t bone = Reader.Read<r2mstudiobone_t>();
		Stream->SetPosition(Position + bone.NameOffset);

		string TagName = Reader.ReadCString();

		Model->Bones.EmplaceBack(TagName, bone.ParentIndex, bone.Position, bone.Rotation);
		BoneBuffer.EmplaceBack(bone);
	}

	if (hdr.numbodyparts)
	{
		string ExportModelPath = IO::Path::Combine(Path, "models");

		List<string> Materials;
		for (int i = 0; i < hdr.numtextures; i++)
		{
			uint64_t Position = (uint64_t)hdr.textureindex + (i * 0x2C);

			Stream->SetPosition(Position);
			uint32_t nameOffset = Reader.Read<uint32_t>();
			Stream->SetPosition(Position + nameOffset);

			Materials.EmplaceBack(IO::Path::GetFileNameWithoutExtension(Reader.ReadCString()));
		}

		Stream->SetPosition(hdr.vvdindex);

		vertexFileHeader_t MeshHeader = Reader.Read<vertexFileHeader_t>(); // actually vvd file header

		Stream->SetPosition(hdr.vvdindex + MeshHeader.fixupTableStart);

		List<RMdlFixup> Fixups;
		for (uint32_t i = 0; i < MeshHeader.numFixups; i++)
			Fixups.EmplaceBack(Reader.Read<RMdlFixup>());

		List<List<RMdlVertex>> VertexBuffers;
		for (uint32_t i = 0; i < MeshHeader.numLODs; i++)
		{
			List<RMdlVertex>& Buffer = VertexBuffers.Emplace();

			if (MeshHeader.numFixups)
			{
				for (uint32_t j = 0; j < MeshHeader.numFixups; j++)
				{
					if (Fixups[j].LodIndex >= i)
					{
						Stream->SetPosition(hdr.vvdindex + MeshHeader.vertexDataStart + Fixups[j].VertexIndex * sizeof(RMdlVertex));

						for (uint32_t v = 0; v < Fixups[j].VertexCount; v++)
							Buffer.EmplaceBack(Reader.Read<RMdlVertex>());
					}
				}
			}
			else
			{
				Stream->SetPosition(hdr.vvdindex + MeshHeader.vertexDataStart);

				for (uint32_t v = 0; v < MeshHeader.numLODVertexes[i]; v++)
					Buffer.EmplaceBack(Reader.Read<RMdlVertex>());
			}
		}

		struct ModelSubmeshList
		{
			RMdlTitanfallModel Model;
			List<RMdlTitanfallLodSubmesh> Meshes;
		};

		List<List<ModelSubmeshList>> PartModelMeshes;

		for (uint32_t i = 0; i < hdr.numbodyparts; i++)
		{
			List<ModelSubmeshList>& NewPart = PartModelMeshes.Emplace();
			uint64_t Position = hdr.bodypartindex + (i * sizeof(mstudiobodyparts_t));

			Stream->SetPosition(Position);
			mstudiobodyparts_t Part = Reader.Read<mstudiobodyparts_t>();

			for (uint32_t p = 0; p < Part.nummodels; p++)
			{
				ModelSubmeshList& NewModel = NewPart.Emplace();
				uint64_t ModelPosition = Position + Part.modelindex + (p * sizeof(RMdlTitanfallModel));

				Stream->SetPosition(ModelPosition);
				NewModel.Model = Reader.Read<RMdlTitanfallModel>();

				for (uint32_t m = 0; m < NewModel.Model.nummeshes; m++)
				{
					uint64_t MeshPosition = ModelPosition + NewModel.Model.meshindex + (m * sizeof(RMdlTitanfallLodSubmesh));

					Stream->SetPosition(MeshPosition);
					NewModel.Meshes.EmplaceBack(Reader.Read<RMdlTitanfallLodSubmesh>());
				}
			}
		}

		Stream->SetPosition(hdr.vtxindex);

		RMdlMeshStreamHeader LodHeader = Reader.Read<RMdlMeshStreamHeader>();

		for (uint32_t i = 0; i < LodHeader.NumBodyParts; i++)
		{
			uint64_t Position = (uint64_t)hdr.vtxindex + LodHeader.BodyPartOffset + (i * sizeof(mstudiobodyparts_short_t));

			Stream->SetPosition(Position);
			mstudiobodyparts_short_t Part = Reader.Read<mstudiobodyparts_short_t>();

			for (uint32_t m = 0; m < Part.nummodels; m++)
			{
				uint64_t ModelPosition = Position + Part.modelindex + (m * sizeof(RMdlModel));

				Stream->SetPosition(ModelPosition);
				RMdlModel RModel = Reader.Read<RMdlModel>();

				uint64_t LodPosition = ModelPosition + RModel.LodOffset;

				Stream->SetPosition(LodPosition);
				RMdlLod Lod = Reader.Read<RMdlLod>();

				ModelSubmeshList& PartMesh = PartModelMeshes[i][m];
				uint32_t VertexOffset = PartMesh.Model.vertexindex / sizeof(RMdlVertex);

				for (uint32_t s = 0; s < Lod.SubmeshCount; s++)
				{
					uint64_t SubmeshPosition = LodPosition + Lod.SubmeshOffset + (s * sizeof(RMdlSubmesh));

					Stream->SetPosition(SubmeshPosition);
					RMdlSubmesh Submesh = Reader.Read<RMdlSubmesh>();

					// there's a good chance that this isn't a good way of doing it, however:
					// it works well enough for now so it will do.
					// this should probably be checked later and likely changed
					// - rex
					Assets::Mesh m;

					// todo: check this
					Model->Meshes.EmplaceBack(m);

					for (uint32_t g = 0; g < Submesh.NumStripGroups; g++)
					{
						uint64_t StripGroupPosition = SubmeshPosition + Submesh.StripGroupOffset + (g * sizeof(RMdlStripGroup));

						Stream->SetPosition(StripGroupPosition);
						RMdlStripGroup StripGroup = Reader.Read<RMdlStripGroup>();

						Stream->SetPosition(StripGroupPosition + StripGroup.VertexOffset);

						for (uint32_t v = 0; v < StripGroup.VertexCount; v++)
						{
							RMdlStripVert Vtx = Reader.Read<RMdlStripVert>();
							RMdlVertex& Vertex = VertexBuffers[0][(uint64_t)Vtx.VertexIndex + VertexOffset];


							// todo: check this
							Assets::Vertex NewVertex = Model->Meshes[s].Vertices.Emplace(Vertex.Position, Vertex.Normal, Assets::VertexColor(), Vertex.UVs);

							for (uint8_t w = 0; w < Vertex.NumWeights; w++)
							{
								NewVertex.SetWeight({ Vertex.WeightIds[w], Vertex.SimpleWeights[w] }, w);
							}
						}

						Stream->SetPosition(StripGroupPosition + StripGroup.IndexOffset);

						for (uint32_t v = 0; v < (StripGroup.IndexCount / 3); v++)
						{
							uint32_t i1 = Reader.Read<uint16_t>();
							uint32_t i2 = Reader.Read<uint16_t>();
							uint32_t i3 = Reader.Read<uint16_t>();

							// todo: check this
							Model->Meshes[s].Faces.EmplaceBack(i1, i2, i3);
						}
					}

					VertexOffset += PartMesh.Meshes[s].LodVertCounts[0];

					// todo: check this
					Model->Meshes[s].MaterialIndices.EmplaceBack(Model->AddMaterial(Materials[PartMesh.Meshes[s].Index], ""));
				}
			}
		}

		string ModelDirectory = IO::Path::Combine(ExportModelPath, Model->Name);
		IO::Directory::CreateDirectory(ModelDirectory);

		this->ModelExporter->ExportModel(*Model.get(), IO::Path::Combine(ModelDirectory, Model->Name + "_LOD0" + (const char*)ModelExporter->ModelExtension()));
	}

	if (hdr.numlocalanim)
	{
		string ExportAnimPath = IO::Path::Combine(Path, "animations");
		string ExportBasePath = IO::Path::Combine(ExportAnimPath, IO::Path::GetFileNameWithoutExtension(hdr.name));

		IO::Directory::CreateDirectory(ExportBasePath);

		for (uint32_t i = 0; i < hdr.numlocalanim; i++)
		{
			uint64_t Position = hdr.localanimindex + (i * sizeof(mstudioanimdescv53_t));

			Stream->SetPosition(Position);
			mstudioanimdescv53_t ASeqHeader = Reader.Read<mstudioanimdescv53_t>();

			auto Anim = std::make_unique<Assets::Animation>(Model->Bones.Count(), ASeqHeader.Framerate);
			Assets::AnimationCurveMode AnimCurveType = Assets::AnimationCurveMode::Absolute;

			for (auto& Bone : Model->Bones)
			{
				Anim->Bones.EmplaceBack(Bone.Name(), Bone.Parent(), Bone.LocalPosition(), Bone.LocalRotation());

				auto& CurveNodes = Anim->GetNodeCurves(Bone.Name());

				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::RotateQuaternion, AnimCurveType);
				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::TranslateX, AnimCurveType);
				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::TranslateY, AnimCurveType);
				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::TranslateZ, AnimCurveType);
				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::ScaleX, AnimCurveType);
				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::ScaleY, AnimCurveType);
				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::ScaleZ, AnimCurveType);
			}

			Anim->Looping = (bool)(ASeqHeader.Flags & 0x20000);

			Stream->SetPosition(Position + ASeqHeader.NameOffset);
			string AnimName = Reader.ReadCString();

			List<uint64_t> AnimChunkOffsets;

			if (ASeqHeader.FrameSplitCount)
			{
				Stream->SetPosition(Position + ASeqHeader.OffsetToChunkOffsetsTable);

				while (true)
				{
					uint32_t Result = Reader.Read<uint32_t>();

					if (Result == 0)
						break;

					AnimChunkOffsets.Add(Position + Result);
				}
			}
			else
				AnimChunkOffsets.Add(ASeqHeader.FirstChunkOffset + Position);

			for (uint32_t Frame = 0; Frame < ASeqHeader.FrameCount; Frame++)
			{
				uint32_t ChunkTableIndex = 0;
				uint32_t ChunkFrame = Frame;
				uint32_t FrameCountOneLess = 0;

				if (ASeqHeader.FrameSplitCount)
				{
					uint32_t FrameCount = ASeqHeader.FrameCount;
					if (FrameCount <= ASeqHeader.FrameSplitCount || (FrameCountOneLess = FrameCount - 1, ChunkFrame != FrameCountOneLess))
					{
						ChunkTableIndex = ChunkFrame / ASeqHeader.FrameSplitCount;
						ChunkFrame -= ASeqHeader.FrameSplitCount * (ChunkFrame / ASeqHeader.FrameSplitCount);
					}
					else
					{
						ChunkFrame = 0;
						ChunkTableIndex = FrameCountOneLess / ASeqHeader.FrameSplitCount + 1;
					}
				}

				Stream->SetPosition(AnimChunkOffsets[ChunkTableIndex]);

				while (true)
				{
					uint64_t TrackDataRead = 0;

					RAnimBoneHeader BoneTrackHeader = Reader.Read<RAnimBoneHeader>();
					uint64_t BoneDataSize = BoneTrackHeader.DataSize - sizeof(RAnimBoneHeader);

					if (BoneTrackHeader.BoneFlags.bStaticTranslation)
					{
						List<Assets::Curve>& Curves = Anim->GetNodeCurves(Anim->Bones[BoneTrackHeader.BoneIndex].Name());

						Curves[1].Keyframes.EmplaceBack(Frame, Math::Half(BoneTrackHeader.TranslationX).ToFloat());
						Curves[2].Keyframes.EmplaceBack(Frame, Math::Half(BoneTrackHeader.TranslationY).ToFloat());
						Curves[3].Keyframes.EmplaceBack(Frame, Math::Half(BoneTrackHeader.TranslationZ).ToFloat());
					}

					if (BoneTrackHeader.BoneFlags.bStaticRotation)
					{
						struct Quat64
						{
							uint64_t X : 21;
							uint64_t Y : 21;
							uint64_t Z : 21;
							uint64_t WNeg : 1;
						};

						Quat64 PackedQuat = *(Quat64*)&BoneTrackHeader.RotationInfo.PackedRotation;

						Math::Quaternion Quat;

						Quat.X = ((int)PackedQuat.X - 1048576) * (1 / 1048576.5f);
						Quat.Y = ((int)PackedQuat.Y - 1048576) * (1 / 1048576.5f);
						Quat.Z = ((int)PackedQuat.Z - 1048576) * (1 / 1048576.5f);
						Quat.W = std::sqrt(1 - Quat.X * Quat.X - Quat.Y * Quat.Y - Quat.Z * Quat.Z);

						if (PackedQuat.WNeg)
							Quat.W = -Quat.W;

						Anim->GetNodeCurves(Anim->Bones[BoneTrackHeader.BoneIndex].Name())[0].Keyframes.Emplace(Frame, Quat);
					}

					if (BoneTrackHeader.BoneFlags.bStaticScale)
					{
						List<Assets::Curve>& Curves = Anim->GetNodeCurves(Anim->Bones[BoneTrackHeader.BoneIndex].Name());

						Curves[4].Keyframes.EmplaceBack(Frame, Math::Half(BoneTrackHeader.ScaleX).ToFloat());
						Curves[5].Keyframes.EmplaceBack(Frame, Math::Half(BoneTrackHeader.ScaleY).ToFloat());
						Curves[6].Keyframes.EmplaceBack(Frame, Math::Half(BoneTrackHeader.ScaleZ).ToFloat());
					}

					if (BoneTrackHeader.DataSize == 0)
					{
						break;
					}

					auto BoneTrackData = Reader.Read(BoneDataSize, TrackDataRead);

					uint16_t* BoneTrackDataPtr = (uint16_t*)BoneTrackData.get();

					if (TrackDataRead > 0)
					{
						if (!BoneTrackHeader.BoneFlags.bStaticTranslation)
						{
							ParseRAnimBoneTranslationTrack(BoneTrackHeader, BoneBuffer[BoneTrackHeader.BoneIndex], &BoneTrackDataPtr, Anim, BoneTrackHeader.BoneIndex, ChunkFrame, Frame);
						}
						
						if (!BoneTrackHeader.BoneFlags.bStaticRotation)
						{
							ParseRAnimBoneRotationTrack(BoneTrackHeader, BoneBuffer[BoneTrackHeader.BoneIndex], &BoneTrackDataPtr, Anim, BoneTrackHeader.BoneIndex, ChunkFrame, Frame);
						}

						if (!BoneTrackHeader.BoneFlags.bStaticScale)
						{
						}
					}
				}
			}

			Anim->RemoveEmptyNodes();

			this->AnimExporter->ExportAnimation(*Anim.get(), IO::Path::Combine(ExportBasePath, AnimName + (const char*)this->AnimExporter->AnimationExtension()));
		}
	}
}

void MdlLib::ParseRAnimBoneTranslationTrack(const RAnimBoneHeader& BoneFlags, const r2mstudiobone_t& Bone, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	printf("***** ParseRAnimBoneTranslationTrack is STUBBED.\n");


	uint16_t* TranslationDataPtr = *BoneTrackData;
	uint8_t* DataPointer = (uint8_t*)TranslationDataPtr;

	uint8_t* TranslationDataX = &DataPointer[BoneFlags.TranslationX - 0x10];

	float Result[3]{ Bone.Position.X, Bone.Position.Y, Bone.Position.Z };

	float v37 = 0, v34 = 0;

	if (BoneFlags.TranslationX)
	{
		//Titanfall2TrackDecompress(Frame, TranslationDataX, BoneFlags.TranslationScale, &v37, &v34);
		Result[0] += v37;
	}

	if (BoneFlags.TranslationY)
	{
		//Titanfall2TrackDecompress(Frame, TranslationDataY, BoneFlags.TranslationScale, &v37, &v34);
		Result[1] += v37;
	}

	if (BoneFlags.TranslationZ)
	{
		//Titanfall2TrackDecompress(Frame, TranslationDataZ, BoneFlags.TranslationScale, &v37, &v34);
		Result[2] += v37;
	}

	auto& Curves = Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name());

	Curves[1].Keyframes.EmplaceBack(FrameIndex, Result[0]);
	Curves[2].Keyframes.EmplaceBack(FrameIndex, Result[1]);
	Curves[3].Keyframes.EmplaceBack(FrameIndex, Result[2]);
}

void MdlLib::ParseRAnimBoneRotationTrack(const RAnimBoneHeader& BoneFlags, const r2mstudiobone_t& Bone, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	printf("ParseRAnimBoneRotationTrack is STUBBED.\n");

	uint16_t* RotationDataPtr = *BoneTrackData;
	uint8_t* DataPointer = (uint8_t*)RotationDataPtr;

	uint8_t* TranslationDataX = &DataPointer[(BoneFlags.RotationInfo.OffsetX - 0x18)];
	uint8_t* TranslationDataY = &DataPointer[(BoneFlags.RotationInfo.OffsetY - 0x18)];
	uint8_t* TranslationDataZ = &DataPointer[(BoneFlags.RotationInfo.OffsetZ - 0x18)];	

	Vector3 BoneRotation = Bone.Rotation.ToEulerAngles();

	float EulerResult[4]{ Math::MathHelper::DegreesToRadians(BoneRotation.X),Math::MathHelper::DegreesToRadians(BoneRotation.Y),Math::MathHelper::DegreesToRadians(BoneRotation.Z),0 };

	float v37 = 0, v34 = 0;
	
	if (BoneFlags.RotationInfo.OffsetX)
	{
		//Titanfall2TrackDecompress(Frame, TranslationDataX, Bone.RotationScale[0], &v37, &v34);
		EulerResult[0] += v37;
	}

	if (BoneFlags.RotationInfo.OffsetY)
	{
		//Titanfall2TrackDecompress(Frame, TranslationDataY, Bone.RotationScale[1], &v37, &v34);
		EulerResult[1] += v37;
	}

	if (BoneFlags.RotationInfo.OffsetZ)
	{
		//Titanfall2TrackDecompress(Frame, TranslationDataZ, Bone.RotationScale[2], &v37, &v34);
		EulerResult[2] += v37;
	}

	Math::Quaternion Result;
	RTech::DecompressConvertRotation((const __m128i*) & EulerResult[0], (float*)&Result);

	Anim->GetNodeCurves(Anim->Bones[BoneIndex].Name())[0].Keyframes.Emplace(FrameIndex, Result);
}
