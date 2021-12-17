#pragma once

#include <Kore.h>
#include "UIXLabel.h"
#include "UIXGroupBox.h"

class LegionAbout : public Forms::Form
{
public:
	LegionAbout();
	virtual ~LegionAbout() = default;

private:
	// Internal routine to setup the component
	void InitializeComponent();

	UIX::UIXGroupBox* aboutGroupBox;
	UIX::UIXLabel* aboutTextLabel;
};

