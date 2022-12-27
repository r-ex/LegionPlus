#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"

const std::vector<std::string> MaterialTypes = { "_rgdu", "_rgdp", "_rgdc", "_sknu", "_sknp", "_sknc", "_wldu", "_wldc", "_ptcu", "_ptcs" };

s3studiohdr_t GetStudioMdl(int assetVersion, char* rmdlBuf)
{
	s3studiohdr_t hdr{};

	if (assetVersion <= 10)
		hdr = *reinterpret_cast<s3studiohdr_t*>(rmdlBuf);
	else {
		switch (assetVersion)
		{
		case 13:
		{
			studiohdr_t_v13 thdr = *reinterpret_cast<studiohdr_t_v13*>(rmdlBuf);
			hdr.flags = thdr.flags;
			hdr.surfacepropindex = thdr.surfacepropindex;
			hdr.contents = thdr.contents;
			hdr.eyeposition = thdr.eyeposition;
			hdr.illumposition = thdr.illumposition;
			hdr.numbones = thdr.numbones;
			hdr.boneindex = thdr.boneindex;
			hdr.numbodyparts = thdr.numbodyparts;
			hdr.bodypartindex = thdr.bodypartindex;
			hdr.numlocalattachments = thdr.numlocalattachments;
			hdr.localattachmentindex = thdr.localattachmentindex;
			hdr.numskinfamilies = thdr.numskinfamilies;
			hdr.skinindex = thdr.skinindex;
			hdr.numskinref = thdr.numskinref;
			hdr.numhitboxsets = thdr.numhitboxsets;
			hdr.hitboxsetindex = thdr.hitboxsetindex;
			hdr.textureindex = thdr.textureindex;
			hdr.numtextures = thdr.numtextures;
			break;
		}
		case 14:
		case 15:
		{
			studiohdr_t_v14 thdr = *reinterpret_cast<studiohdr_t_v14*>(rmdlBuf);
			hdr.flags = thdr.flags;
			hdr.surfacepropindex = thdr.surfacepropindex;
			hdr.contents = thdr.contents;
			hdr.eyeposition = thdr.eyeposition;
			hdr.illumposition = thdr.illumposition;
			hdr.numbones = thdr.numbones;
			hdr.boneindex = thdr.boneindex;
			hdr.numbodyparts = thdr.numbodyparts;
			hdr.bodypartindex = thdr.bodypartindex;
			hdr.numlocalattachments = thdr.numlocalattachments;
			hdr.localattachmentindex = thdr.localattachmentindex;
			hdr.numskinfamilies = thdr.numskinfamilies;
			hdr.skinindex = thdr.skinindex;
			hdr.numskinref = thdr.numskinref;
			hdr.numhitboxsets = thdr.numhitboxsets;
			hdr.hitboxsetindex = thdr.hitboxsetindex;
			hdr.textureindex = thdr.textureindex;
			hdr.numtextures = thdr.numtextures;
			break;
		}
		case 16:
		{
			studiohdr_t_v16 thdr = *reinterpret_cast<studiohdr_t_v16*>(rmdlBuf);
			hdr.flags = thdr.flags;
			hdr.surfacepropindex = thdr.surfacepropindex;
			hdr.illumposition = thdr.illumposition;
			hdr.numbones = thdr.numbones;
			hdr.boneindex = thdr.boneindex;
			hdr.numbodyparts = thdr.numbodyparts;
			hdr.bodypartindex = thdr.bodypartindex;
			hdr.numlocalattachments = thdr.numlocalattachments;
			hdr.localattachmentindex = thdr.localattachmentindex;
			hdr.numskinfamilies = thdr.numskinfamilies;
			hdr.skinindex = thdr.skinindex;
			hdr.numskinref = thdr.numskinref;
			hdr.numhitboxsets = thdr.numhitboxsets;
			hdr.hitboxsetindex = thdr.hitboxsetindex;
			hdr.textureindex = thdr.textureindex;
			hdr.numtextures = thdr.numtextures;
			break;
		}
		default:
			break;
		}
	}

	return hdr;
}
void WriteCommonJiggle(IO::StreamWriter& qc, mstudiojigglebonev54_t*& JiggleBone)
{
	if (JiggleBone->length)
		qc.WriteFmt("\t\tlength %.4f\n", JiggleBone->length);

	if (JiggleBone->tipMass)
		qc.WriteFmt("\t\ttip_mass %.4f\n", JiggleBone->tipMass);

	if (JiggleBone->flags & JIGGLE_HAS_ANGLE_CONSTRAINT && JiggleBone->angleLimit)
		qc.WriteFmt("\t\tangle_constraint %.4f\n", (JiggleBone->angleLimit / Math::MathHelper::PI) * 180.0f);

	if (JiggleBone->flags & JIGGLE_HAS_YAW_CONSTRAINT)
	{
		if (JiggleBone->minYaw || JiggleBone->maxYaw)
			qc.WriteFmt("\t\tyaw_constraint %.4f %.4f\n", (JiggleBone->minYaw / Math::MathHelper::PI) * 180.0f, (JiggleBone->maxYaw / Math::MathHelper::PI) * 180.0f);
	}

	if (JiggleBone->yawFriction)
		qc.WriteFmt("\t\tyaw_friction %.4f\n", JiggleBone->yawFriction);

	if (JiggleBone->yawBounce)
		qc.WriteFmt("\t\tyaw_bounce %.4f\n", JiggleBone->yawBounce);

	if (JiggleBone->flags & JIGGLE_HAS_PITCH_CONSTRAINT)
	{
		if (JiggleBone->minPitch || JiggleBone->maxPitch)
			qc.WriteFmt("\t\tpitch_constraint %.4f %.4f\n", (JiggleBone->minPitch / Math::MathHelper::PI) * 180.0f, (JiggleBone->maxPitch / Math::MathHelper::PI) * 180.0f);
	}

	if (JiggleBone->pitchFriction)
		qc.WriteFmt("\t\tpitch_friction %.4f\n", JiggleBone->pitchFriction);

	if (JiggleBone->pitchBounce)
		qc.WriteFmt("\t\tpitch_bounce %.4f\n", JiggleBone->pitchBounce);
}

void WriteJiggleBoneData(IO::StreamWriter& qc, mstudiojigglebonev54_t*& JiggleBone)
{
	if (JiggleBone->flags & JIGGLE_IS_RIGID)
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
	}

	qc.Write("\t}\n");
};

void RpakLib::ExportQC(int assetVersion, const string& Path, const string& modelPath, const std::unique_ptr<Assets::Model>& Model, char* rmdlBuf, char* phyBuf)
{
	if (assetVersion > 16)
		return;

	IO::StreamWriter qc(IO::File::Create(Path));
	s3studiohdr_t hdr = GetStudioMdl(assetVersion, rmdlBuf);

	qc.WriteFmt("$modelname \"%s\"\n", modelPath.ToCString());
	qc.WriteFmt("$cdmaterials \"\"\n");

	char* surfaceProp = reinterpret_cast<char*>(rmdlBuf + hdr.surfacepropindex);

	qc.WriteFmt("$surfaceprop \"%s\"\n", surfaceProp);

	if (hdr.flags & STUDIOHDR_FLAGS_STATIC_PROP)
		qc.WriteFmt("$staticprop\n\n");

	if (hdr.flags & STUDIOHDR_FLAGS_FORCE_OPAQUE)
		qc.Write("$opaque\n\n");

	qc.WriteFmt("$eyeposition %f %f %f\n", hdr.eyeposition.X, hdr.eyeposition.Y, hdr.eyeposition.Z);
	qc.WriteFmt("$illumposition %f %f %f\n\n", hdr.illumposition.X, hdr.illumposition.Y, hdr.illumposition.Z);

	std::vector<std::string> BoneNames(hdr.numbones);
	std::vector<mstudiobonev54_t> Bones(hdr.numbones);

	uint64_t v16bonedataindex = 0;
	if(assetVersion == 16)
		v16bonedataindex = reinterpret_cast<studiohdr_t_v16*>(rmdlBuf)->bonedataindex;

	for (int i = 0; i < hdr.numbones; i++)
	{
		char* pBone = rmdlBuf + hdr.boneindex;
		if (assetVersion <= 10)
		{
			pBone = pBone + (i * sizeof(mstudiobonev54_t));
			mstudiobonev54_t Bone = *reinterpret_cast<mstudiobonev54_t*>(pBone);
			BoneNames[i] = std::string(reinterpret_cast<char*>(pBone + Bone.sznameindex));
			Bones[i] = Bone;
		}
		else if (assetVersion < 16)
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
		switch (assetVersion)
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
			mstudiomodelv54_t* model = nullptr;

			switch (assetVersion)
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
				model = reinterpret_cast<mstudiomodelv54_t*>(pModel);
				break;
			}

			if (model->nummeshes > 0 || *model->name)
				qc.WriteFmt("\tstudio \"%s.smd\"\n", bodyPartName);
			else
				qc.Write("\tblank\n");

			for (int a = 0; a < model->nummeshes; a++)
			{
				char* pMesh = pModel + model->meshindex;
				mstudiomeshv54_t mesh{};

				switch (assetVersion)
				{
				case 16:
					pMesh = pMesh + (a * sizeof(mstudiomeshv54_t_v16));
					mesh = reinterpret_cast<mstudiomeshv54_t_v16*>(pMesh)->Downgrade();
					break;
				default:
					if (assetVersion <= 10)
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

		Model.get()->BodyPartNames.EmplaceBack(bodyPartName);
		Model.get()->BodyPartMeshIds.EmplaceBack(BodyPartMeshes);

		qc.Write("}\n\n");
	}

	std::vector<string> TextureNames(hdr.numtextures);
	std::vector<string> TextureTypes(hdr.numtextures);

	for (int i = 0; i < hdr.numtextures; i++)
	{
		char* pTexture = rmdlBuf + hdr.textureindex; 
		mstudiotexturev54_t texture{};
		
		if (assetVersion < 16)
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
				RMdlMaterial ParsedMaterial = this->ExtractMaterial(MaterialAsset, "", false, true);

				TextureTypes[i] = MaterialTypes[ParsedMaterial.MaterialType];

				TextureNames[i] = ParsedMaterial.FullMaterialName + TextureTypes[i];
			}

			continue;
		}

		std::string temp = TextureNames[i].ToCString();
		for (int z = 0; z < MaterialTypes.size(); z++)
		{
			std::string MatType = MaterialTypes[z];

			if (temp.find(MatType) != -1)
			{
				TextureTypes[i] = MatType;
				break;
			}
		}
	}

	qc.WriteFmt("//$texturegroup \"skinfamilies\"\n//{\n");

	for (int i = 0; i < hdr.numskinfamilies; i++)
	{
		if (assetVersion >= 16)
			break;

		char* pSkinFamily = rmdlBuf + hdr.skinindex + (i * hdr.numskinref * sizeof(short));

		std::string skinName = "default";

		if (i > 0)
		{
			int sizeofz = (hdr.numskinfamilies * hdr.numskinref * sizeof(short));
			char* pSkinFamilies = (rmdlBuf + hdr.skinindex + sizeofz) + (sizeofz % 4);

			int* pSkinNameIndex = reinterpret_cast<int*>(pSkinFamilies + ((i - 1) * 4));
			skinName = std::string(rmdlBuf + *(pSkinNameIndex));
		}

		qc.WriteFmt("//\t\"%s\" { ", skinName.c_str());
		for (int j = 0; j < hdr.numskinref; j++)
		{
			short texId = *reinterpret_cast<short*>(pSkinFamily + (j * sizeof(short)));

			std::string TextureName = (TextureNames[texId].ToCString());

			while (TextureName.find("\\") != -1)
				TextureName = TextureName.replace(TextureName.find("\\"), std::string("\\").length(), "/");

			qc.WriteFmt("//\"%s\" ", TextureName.c_str());
		}

		qc.Write("//}\n");
	}
	qc.Write("//}\n\n");

	List<std::string> ValidTextures{};
	for (auto& Submesh : Model.get()->Meshes)
	{
		for (auto& Face : Submesh.Faces)
		{
			if (Submesh.MaterialIndices[0] > -1)
			{
				int materialindex = Submesh.MaterialIndices[0];
				string SubMeshTextureName = Model.get()->Materials[materialindex].Name + TextureTypes[materialindex];

				int i = 0;
				for (auto& Texture : TextureNames)
				{
					if (Texture.ToLower().Contains(SubMeshTextureName.ToLower()) && !ValidTextures.Contains(TextureNames[i].ToLower().ToCString()))
						ValidTextures.EmplaceBack(TextureNames[i].ToCString());

					i++;
				}
			}
		}
	}

	for (auto& ValidTexture : ValidTextures)
	{
		while (ValidTexture.find("\\") != -1)
			ValidTexture = ValidTexture.replace(ValidTexture.find("\\"), std::string("\\").length(), "/");

		qc.WriteFmt("$renamematerial \"%s\" \"%s\"\n", IO::Path::GetFileNameWithoutExtension(ValidTexture.c_str()).ToCString(), ValidTexture.c_str());
	}


	qc.Write("\n");

	qc.WriteFmt("$sequence \"ref\" \"%s_ref.smd\" \n\n", Model.get()->Name.ToCString());


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

		if(assetVersion < 16)
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
	qc.Write("\n");

	for (int i = 0; i < jigglebonecount; i++)
	{
		char* pBones = nullptr;
		if (assetVersion <= 10)
			pBones = rmdlBuf + hdr.boneindex + (hdr.numbones * sizeof(mstudiobonev54_t));
		else if (assetVersion > 16)
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

		if (assetVersion < 16)
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

		qc.WriteFmt("$hboxset \"%s\"\n\n", hitboxSetName);

		for (int j = 0; j < hitboxSet.numhitboxes; j++)
		{
			mstudiobboxv54_t hitbox{};

			char* pHitbox = nullptr;

			if (assetVersion < 16)
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

	qc.Close();
}
