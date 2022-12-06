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

void MdlLib::ExportMDLv53(const string& Asset, const string& Path)
{
	IO::BinaryReader Reader = IO::BinaryReader(IO::File::OpenRead(Asset));
	//IO::Stream* Stream = Reader.GetBaseStream();
	//titanfall2::studiohdr_t hdr = Reader.Read<titanfall2::studiohdr_t>();

	int modelLength = IO::File::OpenRead(Asset)->GetLength();;

	char* mdlBuff = new char[modelLength];
	Reader.Read(mdlBuff, 0, modelLength);
	titanfall2::studiohdr_t* mdl = reinterpret_cast<titanfall2::studiohdr_t*>(mdlBuff);

	//printf("length %i \n id %i \n version %i \n", modelLength, mdl->id, mdl->version);

	// using 32 bit unsigned int for position as things should not exceed it

	// id = IDST or version = 53
	if (mdl->id != 0x54534449 || mdl->version != 0x35)
	{
		//printf("I ate ass");
		return;
	}
	
	auto Model = std::make_unique<Assets::Model>(0, 0);

	// get name from sznameindex incase it exceeds 64 bytes (for whatever reason)
	//Stream->SetPosition(hdr.sznameindex);
	//string modelName = Reader.ReadCString();

	Model->Name = IO::Path::GetFileNameWithoutExtension(mdl->modelName());

	std::vector<titanfall2::mstudiobone_t> bones;

	for (int i = 0; i < mdl->numbones; i++)
	{
		titanfall2::mstudiobone_t* newBone = mdl->bone(i);

		titanfall2::mstudiobone_t bone = *newBone;
		char* boneName = newBone->boneName();

		//printf("I am working on bone %s", boneName);

		Model->Bones.EmplaceBack(boneName, bone.parent, bone.pos, bone.quat);
		bones.push_back(bone);
	}

	if (mdl->numbodyparts)
	{
		string ExportModelPath = IO::Path::Combine(Path, "models");

		// do materials
		std::vector<string> materialNames;

		for (int i = 0; i < mdl->numtextures; i++)
		{
			// does not work but we should use this instead
			titanfall2::mstudiotexture_t* texture = mdl->texture(i);

			materialNames.push_back(IO::Path::GetFileNameWithoutExtension(texture->textureName()));

			/*int position = hdr.textureindex + (sizeof(titanfall2::mstudiotexture_t) * i);

			Stream->SetPosition(position);
			titanfall2::mstudiotexture_t texture = Reader.Read<titanfall2::mstudiotexture_t>();

			Stream->SetPosition(position + texture.sznameindex);
			materialNames.push_back(IO::Path::GetFileNameWithoutExtension(Reader.ReadCString()));*/
		}

		// do mesh related stuff
		//Stream->SetPosition(hdr.vtxindex);
		FileHeader_t* fileHeader = mdl->vtx();

		//Stream->SetPosition(hdr.vvdindex);
		vertexFileHeader_t* vertexFileHeader = mdl->vvd(); // actually vvd file header

		printf("num verts %i \n", vertexFileHeader->numLODVertexes);

		vertexColorFileHeader_t* vertexColorFileHeader = nullptr;

		if (mdl->vvcsize)
		{
			vertexColorFileHeader_t* vertexColorFileHeader = mdl->vvc();
		}

		// hard code to one for now
		//for (int i = 0; i < 1; i++)
		//{
		//	std::vector<mstudiovertex_t*> vvdVerts;

		//	if (vertexFileHeader->numFixups)
		//	{
		//		for (int j = 0; j < vertexFileHeader->numFixups; j++)
		//		{
		//			vertexFileFixup_t* vertexFixup = vertexFileHeader->fixup(j);

		//			if (vertexFixup->lod >= i)
		//			{
		//				for (int k = 0; k < vertexFixup->numVertexes; k++)
		//				{
		//					mstudiovertex_t* vvdVert = vertexFileHeader->vertex(vertexFixup->sourceVertexID + k);

		//					printf("vertex %i \n", vertexFixup->sourceVertexID + k);

		//					vvdVerts.push_back(vvdVert);
		//				}
		//			}
		//		}
		//	}
		//	else
		//	{
		//		// may have issues
		//		for (int j = 0; j < vertexFileHeader->numLODVertexes[i]; j++)
		//		{
		//			mstudiovertex_t* vvdVert = vertexFileHeader->vertex(j);

		//			printf("vertex %i \n", j);

		//			vvdVerts.push_back(vvdVert);
		//		}
		//	}

		//	for (int j = 0; j < mdl->numbodyparts; j++)
		//	{

		//	}
		//}


		/*Stream->SetPosition(hdr.vvdindex + MeshHeader.fixupTableStart);

		List<vertexFileFixup_t> Fixups;
		for (int i = 0; i < MeshHeader.numFixups; i++)
			Fixups.EmplaceBack(Reader.Read<vertexFileFixup_t>());

		List<List<mstudiovertex_t>> VertexBuffers;
		for (int i = 0; i < MeshHeader.numLODs; i++)
		{
			List<mstudiovertex_t>& Buffer = VertexBuffers.Emplace();

			if (MeshHeader.numFixups)
			{
				for (int j = 0; j < MeshHeader.numFixups; j++)
				{
					if (Fixups[j].lod >= i)
					{
						Stream->SetPosition(hdr.vvdindex + MeshHeader.vertexDataStart + Fixups[j].sourceVertexID * sizeof(mstudiovertex_t));

						for (uint32_t v = 0; v < Fixups[j].numVertexes; v++)
							Buffer.EmplaceBack(Reader.Read<mstudiovertex_t>());
					}
				}
			}
			else
			{
				Stream->SetPosition(hdr.vvdindex + MeshHeader.vertexDataStart);

				for (int v = 0; v < MeshHeader.numLODVertexes[i]; v++)
					Buffer.EmplaceBack(Reader.Read<mstudiovertex_t>());
			}
		}

		struct ModelSubmeshList
		{
			titanfall2::mstudiomodel_t Model;
			List<titanfall2::mstudiomesh_t> Meshes;
		};

		List<List<ModelSubmeshList>> PartModelMeshes;

		for (int i = 0; i < hdr.numbodyparts; i++)
		{
			List<ModelSubmeshList>& NewPart = PartModelMeshes.Emplace();
			int Position = hdr.bodypartindex + (i * sizeof(mstudiobodyparts_t));

			Stream->SetPosition(Position);
			mstudiobodyparts_t Part = Reader.Read<mstudiobodyparts_t>();

			for (int p = 0; p < Part.nummodels; p++)
			{
				ModelSubmeshList& NewModel = NewPart.Emplace();
				int ModelPosition = Position + Part.modelindex + (p * sizeof(titanfall2::mstudiomodel_t));

				Stream->SetPosition(ModelPosition);
				NewModel.Model = Reader.Read<titanfall2::mstudiomodel_t>();

				for (int m = 0; m < NewModel.Model.nummeshes; m++)
				{
					int MeshPosition = ModelPosition + NewModel.Model.meshindex + (m * sizeof(titanfall2::mstudiomesh_t));

					Stream->SetPosition(MeshPosition);
					NewModel.Meshes.EmplaceBack(Reader.Read<titanfall2::mstudiomesh_t>());
				}
			}
		}

		Stream->SetPosition(hdr.vtxindex);

		FileHeader_t vtxHeader = Reader.Read<FileHeader_t>();

		for (int i = 0; i < vtxHeader.numBodyParts; i++)
		{
			int Position = (int)hdr.vtxindex + vtxHeader.bodyPartOffset + (i * sizeof(BodyPartHeader_t));

			Stream->SetPosition(Position);
			BodyPartHeader_t Part = Reader.Read<BodyPartHeader_t>();

			for (int m = 0; m < Part.numModels; m++)
			{
				int ModelPosition = Position + Part.modelOffset + (m * sizeof(ModelHeader_t));

				Stream->SetPosition(ModelPosition);
				ModelHeader_t RModel = Reader.Read<ModelHeader_t>();

				int LodPosition = ModelPosition + RModel.lodOffset;

				Stream->SetPosition(LodPosition);
				ModelLODHeader_t Lod = Reader.Read<ModelLODHeader_t>();

				ModelSubmeshList& PartMesh = PartModelMeshes[i][m];
				int VertexOffset = PartMesh.Model.vertexindex / sizeof(mstudiovertex_t);

				for (int s = 0; s < Lod.numMeshes; s++)
				{
					int SubmeshPosition = LodPosition + Lod.meshOffset + (s * sizeof(MeshHeader_t));

					Stream->SetPosition(SubmeshPosition);
					MeshHeader_t Submesh = Reader.Read<MeshHeader_t>();

					// there's a good chance that this isn't a good way of doing it, however:
					// it works well enough for now so it will do.
					// this should probably be checked later and likely changed
					// - rex
					Assets::Mesh m;

					// todo: check this
					Model->Meshes.EmplaceBack(m);

					for (int g = 0; g < Submesh.numStripGroups; g++)
					{
						int StripGroupPosition = SubmeshPosition + Submesh.stripGroupHeaderOffset + (g * sizeof(StripGroupHeader_t));

						Stream->SetPosition(StripGroupPosition);
						StripGroupHeader_t StripGroup = Reader.Read<StripGroupHeader_t>();

						Stream->SetPosition(StripGroupPosition + StripGroup.vertOffset);

						for (int v = 0; v < StripGroup.numVerts; v++)
						{
							Vertex_t Vtx = Reader.Read<Vertex_t>();
							mstudiovertex_t& Vertex = VertexBuffers[0][(int)Vtx.origMeshVertID + VertexOffset];

							//VertexColor_t vertexColor;

							// todo: check this
							//Assets::Vertex NewVertex = Model->Meshes[s].Vertices.Emplace(Vertex.m_vecPosition, Vertex.m_vecNormal, Assets::VertexColor(), Vertex.m_vecTexCoord);

							//for (uint8_t w = 0; w < Vertex.m_BoneWeights.numbones; w++)
							//{
							//	NewVertex.SetWeight({ Vertex.m_BoneWeights.bone[w], Vertex.m_BoneWeights.weight[w] }, w);
							//}
						}

						Stream->SetPosition(StripGroupPosition + StripGroup.indexOffset);

						for (uint32_t v = 0; v < (StripGroup.numIndices / 3); v++)
						{
							uint32_t i1 = Reader.Read<uint16_t>();
							uint32_t i2 = Reader.Read<uint16_t>();
							uint32_t i3 = Reader.Read<uint16_t>();

							// todo: check this
							Model->Meshes[s].Faces.EmplaceBack(i1, i2, i3);
						}
					}

					VertexOffset += PartMesh.Meshes[s].vertexloddata.numLODVertexes[0];

					// todo: check this
					Model->Meshes[s].MaterialIndices.EmplaceBack(Model->AddMaterial(Materials[PartMesh.Meshes[s].material], ""));
				}
			}
		}*/

		string ModelDirectory = IO::Path::Combine(ExportModelPath, Model->Name);
		IO::Directory::CreateDirectory(ModelDirectory);

		this->ModelExporter->ExportModel(*Model.get(), IO::Path::Combine(ModelDirectory, Model->Name + (const char*)ModelExporter->ModelExtension()));

		printf("model has been exported");
	}

	/*if (hdr.numlocalanim)
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
	}*/
}

void MdlLib::ParseRAnimBoneTranslationTrack(const RAnimBoneHeader& BoneFlags, const titanfall2::mstudiobone_t& Bone, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	printf("***** ParseRAnimBoneTranslationTrack is STUBBED.\n");


	uint16_t* TranslationDataPtr = *BoneTrackData;
	uint8_t* DataPointer = (uint8_t*)TranslationDataPtr;

	uint8_t* TranslationDataX = &DataPointer[BoneFlags.TranslationX - 0x10];

	float Result[3]{ Bone.pos.X, Bone.pos.Y, Bone.pos.Z };

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

void MdlLib::ParseRAnimBoneRotationTrack(const RAnimBoneHeader& BoneFlags, const titanfall2::mstudiobone_t& Bone, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex)
{
	printf("ParseRAnimBoneRotationTrack is STUBBED.\n");

	uint16_t* RotationDataPtr = *BoneTrackData;
	uint8_t* DataPointer = (uint8_t*)RotationDataPtr;

	uint8_t* TranslationDataX = &DataPointer[(BoneFlags.RotationInfo.OffsetX - 0x18)];
	uint8_t* TranslationDataY = &DataPointer[(BoneFlags.RotationInfo.OffsetY - 0x18)];
	uint8_t* TranslationDataZ = &DataPointer[(BoneFlags.RotationInfo.OffsetZ - 0x18)];	

	Vector3 BoneRotation = Bone.quat.ToEulerAngles();

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
