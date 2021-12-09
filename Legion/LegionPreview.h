#pragma once

#include <Kore.h>

class LegionPreview : public Forms::Form
{
public:
	LegionPreview();
	virtual ~LegionPreview() = default;

	// Assign the preview model
	void AssignPreviewModel(const Assets::Model& Model, const string& Name);
	// Assign the preview image
	void AssignPreviewImage(const Assets::Texture& Texture, const string& Name);

	// Sets the material preview loading routine
	void SetMaterialStreamer(Assets::AssetRenderer::MaterialStreamCallback Callback);

private:
	// Internal routine to setup the component
	void InitializeComponent();

	// Internal controls reference
	Assets::AssetRenderer* ModelPreview;
};