/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "grphmsct.h"

#include "grphmenu.h"
#include "grphmitm.h"
#include "grphmsct.h"
#include "ini.h"
#include "keyboard.h"
#include "wstring.h"


unsigned int GM_Build_Key(Wstring const & list); // RA2: Wstring

/// <summary>
/// Creates a shortcut menu item from the INI database.
/// This routine is used while a graphic menu is being built up from its INI
/// description. The section must name the menu item it stands for; the key list is
/// optional and an item with no keys simply never answers to the keyboard.
/// </summary>
/// <param name="name">The INI section to read the item from.</param>
/// <returns>Returns with a pointer to the item created. If the section does not identify a
/// menu item, NULL is returned.</returns>
GraphicMenuItem * GM_Read_Shortcut_Item(Wstring const & name, INIClass const & ini) // RA2: Wstring
{
	int id = ini.Get_Int(name, Wstring("ID"), -1);
	if (id == -1) {
		return(NULL);
	}
	Wstring buffer;
	{
		Wstring defvalue("");
		Wstring entry("Keys");
		ini.Get_String(name, entry, defvalue, buffer);
	}

	GraphicMenuItem * item = new GraphicMenuShortcutItem(id, buffer);
	return(item);
}


/// <summary>
/// Creates a shortcut item for a graphic menu.
/// The key list is split apart and each description is converted into a key code
/// that the item will answer to. Descriptions that cannot be recognized are quietly
/// discarded.
/// </summary>
/// <param name="keys">The shortcut key descriptions, separated by commas.</param>
GraphicMenuShortcutItem::GraphicMenuShortcutItem(int id, Wstring const & keys) :
	GraphicMenuItem(id)
{
	Wstring tok; // RA2: the keys are a Wstring, split with getToken
	int pos = keys.getToken(0, ",", tok);
	while (tok.length() > 0) {
		unsigned int key = GM_Build_Key(tok);
		if (key != 0) {
			Shortcuts.Add(key);
		}
		pos = keys.getToken(pos, ",;", tok);
		if (pos == -1) {
			tok.clear();
		}
	}
}


/// <summary>
/// Converts a shortcut key description into a key code.
/// This routine is used to translate the human readable key names stored in the INI
/// database into the key codes the keyboard system deals in. A leading "ctrl-",
/// "alt-", or "shift-" prefix contributes the matching modifier bit, and the
/// remainder may be either a single character or one of the recognized key names.
/// </summary>
/// <param name="list">The shortcut description to convert, modifier prefixes included.</param>
/// <returns>Returns with the key code and modifier bits. If the description could not be
/// recognized, VK_NONE is returned.</returns>
unsigned int GM_Build_Key(Wstring const & list) // RA2: Wstring
{
	static struct {
		char const * Name;
		unsigned int Key;
	} _name2key[] = {
		{ "backspace", KN_BACKSPACE },
		{ "esc", KN_ESC }
	};

	unsigned int modifiers = 0;
	Wstring lower = list;
	lower.toLower();

	Wstring lst = lower;
	lst.truncate(5u);
	if (lst == "ctrl-") {
		modifiers |= KN_CTRL_BIT;
		lower.remove(0, 5);
	}
	lst = lower;
	lst.truncate(4u);
	if (lst == "alt-") {
		modifiers |= KN_ALT_BIT;
		lower.remove(0, 4);
	}
	lst = lower;
	lst.truncate(6u);
	if (lst == "shift-") {
		modifiers |= KN_SHIFT_BIT;
		lower.remove(0, 6);
	}

	unsigned int key;
	switch (lower.length()) {
		case 0:
			key = 0;
			break;

		case 1:
			key = lower.get(0) - 0x20;
			break;

		default: {
			int k = -1;
			for (int i = ARRAY_SIZE(_name2key) - 1; i >= 0; i--) {
				if (!(lower != _name2key[i].Name)) {
					k = i;
					break;
				}
			}
			if (k >= 0) {
				key = _name2key[k].Key;
			} else {
				key = 0;
			}
			break;
		}
	}

	if (key != 0) {
		modifiers |= key;
	} else {
		modifiers = 0;
	}
	return(modifiers);
}


/// <summary>
/// Destroys the graphic menu shortcut item.
/// The shortcut list cleans up after itself, so there is nothing left for this
/// routine to do.
/// </summary>
GraphicMenuShortcutItem::~GraphicMenuShortcutItem(void)
{
	//nothing
}


/// <summary>
/// Determines if a key press triggers this menu item.
/// This routine is called by the graphic menu when it is looking for the item that
/// should respond to a key press. A disabled item never claims a key.
/// </summary>
/// <returns>bool; Is this key one of the item's shortcuts?</returns>
bool GraphicMenuShortcutItem::Is_Input_Key(KeyNumType key)
{
	if (!Enabled) {
		return(false);
	}

	SHORTCUT_ITER it = Shortcuts;
	for (; it; it++) {
		if (*it == (unsigned int)key) {
			break;
		}
	}

	return(it);
}

