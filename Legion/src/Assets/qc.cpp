#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"

void RpakLib::ExportQC(int assetVersion, const string& Path, const string& modelPath, char* rmdlBuf, char* phyBuf)
{
	IO::StreamWriter qc(IO::File::Create(Path));
	if (assetVersion <= 10)
	{
		s3studiohdr_t* hdr = reinterpret_cast<s3studiohdr_t*>(rmdlBuf);

		qc.WriteFmt("$modelname \"%s\"\n", modelPath.ToCString());
		qc.WriteFmt("$cdmaterials \"\"\n");

		char* surfaceProp = reinterpret_cast<char*>(rmdlBuf + hdr->surfacepropindex);

		qc.WriteFmt("$surfaceprop \"%s\"\n", surfaceProp);

		qc.WriteFmt("$contents \"%s\"\n\n", (hdr->contents & 1) == 1 ? "solid" : "notsolid");

		if (hdr->flags & STUDIOHDR_FLAGS_FORCE_OPAQUE)
			qc.Write("$opaque\n\n");

		qc.WriteFmt("$eyeposition %f %f %f\n", hdr->eyeposition.X, hdr->eyeposition.Y, hdr->eyeposition.Z);
		qc.WriteFmt("$illumposition %f %f %f\n\n", hdr->illumposition.X, hdr->illumposition.Y, hdr->illumposition.Z);

		for (int i = 0; i < hdr->numbodyparts; ++i)
		{
			char* pBodyPart = rmdlBuf + hdr->bodypartindex + (i * sizeof(mstudiobodyparts_t));
			mstudiobodyparts_t* bodyPart = reinterpret_cast<mstudiobodyparts_t*>(pBodyPart);
			char* bodyPartName = reinterpret_cast<char*>(pBodyPart + bodyPart->sznameindex);

			qc.WriteFmt("$bodygroup \"%s\"\n{\n", bodyPartName);

			for (int j = 0; j < bodyPart->nummodels; ++j)
			{
				string baseModelName = IO::Path::GetFileNameWithoutExtension(modelPath);
				mstudiomodelv54_t model = *reinterpret_cast<mstudiomodelv54_t*>(pBodyPart + bodyPart->modelindex + (j * sizeof(mstudiomodelv54_t)));
				
				if (!*model.name)
					qc.Write("\tblank\n");
				else
					qc.WriteFmt("\tstudio \"%s_%s_%i_LOD0.smd\"\n", baseModelName.ToCString(), bodyPartName, j);
			
			}
			qc.Write("}\n\n");
		}

		qc.WriteFmt("$texturegroup \"skinfamilies\"\n{\n");
		for (int i = 0; i < hdr->numskinfamilies; ++i)
		{
			char* pSkinFamily = rmdlBuf + hdr->skinindex + (i * hdr->numskinref * sizeof(short));

			char* skinName = (char*)"default";

			if (i > 0)
			{
				int* pSkinNameIndex = reinterpret_cast<int*>(rmdlBuf + hdr->skinindex + (hdr->numskinfamilies * hdr->numskinref * sizeof(short)) + ((i-1) * sizeof(int)));
				skinName = rmdlBuf + *pSkinNameIndex;
			}

			qc.WriteFmt("\t\"%s\" { ", skinName);
			for (int j = 0; j < hdr->numskinref; ++j)
			{
				// texture index
				short texId = *reinterpret_cast<short*>(pSkinFamily + (j * sizeof(short)));

				// pointer to texture entry
				char* pTexture = rmdlBuf + hdr->textureindex + (texId * sizeof(mstudiotexturev54_t));

				// texture entry
				mstudiotexturev54_t texture = *reinterpret_cast<mstudiotexturev54_t*>(pTexture);

				// texture name
				char* textureName = pTexture + texture.sznameindex;

				qc.WriteFmt("\"%s\" ", textureName);
			}
			qc.Write("}\n");
		}
		qc.Write("}\n\n");

		if (hdr->numlocalattachments)
			qc.Write("// !!! attachment rotation angles may be wrong !!!\n");
		for (int i = 0; i < hdr->numlocalattachments; ++i)
		{
			// get attachment
			char* pAttachment = rmdlBuf + hdr->localattachmentindex + (i * sizeof(mstudioattachmentv54_t));
			mstudioattachmentv54_t* attachment = reinterpret_cast<mstudioattachmentv54_t*>(pAttachment);

			char* attachmentName = pAttachment + attachment->sznameindex;

			// get attachment's bone
			char* pBone = rmdlBuf + hdr->boneindex + (attachment->localbone * sizeof(mstudiobonev54_t));
			mstudiobonev54_t* bone = reinterpret_cast<mstudiobonev54_t*>(pBone);

			char* boneName = pBone + bone->sznameindex;

			Vector3 angles = attachment->localmatrix.GetRotationMatrixAsDegrees();

			qc.WriteFmt("$attachment \"%s\" \"%s\" %f %f %f rotate %f %f %f\n",
				attachmentName,
				boneName,
				attachment->localmatrix.c3r0, attachment->localmatrix.c3r1, attachment->localmatrix.c3r2,
				angles.X, angles.Y, angles.Z
			);
		}
		qc.Write("\n");

		for (int i = 0; i < hdr->numhitboxsets; ++i)
		{
			char* pHitboxSet = rmdlBuf + hdr->hitboxsetindex + (i * sizeof(mstudiohitboxset_t));
			mstudiohitboxset_t* hitboxSet = reinterpret_cast<mstudiohitboxset_t*>(pHitboxSet);

			// get hboxset name
			char* hitboxSetName = pHitboxSet + hitboxSet->sznameindex;

			qc.WriteFmt("$hboxset \"%s\"\n\n", hitboxSetName);

			for (int j = 0; j < hitboxSet->numhitboxes; ++j)
			{
				char* pHitbox = pHitboxSet + hitboxSet->hitboxindex + (j*sizeof(mstudiobboxv54_t));
				mstudiobboxv54_t* hitbox = reinterpret_cast<mstudiobboxv54_t*>(pHitbox);

				// get bone name
				char* pBone = rmdlBuf + hdr->boneindex + (hitbox->bone * sizeof(mstudiobonev54_t));
				mstudiobonev54_t* bone = reinterpret_cast<mstudiobonev54_t*>(pBone);
				char* boneName = pBone + bone->sznameindex;

				qc.WriteFmt("$hbox %i \"%s\" %f %f %f %f %f %f\n",
					hitbox->group,
					boneName,
					hitbox->bbmin.X, hitbox->bbmin.Y, hitbox->bbmin.Z,
					hitbox->bbmax.X, hitbox->bbmax.Y, hitbox->bbmax.Z
				);
			}
			qc.Write("\n");
		}
	}

	qc.Close();
}