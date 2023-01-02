#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include <MathHelper.h>
const std::vector<std::string> MaterialTypes = { "_rgdu", "_rgdp", "_rgdc", "_sknu", "_sknp", "_sknc", "_wldu", "_wldc", "_ptcu", "_ptcs" };

#define RadiansToDegrees(r) ((r / Math::MathHelper::PI) * 180.0f)

void WriteCommonJiggle(IO::StreamWriter& qc, mstudiojigglebonev54_t*& JiggleBone)
{
	if (JiggleBone->length)
		qc.WriteFmt("\t\tlength %.4f\n", JiggleBone->length);

	if (JiggleBone->tipMass)
		qc.WriteFmt("\t\ttip_mass %.4f\n", JiggleBone->tipMass);

	if (JiggleBone->flags & JIGGLE_HAS_ANGLE_CONSTRAINT)
		qc.WriteFmt("\t\tangle_constraint %.4f\n", RadiansToDegrees(JiggleBone->angleLimit));

	if (JiggleBone->flags & JIGGLE_HAS_YAW_CONSTRAINT)
		qc.WriteFmt("\t\tyaw_constraint %.4f %.4f\n", RadiansToDegrees(JiggleBone->minYaw), RadiansToDegrees(JiggleBone->maxYaw));

	if (JiggleBone->yawFriction)
		qc.WriteFmt("\t\tyaw_friction %.4f\n", JiggleBone->yawFriction);

	if (JiggleBone->yawBounce)
		qc.WriteFmt("\t\tyaw_bounce %.4f\n", JiggleBone->yawBounce);

	if (JiggleBone->flags & JIGGLE_HAS_PITCH_CONSTRAINT)
		qc.WriteFmt("\t\tpitch_constraint %.4f %.4f\n", RadiansToDegrees(JiggleBone->minPitch), RadiansToDegrees(JiggleBone->maxPitch));

	if (JiggleBone->pitchFriction)
		qc.WriteFmt("\t\tpitch_friction %.4f\n", JiggleBone->pitchFriction);

	if (JiggleBone->pitchBounce)
		qc.WriteFmt("\t\tpitch_bounce %.4f\n", JiggleBone->pitchBounce);
}

void WriteJiggleBoneData(IO::StreamWriter& qc, mstudiojigglebonev54_t*& JiggleBone)
{
	if ((JiggleBone->flags & JIGGLE_IS_RIGID) && !(JiggleBone->flags & JIGGLE_IS_FLEXIBLE))
	{
		qc.Write("\tis_rigid {\n");

		WriteCommonJiggle(qc, JiggleBone);

		qc.Write("\t}\n\n");
	}

	if (JiggleBone->flags & JIGGLE_IS_FLEXIBLE)
	{
		qc.Write("\tis_flexible {\n");

		WriteCommonJiggle(qc, JiggleBone);

		if (JiggleBone->yawStiffness)
			qc.WriteFmt("\t\tyaw_stiffness %.4f\n", JiggleBone->yawStiffness);

		if (JiggleBone->yawDamping)
			qc.WriteFmt("\t\tyaw_damping %.4f\n", JiggleBone->yawDamping);

		if (JiggleBone->pitchStiffness)
			qc.WriteFmt("\t\tpitch_stiffness %.4f\n", JiggleBone->pitchStiffness);

		if (JiggleBone->pitchDamping)
			qc.WriteFmt("\t\tpitch_damping %.4f\n", JiggleBone->pitchDamping);

		if (JiggleBone->flags & JIGGLE_HAS_LENGTH_CONSTRAINT)
			qc.Write("\t\tallow_length_flex\n");

		if (JiggleBone->alongStiffness)
			qc.WriteFmt("\t\talong_stiffness %.4f\n", JiggleBone->alongStiffness);

		if (JiggleBone->alongDamping)
			qc.WriteFmt("\t\talong_damping %.4f\n", JiggleBone->alongDamping);

		qc.Write("\t}\n\n");
	}

	if (JiggleBone->flags & JIGGLE_HAS_BASE_SPRING)
	{
		qc.Write("\thas_base_spring {\n");

		WriteCommonJiggle(qc, JiggleBone);

		if (JiggleBone->baseStiffness)
			qc.WriteFmt("\t\tstiffness %.4f\n", JiggleBone->baseStiffness);

		if (JiggleBone->baseDamping)
			qc.WriteFmt("\t\tdamping %.4f\n", JiggleBone->baseDamping);

		if (JiggleBone->baseMinLeft || JiggleBone->baseMaxLeft)
			qc.WriteFmt("\t\tleft_constraint %.4f %.4f\n", JiggleBone->baseMinLeft, JiggleBone->baseMaxLeft);

		if (JiggleBone->baseLeftFriction)
			qc.WriteFmt("\t\tleft_friction %.4f\n", JiggleBone->baseLeftFriction);

		if (JiggleBone->baseMinUp || JiggleBone->baseMaxUp)
			qc.WriteFmt("\t\tup_constraint %.4f %.4f\n", JiggleBone->baseMinUp, JiggleBone->baseMaxUp);

		if (JiggleBone->baseUpFriction)
			qc.WriteFmt("\t\tup_friction %.4f\n", JiggleBone->baseUpFriction);

		if (JiggleBone->baseMinForward || JiggleBone->baseMaxForward)
			qc.WriteFmt("\t\tforward_constraint %.4f %.4f\n", JiggleBone->baseMinForward, JiggleBone->baseMaxForward);

		if (JiggleBone->baseForwardFriction)
			qc.WriteFmt("\t\tforward_friction %.4f\n", JiggleBone->baseForwardFriction);

		if (JiggleBone->baseMass)
			qc.WriteFmt("\t\tbase_mass %.4f\n", JiggleBone->baseMass);

		qc.Write("\t}\n\n");
	}
};

void SMDWriteRefAnim(const string& Path, const List<Assets::Bone>& Bones, string Name)
{
	string RefPath = IO::Path::Combine(IO::Path::GetDirectoryName(Path), (Name + "_ref.smd"));

	IO::StreamWriter Writer = IO::StreamWriter(IO::File::Create(RefPath));

	Writer.WriteLine(
		"version 1\n"
		"nodes"
	);

	uint32_t BoneIndex = 0;

	for (Assets::Bone& Bone : Bones)
	{
		Writer.WriteLineFmt("\t%d \"%s\" %d", BoneIndex, (char*)Bone.Name(), Bone.Parent());
		BoneIndex++;
	}

	Writer.WriteLine(
		"end\n"
		"skeleton\n"
		"time 0"
	);

	BoneIndex = 0;

	for (Assets::Bone& Bone : Bones)
	{
		auto Euler = Bone.LocalRotation().ToEulerAngles();
		Writer.WriteLineFmt("\t%d %f %f %f %f %f %f", BoneIndex, Bone.LocalPosition().X, Bone.LocalPosition().Y, Bone.LocalPosition().Z, Math::MathHelper::DegreesToRadians(Euler.X), Math::MathHelper::DegreesToRadians(Euler.Y), Math::MathHelper::DegreesToRadians(Euler.Z));
		BoneIndex++;
	}

	Writer.WriteLine("end");

	Writer.Close();
}

void RpakLib::ExportQC(const RpakLoadAsset& Asset, const string& Path, const string& modelPath, const std::unique_ptr<Assets::Model>& Model, char* rmdlBuf, char* phyBuf)
{
	int AssetVersion = Asset.AssetVersion;
	bool IsRig = Asset.AssetType == (uint32_t)AssetType_t::AnimationRig;

	if (IsRig) // stupid way of getting the highest Asset Version
	{
		int MaxVersion = 0;
		for (auto& Asset : this->Assets)
		{
			auto& val = Asset.Value();
			if (val.AssetType == (uint32_t)AssetType_t::Model && val.AssetVersion > MaxVersion)
				MaxVersion = val.AssetVersion;
		}

		AssetVersion = MaxVersion;
	}

	if (AssetVersion > 16)
		return;

	IO::StreamWriter qc(IO::File::Create(Path));

	s3studiohdr_t hdr{};

	switch (AssetVersion)
	{
	case 13:
		hdr = reinterpret_cast<studiohdr_t_v13*>(rmdlBuf)->Downgrade();
		break;
	case 14:
	case 15:
		hdr = reinterpret_cast<studiohdr_t_v14*>(rmdlBuf)->Downgrade();
		break;
	case 16:
		hdr = reinterpret_cast<studiohdr_t_v16*>(rmdlBuf)->Downgrade();
		break;
	default:
		hdr = *reinterpret_cast<s3studiohdr_t*>(rmdlBuf);
		break;
	}

	qc.WriteFmt("$modelname \"%s\"\n\n", modelPath.ToCString());

	char* surfaceProp = reinterpret_cast<char*>(rmdlBuf + hdr.surfacepropindex);

	qc.Write("$maxverts 65535 65535\n\n");

	qc.WriteFmt("$surfaceprop \"%s\"\n", surfaceProp);

	if (hdr.flags & STUDIOHDR_FLAGS_STATIC_PROP)
		qc.WriteFmt("$staticprop\n\n");

	if (hdr.flags & STUDIOHDR_FLAGS_FORCE_OPAQUE)
		qc.Write("$opaque\n\n");


	std::vector<std::string> BoneNames(hdr.numbones);
	std::vector<mstudiobonev54_t> Bones(hdr.numbones);

	uint64_t v16bonedataindex = 0;
	if (AssetVersion == 16)
		v16bonedataindex = reinterpret_cast<studiohdr_t_v16*>(rmdlBuf)->bonedataindex;

	for (int i = 0; i < hdr.numbones; i++)
	{
		char* pBone = rmdlBuf + hdr.boneindex;
		if (AssetVersion <= 10)
		{
			pBone = pBone + (i * sizeof(mstudiobonev54_t));
			mstudiobonev54_t Bone = *reinterpret_cast<mstudiobonev54_t*>(pBone);
			BoneNames[i] = std::string(reinterpret_cast<char*>(pBone + Bone.sznameindex));
			Bones[i] = Bone;
		}
		else if (AssetVersion < 16)
		{
			pBone = pBone + (i * sizeof(mstudiobonev54_t_v121));
			mstudiobonev54_t_v121 Bone = *reinterpret_cast<mstudiobonev54_t_v121*>(pBone);
			BoneNames[i] = std::string(reinterpret_cast<char*>(pBone + Bone.sznameindex));
			Bones[i] = Bone.Downgrade();
		}
		else {
			pBone = pBone + (i * sizeof(mstudiobone_t_v16));
			mstudiobone_t_v16 Bone = *reinterpret_cast<mstudiobone_t_v16*>(pBone);
			BoneNames[i] = std::string(reinterpret_cast<char*>(pBone + Bone.sznameindex));
			mstudiobonedata_t_v16 BoneData = *reinterpret_cast<mstudiobonedata_t_v16*>(rmdlBuf + v16bonedataindex + (i * sizeof(mstudiobonedata_t_v16)));
			mstudiobonev54_t out{};
			out.ConstructFromV16(Bone, BoneData);
			Bones[i] = out;

			if (out.contents)
				hdr.contents = out.contents;
		}
	}

	qc.WriteFmt("$contents \"%s\"\n\n", (hdr.contents & 1) == 1 ? "solid" : "notsolid");

	int jigglebonecount = 0;
	for (int i = 0; i < hdr.numbones; i++)
	{
		if (Bones[i].proctype == 5) {
			jigglebonecount++;
		}
	}

	for (int i = 0; i < hdr.numbodyparts; i++)
	{
		char* pBodyPart = rmdlBuf + hdr.bodypartindex;
		mstudiobodyparts_t bodyPart{};
		switch (AssetVersion)
		{
		case 15:
			pBodyPart = pBodyPart + (i * sizeof(mstudiobodyparts_t_v15));
			bodyPart = reinterpret_cast<mstudiobodyparts_t_v15*>(pBodyPart)->Downgrade();
			break;
		case 16:
			pBodyPart = pBodyPart + (i * sizeof(mstudiobodyparts_t_v16));
			bodyPart = reinterpret_cast<mstudiobodyparts_t_v16*>(pBodyPart)->Downgrade();
			break;
		default:
			pBodyPart = pBodyPart + (i * sizeof(mstudiobodyparts_t));
			bodyPart = *reinterpret_cast<mstudiobodyparts_t*>(pBodyPart);
			break;
		}

		char* bodyPartName = reinterpret_cast<char*>(pBodyPart + bodyPart.sznameindex);

		qc.WriteFmt("$bodygroup \"%s\"\n{\n", bodyPartName);

		List<int> BodyPartMeshes;
		for (int j = 0; j < bodyPart.nummodels; j++)
		{
			char* pModel = pBodyPart + bodyPart.modelindex;
			mstudiomodelv54_t model{};

			switch (AssetVersion)
			{
			case 13:
				pModel = pModel + (j * sizeof(mstudiomodelv54_t_v13));
				model = reinterpret_cast<mstudiomodelv54_t_v13*>(pModel)->Downgrade();
				break;
			case 14:
			case 15:
				pModel = pModel + (j * sizeof(mstudiomodelv54_t_v14));
				model = reinterpret_cast<mstudiomodelv54_t_v14*>(pModel)->Downgrade();
				break;
			case 16:
				pModel = pModel + (j * sizeof(mstudiomodelv54_t_v16));
				model = reinterpret_cast<mstudiomodelv54_t_v16*>(pModel)->Downgrade();
				break;
			default:
				pModel = pModel + (j * sizeof(mstudiomodelv54_t));
				model = *reinterpret_cast<mstudiomodelv54_t*>(pModel);
				break;
			}

			if (model.nummeshes > 0 || *model.name)
				qc.WriteFmt("\tstudio \"%s.smd\"\n", bodyPartName);
			else
				qc.Write("\tblank\n");

			for (int a = 0; a < model.nummeshes; a++)
			{
				char* pMesh = pModel + model.meshindex;
				mstudiomeshv54_t mesh{};

				switch (AssetVersion)
				{
				case 16:
					pMesh = pMesh + (a * sizeof(mstudiomeshv54_t_v16));
					mesh = reinterpret_cast<mstudiomeshv54_t_v16*>(pMesh)->Downgrade();
					break;
				default:
					if (AssetVersion <= 10)
					{
						pMesh = pMesh + (a * sizeof(mstudiomeshv54_t));
						mesh = *reinterpret_cast<mstudiomeshv54_t*>(pMesh);
					}
					else
					{
						pMesh = pMesh + (a * sizeof(mstudiomeshv54_t_v121));
						mesh = reinterpret_cast<mstudiomeshv54_t_v121*>(pMesh)->Downgrade();
					}
					break;
				}

				BodyPartMeshes.EmplaceBack(mesh.meshid);
			}
		}

		Model->BodyPartNames.EmplaceBack(bodyPartName);
		Model->BodyPartMeshIds.EmplaceBack(BodyPartMeshes);

		qc.Write("}\n\n");
	}

	qc.WriteFmt("$eyeposition %f %f %f\n", hdr.eyeposition.X, hdr.eyeposition.Y, hdr.eyeposition.Z);
	qc.WriteFmt("$illumposition %f %f %f\n\n", hdr.illumposition.X, hdr.illumposition.Y, hdr.illumposition.Z);

	qc.Write("$cdmaterials \"\"\n\n");

	int MaxTextureResize = Model->Materials.Count() < hdr.numtextures ? hdr.numtextures : Model->Materials.Count();

	std::vector<string> TextureNames(MaxTextureResize);
	std::vector<string> TextureTypes(MaxTextureResize);

	for (int i = 0; i < hdr.numtextures; i++)
	{
		char* pTexture = rmdlBuf + hdr.textureindex;
		mstudiotexturev54_t texture{};

		if (AssetVersion < 16)
		{
			pTexture = pTexture + (i * sizeof(mstudiotexturev54_t));
			texture = *reinterpret_cast<mstudiotexturev54_t*>(pTexture);
			TextureNames[i] = string(pTexture + texture.sznameindex);
		}
		else {
			pTexture = pTexture + (i * sizeof(mstudiotexturev54_t_v16));
			texture = reinterpret_cast<mstudiotexturev54_t_v16*>(pTexture)->Downgrade();

			if (Assets.ContainsKey(texture.guid))
			{
				RpakLoadAsset& MaterialAsset = Assets[texture.guid];
				RMdlMaterial ParsedMaterial = this->ExtractMaterialSilent(MaterialAsset, "", false, true);

				TextureTypes[i] = MaterialTypes[ParsedMaterial.MaterialType];
				TextureNames[i] = ParsedMaterial.FullMaterialName.ToLower() + TextureTypes[i];
			}

			continue;
		}

		string temp = string(TextureNames[i].ToCString()).Replace("\\", "/");

		TextureNames[i] = temp;
		for (int z = 0; z < MaterialTypes.size(); z++)
		{
			string MatType = MaterialTypes[z].c_str();

			if (temp.IndexOf(MatType) != -1)
			{
				TextureTypes[i] = MatType;
				break;
			}
		}
	}

	if (Model != nullptr)
	{
		//qc.WriteFmt("//$texturegroup \"skinfamilies\"\n//{\n");
		//
		//for (int i = 0; i < hdr.numskinfamilies; i++)
		//{
		//	if (AssetVersion >= 16)
		//		break;
		//
		//	char* pSkinFamily = rmdlBuf + hdr.skinindex + (i * hdr.numskinref * sizeof(short));
		//
		//	std::string skinName = "default";
		//
		//	if (i > 0)
		//	{
		//		int sizeofz = (hdr.numskinfamilies * hdr.numskinref * sizeof(short));
		//		char* pSkinFamilies = (rmdlBuf + hdr.skinindex + sizeofz) + (sizeofz % 4);
		//
		//		int* pSkinNameIndex = reinterpret_cast<int*>(pSkinFamilies + ((i - 1) * 4));
		//		skinName = std::string(rmdlBuf + *(pSkinNameIndex));
		//	}
		//
		//	qc.WriteFmt("//\t\"%s\" { ", skinName.c_str());
		//	for (int j = 0; j < hdr.numskinref; j++)
		//	{
		//		short texId = *reinterpret_cast<short*>(pSkinFamily + (j * sizeof(short)));
		//		string TextureName = string(TextureNames[texId].ToCString()).Replace("\\", "/");
		//
		//		qc.WriteFmt("//\"%s\" ", TextureName.ToCString());
		//	}
		//
		//	qc.Write("//}\n");
		//}
		//qc.Write("//}\n\n");

		List<std::string> ValidTextures{};
		for (Assets::Mesh& Submesh : Model->Meshes)
		{
			for (Assets::Face& Face : Submesh.Faces)
			{
				if (Submesh.MaterialIndices[0] > -1)
				{
					int materialindex = Submesh.MaterialIndices[0];
					string SubMeshTextureName = Model->Materials[materialindex].Name.ToLower() + TextureTypes[materialindex];

					for (int i = 0; i < TextureNames.size(); i++)
					{
						string Texture = TextureNames[i];
						bool bIsSubmeshMaterial = Texture.Contains(SubMeshTextureName);

						if (!bIsSubmeshMaterial)
							continue;

						bool bExistsInValidList = ValidTextures.Contains(TextureNames[i].ToLower().ToCString());

						if (!bExistsInValidList)
							ValidTextures.EmplaceBack(TextureNames[i].ToCString());

						break;
					}
				}
			}
		}

		for (auto& ValidTexture : ValidTextures)
			qc.WriteFmt("$renamematerial \"%s\" \"%s\"\n", IO::Path::GetFileNameWithoutExtension(ValidTexture.c_str()).ToCString(), ValidTexture.c_str());

		qc.Write("\n");
	}

	qc.WriteFmt("$unlockdefinebones\n\n");

	for (int i = 0; i < hdr.numbones; i++)
	{
		std::string BoneParentName = "";
		std::string BoneName = BoneNames[i];

		mstudiobonev54_t& bone = Bones[i];

		if (bone.parent != -1)
			BoneParentName = BoneNames[bone.parent];

		qc.WriteFmt("$definebone \"%s\" \"%s\" %f %f %f %f %f %f 0 0 0 0 0 0\n", BoneName.c_str(), BoneParentName.c_str(), bone.pos.X, bone.pos.Y, bone.pos.Z, bone.rot.X, bone.rot.Y, bone.rot.Z);
	}
	qc.Write("\n");

	for (int i = 0; i < hdr.numbones; i++)
	{
		std::string BoneName = BoneNames[i];
		qc.WriteFmt("$bonemerge \"%s\" \n", BoneName.c_str());
	}
	qc.Write("\n");

	if (hdr.numlocalattachments)
		qc.Write("// !!! attachment rotation angles may be wrong !!!\n");
	for (int i = 0; i < hdr.numlocalattachments; i++)
	{
		// get attachment
		char* pAttachment = rmdlBuf + hdr.localattachmentindex;
		mstudioattachmentv54_t attachment{};

		if (AssetVersion < 16)
		{
			pAttachment = pAttachment + (i * sizeof(mstudioattachmentv54_t));
			attachment = *reinterpret_cast<mstudioattachmentv54_t*>(pAttachment);
		}
		else
		{
			pAttachment = pAttachment + (i * sizeof(mstudioattachmentv54_t_v16));
			attachment = reinterpret_cast<mstudioattachmentv54_t_v16*>(pAttachment)->Downgrade();
		}

		char* attachmentName = pAttachment + attachment.sznameindex;

		// get attachment's bone
		Vector3 angles = attachment.localmatrix.GetRotationMatrixAsDegrees();

		qc.WriteFmt("$attachment \"%s\" \"%s\" %f %f %f rotate %f %f %f\n",
			attachmentName,
			BoneNames[attachment.localbone].c_str(),
			attachment.localmatrix.c3r0, attachment.localmatrix.c3r1, attachment.localmatrix.c3r2,
			angles.X, angles.Y, angles.Z
		);

	}

	if (hdr.numlocalattachments)
		qc.Write("\n");

	for (int i = 0; i < jigglebonecount; i++)
	{
		char* pBones = nullptr;
		if (AssetVersion <= 10)
			pBones = rmdlBuf + hdr.boneindex + (hdr.numbones * sizeof(mstudiobonev54_t));
		else if (AssetVersion < 16)
			pBones = rmdlBuf + hdr.boneindex + (hdr.numbones * sizeof(mstudiobonev54_t_v121));
		else
			pBones = rmdlBuf + v16bonedataindex + (hdr.numbones * sizeof(mstudiobonedata_t_v16));

		mstudiojigglebonev54_t* JiggleBone = reinterpret_cast<mstudiojigglebonev54_t*>(pBones + (i * sizeof(mstudiojigglebonev54_t)));

		qc.WriteFmt("$jigglebone \"%s\"\n{\n", BoneNames[JiggleBone->bone].c_str());

		WriteJiggleBoneData(qc, JiggleBone);

		qc.Write("}\n\n");
	}

	for (int i = 0; i < hdr.numhitboxsets; i++)
	{
		char* pHitboxSet = rmdlBuf + hdr.hitboxsetindex;
		mstudiohitboxset_t hitboxSet{};

		if (AssetVersion < 16)
		{
			pHitboxSet = pHitboxSet + (i * sizeof(mstudiohitboxset_t));
			hitboxSet = *reinterpret_cast<mstudiohitboxset_t*>(pHitboxSet);
		}
		else
		{
			pHitboxSet = pHitboxSet + (i * sizeof(mstudiohitboxset_t_v16));
			hitboxSet = reinterpret_cast<mstudiohitboxset_t_v16*>(pHitboxSet)->Downgrade();
		}

		// get hboxset name
		char* hitboxSetName = pHitboxSet + hitboxSet.sznameindex;

		qc.WriteFmt("$hboxset \"%s\"\n", hitboxSetName);

		for (int j = 0; j < hitboxSet.numhitboxes; j++)
		{
			mstudiobboxv54_t hitbox{};

			char* pHitbox = nullptr;

			if (AssetVersion < 16)
			{
				pHitbox = pHitboxSet + (j * sizeof(mstudiobboxv54_t));
				hitbox = *reinterpret_cast<mstudiobboxv54_t*>(pHitbox + hitboxSet.hitboxindex);
			}
			else
			{
				pHitbox = pHitboxSet + (j * sizeof(mstudiobboxv54_t_v16));
				hitbox = reinterpret_cast<mstudiobboxv54_t_v16*>(pHitbox + hitboxSet.hitboxindex)->Downgrade();
			}

			qc.WriteFmt("$hbox %i \"%s\" %f %f %f %f %f %f\n",
				hitbox.group,
				BoneNames[hitbox.bone].c_str(),
				hitbox.bbmin.X, hitbox.bbmin.Y, hitbox.bbmin.Z,
				hitbox.bbmax.X, hitbox.bbmax.Y, hitbox.bbmax.Z
			);
		}
		qc.Write("\n");
	}

	List<string> PoseParameters;
	char* pPoseParams = rmdlBuf + hdr.localposeparamindex;
	for (int i = 0; i < hdr.numlocalposeparameters; i++)
	{
		mstudioposeparamdescv54_t PoseParam{};

		char* pPoseParam = nullptr;
		if (AssetVersion < 16)
		{
			pPoseParam = pPoseParams + (i * sizeof(mstudioposeparamdescv54_t));
			PoseParam = *reinterpret_cast<mstudioposeparamdescv54_t*>(pPoseParam);
		}
		else {
			pPoseParam = pPoseParams + (i * sizeof(mstudioposeparamdescv54_t_v16));
			PoseParam = reinterpret_cast<mstudioposeparamdescv54_t_v16*>(pPoseParam)->Downgrade();
			PoseParam.sznameindex = FIX_OFFSET(PoseParam.sznameindex);
		}

		string PoseName = string(pPoseParam + PoseParam.sznameindex);

		qc.WriteFmt("$poseparameter \"%s\" %.4f %.4f", PoseName.ToCString(), PoseParam.start, PoseParam.start);

		if (PoseParam.flags & STUDIO_LOOPING)
			qc.WriteFmt(" loop %f", PoseParam.loop);

		qc.Write("\n");

		PoseParameters.EmplaceBack(PoseName);
	}

	if (hdr.numlocalposeparameters)
		qc.Write("\n");

	qc.WriteFmt("$sequence \"ref\" \"%s_ref.smd\" \n\n", Model->Name.ToCString());

	SMDWriteRefAnim(Path, Model->Bones, Model->Name);



	if (IsRig)
	{
		auto RpakStream = this->GetFileStream(Asset);
		IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

		RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));
		AnimRigHeader RigHeader{};
		RigHeader.ReadFromAssetStream(&RpakStream, Asset.AssetVersion);

		const uint64_t ReferenceOffset = this->GetFileOffset(Asset, RigHeader.animSeqs);

		List<uint64_t> Hashes;
		List<string> AnimationNames;

		for (uint32_t i = 0; i < RigHeader.animSeqCount; i++)
		{
			RpakStream->SetPosition(ReferenceOffset + ((uint64_t)i * 0x8));

			uint64_t AnimHash = Reader.Read<uint64_t>();

			if (AnimHash == 0xDF5 || !Assets.ContainsKey(AnimHash))
				continue;


			this->QCWriteAseqData(qc, Path, AnimHash, Asset, {}, AnimationNames, true);

			Hashes.EmplaceBack(AnimHash);
		}

		Hashes.Clear();
		AnimationNames.Clear();
		for (uint32_t i = 0; i < RigHeader.animSeqCount; i++)
		{
			RpakStream->SetPosition(ReferenceOffset + ((uint64_t)i * 0x8));

			uint64_t AnimHash = Reader.Read<uint64_t>();

			if (AnimHash == 0xDF5 || !Assets.ContainsKey(AnimHash))
				continue;


			this->QCWriteAseqData(qc, Path, AnimHash, Asset, PoseParameters, AnimationNames, false);

			Hashes.EmplaceBack(AnimHash);
		}
	}

	qc.Close();
}

void RpakLib::QCWriteAseqData(IO::StreamWriter& qc, const string& Path, uint64_t AnimHash, const RpakLoadAsset& RigAsset, List<string> PoseParameters, List<string>& AnimationNames, bool WriteAnimations)
{
	if (this->Assets.ContainsKey(AnimHash))
	{
		RpakLoadAsset AseqAsset = this->Assets[AnimHash];
		std::unique_ptr<IO::MemoryStream> RpakStream = this->GetFileStream(AseqAsset);
		IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);
		RpakStream->SetPosition(this->GetFileOffset(AseqAsset, AseqAsset.SubHeaderIndex, AseqAsset.SubHeaderOffset));

		ASeqHeaderV10 AnHeader{};
		switch (AseqAsset.SubHeaderSize)
		{
		case 0x30: // 7
			AnHeader = Reader.Read<ASeqHeader>().Upgrade();
			break;
		case 0x38: // 7.1
			AnHeader = Reader.Read<ASeqHeaderV71>().Upgrade();
			break;
		case 0x40: // 10
			AnHeader = Reader.Read<ASeqHeaderV10>();
			break;
		}

		RpakStream->SetPosition(this->GetFileOffset(AseqAsset, AnHeader.pName));
		string AnimSetNameFull = Reader.ReadCString().Replace(".rseq", "");
		string AnimSetName = IO::Path::GetFileNameWithoutExtension(AnimSetNameFull);
		string AnimSetNameDir = IO::Path::GetDirectoryName(AnimSetNameFull).ToCString();
		string FolderPath = IO::Path::Combine(IO::Path::GetDirectoryName(AnimSetNameFull), IO::Path::GetFileNameWithoutExtension(AnimSetNameFull)).Replace("\\", "/");

		uint64_t AseqDataOffset = this->GetFileOffset(AseqAsset, AnHeader.pAnimation);
		RpakStream->SetPosition(AseqDataOffset);
		mstudioseqdesc_t_v16 seqdesc{};

		if (AseqAsset.AssetVersion < 11)
			seqdesc = Reader.Read<mstudioseqdesc_t>().Upgrade();
		else
			seqdesc = Reader.Read<mstudioseqdesc_t_v16>();

		int blends = seqdesc.groupsize[0] * seqdesc.groupsize[1];

		List<mstudioanimdescv54_t_v16> AnimDescs;
		List<string> AnimNames{};
		for (int i = 0; i < blends; i++)
		{
			RpakStream->SetPosition(AseqDataOffset + seqdesc.animindexindex);

			uint64_t blendoffset = 0;
			if (AseqAsset.AssetVersion <= 10)
			{
				RpakStream->SetPosition(RpakStream->GetPosition() + (i * sizeof(int)));
				blendoffset = Reader.Read<int>();
			}
			else
			{
				RpakStream->SetPosition(RpakStream->GetPosition() + (i * sizeof(short)));
				blendoffset = Reader.Read<short>();
			}

			RpakStream->SetPosition(AseqDataOffset + blendoffset);

			mstudioanimdescv54_t_v16 animdesc{};

			switch (AseqAsset.SubHeaderSize)
			{
			case 0x30: // 7
				animdesc = Reader.Read<mstudioanimdescv54_t>().Upgrade();
				break;
			case 0x38: // 7.1
			case 0x40: // 10
			{
				if (AseqAsset.AssetVersion <= 10)
					animdesc = Reader.Read<mstudioanimdescv54_t_v121>().Upgrade();
				else
				{
					animdesc = Reader.Read<mstudioanimdescv54_t_v16>();
					animdesc.sznameindex = FIX_OFFSET((int)animdesc.sznameindex);
				}

				break;
			}
			}

			RpakStream->SetPosition(AseqDataOffset + blendoffset + animdesc.sznameindex);
			string namez = Reader.ReadCString();

			AnimNames.EmplaceBack(namez);
			AnimDescs.EmplaceBack(animdesc);
		}


		List<mstudioanimdescv54_t_v16> ValidAnimDescs;
		List<string> ValidAnimNames;
		for (int i = 0; i < AnimDescs.Count(); i++)
		{
			string directory = string::Format("%s\\Anims\\%s\\%s.smd", IO::Path::GetDirectoryName(Path).ToCString(), AnimSetName.ToCString(), AnimNames[i].ToCString());

			if (IO::File::Exists(directory))
			{
				ValidAnimDescs.EmplaceBack(AnimDescs[i]);
				ValidAnimNames.EmplaceBack(AnimNames[i]);
			}

		}
		AnimNames = ValidAnimNames;
		AnimDescs = ValidAnimDescs;

		if (!AnimDescs.Count())
			return;

		if(WriteAnimations)
		{
			// seperator
			for (int i = 0; i < AnimDescs.Count(); i++)
			{
				mstudioanimdescv54_t_v16 AnimDesc = AnimDescs[i];

				if (!(AnimDesc.flags & STUDIO_ALLZEROS) && !AnimationNames.Contains(AnimNames[i]))
				{
					AnimationNames.EmplaceBack(AnimNames[i]);

					qc.WriteFmt("$animation \"%s\" \"Anims/%s/%s.smd\" {\n", AnimNames[i].ToCString(), AnimSetName.ToCString(), AnimNames[i].ToCString());
					{
						qc.WriteFmt("\tfps %d\n", int(AnimDesc.fps + 1.0));

						if (AnimDesc.flags & STUDIO_LOOPING)
							qc.Write("\tloop\n");

						if (AnimDesc.flags & STUDIO_NOFORCELOOP)
							qc.Write("\tnoforceloop\n");

						if (AnimDesc.flags & STUDIO_SNAP)
							qc.Write("\tsnap\n");

						if (AnimDesc.flags & STUDIO_POST)
							qc.Write("\tpost\n");

						qc.Write("\n}\n\n");
					}
				}
			}

			return;
		}


		qc.WriteFmt("$sequence \"%s\" {\n", AnimSetNameFull.ToCString());
		{
			for (int i = 0; i <1; i++)
			{
				if (!(AnimDescs[i].flags & STUDIO_ALLZEROS))
					qc.WriteFmt("\t\"%s\"\n", AnimNames[i].ToCString());
			}

			qc.Write("\n\n");

			// activity weight will never be 0 if an activity is set
			if (seqdesc.actweight)
			{
				RpakStream->SetPosition(AseqDataOffset + seqdesc.szactivitynameindex);
				qc.WriteFmt("\tactivity %s %d\n\n", Reader.ReadCString().ToCString(), seqdesc.actweight);
			}

			//if (seqdesc.flags & STUDIO_LOOPING)
			//	qc.Write("\tloop\n\n");
			//
			//if (seqdesc.flags & STUDIO_NOFORCELOOP)
			//	qc.Write("\tnoforceloop\n\n");
			//
			//if (seqdesc.flags & STUDIO_SNAP)
			//	qc.Write("\tsnap\n\n");


			qc.WriteFmt("\tfadein %.1f\n\tfadeout %.1f\n\n", seqdesc.fadeintime, seqdesc.fadeouttime);

			// ACTIVITY MODIFIERS
			{
				if (seqdesc.numactivitymodifiers)
					qc.Write("\n");

				for (int i = 0; i < seqdesc.numactivitymodifiers; i++)
				{
					uint64_t offset = AseqDataOffset + seqdesc.activitymodifierindex;

					if (AseqAsset.AssetVersion > 10)
						offset += (i * sizeof(mstudioactivitymodifierv54_t_v16));
					else
						offset += (i * sizeof(mstudioactivitymodifierv53_t));

					RpakStream->SetPosition(offset);

					mstudioactivitymodifierv53_t layer{};
					if (AseqAsset.AssetVersion < 10)
						layer = Reader.Read<mstudioactivitymodifierv53_t>();
					else
						layer = Reader.Read<mstudioactivitymodifierv54_t_v16>().Downgrade();

					RpakStream->SetPosition(offset + layer.sznameindex);
					string ActivityMod = Reader.ReadCString();

					if (!string::IsNullOrEmpty(ActivityMod))
						qc.WriteFmt("\tactivitymodifier %s\n", ActivityMod.ToCString());
				}

				if (seqdesc.numactivitymodifiers)
					qc.Write("\n");
			}

			// AUTO LAYERS
			{
				if (seqdesc.numautolayers)
					qc.Write("\n");

				for (int i = 0; i < seqdesc.numautolayers; i++)
				{
					RpakStream->SetPosition(AseqDataOffset + seqdesc.autolayerindex + (i * sizeof(mstudioautolayerv54_t)));
					mstudioautolayerv54_t layer = Reader.Read<mstudioautolayerv54_t>();

					if (this->Assets.ContainsKey(layer.guidSequence))
					{
						auto& LayerAsset = this->Assets[layer.guidSequence];

						std::string options = "";

						if (layer.flags & STUDIO_AL_XFADE)
							options += " xfade";

						if (layer.flags & STUDIO_AL_SPLINE)
							options += " spline";

						if (layer.flags & STUDIO_AL_NOBLEND)
							options += " noblend";

						if (layer.flags & STUDIO_AL_LOCAL)
							options += " local";

						int start = 0;
						int peak = 0;
						int tail = 0;
						int end = 0;

						if (layer.flags & STUDIO_AL_POSE)
						{
							options += string::Format(" poseparameter %s", PoseParameters[layer.iPose].ToCString());

							mstudioanimdescv54_t_v16 AnimDesc = AnimDescs[0];

							start = int(layer.start * (AnimDesc.fps * -1));
							peak = int(layer.peak * (AnimDesc.fps * -1));
							tail = int(layer.tail * (AnimDesc.fps * -1));
							end = int(layer.end * (AnimDesc.fps * -1));
						}
						else
						{
							start = int(layer.start);
							peak = int(layer.peak);
							tail = int(layer.tail);
							end = int(layer.end);
						}

						qc.WriteFmt("\n\taddlayer \"%s\"", this->ExtractAnimationSeq(LayerAsset).Replace(".rseq", "").ToCString());
						//qc.WriteFmt("\n\tblendlayer \"%s\" %d %d %d %d %s", this->ExtractAnimationSeq(LayerAsset).Replace(".rseq", "").ToCString(), start, peak, tail, end, options.c_str());
					}
					else
					{
						qc.WriteFmt("\t//blendlayer LAYER NOT FOUND IN LOADED RPAKs\n");
					}
				}

				if (seqdesc.numautolayers)
					qc.Write("\n\n");
			}

			// EVENTS
			{
				for (int i = 0; i < seqdesc.numevents; i++)
				{
					mstudioeventv54 Event{};
					if (AseqAsset.AssetVersion <= 10)
					{
						switch (AseqAsset.SubHeaderSize)
						{
						case 0x30: // 7 / 7.1
						case 0x38:
							RpakStream->SetPosition(AseqDataOffset + seqdesc.eventindex + (i * sizeof(mstudioeventv54_t)));
							break;
						case 0x40: // 10
							RpakStream->SetPosition(AseqDataOffset + seqdesc.eventindex + (i * sizeof(mstudioeventv54_t_v122)));
							break;
						}
					}
					else
					{
						RpakStream->SetPosition(AseqDataOffset + seqdesc.eventindex + (i * sizeof(mstudioeventv54_t_v16)));
					}


					Event.Init(AseqAsset.AssetVersion, AseqAsset.SubHeaderSize, Reader);

					// disabled for now until a fix is found to get the correct frame
					int frame_reversed = 0;//(int)(Event.cycle * float(AnimDescs[0].fps - 1));

					qc.WriteFmt("\t{ event %s %d \"%s\" }\n", Event.szevent.ToCString(), frame_reversed, Event.szoptions.ToCString());
				}

				qc.Write("\n}\n\n");
			}
		}
	}
}