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
			{
				Rot = KeyFrame.Value.Vector4.ToEulerAngles();
				break;
			}
		}

		for (Assets::CurveKeyframe& KeyFrame : Curves[1].Keyframes)
		{
			if (KeyFrame.Frame.Integer32 == frame)
			{
				Pos.X = KeyFrame.Value.Float;
				break;
			}
		}

		for (Assets::CurveKeyframe& KeyFrame : Curves[2].Keyframes)
		{
			if (KeyFrame.Frame.Integer32 == frame)
			{
				Pos.Y = KeyFrame.Value.Float;
				break;
			}
		}

		for (Assets::CurveKeyframe& KeyFrame : Curves[3].Keyframes)
		{
			if (KeyFrame.Frame.Integer32 == frame)
			{
				Pos.Z = KeyFrame.Value.Float;
				break;
			}
		}
	}

	bool ValveSMD::ExportAnimation(const Animation& Animation, const string& Path)
	{
		IO::StreamWriter Writer = IO::StreamWriter(IO::File::Create(Path));

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

		std::vector<bool> UsedFirstLocalPos(Animation.Bones.Count());

		for (int i = 0; i < Animation.FrameCount(); i++)
		{
			Writer.WriteLineFmt("time %d", i);

			UsedFirstLocalPos.clear();
			UsedFirstLocalPos.resize(Animation.Bones.Count());

			for (int j = 0; j < Animation.Bones.Count(); j++)
			{
				Assets::Bone& Bone = Animation.Bones[j];
				auto& Curves = Animation.Curves[Bone.Name()];

				Vector3 Pos{}, Rot{};
				GetBoneAnimation(i, Curves, Pos, Rot);

				if (Pos == Vector3(0, 0, 0) && !UsedFirstLocalPos[j])
				{
					Pos = Bone.LocalPosition();

					if (Rot == Vector3(0, 0, 0))
						Rot = Bone.LocalRotation().ToEulerAngles();

					UsedFirstLocalPos[j] = true;
				}

				if (Pos == Vector3(0, 0, 0) && Rot == Vector3(0, 0, 0))
					continue;

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
					Writer.WriteLine((Model.Materials[Submesh.MaterialIndices[0]].Name));
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
		for (auto& Bodypart : Model.BodyParts)
		{
			string NewPath = IO::Path::Combine(IO::Path::GetDirectoryName(Path), Bodypart.Name) + ModelExtension().ToCString();

			if (Bodypart.NumModels >= 3)
			{
				for (int i = 0; i < Bodypart.MeshIndexes.Count(); i++)
				{
					NewPath = IO::Path::Combine(IO::Path::GetDirectoryName(Path), Bodypart.Name) + (string::Format("_%d", i).ToCString()) + ModelExtension().ToCString();

					Assets::MatIndexBuffer NewMeshIds{};
					NewMeshIds.EmplaceBack(Bodypart.MeshIndexes[i]);

					IO::StreamWriter Writer = IO::StreamWriter(IO::File::Create(NewPath));
					WriteSubMesh(Writer, Model, NewMeshIds, NewPath);
				}
			}
			else
			{
				IO::StreamWriter Writer = IO::StreamWriter(IO::File::Create(NewPath));
				WriteSubMesh(Writer, Model, Bodypart.MeshIndexes, NewPath);
			}

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