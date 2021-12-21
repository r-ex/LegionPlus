#pragma once

#include <Kore.h>
#include "UIXButton.h"
#include "UIXLabel.h"
#include "UIXCheckBox.h"
#include "UIXComboBox.h"
#include "UIXGroupBox.h"
#include "UIXRadioButton.h"
#include "MdlLib.h"

class LegionTitanfallConverter : public Forms::Form
{
public:
	LegionTitanfallConverter();
	virtual ~LegionTitanfallConverter() = default;

	virtual void OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs);

	static void FormDragEnter(const std::unique_ptr<Forms::DragEventArgs>& EventArgs, Forms::Control* Sender);
	static void FormDragDrop(const std::unique_ptr<Forms::DragEventArgs>& EventArgs, Forms::Control* Sender);

private:
	// Internal routine to setup the component
	void InitializeComponent();
	// Internal routine to convert the files
	void HandleFileConvert(List<string>& FilesToConvert);

	// Internal vpk system
	std::unique_ptr<MdlLib> VpkFileSystem;
};
