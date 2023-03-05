#include "pch.h"
#include "MdlLib.h"
#include "File.h"
#include "Directory.h"
#include "Path.h"
#include "Half.h"
#include <math.h>

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

// extract vertex data from normal source style models: vtx, vvd, vvc, vvw.
void MdlLib::ExtractValveVertexData(titanfall2::studiohdr_t* pHdr, vtx::FileHeader_t* pVtx, vvd::vertexFileHeader_t* pVVD, vvc::vertexColorFileHeader_t* pVVC, vvw::vertexBoneWeightsExtraFileHeader_t* pVVW, 
									std::unique_ptr<Assets::Model> &ExportModel, const string& Path)
{
	string ExportModelPath = IO::Path::Combine(Path, "models");

	for (int i = 0; i < pVtx->numLODs; i++)
	{
		std::vector<vvd::mstudiovertex_t*> vvdVerts;
		std::vector<vvc::VertexColor_t*> vvcColors;
		std::vector<Vector2*> vvcUV2s;

		int vertexOffset = 0;

		// rebuild vertex vector per lod just incase it has fixups
		if (pVVD->numFixups)
		{
			for (int j = 0; j < pVVD->numFixups; j++)
			{
				vvd::vertexFileFixup_t* vertexFixup = pVVD->fixup(j);

				if (vertexFixup->lod >= i)
				{
					for (int k = 0; k < vertexFixup->numVertexes; k++)
					{
						vvd::mstudiovertex_t* vvdVert = pVVD->vertex(vertexFixup->sourceVertexID + k);

						vvdVerts.push_back(vvdVert);

						// vvc
						if (pVVC)
						{
							// doesn't matter which we pack as long as it has vvc, we will only used what's needed later
							vvc::VertexColor_t* vvcColor = pVVC->color(vertexFixup->sourceVertexID + k);
							Vector2* vvcUV2 = pVVC->uv2(vertexFixup->sourceVertexID + k);

							vvcColors.push_back(vvcColor);
							vvcUV2s.push_back(vvcUV2);
						}
					}
				}
			}
		}
		else
		{
			// using per lod vertex count may have issues (tbd)
			for (int j = 0; j < pVVD->numLODVertexes[i]; j++)
			{
				vvd::mstudiovertex_t* vvdVert = pVVD->vertex(j);

				vvdVerts.push_back(vvdVert);

				// vvc
				if (pVVC)
				{
					// doesn't matter which we pack as long as it has vvc, we will only used what's needed later
					vvc::VertexColor_t* vvcColor = pVVC->color(j);
					Vector2* vvcUV2 = pVVC->uv2(j);

					vvcColors.push_back(vvcColor);
					vvcUV2s.push_back(vvcUV2);
				}
			}
		}

		// some basic error checks to avoid crashes
		if (vvdVerts.empty() || (vvcColors.empty() && (pHdr->flags & STUDIOHDR_FLAGS_USES_VERTEX_COLOR)) || (vvcUV2s.empty() && (pHdr->flags & STUDIOHDR_FLAGS_USES_UV2)))
			return;

		// eventually we should do checks on all of these to confirm they match
		for (int j = 0; j < pHdr->numbodyparts; j++)
		{
			titanfall2::mstudiobodyparts_t* mdlBodypart = pHdr->mdlBodypart(j);
			vtx::BodyPartHeader_t* vtxBodypart = pVtx->vtxBodypart(j);

			for (int k = 0; k < mdlBodypart->nummodels; k++)
			{
				titanfall2::mstudiomodel_t* mdlModel = mdlBodypart->mdlModel(k);
				vtx::ModelHeader_t* vtxModel = vtxBodypart->vtxModel(k);

				// lod
				vtx::ModelLODHeader_t* vtxLod = vtxModel->vtxLOD(i);

				for (int l = 0; l < mdlModel->nummeshes; l++)
				{
					titanfall2::mstudiomesh_t* mdlMesh = mdlModel->mdlMesh(l);
					vtx::MeshHeader_t* vtxMesh = vtxLod->vtxMesh(l);

					// so we don't make empty meshes
					if (mdlMesh->vertexloddata.numLODVertexes[i] > 0)
					{
						// maxinfluences is max weights, check if has extra weights, if yes 16 weights max, if no 3 weights max
						// also sets uv count depending on flags
						Assets::Mesh& exportMesh = ExportModel->Meshes.Emplace(((pHdr->flags & STUDIOHDR_FLAGS_USES_EXTRA_BONE_WEIGHTS) && (pHdr->version == 54)) ? 16 : 3, (pHdr->flags & STUDIOHDR_FLAGS_USES_UV2) ? 2 : 1); // set uv count, two uvs used rarely in v53

						// set "texture" aka material
						titanfall2::mstudiotexture_t* meshMaterial = pHdr->texture(mdlMesh->material);
						exportMesh.MaterialIndices.EmplaceBack(ExportModel->AddMaterial(IO::Path::GetFileNameWithoutExtension(meshMaterial->textureName()), ""));

						for (int m = 0; m < vtxMesh->numStripGroups; m++)
						{
							vtx::StripGroupHeader_t* vtxStripGroup = vtxMesh->vtxStripGrp(m);

							for (int n = 0; n < vtxStripGroup->numVerts; n++)
							{
								vtx::Vertex_t* vtxVert = vtxStripGroup->vtxVert(n);
								vvd::mstudiovertex_t* vvdVert = vvdVerts.at(vertexOffset + vtxVert->origMeshVertID);

								Assets::VertexColor vertexColor;

								// inserts color data into mesh if needed
								if (pHdr->flags & STUDIOHDR_FLAGS_USES_VERTEX_COLOR)
								{
									vvc::VertexColor_t* vvcColor = vvcColors.at(vertexOffset + vtxVert->origMeshVertID);
									vertexColor = Assets::VertexColor::VertexColor(vvcColor->r, vvcColor->g, vvcColor->b, vvcColor->a);
								}

								Assets::Vertex newVert = exportMesh.Vertices.Emplace(vvdVert->m_vecPosition, vvdVert->m_vecNormal, vertexColor, vvdVert->m_vecTexCoord);

								// inserts uv data into mesh if needed, should work but needs testing
								if (pHdr->flags & STUDIOHDR_FLAGS_USES_UV2)
								{
									Vector2* vvcUV2 = vvcUV2s.at(vertexOffset + vtxVert->origMeshVertID);
									newVert.SetUVLayer(*vvcUV2, 1);
								}

								// check if this model has extra bone weights (verts exceeding three bone weights), extra weights part untested
								if ((pHdr->flags & STUDIOHDR_FLAGS_USES_EXTRA_BONE_WEIGHTS) && (pHdr->version == STUDIO_VERSION_APEX_LEGENDS))
								{
									for (char w = 0; w < vvdVert->m_BoneWeights.numbones; w++)
									{
										if (w >= 3)
										{
											newVert.SetWeight({ (uint32_t)(pVVW->GetWeightData(vvdVert->m_BoneWeights.weightextra.extraweightindex + (w - 3))->bone), static_cast<float>(pVVW->GetWeightData(vvdVert->m_BoneWeights.weightextra.extraweightindex + (w - 3))->weight / 32767.0) }, w);
										}
										else
										{
											newVert.SetWeight({ vvdVert->m_BoneWeights.bone[w], static_cast<float>(vvdVert->m_BoneWeights.weightextra.weight[w] / 32767.0) }, w);
										}
									}
								}
								else // do weights normally is flag isn't set, this is pretty straight forward thankfully
								{
									for (char w = 0; w < vvdVert->m_BoneWeights.numbones; w++)
									{
										newVert.SetWeight({ vvdVert->m_BoneWeights.bone[w], vvdVert->m_BoneWeights.weight[w] }, w);
									}
								}
							}

							for (int n = 0; n < vtxStripGroup->numIndices; n += 3)
							{
								unsigned short i1 = *vtxStripGroup->vtxIndice(n);
								unsigned short i2 = *vtxStripGroup->vtxIndice(n + 1);
								unsigned short i3 = *vtxStripGroup->vtxIndice(n + 2);

								exportMesh.Faces.EmplaceBack(i1, i2, i3);
							}
						}

						vertexOffset += mdlMesh->vertexloddata.numLODVertexes[i];
					}
				}
			}
		}
	}

	string ModelDirectory = IO::Path::Combine(ExportModelPath, ExportModel->Name);
	IO::Directory::CreateDirectory(ModelDirectory);

	this->ModelExporter->ExportModel(*ExportModel.get(), IO::Path::Combine(ModelDirectory, ExportModel->Name + (const char*)ModelExporter->ModelExtension()));
	g_Logger.Info("Exported: " + ExportModel->Name + ".mdl\n");
}

// because I can't be asked to port source's vec/quat classes
// check if the Vector3 has valid values
inline bool isValidVec(Vector3 vec)
{
	return isfinite(vec.X) && isfinite(vec.Y) && isfinite(vec.Z);
}

// unpack Vector48 into Vector3
inline Vector3 UnpackVector48(Vector48 vec48)
{
	Vector3 vec;

	vec.X = Math::Half::ToFloat(vec48.x);
	vec.Y = Math::Half::ToFloat(vec48.y);
	vec.Z = Math::Half::ToFloat(vec48.z);

	return vec;
}

// check if the Quaternion has valid values
inline bool isValidQuat(Quaternion quat)
{
	return isfinite(quat.X) && isfinite(quat.Y) && isfinite(quat.Z) && isfinite(quat.W);
}

// set initial values of Quaternion
inline Quaternion initQuat(float ix, float iy, float iz, float iw)
{
	Quaternion quat;

	quat.X = ix;
	quat.Y = iy;
	quat.Z = iz;
	quat.W = iw;

	return quat;
}

// unpack Quaternion64 into Quaternion
inline Quaternion UnpackQuat64(Quaternion64 quat64)
{
	Quaternion quat;

	quat.X = ((int)quat64.x - 1048576) * (1 / 1048576.5f);
	quat.Y = ((int)quat64.y - 1048576) * (1 / 1048576.5f);
	quat.Z = ((int)quat64.z - 1048576) * (1 / 1048576.5f);
	quat.W = sqrt(1 - quat.X * quat.X - quat.Y * quat.Y - quat.Z * quat.Z);
	if (quat64.wneg)
		quat.W = -quat.W;

	return quat;
}

// this is so awful
inline float QuaternionNormalize(Quaternion& q)
{
	float radius, iradius;

	assert(isValidQuat(q));

	radius = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];

	if (radius) // > FLT_EPSILON && ((radius < 1.0f - 4*FLT_EPSILON) || (radius > 1.0f + 4*FLT_EPSILON))
	{
		radius = sqrt(radius);
		iradius = 1.0f / radius;
		q[3] *= iradius;
		q[2] *= iradius;
		q[1] *= iradius;
		q[0] *= iradius;
	}
	return radius;
}

inline void QuaternionBlendNoAlign(const Quaternion& p, const Quaternion& q, float t, Quaternion& qt)
{
	float sclp, sclq;
	int i;

	// 0.0 returns p, 1.0 return q.
	sclp = 1.0f - t;
	sclq = t;
	for (i = 0; i < 4; i++) {
		qt[i] = sclp * p[i] + sclq * q[i];
	}
	QuaternionNormalize(qt);
}

inline void QuaternionAlign(const Quaternion& p, const Quaternion& q, Quaternion& qt)
{
	int i;
	// decide if one of the quaternions is backwards
	float a = 0;
	float b = 0;
	for (i = 0; i < 4; i++)
	{
		a += (p[i] - q[i]) * (p[i] - q[i]);
		b += (p[i] + q[i]) * (p[i] + q[i]);
	}
	if (a > b)
	{
		for (i = 0; i < 4; i++)
		{
			qt[i] = -q[i];
		}
	}
	else if (&qt != &q)
	{
		for (i = 0; i < 4; i++)
		{
			qt[i] = q[i];
		}
	}
}

inline void QuaternionBlend(const Quaternion& p, const Quaternion& q, float t, Quaternion& qt)
{

	// decide if one of the quaternions is backwards
	Quaternion q2;
	QuaternionAlign(p, q, q2);
	QuaternionBlendNoAlign(p, q2, t, qt);
}

// could probably not have this but replicating source functionality is nice
inline void SinCos(float x, float* fsin, float* fcos)
{
	*fsin = sin(x);
	*fcos = cos(x);
}

// this is the same func as 'RTech::DecompressConvertRotation'
void AngleQuaternion(const RadianEuler& angles, Quaternion& outQuat)
{
	// pitch = x, yaw = y, roll = z
	float sr, sp, sy, cr, cp, cy;

	SinCos(angles.Z * 0.5f, &sy, &cy);
	SinCos(angles.Y * 0.5f, &sp, &cp);
	SinCos(angles.X * 0.5f, &sr, &cr);
	
	float srXcp = sr * cp, crXsp = cr * sp;
	outQuat.X = srXcp * cy - crXsp * sy; // X
	outQuat.Y = crXsp * cy + srXcp * sy; // Y

	float crXcp = cr * cp, srXsp = sr * sp;
	outQuat.Z = crXcp * sy - srXsp * cy; // Z
	outQuat.W = crXcp * cy + srXsp * sy; // W (real component)
}

// extract mstudioanimvalue_t
void MdlLib::ExtractAnimValue(int frame, mstudioanimvalue_t* panimvalue, float scale, float& v1, float& v2)
{
	if (!panimvalue)
	{
		v1 = v2 = 0;
		return;
	}

	// Avoids a crash reading off the end of the data
	if ((panimvalue->num.total == 1) && (panimvalue->num.valid == 1))
	{
		v1 = v2 = panimvalue[1].value * scale;
		return;
	}

	int k = frame;

	// find the data list that has the frame
	while (panimvalue->num.total <= k)
	{
		k -= panimvalue->num.total;
		panimvalue += panimvalue->num.valid + 1;
		if (panimvalue->num.total == 0)
		{
			assert(0); // running off the end of the animation stream is bad
			v1 = v2 = 0;
			return;
		}
	}
	if (panimvalue->num.valid > k)
	{
		// has valid animation data
		v1 = panimvalue[k + 1].value * scale;

		if (panimvalue->num.valid > k + 1)
		{
			// has valid animation blend data
			v2 = panimvalue[k + 2].value * scale;
		}
		else
		{
			if (panimvalue->num.total > k + 1)
			{
				// data repeats, no blend
				v2 = v1;
			}
			else
			{
				// pull blend from first data block in next list
				v2 = panimvalue[panimvalue->num.valid + 2].value * scale;
			}
		}
	}
	else
	{
		// get last valid data block
		v1 = panimvalue[panimvalue->num.valid].value * scale;
		if (panimvalue->num.total > k + 1)
		{
			// data repeats, no blend
			v2 = v1;
		}
		else
		{
			// pull blend from first data block in next list
			v2 = panimvalue[panimvalue->num.valid + 2].value * scale;
		}
	}
}

// extract mstudioanimvalue_t
void MdlLib::ExtractAnimValue(int frame, mstudioanimvalue_t* panimvalue, float scale, float& v1)
{
	if (!panimvalue)
	{
		v1 = 0;
		return;
	}

	int k = frame;

	while (panimvalue->num.total <= k)
	{
		k -= panimvalue->num.total;
		panimvalue += panimvalue->num.valid + 1;
		if (panimvalue->num.total == 0)
		{
			assert(0); // running off the end of the animation stream is bad
			v1 = 0;
			return;
		}
	}
	if (panimvalue->num.valid > k)
	{
		v1 = panimvalue[k + 1].value * scale;
	}
	else
	{
		// get last valid data block
		v1 = panimvalue[panimvalue->num.valid].value * scale;
	}
}

// parse rle animation
void MdlLib::CalcBoneQuaternion(int frame, float s,
								const Quaternion& baseQuat, const RadianEuler& baseRot, const Vector3& baseRotScale,
								int iBaseFlags, const Quaternion& baseAlignment,
								const titanfall2::mstudio_rle_anim_t* panim, Quaternion& q)
{
	if (panim->flags & STUDIO_ANIM_RAWROT)
	{
		q = UnpackQuat64(*(panim->pQuat64()));
		assert(isValidQuat(q));
		return;
	}

	if (panim->flags & STUDIO_ANIM_NOROT)
	{
		if (panim->flags & STUDIO_ANIM_DELTA)
		{
			q = initQuat(0.0f, 0.0f, 0.0f, 1.0f);
		}
		else
		{
			q = baseQuat;
		}
		return;
	}

	titanfall2::mstudioanim_valueptr_t* pValuesPtr = panim->pRotV();

	if (s > 0.001f)
	{
		Quaternion			q1, q2; // QuaternionAligned, please no simd for now (thank you)
		RadianEuler			angle1, angle2;

		ExtractAnimValue(frame, pValuesPtr->pAnimvalue(0), baseRotScale.X, angle1.X, angle2.X);
		ExtractAnimValue(frame, pValuesPtr->pAnimvalue(1), baseRotScale.Y, angle1.Y, angle2.Y);
		ExtractAnimValue(frame, pValuesPtr->pAnimvalue(2), baseRotScale.Z, angle1.Z, angle2.Z);

		if (!(panim->flags & STUDIO_ANIM_DELTA))
		{
			angle1.X = angle1.X + baseRot.X;
			angle1.Y = angle1.Y + baseRot.Y;
			angle1.Z = angle1.Z + baseRot.Z;
			angle2.X = angle2.X + baseRot.X;
			angle2.Y = angle2.Y + baseRot.Y;
			angle2.Z = angle2.Z + baseRot.Z;
		}

		assert(isValidVec(angle1) && isValidVec(angle2));
		if (angle1.X != angle2.X || angle1.Y != angle2.Y || angle1.Z != angle2.Z)
		{
			AngleQuaternion(angle1, q1);
			AngleQuaternion(angle2, q2);

			QuaternionBlend(q1, q2, s, q);
		}
		else
		{
			AngleQuaternion(angle1, q);
		}
	}
	else
	{
		RadianEuler			angle;

		ExtractAnimValue(frame, pValuesPtr->pAnimvalue(0), baseRotScale.X, angle.X);
		ExtractAnimValue(frame, pValuesPtr->pAnimvalue(1), baseRotScale.Y, angle.Y);
		ExtractAnimValue(frame, pValuesPtr->pAnimvalue(2), baseRotScale.Z, angle.Z);

		if (!(panim->flags & STUDIO_ANIM_DELTA))
		{
			angle.X = angle.X + baseRot.X;
			angle.Y = angle.Y + baseRot.Y;
			angle.Z = angle.Z + baseRot.Z;
		}

		assert(isValidVec(angle));
		AngleQuaternion(angle, q);
	}

	assert(isValidQuat(q));

	// align to unified bone
	if (!(panim->flags & STUDIO_ANIM_DELTA) && (iBaseFlags & BONE_FIXED_ALIGNMENT))
	{
		QuaternionAlign(baseAlignment, q, q);
	}
}

// parse rle animation
inline void MdlLib::CalcBoneQuaternion(int frame, float s,
	const titanfall2::mstudiobone_t* pBone,
	const titanfall2::mstudiolinearbone_t* pLinearBones,
	const titanfall2::mstudio_rle_anim_t* panim, Quaternion& q)
{
	if (pLinearBones)
	{
		MdlLib::CalcBoneQuaternion(frame, s, *pLinearBones->pQuat(panim->bone), *pLinearBones->pRot(panim->bone), *pLinearBones->pRotScale(panim->bone), *pLinearBones->pFlags(panim->bone), *pLinearBones->pQAlignment(panim->bone), panim, q);
	}
	else
	{
		MdlLib::CalcBoneQuaternion(frame, s, pBone->quat, pBone->rot, pBone->rotscale, pBone->flags, pBone->qAlignment, panim, q);
	}
}

// parse rle animation
void MdlLib::CalcBonePosition(int frame, float s,
					const Vector3& basePos, const float& baseBoneScale,
					const titanfall2::mstudio_rle_anim_t* panim, Vector3& pos)
{
	if (panim->flags & STUDIO_ANIM_RAWPOS)
	{
		pos = UnpackVector48(*(panim->pPos()));
		assert(isValidVec(pos));

		return;
	}

	titanfall2::mstudioanim_valueptr_t* pPosV = panim->pPosV();
	int					j;

	if (s > 0.001f)
	{
		float v1, v2;
		for (j = 0; j < 3; j++)
		{
			MdlLib::ExtractAnimValue(frame, pPosV->pAnimvalue(j), baseBoneScale, v1, v2);
			pos[j] = v1 * (1.0 - s) + v2 * s;
		}
	}
	else
	{
		for (j = 0; j < 3; j++)
		{
			MdlLib::ExtractAnimValue(frame, pPosV->pAnimvalue(j), baseBoneScale, pos[j]);
		}
	}

	if (!(panim->flags & STUDIO_ANIM_DELTA))
	{
		pos.X = pos.X + basePos.X;
		pos.Y = pos.Y + basePos.Y;
		pos.Z = pos.Z + basePos.Z;
	}

	assert(isValidVec(pos));
}

// parse rle animation
inline void MdlLib::CalcBonePosition(int frame, float s,
	const titanfall2::mstudiobone_t* pBone,
	const titanfall2::mstudiolinearbone_t* pLinearBones,
	const titanfall2::mstudio_rle_anim_t* panim, Vector3& pos)
{
	if (pLinearBones)
	{
		MdlLib::CalcBonePosition(frame, s, *pLinearBones->pPos(panim->bone), panim->posscale, panim, pos);
	}
	else
	{
		MdlLib::CalcBonePosition(frame, s, pBone->pos, panim->posscale, panim, pos);
	}
}

void MdlLib::CalcBoneScale(int frame, float s,
	const Vector3& baseScale, Vector3& baseScaleScale,
	const titanfall2::mstudio_rle_anim_t* panim, Vector3& scale)
{
	if (panim->flags & STUDIO_ANIM_RAWSCALE)
	{
		scale = UnpackVector48(*(panim->pScale()));
		assert(isValidVec(scale));

		return;
	}

	titanfall2::mstudioanim_valueptr_t* pScaleV = panim->pScaleV();
	int					j;

	if (s > 0.001f)
	{
		float v1, v2;
		for (j = 0; j < 3; j++)
		{
			ExtractAnimValue(frame, pScaleV->pAnimvalue(j), baseScaleScale[j], v1, v2);
			scale[j] = v1 * (1.0 - s) + v2 * s;
		}
	}
	else
	{
		for (j = 0; j < 3; j++)
		{
			ExtractAnimValue(frame, pScaleV->pAnimvalue(j), baseScaleScale[j], scale[j]);
		}
	}

	if (!(panim->flags & STUDIO_ANIM_DELTA))
	{
		scale.X = scale.X + baseScale.X;
		scale.Y = scale.Y + baseScale.Y;
		scale.Z = scale.Z + baseScale.Z;
	}

	assert(isValidVec(scale));
}

titanfall2::mstudio_rle_anim_t* titanfall2::mstudioanimdesc_t::pAnim(int* piFrame) const
{
	mstudio_rle_anim_t* panim = nullptr;

	int index = animindex;
	int section = 0;

	if (sectionframes != 0)
	{
		if (numframes > sectionframes && *piFrame == numframes - 1)
		{
			// last frame on long anims is stored separately
			*piFrame = 0;
			section = (numframes / sectionframes) + 1;
		}
		/*else
		{
			section = *piFrame / sectionframes;
			*piFrame -= section * sectionframes;
		}*/
		
		// looks like this in ida
		section = *piFrame / sectionframes;
		*piFrame -= section * sectionframes;

		index = pSection(section)->animindex;
	}

	return panim = reinterpret_cast<mstudio_rle_anim_t*>((char*)this + index);
}

void MdlLib::ExportMDLv53(const string& Asset, const string& Path)
{
	IO::BinaryReader Reader = IO::BinaryReader(IO::File::OpenRead(Asset));

	int modelLength = IO::File::OpenRead(Asset)->GetLength();;

	std::unique_ptr<char[]> mdlBuff(new char[modelLength]);
	Reader.Read(mdlBuff.get(), 0, modelLength);
	titanfall2::studiohdr_t* pHdr = reinterpret_cast<titanfall2::studiohdr_t*>(mdlBuff.get());

	// id = IDST or version = 53
	if (pHdr->id != 0x54534449 || pHdr->version != STUDIO_VERSION_TITANFALL2)
		return;
	
	auto Model = std::make_unique<Assets::Model>(0, 0);

	// get name from sznameindex incase it exceeds 64 bytes (for whatever reason)
	Model->Name = IO::Path::GetFileNameWithoutExtension(pHdr->mdlName());

	std::vector<titanfall2::mstudiobone_t> bones;

	for (int i = 0; i < pHdr->numbones; i++)
	{
		titanfall2::mstudiobone_t* newBone = pHdr->pBone(i);
		titanfall2::mstudiobone_t bone = *newBone; // do something better with this

		Model->Bones.EmplaceBack(newBone->pszName(), bone.parent, bone.pos, bone.quat/*, bone.scale, Assets::BoneFlags::HasScale*/);
		bones.push_back(bone);
	}

	if (pHdr->numbodyparts)
	{
		ExtractValveVertexData(pHdr, pHdr->GetVTX(), pHdr->GetVVD(), pHdr->GetVVC(), nullptr, Model, Path);
	}

	if (pHdr->numlocalanim)
	{
		string ExportAnimPath = IO::Path::Combine(Path, "animations");
		string ExportBasePath = IO::Path::Combine(ExportAnimPath, IO::Path::GetFileNameWithoutExtension(pHdr->mdlName()));

		IO::Directory::CreateDirectory(ExportBasePath);

		for (int i = 0; i < pHdr->numlocalanim; i++)
		{
			titanfall2::mstudioanimdesc_t* animdesc = pHdr->anim(i);

			auto Anim = std::make_unique<Assets::Animation>(Model->Bones.Count(), animdesc->fps);
			Assets::AnimationCurveMode AnimCurveType = (animdesc->flags & STUDIO_DELTA) ? Assets::AnimationCurveMode::Additive : Assets::AnimationCurveMode::Absolute; // technically this should change based on flags

			for (Assets::Bone& Bone : Model->Bones)
			{
				Anim->Bones.EmplaceBack(Bone.Name(), Bone.Parent(), Bone.LocalPosition(), Bone.LocalRotation());

				List<Assets::Curve>& CurveNodes = Anim->GetNodeCurves(Bone.Name());

				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::RotateQuaternion, AnimCurveType);
				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::TranslateX, AnimCurveType);
				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::TranslateY, AnimCurveType);
				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::TranslateZ, AnimCurveType);
				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::ScaleX, AnimCurveType);
				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::ScaleY, AnimCurveType);
				CurveNodes.EmplaceBack(Bone.Name(), Assets::CurveProperty::ScaleZ, AnimCurveType);
			}

			Anim->Looping = (animdesc->flags & STUDIO_LOOPING) ? true : false; // previously '0x20000', why?

			for (int frameIdx = 0; frameIdx < animdesc->numframes; frameIdx++)
			{
				// cut this if it doesn't affect anything?
				float cycle = static_cast<float>(frameIdx / animdesc->numframes); // don't think this is correct

				float fFrame = cycle * (animdesc->numframes - 1);

				int iFrame = (int)fFrame;
				float s = (fFrame - iFrame);
				//float s = 0;

				int iLocalFrame = frameIdx;

				titanfall2::mstudio_rle_anim_t* pAnim = animdesc->pAnim(&iLocalFrame);

				//printf("local frame %i\n", iLocalFrame + 1);

				for (int boneIdx = 0; boneIdx < pHdr->numbones; boneIdx++)
				{
					unsigned char boneId = pAnim->bone; // unsigned char as bone limit is 256

					// this may not be the best solution
					if (!pAnim->flags && (boneId == 0xff))
						break;

					// rotation
					Quaternion quat;

					CalcBoneQuaternion(iLocalFrame, s, pHdr->pBone(boneId), pHdr->pLinearBones(), pAnim, quat);

					Anim->GetNodeCurves(Anim->Bones[boneId].Name())[0].Keyframes.Emplace(static_cast<uint32_t>(frameIdx), quat);

					// position
					Vector3 pos;

					CalcBonePosition(iLocalFrame, s, pHdr->pBone(boneId), pHdr->pLinearBones(), pAnim, pos);

					Anim->GetNodeCurves(Anim->Bones[boneId].Name())[1].Keyframes.EmplaceBack(static_cast<uint32_t>(frameIdx), pos.X);
					Anim->GetNodeCurves(Anim->Bones[boneId].Name())[2].Keyframes.EmplaceBack(static_cast<uint32_t>(frameIdx), pos.Y);
					Anim->GetNodeCurves(Anim->Bones[boneId].Name())[3].Keyframes.EmplaceBack(static_cast<uint32_t>(frameIdx), pos.Z);

					// scale
					Vector3 scale;

					CalcBoneScale(iLocalFrame, s, pHdr->pBone(boneId)->scale, pHdr->pBone(boneId)->scalescale, pAnim, scale);

					Anim->GetNodeCurves(Anim->Bones[boneId].Name())[4].Keyframes.EmplaceBack(static_cast<uint32_t>(frameIdx), scale.X);
					Anim->GetNodeCurves(Anim->Bones[boneId].Name())[5].Keyframes.EmplaceBack(static_cast<uint32_t>(frameIdx), scale.Y);
					Anim->GetNodeCurves(Anim->Bones[boneId].Name())[6].Keyframes.EmplaceBack(static_cast<uint32_t>(frameIdx), scale.Z);

					//printf("bone: %i \n rot: rx %f, ry %f, rz %f, rw %f\n pos: px %f, py %f, pz %f\n scale: sx %f, sy %f, sz %f\n", boneId, quat.X, quat.Y, quat.Z, quat.W, pos.X, pos.Y, pos.Z, scale.X, scale.Y, scale.Z);

					if (!(*pAnim->pNextOffset()))
						break;

					pAnim = reinterpret_cast<titanfall2::mstudio_rle_anim_t*>((char*)pAnim + *pAnim->pNextOffset());
				}
			}

			Anim->RemoveEmptyNodes();

			string animName = animdesc->pszName();
			this->AnimExporter->ExportAnimation(*Anim.get(), IO::Path::Combine(ExportBasePath, animName + (const char*)this->AnimExporter->AnimationExtension()));

			g_Logger.Info("Exported: " + animName + ".\n");
		}
	}
}