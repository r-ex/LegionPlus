#pragma once

#include <Kore.h>
#include "RpakAssets.h"
#include <UIXListView.h>
#include <UIXButton.h>

class LegionTablePreview : public Forms::Form
{
public:
	LegionTablePreview();
	virtual ~LegionTablePreview() = default;

	bool bShowVectorColors = false;


	void SetDataTable(List<List<DataTableColumnData>> _DataTable)
	{
		DataTable = _DataTable;
		this->AssetsListView->SetVirtualListSize(DataTable.Count() - 1);
		auto First = _DataTable[0];

		for (DataTableColumnData col : First)
		{
			this->AssetsListView->Columns.Add({ col.stringValue, 751 / (int)First.Count() });
		}
	}

private:
	List<List<DataTableColumnData>> DataTable;
	UIX::UIXListView* AssetsListView;
	UIX::UIXButton* ToggleVectorColorsButton;


	// Internal routine to setup the component
	void InitializeComponent();
	static void GetVirtualItem(const std::unique_ptr<Forms::RetrieveVirtualItemEventArgs>& EventArgs, Forms::Control* Sender);
	static void ToggleShowVectorColors(Forms::Control* Sender);

};

