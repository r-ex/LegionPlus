#include "pch.h"
#include "RpakLib.h"
#include "Path.h"
#include "Directory.h"
#include <rtech.h>

void RpakLib::BuildUIImageAtlasInfo(const RpakLoadAsset& Asset, ApexAsset& Info)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	UIAtlasHeader Header = Reader.Read<UIAtlasHeader>();

	string AssetName = string::Format("atlas_0x%llx", Asset.NameHash);
	string TextureName = "";
	this->ExtractTextureName(Assets[Header.TextureGuid], TextureName);

	if (TextureName.Length() > 0)
	{
		string OriginalAtlasPath = "ui_image_atlas/" + TextureName + ".rpak";
		if (Asset.NameHash == RTech::StringToGuid(OriginalAtlasPath.ToCString()))
			AssetName = OriginalAtlasPath;
	}

	Info.Name = ExportManager::Config.GetBool("UseFullPaths") ? AssetName : IO::Path::GetFileNameWithoutExtension(AssetName);;
	Info.Type = ApexAssetType::UIImageAtlas;
	Info.Status = ApexAssetStatus::Loaded;
	Info.Info = string::Format("Textures: %i", Header.TexturesCount);
}

void RpakLib::ExportUIImageAtlas(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	UIAtlasHeader Header = Reader.Read<UIAtlasHeader>();

	if (!Assets.ContainsKey(Header.TextureGuid)) // can't get the images without texture data so let's head out
		return;

	string AtlasPath = IO::Path::Combine(Path, string::Format("0x%llx", Asset.NameHash));

	string TextureName = "";
	this->ExtractTextureName(Assets[Header.TextureGuid], TextureName);

	if (TextureName.Length() > 0)
	{
		string OriginalAtlasPath = "ui_image_atlas/" + TextureName + ".rpak";
		if (Asset.NameHash == RTech::StringToGuid(OriginalAtlasPath.ToCString()))
			AtlasPath = IO::Path::Combine(Path, IO::Path::GetFileName(TextureName));
	}

	if (!IO::Directory::Exists(AtlasPath))
		IO::Directory::CreateDirectory(AtlasPath);

	this->ExtractUIImageAtlas(Asset, AtlasPath);
}

void RpakLib::ExtractUIImageAtlas(const RpakLoadAsset& Asset, const string& Path)
{
	auto RpakStream = this->GetFileStream(Asset);
	IO::BinaryReader Reader = IO::BinaryReader(RpakStream.get(), true);

	if (Asset.RawDataIndex == -1) // no uvs - we shouldn't be able to get to this point
		return;

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.SubHeaderIndex, Asset.SubHeaderOffset));

	UIAtlasHeader Header = Reader.Read<UIAtlasHeader>();

	RpakStream->SetPosition(this->GetFileOffset(Asset, Asset.RawDataIndex, Asset.RawDataOffset));

	List<UIAtlasImage> UIAtlasImages(Header.TexturesCount, true);

	for (int i = 0; i < Header.TexturesCount; ++i)
	{
		UIAtlasUV uvs = Reader.Read<UIAtlasUV>();
		UIAtlasImages[i].uvs = uvs;
		UIAtlasImages[i].PosX = uvs.uv0x * Header.Width;
		UIAtlasImages[i].PosY = uvs.uv0y * Header.Height;
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, Header.TextureOffsetsIndex, Header.TextureOffsetsOffset));

	for (int i = 0; i < Header.TexturesCount; ++i)
	{
		UIAtlasOffset offset = Reader.Read<UIAtlasOffset>();
		UIAtlasImages[i].offsets = offset;
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, Header.TextureDimsIndex, Header.TextureDimsOffset));

	for (int i = 0; i < Header.TexturesCount; ++i)
	{
		UIAtlasOffset offsets = UIAtlasImages[i].offsets;
		UIAtlasUV uvs = UIAtlasImages[i].uvs;

		UIAtlasImages[i].Width = Reader.Read<uint16_t>();
		UIAtlasImages[i].Height = Reader.Read<uint16_t>();

		// ya
		if (offsets.endX < 1 && uvs.uv1x != 0)
			UIAtlasImages[i].Width = Header.Width * uvs.uv1x;
		if (offsets.endY < 1 && uvs.uv1y != 0)
			UIAtlasImages[i].Height = Header.Height * uvs.uv1y;
	}

	RpakStream->SetPosition(this->GetFileOffset(Asset, Header.TextureHashesIndex, Header.TextureHashesOffset));

	for (int i = 0; i < Header.TexturesCount; ++i)
	{
		UIAtlasImages[i].Hash = Reader.Read<uint32_t>();
		UIAtlasImages[i].PathTableOffset = Reader.Read<uint32_t>(); // this got changed to uint64_t at some point after s3
	}

	if (Header.TextureNamesIndex != 0 || Header.TextureNamesOffset != 0)
	{
		RpakStream->SetPosition(this->GetFileOffset(Asset, Header.TextureNamesIndex, Header.TextureNamesOffset));
		for (int i = 0; i < Header.TexturesCount; ++i)
		{
			UIAtlasImages[i].Path = Reader.ReadCString();
		}
	}

	std::unique_ptr<Assets::Texture> Texture = nullptr;
	string Name;

	this->ExtractTexture(Assets[Header.TextureGuid], Texture, Name);
	Texture->ConvertToFormat(DirectX::IsSRGB(Texture->Format()) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);

	for (auto& img : UIAtlasImages)
	{
		std::unique_ptr<Assets::Texture> tmp = std::make_unique<Assets::Texture>(img.Width, img.Height, Texture.get()->Format());
		uint32_t BytesPerPixel = tmp->GetBpp() / 8;

		DirectX::Rect srcRect{ img.PosX, img.PosY, img.Width, img.Height };

		tmp->CopyTextureSlice(Texture, srcRect, 0, 0);

		string ImageName = string::Format("0x%x%s", img.Hash, (const char*)ImageExtension);

		if (img.Path.Length() > 0)
			ImageName = string::Format("%s%s", IO::Path::GetFileNameWithoutExtension(img.Path).ToCString(), (const char*)ImageExtension);

		tmp->Save(IO::Path::Combine(Path, ImageName), ImageSaveType);
	}
}