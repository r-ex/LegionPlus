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
		this->DataTable = _DataTable;
		this->AssetsListView->SetVirtualListSize(DataTable.Count() - 1);
		auto& First = DataTable[0];


		/*this->DataTableColumnWeight.Clear();
		this->DataTableColumnWeight.AddRange(0, DataTable.Count() - 1);*/

		/*for (int i = 1; i < this->DataTable.Count(); i++)
		{
			for (int j = 0; j < this->DataTable[i].Count(); j++)
			{
				DataTableColumnWeight[j] += DataTable[i][j].stringValue.Length() + std::to_string(DataTable[i][j].fValue).Length()
			}
		}*/


		for (DataTableColumnData& col : First)
		{
			this->AssetsListView->Columns.Add({ col.stringValue, this->Size().Width / (int)First.Count()});
		}
	}
	void ResizeTableToWindow();

private:
	List<List<DataTableColumnData>> DataTable;
	List<float> DataTableColumnWeight;

	UIX::UIXListView* AssetsListView;
	UIX::UIXButton* ToggleVectorColorsButton;


	// Internal routine to setup the component
	void InitializeComponent();
	static void GetVirtualItem(const std::unique_ptr<Forms::RetrieveVirtualItemEventArgs>& EventArgs, Forms::Control* Sender);
	static void ToggleShowVectorColors(Forms::Control* Sender);
	static void OnResized(Forms::Control* Sender);

};

