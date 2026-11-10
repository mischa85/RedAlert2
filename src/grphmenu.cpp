/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "grphmenu.h"

#include "_keyboar.h"
#include "_surface.h"
#include "ccfile.h"
#include "globals.h"
#include "grphmitm.h"
#include "ini.h"
#include "keyboard.h"
#include "misc.h" // RA2: the Bink rect is the video mode's size
#include "msanim.h"
#include "ownrdraw.h"
#include "theme.h"
#include "wstring.h" // RA2: names and file names are Wstrings

GraphicMenu * _Graphic_Menu(INIClass const & ini, const char * name);
GraphicMenuItem * GM_Create_Item_From_INI(Wstring const & name, INIClass const & ini, MSEngine & engine, Point2D & image_size); // RA2: Wstring name


/// <summary>
/// Creates a graphic menu described by an INI file.
/// Use this routine to build one of the shell menu pages. The file is fetched through
/// the mix file system and the named section within it describes the page. The menu is
/// only built here -- the caller must run it and dispose of it.
/// </summary>
/// <param name="ini">The name of the INI file that holds the menu descriptions.</param>
/// <param name="name">The name of the section that describes this menu.</param>
/// <returns>Returns with a pointer to the menu built, or NULL if the file is missing or
/// unreadable.</returns>
GraphicMenu * __fastcall Do_Graphic_Menu(Wstring const & ini, Wstring const & name) // RA2: __fastcall, the file and menu names as Wstrings
{
	CCFileClass file(ini);
	if (!file.Is_Available()) {
		return(NULL);
	}
	INIClass _ini;
	GraphicMenu * menu = _ini.Load(file) > 0 ? _Graphic_Menu(_ini, name) : NULL;
	return(menu);
}


/// <summary>
/// Creates a graphic menu from an INI section.
/// This routine builds the menu, gives it whatever backdrop animation and music theme
/// the section calls for, then asks the item factory to create each listed item in
/// turn. A missing backdrop animation is not fatal -- the menu simply has none.
/// </summary>
/// <param name="ini">The database to read the menu description from.</param>
/// <param name="name">The name of the section that describes this menu.</param>
/// <returns>Returns with a pointer to the menu built, or NULL if it could not be
/// allocated.</returns>
GraphicMenu * __fastcall _Graphic_Menu(INIClass & ini, Wstring const & name) // RA2: __fastcall, a non-const INI and a Wstring name
{
	if (!ini.Section_Present(name.text())) { // RA2: the section must exist
		return(NULL); // RA2: no menu without it
	}
	GraphicMenu * menu = new GraphicMenu;
	if (menu == NULL) {
		return(NULL);
	}

	char entry[8]; // RA2: the entry buffer is declared first
	Wstring buffer; // RA2: the INI string is a Wstring
	bool has_background = ini.Get_String(name, "Background", "", buffer) > 0; // RA2: read into that Wstring
	Wstring pcx = buffer + ".PCX"; // RA2: name + .PCX
	menu->BackgroundName = pcx; // RA2: kept as the background name

	Point2D pt(0,0);

	if (has_background) {
		Wstring vqa = buffer + ".VQA"; // RA2: name + .VQA
		MSAnim * anim = NULL;
		if (CCFileClass(vqa).Is_Available(false)) { // RA2: Is_Available(false)
			anim = new MSVQAnim(vqa, AlternateSurface, menu->Engine.Get_Anims(), true); // RA2: the VQA by name
		}
		if (anim == NULL) {
			Wstring bik = buffer + ".BIK"; // RA2: without a VQA, name + .BIK
			if (CCFileClass(bik).Is_Available(false)) { // RA2: Is_Available(false)
				Rect rect(0, 0, VideoModeWidth, VideoModeHeight); // RA2: a full-screen rect
				anim = new MSBinkAnim(bik, AlternateSurface, menu->Engine.Get_Anims(), rect, true); // RA2: a Bink animation
			}
		}
		if (anim == NULL) { // RA2: the PCX only when there is neither
			anim = new MSPCXAnim(pcx.get(), menu->Engine.Get_Anims(), true); // RA2: from the PCX name
		}

		menu->Set_Animation(anim);
		pt = anim->Get_Rect().TopLeft;
	}

	if (ini.Get_String(name, "Theme", "", buffer) > 0) { // RA2: read into the Wstring
		menu->Set_Theme_Name(buffer);
	}

	int item_max = ini.Get_Int(name, "ItemMax", 100);
	for (int i = 0; i <= item_max; i++) {
		sprintf(entry, "%d", i);
		if (ini.Get_String(name, entry, "", buffer) > 0) { // RA2: read into the Wstring
			GraphicMenuItem * item = GM_Create_Item_From_INI(buffer, ini, menu->Engine, pt);
			if (item != NULL) {
				menu->Add_Item(item);
			}
		}
	}

	return(menu);
}


/// <summary>
/// Creates an empty graphic menu.
/// The menu comes up with the title screen behind it and the intro music playing. The
/// INI reader supplies the real backdrop, theme, and items afterward.
/// </summary>
GraphicMenu::GraphicMenu(void) :
	Engine(),
	BackgroundName("Title.PCX"), // RA2: the names are initialised in the list
	ThemeName("Intro"), // RA2: the names are initialised in the list
	CurrentAnim(NULL), // RA2: CurrentAnim before Items
	Items() // RA2: Items last
{
}


/// <summary>
/// Destroys the menu along with everything on it.
/// The menu owns the items that were added to it, so they are deleted here.
/// </summary>
GraphicMenu::~GraphicMenu(void)
{
	for (ITEM_ITER iter = Items; iter; iter++) {
		delete *iter;
	}
	AlternateSurface->Fill(0); // RA2: the surfaces are cleared
	HiddenSurface->Fill(0); // RA2: the surfaces are cleared
	VisibleSurface->Blit_From(*HiddenSurface, false, true); // RA2: and the hidden one shown when the menu goes
}


/// <summary>
/// Enables or disables the menu items carrying an identifier.
/// The shell menu handler uses this routine to shut off the choices that do not apply
/// to the page it is about to display.
/// </summary>
/// <param name="id">The identifier of the items to change.</param>
/// <param name="enabled">Should the items be enabled?</param>
void GraphicMenu::Set_Item_Enabled(int id, bool enabled)
{
	ITEM_ITER it = Items;
	for (; it; it++) {
		if (it->Get_ID() == id) {
			it->Set_Enabled(enabled);
		}
	}
}




/// <summary>
/// Runs the menu until the player picks something.
/// This routine starts the menu's theme and then takes over the mouse and keyboard,
/// highlighting whichever item the player is pointing at, until an item is chosen.
/// The chosen item performs its action before control is handed back.
/// </summary>
/// <returns>Returns with the identifier of the menu item the player chose.</returns>
int GraphicMenu::Presentation(void) // RA2: defined here, after Set_Item_Enabled (0x004DE9E0)
{
	Theme.Play_Song(Theme.From_Name(ThemeName.get())); // RA2: Wstring's text

	AlternateSurface->Fill(0); // RA2: the mouse is not captured here
	HiddenSurface->Fill(0);
	VisibleSurface->Blit_From(*HiddenSurface); // RA2: the blank screen is shown before the loop

	bool done = false;
	GraphicMenuItem * item = NULL;

	Keyboard->Clear();

	if (CurrentAnim != NULL) {
		Engine.Wait_For_Anim(CurrentAnim);
	}

	Engine.Restore_Anims(AlternateSurface->Get_Rect());
	Engine.Restore_And_Advance();

	do { // RA2: a do-while loop
		Hide_Mouse();
		Engine.Wait_For_Focus();
		Show_Mouse();

		Point2D mouse(Get_Mouse_X(), Get_Mouse_Y());

		if (Keyboard->Check() != KN_NONE) {
			KeyNumType key = Keyboard->Get();
			GraphicMenuItem * temp = (key == KN_LMOUSE || key == KN_RETURN) ? Get_Item_Under_Mouse(mouse) : Get_Item_For_Key(key);

			if (temp != NULL) {
				if (item != temp) {
					if (item != NULL) {
						item->Set_Selected(false);
					}
					item = temp;
					if (temp != NULL) {
						temp->Set_Selected(true);
					}
				}
				done = true;
			}
		} else {
			GraphicMenuItem * temp = Get_Item_Under_Mouse(mouse);
			if (temp != item) {
				if (item != NULL) {
					item->Set_Selected(false);
				}
				item = temp;
				if (temp != NULL) {
					temp->Set_Selected(true);
				}
			}
		}

		Engine.Wait_Delay(1);
	} while (!done); // RA2: tested at the end

	if (item != NULL) {
		item->Action(&Engine);
	}

	OwnerDraw::Release_Mouse();

	Theme.Fade_Out();

	if (item != NULL) {
		return(item->Get_ID());
	}

	return(-1);
}


/// <summary>
/// Sets the animation that plays behind the menu.
/// The animation is handed to the menu's animation engine, taking the place of
/// whatever backdrop was there before.
/// </summary>
void GraphicMenu::Set_Animation(MSAnim *anim)
{
	MSAnim * old = CurrentAnim;
	if (old != NULL) {
		Engine.Replace_Anim(old, anim);
	} else {
		Engine.Add_Animation(anim);
	}
	CurrentAnim = anim;
}


/// <summary>
/// Sets the music theme that plays while this menu is up.
/// </summary>
void GraphicMenu::Set_Theme_Name(Wstring const & name) // RA2: a Wstring name
{
	ThemeName = name; // RA2: Wstring assignment
}


/// <summary>
/// Adds an item to the menu.
/// </summary>
/// <remarks>The menu takes ownership of the item and will delete it when the menu is
/// destroyed.</remarks>
void GraphicMenu::Add_Item(GraphicMenuItem * item)
{
	Items.Add(item);
}


/// <summary>
/// Fetches the menu item lying under the mouse.
/// </summary>
/// <returns>Returns with a pointer to the item beneath the mouse, or NULL if the mouse
/// is over none of them.</returns>
GraphicMenuItem * GraphicMenu::Get_Item_Under_Mouse(Point2D const & mouse)
{
	ITEM_ITER it = Items;
	for (; it; it++) {
		if (it->Is_Mouse_Over(mouse)) {
			break;
		}
	}
	if (it) {
		return(*it);
	}
	return(NULL);
}


/// <summary>
/// Fetches the menu item bound to the specified key.
/// </summary>
/// <returns>Returns with a pointer to the item that answers to the key, or NULL if no
/// item claims it.</returns>
GraphicMenuItem * GraphicMenu::Get_Item_For_Key(KeyNumType key)
{
	ITEM_ITER it = Items;
	for (; it; it++) {
		if (it->Is_Input_Key(key)) {
			break;
		}
	}
	if (it) {
		return(*it);
	}
	return(NULL);
}


/// <summary>
/// Sets the name of the menu's background PCX.
/// </summary>
void GraphicMenu::Set_Background_Name(Wstring const & name) // RA2: sets the background PCX name
{
	BackgroundName = name; // RA2: Wstring assignment
}

