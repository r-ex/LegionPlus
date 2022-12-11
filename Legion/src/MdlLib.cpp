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

	// id = IDST or version = 53
	if (mdl->id != 0x54534449 || mdl->version != 0x35)
		return;
	
	auto Model = std::make_unique<Assets::Model>(0, 0);

	// get name from sznameindex incase it exceeds 64 bytes (for whatever reason)
	Model->Name = IO::Path::GetFileNameWithoutExtension(mdl->mdlName());

	std::vector<titanfall2::mstudiobone_t> bones;

	for (int i = 0; i < mdl->numbones; i++)
	{
		titanfall2::mstudiobone_t* newBone = mdl->bone(i);

		titanfall2::mstudiobone_t bone = *newBone;

		//printf("I am working on bone %s", newBone->boneName());

		Model->Bones.EmplaceBack(newBone->boneName(), bone.parent, bone.pos, bone.quat);
		bones.push_back(bone);
	}

	if (mdl->numbodyparts)
	{
		string ExportModelPath = IO::Path::Combine(Path, "models");

		// we should run a checksum check here just in case someone tries to do funny business
		FileHeader_t* fileHeader = mdl->vtx();
		vertexFileHeader_t* vertexFileHeader = mdl->vvd(); // actually vvd file header
		vertexColorFileHeader_t* vertexColorFileHeader = nullptr;

		// if vvc exists
		if (mdl->vvcsize)
			vertexColorFileHeader = mdl->vvc();

		// hard code to one for now
		for (int i = 0; i < fileHeader->numLODs; i++)
		{
			std::vector<mstudiovertex_t*> vvdVerts;
			std::vector<VertexColor_t*> vvcColors;
			std::vector<Vector2*> vvcUV2s;

			int vertexOffset = 0;

			// rebuild vertex vector per lod just incase it has fixups
			if (vertexFileHeader->numFixups)
			{
				for (int j = 0; j < vertexFileHeader->numFixups; j++)
				{
					vertexFileFixup_t* vertexFixup = vertexFileHeader->fixup(j);

					//printf("num verts %i vert offset %i lod %i \n", vertexFixup->numVertexes, vertexFixup->sourceVertexID, vertexFixup->lod);

					if (vertexFixup->lod >= i)
					{
						for (int k = 0; k < vertexFixup->numVertexes; k++)
						{
							mstudiovertex_t* vvdVert = vertexFileHeader->vertex(vertexFixup->sourceVertexID + k);

							//printf("fixup %i vertex %i \n", j, vertexFixup->sourceVertexID + k);

							vvdVerts.push_back(vvdVert);

							// vvc
							if (vertexColorFileHeader)
							{
								// doesn't matter which we pack as long as it has vvc, we will only used what's needed later
								VertexColor_t* vvcColor = vertexColorFileHeader->color(vertexFixup->sourceVertexID + k);
								Vector2* vvcUV2 = vertexColorFileHeader->uv2(vertexFixup->sourceVertexID + k);

								vvcColors.push_back(vvcColor);
								vvcUV2s.push_back(vvcUV2);

								//printf("did color and uv \n");
							}
						}
					}
				}
			}
			else
			{
				// using per lod vertex count may have issues (tbd)
				for (int j = 0; j < vertexFileHeader->numLODVertexes[i]; j++)
				{
					mstudiovertex_t* vvdVert = vertexFileHeader->vertex(j);

					//printf("vertex %i \n", j);

					vvdVerts.push_back(vvdVert);

					// vvc
					if (vertexColorFileHeader)
					{
						// doesn't matter which we pack as long as it has vvc, we will only used what's needed later
						VertexColor_t* vvcColor = vertexColorFileHeader->color(j);
						Vector2* vvcUV2 = vertexColorFileHeader->uv2(j);

						vvcColors.push_back(vvcColor);
						vvcUV2s.push_back(vvcUV2);

						//printf("did color and uv \n");
					}
				}
			}

			//printf("num verts in vector %i \n", vvdVerts.size());

			// some basic error checks to avoid crashes
			if (vvdVerts.empty() || (vvcColors.empty() && (mdl->flags & STUDIOHDR_FLAGS_USES_VERTEX_COLOR)) || (vvcUV2s.empty() && (mdl->flags & STUDIOHDR_FLAGS_USES_UV2)))
				return;

			// eventually we should do checks on all of these to confirm they match
			for (int j = 0; j < mdl->numbodyparts; j++)
			{
				titanfall2::mstudiobodyparts_t* mdlBodypart = mdl->mdlBodypart(j);
				BodyPartHeader_t* vtxBodypart = fileHeader->vtxBodypart(j);

				//printf("models %i \n", mdlBodypart->nummodels);

				for (int k = 0; k < mdlBodypart->nummodels; k++)
				{
					titanfall2::mstudiomodel_t* mdlModel = mdlBodypart->mdlModel(k);
					ModelHeader_t* vtxModel = vtxBodypart->vtxModel(k);

					//printf("lods %i offset %i \n", vtxModel->numLODs, vtxModel->lodOffset);

					// lod
					ModelLODHeader_t* vtxLod = vtxModel->vtxLOD(i);

					//printf("num meshes %i mesh offset %i \n", vtxLod->numMeshes, vtxLod->meshOffset);

					for (int l = 0; l < mdlModel->nummeshes; l++)
					{
						titanfall2::mstudiomesh_t* mdlMesh = mdlModel->mdlMesh(l);
						MeshHeader_t* vtxMesh = vtxLod->vtxMesh(l);

						// so we don't make empty meshes
						if (mdlMesh->vertexloddata.numLODVertexes[i] > 0)
						{
							// maxinfluences is max weights, set to 3 as that is the max it should have (for v53)
							Assets::Mesh& exportMesh = Model->Meshes.Emplace(3, (mdl->flags & STUDIOHDR_FLAGS_USES_UV2) ? 2 : 1); // set uv count, two uvs used rarely in v53

							// set "texture" aka material
							titanfall2::mstudiotexture_t* meshMaterial = mdl->texture(mdlMesh->material);
							exportMesh.MaterialIndices.EmplaceBack(Model->AddMaterial(IO::Path::GetFileNameWithoutExtension(meshMaterial->textureName()), ""));

							//printf("strip grp offset %i num strips %i \n", vtxMesh->stripGroupHeaderOffset, vtxMesh->numStripGroups);

							for (int m = 0; m < vtxMesh->numStripGroups; m++)
							{
								StripGroupHeader_t* vtxStripGroup = vtxMesh->vtxStripGrp(m);

								//printf("num verts %i num indices %i \n", vtxStripGroup->numVerts, vtxStripGroup->numIndices);

								for (int n = 0; n < vtxStripGroup->numVerts; n++)
								{
									Vertex_t* vtxVert = vtxStripGroup->vtxVert(n);
									mstudiovertex_t* vvdVert = vvdVerts.at(vertexOffset + vtxVert->origMeshVertID);

									//printf("vertex %i \n", vertexOffset + vtxVert->origMeshVertID);

									Assets::VertexColor vertexColor;

									if (mdl->flags & STUDIOHDR_FLAGS_USES_VERTEX_COLOR)
									{
										VertexColor_t* vvcColor = vvcColors.at(vertexOffset + vtxVert->origMeshVertID);
										vertexColor = Assets::VertexColor::VertexColor(vvcColor->r, vvcColor->g, vvcColor->b, vvcColor->a);
									}

									Assets::Vertex newVert = exportMesh.Vertices.Emplace(vvdVert->m_vecPosition, vvdVert->m_vecNormal, vertexColor, vvdVert->m_vecTexCoord);

									// untested
									if (mdl->flags & STUDIOHDR_FLAGS_USES_UV2)
									{
										Vector2* vvcUV2 = vvcUV2s.at(vertexOffset + vtxVert->origMeshVertID);
										newVert.SetUVLayer(*vvcUV2, 1);
									}

									for (char w = 0; w < vvdVert->m_BoneWeights.numbones; w++)
									{
										newVert.SetWeight({ vvdVert->m_BoneWeights.bone[w], vvdVert->m_BoneWeights.weight[w] }, w); // how does this idx work, causing crashes
									}
								}

								for (int n = 0; n < vtxStripGroup->numIndices; n += 3)
								{
									//printf("indice %i \n", n);
									unsigned short i1 = *vtxStripGroup->vtxIndice(n);
									unsigned short i2 = *vtxStripGroup->vtxIndice(n + 1);
									unsigned short i3 = *vtxStripGroup->vtxIndice(n + 2);

									//printf("idx %i %i %i \n", i1, i2, i3);

									exportMesh.Faces.EmplaceBack(i1, i2, i3);
								}
							}

							vertexOffset += mdlMesh->vertexloddata.numLODVertexes[i];
						}
					}
				}
			}
		}

		string ModelDirectory = IO::Path::Combine(ExportModelPath, Model->Name);
		IO::Directory::CreateDirectory(ModelDirectory);

		this->ModelExporter->ExportModel(*Model.get(), IO::Path::Combine(ModelDirectory, Model->Name + (const char*)ModelExporter->ModelExtension()));

		// for testing
		g_Logger.Info("Exported: " + Model->Name + ".mdl\n");
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

	// should be ok?
	delete[] mdlBuff;
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
