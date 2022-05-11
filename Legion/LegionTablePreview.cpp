#include "pch.h"
#include "LegionTablePreview.h"
#include "Kore.h"
#include "UIXListView.h"
#include <sstream>

LegionTablePreview::LegionTablePreview()
{
	this->InitializeComponent();
}


void LegionTablePreview::InitializeComponent()
{
	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText("Table Asset Preview");
	this->SetClientSize({ 775, 481 });
	this->SetMinimumSize({ 791, 520 });
	this->SetStartPosition(Forms::FormStartPosition::CenterParent);

	this->SetBackColor({ 33, 33, 33 });

	this->ToggleVectorColorsButton = new UIX::UIXButton;
	this->ToggleVectorColorsButton->SetSize({ 120, 27 });
	this->ToggleVectorColorsButton->SetLocation({ 12, 442 });
	this->ToggleVectorColorsButton->SetTabIndex(2);
	this->ToggleVectorColorsButton->SetText("Toggle Vector Colors");
	this->ToggleVectorColorsButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->ToggleVectorColorsButton->Click += &ToggleShowVectorColors;
	this->AddControl(this->ToggleVectorColorsButton);

	this->AssetsListView = new UIX::UIXListView;
	this->AssetsListView->SetSize({ 751, 398 });
	this->AssetsListView->SetLocation({ 12, 38 });
	this->AssetsListView->SetTabIndex(0);
	this->AssetsListView->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left | Forms::AnchorStyles::Right);
	this->AssetsListView->SetView(Forms::View::Details);
	this->AssetsListView->SetVirtualMode(true);
	this->AssetsListView->SetFullRowSelect(true);
	this->AddControl(AssetsListView);

	this->Resize += OnResized;


	//this->AssetsListView->VirtualItemsSelectionRangeChanged += &OnSelectedIndicesChanged;
	/*this->AssetsListView->DoubleClick += &OnListDoubleClick;
	this->AssetsListView->KeyUp += &OnListKeyUp;
	this->AssetsListView->KeyPress += &OnListKeyPressed;*/
	//this->AddControl(this->AssetsListView);


	this->ResumeLayout(false);
	this->PerformLayout();


	this->AssetsListView->RetrieveVirtualItem += &GetVirtualItem;
}


void LegionTablePreview::GetVirtualItem(const std::unique_ptr<Forms::RetrieveVirtualItemEventArgs>& EventArgs, Forms::Control* Sender)
{
	auto ThisPtr = (LegionTablePreview*)Sender->FindForm();

	if (EventArgs->ItemIndex % 2 == 0) {
		EventArgs->Style.BackColor = Sender->BackColor();
		EventArgs->Style.ForeColor = { 255, 255, 255 };
	}
	else {
		EventArgs->Style.BackColor = { static_cast<BYTE>(Sender->BackColor().GetR() - 20), static_cast<BYTE>(Sender->BackColor().GetG() - 20), static_cast<BYTE>(Sender->BackColor().GetB() - 20)};
		EventArgs->Style.ForeColor = { 200, 200, 200 };
	}


	std::stringstream CellValue;
	DataTableColumnData Cell = ThisPtr->DataTable[EventArgs->ItemIndex + 1][EventArgs->SubItemIndex];

	switch (Cell.Type)
	{
	case DataTableColumnDataType::Int: {
		CellValue << "" << Cell.iValue;
		break;
	}

	case DataTableColumnDataType::Bool: {
		CellValue << "" << Cell.bValue ? "True" : "False";
		break;
	}

	case DataTableColumnDataType::Float: {
		CellValue << Cell.fValue << "f";
		break;
	}

	case DataTableColumnDataType::Vector: {
		CellValue << "" << "X=" << Cell.vValue.X << " Y=" << Cell.vValue.Y << " Z=" << Cell.vValue.Z;
		if (ThisPtr->bShowVectorColors)
		{
			EventArgs->Style.BackColor = { static_cast<BYTE>(Cell.vValue.X), static_cast<BYTE>(Cell.vValue.Y), static_cast<BYTE>(Cell.vValue.Z) };
			EventArgs->Style.ForeColor = { static_cast<BYTE>(255 - Cell.vValue.X), static_cast<BYTE>(255 - Cell.vValue.Y), static_cast<BYTE>(255 - Cell.vValue.Z) };
		}
			
		break;
	}

	case DataTableColumnDataType::Asset: {
		CellValue << "$" << "\"" << Cell.assetValue << "\"";
		break;
	}

	case DataTableColumnDataType::AssetNoPrecache: {
		CellValue << "$" << "\"" << Cell.assetNPValue << "\"";
		break;
	}

	case DataTableColumnDataType::StringT: {
		CellValue << "" << "\"" << Cell.stringValue << "\"";
		break;
	}

	}
	EventArgs->Text = CellValue.str();

}

void LegionTablePreview::ToggleShowVectorColors(Forms::Control* Sender)
{
	auto ThisPtr = (LegionTablePreview*)Sender->FindForm();
	ThisPtr->bShowVectorColors = !(ThisPtr->bShowVectorColors);
	ThisPtr->Refresh();
}

void LegionTablePreview::ResizeTableToWindow()
{
	if (DataTable.Count() > 0)
	{
		auto First = DataTable[0];
		for (auto col : this->AssetsListView->Columns)
		{
			col.SetWidth(this->Size().Width / (int)First.Count());
		}
	}
}


void LegionTablePreview::OnResized(Forms::Control* Sender)
{
	auto ThisPtr = (LegionTablePreview*)Sender->FindForm();
	ThisPtr->ResizeTableToWindow();
}