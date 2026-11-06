/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "cctooltip.h"

#include "_map.h"
#include "_rect.h"
#include "_surface.h"
#include "_xmouse.h"
#include "dialog.h"
#include "dsurface.h"
#include "goptions.h"
#include "scheme.h"
#include "wwfont.h"
#include "bitfont.h" // RA2: text is drawn with BitFont
#include "bittext.h" // RA2: and BitText

extern RGBClass TooltipColor; // RA2: the outline colour of a tooltip

#include "color.hh"


/// <summary>
/// Prepares a tooltip for display.
/// This routine measures the text, word wraps it to fit whichever region it will appear
/// over -- the tactical map or the sidebar -- and then nudges the box so that it stays
/// within that region rather than hanging off the edge of it.
/// </summary>
/// <returns>bool; Is the tooltip fit to be shown? A hidden mouse suppresses it.</returns>
bool CCToolTip::Update(ToolTipText * text) // RA2: measures with BitFont; places an anchored tooltip at its region
{
	if (Get_Mouse_State() >= 0) {
		BitFont *font = BitFont::Instance;
		Rect *trect;
		POINT &position = text->Pos;
		if (Options.IsSidebarOnRight == true) {
			if (position.x <= TacticalRect.X + TacticalRect.Width) {
				trect = &TacticalRect;
			} else {
				trect = &SidebarRect;
				Map.SidebarClass::IsToRedraw = true;
			}
		} else {
			if (position.x <= SidebarRect.X + SidebarRect.Width) {
				trect = &SidebarRect;
				Map.SidebarClass::IsToRedraw = true;
			} else {
				trect = &TacticalRect;
			}
		}

		if (trect) {
			int width;
			int height;
			font->Text_Dimensions(text->Text, &width, &height, trect->Width);
			width += 4;
			height += 3;
			text->TextWidth = max(width, text->TextWidth);
			text->TextHeight = max(height, text->TextHeight);
			if (text->TextWidth >= trect->Width) {
				font->Text_Dimensions(text->Text, &width, &height, trect->Width - 4);
				width += 4;
				height += 3;
				text->TextWidth = max(width, text->TextWidth);
				text->TextHeight = max(height, text->TextHeight);
			}
			if (CurrentToolTip && CurrentToolTip->IsAnchored) {
				Rect owner = CurrentToolTip->Region;
				if (owner.X < trect->X + trect->Width / 2) {
					position.x = owner.X + owner.Width - 5;
				} else {
					position.x = owner.X - width + 5;
				}
				position.x = MAX(trect->X, position.x);
				if (owner.Y < trect->Y + trect->Height / 2) {
					position.y = owner.Y + owner.Height - 5;
				} else {
					position.y = owner.Y - height + 5;
				}
				position.y = MAX(trect->Y, position.y);
			} else {
				int x = position.x + text->TextWidth - trect->Width - trect->X;
				if (x > 0) {
					position.x -= x;
				}

				position.y += 16;
				if (position.y + text->TextHeight - trect->Height - trect->Y > 0) {
					position.y = position.y - text->TextHeight - 16;
				}
			}
		}
		return(true);
	}

	return(false);
}


/// <summary>
/// Takes down the tooltip that is being displayed.
/// This routine flags whatever the tooltip was covering for a redraw, so that the map or
/// the sidebar paints back over the hole it leaves, and then lets the tooltip manager
/// forget it.
/// </summary>
void CCToolTip::Reset(const ToolTipText * text)
{
	bool redraw = false;
	if (Options.IsSidebarOnRight == true) {
		if (text->Pos.x >= TacticalRect.X + TacticalRect.Width) {
			redraw = true;
		}
	} else {
		if (text->Pos.x <= SidebarRect.X + SidebarRect.Width) {
			redraw = true;
		}
	}

	if (redraw) {
		Map.SidebarClass::IsToRedraw = true;
		Map.SidebarClass::IsForceCompleteRedraw = true;
	}
	Map.Flag_To_Redraw();
	BASECLASS::Reset(text);
}


/// <summary>
/// Draws the tooltip that is currently active.
/// This routine records which surface the draw is destined for before handing the work
/// over to the tooltip manager.
/// </summary>
/// <param name="sidebar">Is the sidebar surface the one being drawn to?</param>
void CCToolTip::Draw_Current(bool sidebar)
{
	UseSidebarSurface = sidebar;
	BASECLASS::Draw_Current(sidebar == 0);
}


/// <summary>
/// Draws the tooltip box and its text.
/// This routine works out which of the game's surfaces the tooltip falls upon -- the
/// tactical composite or the sidebar -- and paints the frame and the text there. A
/// tooltip that lands across the boundary between the two is not drawn at all.
/// </summary>
void CCToolTip::Draw(ToolTipText const * text) // RA2: draws a framed box and the text with BitText
{
	Rect drawrect;
	Point2D point(text->Pos.x, text->Pos.y);
	Surface * surface = NULL;
	if (Options.IsSidebarOnRight == true) {
		int offset = TacticalRect.X + TacticalRect.Width;
		if (point.X + text->TextWidth <= offset) {
			surface = CompositeSurface;
		} else if (UseSidebarSurface == true && point.X >= offset) {
			surface = SidebarSurface;
			point.X -= offset;
			Map.IsToBlitSidebar = true;
		}
	} else {
		int offset = SidebarRect.X + SidebarRect.Width;
		if (point.X >= offset) {
			surface = CompositeSurface;
			point.X -= offset;
		} else if (UseSidebarSurface == true && point.X + text->TextWidth < offset) {
			surface = SidebarSurface;
			Map.IsToBlitSidebar = true;
		}
	}
	if (surface) {
		LTRBStruct bounds;
		bounds.Right = bounds.Left = point.X;
		bounds.Bottom = bounds.Top = point.Y;
		BitFont * font = BitFont::Instance;
		int textwidth;
		int textheight;
		font->Text_Dimensions(text->Text, &textwidth, &textheight, surface->Get_Width());
		bounds.Right = bounds.Left + textwidth;
		bounds.Bottom = bounds.Top + textheight;
		int width = bounds.Right - bounds.Left + 8;
		int height = bounds.Bottom - bounds.Top + 4;
		if (bounds.Left + width > surface->Get_Width()) bounds.Left = surface->Get_Width() - width;
		bounds.Right = bounds.Left + width;
		bounds.Bottom = bounds.Top + height;
		int color = DSurface::Build_Hicolor_Pixel(TooltipColor);
		drawrect.Set(bounds.Left, bounds.Top, width, height);
		surface->Fill_Rect(drawrect, TBLACK);
		surface->Draw_Rect(drawrect, color);
		font->Set_Clip(true);
		font->Set_Bounds(&bounds);
		font->Set_Color(color);
		BitText::Instance->Draw_Text(font, surface, text->Text, bounds.Left + 4, bounds.Top + 2,
			bounds.Right - bounds.Left, bounds.Bottom - bounds.Top, 0, 0, 0);
	}
}


/// <summary>
/// Fetches the text to display for a tooltip.
/// This routine is called by the tooltip manager when a region becomes due for display.
/// The game map supplies the string, and nothing is offered while the mouse is hidden.
/// </summary>
/// <returns>Returns with a pointer to the help text, or NULL if there is none.</returns>
const char * CCToolTip::ToolTip_Text(int id)
{
	if (Get_Mouse_State() < 0) {
		return(NULL);
	}
	return(Map.Help_Text(id));
}
