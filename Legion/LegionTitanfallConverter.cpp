#include "pch.h"
#include "LegionTitanfallConverter.h"
#include "ExportManager.h"

LegionTitanfallConverter::LegionTitanfallConverter()
	: Forms::Form()
{
	this->InitializeComponent();
}

void LegionTitanfallConverter::OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs)
{
	Form::OnPaint(EventArgs);
	
	Drawing::SolidBrush Brush(Drawing::Color::White);
	Drawing::Pen Pen(&Brush, 2);
	Pen.SetDashStyle(Gdiplus::DashStyle::DashStyleDash);

	Drawing::Rectangle Region(this->ClientRectangle());
	Region.Inflate(-20, -20);

	auto SmMode = EventArgs->Graphics->GetSmoothingMode();
	{

		EventArgs->Graphics->SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
		Drawing::DrawRoundRectangle(EventArgs->Graphics.get(), &Pen, Region, 6);
	}
	EventArgs->Graphics->SetSmoothingMode(SmMode);
	Gdiplus::Font d(L"Arial", 18);
	Drawing::Font ft(this->GetHandle(), d);
	Drawing::TextRenderer::DrawText(EventArgs->Graphics, "Drag and drop *.mdl files here to convert", ft, Region, Drawing::Color::White, Drawing::TextFormatFlags::VerticalCenter | Drawing::TextFormatFlags::HorizontalCenter | Drawing::TextFormatFlags::SingleLine);
}

void LegionTitanfallConverter::FormDragEnter(const std::unique_ptr<Forms::DragEventArgs>& EventArgs, Forms::Control* Sender)
{
	EventArgs->Effect = Forms::DragDropEffects::Copy;
}

void LegionTitanfallConverter::FormDragDrop(const std::unique_ptr<Forms::DragEventArgs>& EventArgs, Forms::Control* Sender)
{
	STGMEDIUM rSM{};
	FORMATETC rFmt{};
	rFmt.cfFormat = CF_HDROP;
	rFmt.dwAspect = DVASPECT_CONTENT;
	rFmt.tymed = TYMED_HGLOBAL;

	if (EventArgs->Data->GetData(&rFmt, &rSM) == S_OK)
	{
		auto FileCount = (int32_t)DragQueryFileA((HDROP)rSM.hGlobal, -1, 0, 0);
		List<string> Files;

		for (int32_t i = 0; i < FileCount; i++)
		{
			char Buffer[MAX_PATH]{};
			DragQueryFileA((HDROP)rSM.hGlobal, i, Buffer, sizeof(Buffer));

			Files.EmplaceBack(Buffer);
		}

		// Pass off to converter...
		((LegionTitanfallConverter*)Sender->FindForm())->HandleFileConvert(Files);
	}
}

void LegionTitanfallConverter::InitializeComponent()
{
	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText("Legion+ | Titanfall 2 Model & Animation Converter");
	this->SetClientSize({ 658, 410 });
	this->SetStartPosition(Forms::FormStartPosition::CenterParent);
	this->SetMinimizeBox(false);
	this->SetMaximizeBox(false);
	this->SetFormBorderStyle(Forms::FormBorderStyle::FixedSingle);

	this->ResumeLayout(false);
	this->PerformLayout();
	// END DESIGNER CODE

	this->SetBackColor({ 33, 33, 33 });

	this->DragEnter += &FormDragEnter;
	this->DragDrop += &FormDragDrop;
	this->SetAllowDrop(true);

	this->MdlFS = std::make_unique<MdlLib>();
}

void LegionTitanfallConverter::HandleFileConvert(List<string>& FilesToConvert)
{
	ExportManager::ExportMdlAssets(this->MdlFS, FilesToConvert);
}
