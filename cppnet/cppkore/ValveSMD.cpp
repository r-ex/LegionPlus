#include "stdafx.h"
#include "ValveSMD.h"

#include "File.h"
#include "Path.h"
#include "MathHelper.h"
#include "StreamWriter.h"
#include <vector>

namespace Assets::Exporters
{
	void ProcessVertex(IO::StreamWriter& Writer, const Vertex& Vertex)
	{
		auto Normal = Vertex.Normal().GetNormalized();
		const Vector3& Position = Vertex.Position();
		const Vector2& UVLayer = Vertex.UVLayers(0);

		Writer.WriteFmt("\t0 %f %f %f %f %f %f %f %f %d ", Position.X, Position.Y, Position.Z, Normal.X, Normal.Y, Normal.Z, UVLayer.U, (1 - UVLayer.V), Vertex.WeightCount());

		for (uint8_t i = 0; i < Vertex.WeightCount(); i++)
			Writer.WriteFmt("%d %f ", Vertex.Weights(i).Bone, Vertex.Weights(i).Value);

		Writer.Write("\n");
	}

	void GetBoneAnimation(int frame, List <Assets::Curve>& Curves, Vector3& Pos, Vector3& Rot)
	{
		for (Assets::CurveKeyframe& KeyFrame : Curves[0].Keyframes)
		{
			if (KeyFrame.Frame.Integer32 == frame)
				Rot = KeyFrame.Value.Vector4.ToEulerAngles();
		}

		for (Assets::CurveKeyframe& KeyFrame : Curves[1].Keyframes)
		{
			if (KeyFrame.Frame.Integer32 == frame)
				Pos.X = KeyFrame.Value.Float;
		}

		for (Assets::CurveKeyframe& KeyFrame : Curves[2].Keyframes)
		{
			if (KeyFrame.Frame.Integer32 == frame)
				Pos.Y = KeyFrame.Value.Float;
		}

		for (Assets::CurveKeyframe& KeyFrame : Curves[3].Keyframes)
		{
			if (KeyFrame.Frame.Integer32 == frame)
				Pos.Z = KeyFrame.Value.Float;
		}
	}

	// Find Last Parent Bone Frame Before Bone Position
	bool GetParentBoneAnimation(int frame, List <Assets::Curve>& Curves, Vector3& Pos)
	{
		
		int MaxXIndex = -1;
		auto& XKeyFrames = Curves[1].Keyframes;
		for (int i = 0; i < XKeyFrames.Count(); i++)
		{
			if (XKeyFrames[i].Frame.Integer32 < frame)
				MaxXIndex = i;
		}

		int MaxYIndex = -1;
		auto& YKeyFrames = Curves[2].Keyframes;
		for (int i = 0; i < YKeyFrames.Count(); i++)
		{
			if (YKeyFrames[i].Frame.Integer32 < frame)
				MaxYIndex = i;
		}

		int MaxZIndex = -1;
		auto& ZKeyFrames = Curves[3].Keyframes;
		for (int i = 0; i < ZKeyFrames.Count(); i++)
		{
			if (ZKeyFrames[i].Frame.Integer32 < frame)
				MaxZIndex = i;
		}

		if (MaxXIndex == -1 && MaxYIndex == -1 && MaxZIndex == -1)
			return false;

		if (MaxXIndex < XKeyFrames.Count())
			Pos.X = XKeyFrames[MaxXIndex].Value.Float;

		if (MaxYIndex < YKeyFrames.Count())
			Pos.X = YKeyFrames[MaxYIndex].Value.Float;

		if (MaxZIndex < ZKeyFrames.Count())
			Pos.Z = ZKeyFrames[MaxZIndex].Value.Float;

		return true;
	}

	bool ValveSMD::ExportAnimation(const Animation& Animation, const string& Path)
	{
		auto Writer = IO::StreamWriter(IO::File::Create(Path));

		Writer.WriteLine(
			"version 1\n"
			"nodes"
		);

		uint32_t BoneIndex = 0;

		for (Assets::Bone& Bone : Animation.Bones)
		{
			Writer.WriteLineFmt("\t%d \"%s\" %d", BoneIndex, (char*)Bone.Name(), Bone.Parent());
			BoneIndex++;
		}

		Writer.Write(
			"end\n"
			"skeleton\n"
		);

		for (int i = 0; i < Animation.FrameCount(); i++)
		{
			Writer.WriteLineFmt("time %d", i);

			for (int j = 0; j < Animation.Bones.Count(); j++)
			{
				Assets::Bone& Bone = Animation.Bones[j];
				auto& Curves = Animation.Curves[Bone.Name()];

				if (!Animation.Curves.ContainsKey(Bone.Name()))
					continue;

				Vector3 Pos{}, Rot{};

				GetBoneAnimation(i, Curves, Pos, Rot);

				if (Pos == Vector3(0, 0, 0) && Rot == Vector3(0, 0, 0))
					continue;

				// needs to be fixed later
				if (Bone.Parent() > -1)
				{
					Assets::Bone& pBone = Animation.Bones[j];
					auto& pCurves = Animation.Curves[pBone.Name()];

					if (!Animation.Curves.ContainsKey(pBone.Name()))
						break;

					Vector3 pPos{};
					if (GetParentBoneAnimation(i, pCurves, pPos))
					     Pos = pPos - Pos;
					else Pos = Bone.GlobalPosition() - Pos;

				} else Pos = Bone.GlobalPosition() - Pos;

				Writer.WriteLineFmt("\t%d %f %f %f %f %f %f", j, Pos.X, Pos.Y, Pos.Z, MathHelper::DegreesToRadians(Rot.X), MathHelper::DegreesToRadians(Rot.Y), MathHelper::DegreesToRadians(Rot.Z));
			}
		}

		Writer.WriteLine("end");

		Writer.Close();

		return false;
	}

	void WriteSubMesh(IO::StreamWriter& Writer, const Model& Model, const List<int>& MeshIds, const string& Path)
	{
		Writer.WriteLine(
			"version 1\n"
			"nodes"
		);

		uint32_t BoneIndex = 0;

		for (Assets::Bone& Bone : Model.Bones)
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

		for (Assets::Bone& Bone : Model.Bones)
		{
			Vector3 Euler = Bone.LocalRotation().ToEulerAngles();

			Writer.WriteLineFmt("  %d %f %f %f %f %f %f", BoneIndex, Bone.LocalPosition().X, Bone.LocalPosition().Y, Bone.LocalPosition().Z, MathHelper::DegreesToRadians(Euler.X), MathHelper::DegreesToRadians(Euler.Y), MathHelper::DegreesToRadians(Euler.Z));
			BoneIndex++;
		}

		Writer.WriteLine("end");

		Writer.WriteLine("triangles");

		for (auto& SubmeshId : MeshIds)
		{
			Assets::Mesh& Submesh = Model.Meshes[SubmeshId];

			for (auto& Face : Submesh.Faces)
			{
				if (Submesh.MaterialIndices[0] > -1)
					Writer.WriteLine((Model.Materials[Submesh.MaterialIndices[0]].Name + "_sknp"));
				else
					Writer.WriteLine("default_material");

				ProcessVertex(Writer, Submesh.Vertices[Face[2]]);
				ProcessVertex(Writer, Submesh.Vertices[Face[1]]);
				ProcessVertex(Writer, Submesh.Vertices[Face[0]]);
			}
		}

		Writer.WriteLine("end");
	}

	bool ValveSMD::ExportModel(const Model& Model, const string& Path)
	{
		int bodypart_index = 0;

		for (auto& Submesh : Model.BodyPartNames)
		{
			auto& MeshIds = Model.BodyPartMeshIds[bodypart_index];

			string NewPath = IO::Path::Combine(IO::Path::GetDirectoryName(Path), Submesh) + ModelExtension().ToCString();

			auto Writer = IO::StreamWriter(IO::File::Create(NewPath));

			WriteSubMesh(Writer, Model, MeshIds, NewPath);

			bodypart_index++;
		}

		return true;
	}

	imstring ValveSMD::ModelExtension()
	{
		return ".smd";
	}

	imstring ValveSMD::AnimationExtension()
	{
		return ".smd";
	}

	ExporterScale ValveSMD::ExportScale()
	{
		// Evaluate whether or not SMDs need a scale constant.
		return ExporterScale::Default;
	}

	bool ValveSMD::SupportsAnimations()
	{
		return true;
	}

	bool ValveSMD::SupportsModels()
	{
		return true;
	}
}