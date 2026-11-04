#include "Menu.h"
#include "Input.h"
#include "Sound.h"

// Shared byte state is defined apart from its callers to preserve byte loads.
// GLOBAL: CMR2 0x0052af58
BYTE g_unk0x0052af58[2];

// GLOBAL: CMR2 0x0059f8fc
char g_unk0x0059f8fc;
// GLOBAL: CMR2 0x0059f908
Menu *g_menuNextAction;
// GLOBAL: CMR2 0x0059fa14
char g_unk0x0059fa14;
// GLOBAL: CMR2 0x0059f904
char g_menuActionPending;
// GLOBAL: CMR2 0x0059fa15
char g_unk0x0059fa15;
// GLOBAL: CMR2 0x0059fa16
char g_unk0x0059fa16;
// GLOBAL: CMR2 0x0059fa18
unsigned int g_menuLastInput;
// GLOBAL: CMR2 0x0059fa17
char g_unk0x0059fa17;

// FUNCTION: CMR2 0x0049ffd0
void Menu_Init(Menu *pMenu, char *text, short param3, int param4, Menu *pParent, MenuItemCallbacks *pItemCallbacks, BYTE flag4, BYTE defaultCursor, BYTE layout)
{
    pMenu->text = text;
    pMenu->field_0x4 = param3;
    pMenu->pParent = pParent == (Menu *)-1 ? NULL : pParent;
    pMenu->itemCount = 0;
    pMenu->cursor = defaultCursor;
    pMenu->layout = layout;
    pMenu->defaultCursor = -1;
    pMenu->pfnCallback0 = NULL;
    pMenu->pfnCallback1 = NULL;
    pMenu->pfnCallback2 = NULL;
    pMenu->pfnCallback3 = NULL;
    pMenu->flag0 = 1;
    pMenu->flag1 = 1;
    pMenu->flag2 = 1;
    pMenu->flag3 = 1;
    pMenu->flag4 = flag4;
    pMenu->flag5 = 1;
    pMenu->flag6 = pParent == (Menu *)-1;
    pMenu->flag7 = 0;   // builders opt into both-pairs navigation explicitly
    pMenu->value &= 0x80;
    pMenu->pItemCallbacks = pItemCallbacks;
}

// FUNCTION: CMR2 0x004a0060
void Menu_ClearNextItem(Menu *pMenu)
{
    MenuItem *pItem;

    pItem = &pMenu->items[pMenu->itemCount];
    pItem->text = 0;
    pItem->type = 0;
    pItem->value = 0;
    pItem->action = NULL;
    pItem->min = 0;
    pItem->max = 0;
    pItem->pSubMenu = NULL;
    pItem->id = -1;
    pItem->enabled = 1;
    pItem->visible = 1;
    pItem->flag2 = 0;
    pItem->flag3 = 0;
}

// FUNCTION: CMR2 0x004a00a0
void Menu_AddItemType3(Menu *pMenu, char *text, short id, BYTE min, BYTE max, BYTE flag2, int unused, MenuItemAction action, short value)
{
    Menu_ClearNextItem(pMenu);
    pMenu->items[pMenu->itemCount].text = text;
    pMenu->items[pMenu->itemCount].id = id;
    pMenu->items[pMenu->itemCount].type = 3;
    pMenu->items[pMenu->itemCount].min = min;
    pMenu->items[pMenu->itemCount].max = max;
    pMenu->items[pMenu->itemCount].flag2 = flag2;
    pMenu->items[pMenu->itemCount].value = value;
    pMenu->items[pMenu->itemCount].action = action;
    pMenu->itemCount++;
}

// FUNCTION: CMR2 0x004a0150
void Menu_AddItemType6(Menu *pMenu, char *text, short id, BYTE min, BYTE max, BYTE flag2, int unused, MenuItemAction action, short value)
{
    Menu_ClearNextItem(pMenu);
    pMenu->items[pMenu->itemCount].text = text;
    pMenu->items[pMenu->itemCount].id = id;
    pMenu->items[pMenu->itemCount].type = 6;
    pMenu->items[pMenu->itemCount].min = min;
    pMenu->items[pMenu->itemCount].max = max;
    pMenu->items[pMenu->itemCount].flag2 = flag2;
    pMenu->items[pMenu->itemCount].value = value;
    pMenu->items[pMenu->itemCount].action = action;
    pMenu->itemCount++;
}

// FUNCTION: CMR2 0x004a0200
void Menu_AddItemType4(Menu *pMenu, char *text, short id, MenuItemAction action, short value)
{
    Menu_ClearNextItem(pMenu);
    pMenu->items[pMenu->itemCount].text = text;
    pMenu->items[pMenu->itemCount].id = id;
    pMenu->items[pMenu->itemCount].type = 4;
    pMenu->items[pMenu->itemCount].value = value;
    pMenu->items[pMenu->itemCount].action = action;
    pMenu->itemCount++;
}

// FUNCTION: CMR2 0x004a0270
void Menu_AddItemType1(Menu *pMenu, char *text, short id, MenuItemAction action, short value)
{
    Menu_ClearNextItem(pMenu);
    pMenu->items[pMenu->itemCount].text = text;
    pMenu->items[pMenu->itemCount].id = id;
    pMenu->items[pMenu->itemCount].type = 1;
    pMenu->items[pMenu->itemCount].value = value;
    pMenu->items[pMenu->itemCount].action = action;
    pMenu->itemCount++;
}

// FUNCTION: CMR2 0x004a02e0
void Menu_AddItemType2(Menu *pMenu, char *text, short id, Menu *pSubMenu, MenuItemAction action, short value)
{
    Menu_ClearNextItem(pMenu);
    pMenu->items[pMenu->itemCount].text = text;
    pMenu->items[pMenu->itemCount].id = id;
    pMenu->items[pMenu->itemCount].type = 2;
    pMenu->items[pMenu->itemCount].pSubMenu = pSubMenu;
    pMenu->items[pMenu->itemCount].value = value;
    pMenu->items[pMenu->itemCount].action = action;
    pMenu->itemCount++;
}

// FUNCTION: CMR2 0x004a0360
void Menu_SetCursor(Menu *pMenu, BYTE cursor)
{
    pMenu->cursor = cursor;
}

// FUNCTION: CMR2 0x004a0370
void Menu_SetParent(Menu *pMenu, Menu *pParent)
{
    pMenu->pParent = pParent;
}

// Searches the item tag stored at +8, as in the original lookup.
// FUNCTION: CMR2 0x004a0380
int Menu_FindItem(Menu *pMenu, int id)
{
    int i;
    int count;

    count = pMenu->itemCount;
    for (i = 0; i < count; i++) {
        if (pMenu->items[i].value == id)
            return i;
    }
    return -1;
}

// FUNCTION: CMR2 0x004a03b0
MenuItem *Menu_GetItem(Menu *pMenu, int id)
{
    int i;

    i = Menu_FindItem(pMenu, id);
    if (i == -1)
        return NULL;
    return &pMenu->items[i];
}

// FUNCTION: CMR2 0x004a03e0
void Menu_SetItemSubMenu(Menu *pMenu, int id, Menu *pSubMenu)
{
    MenuItem *pItem;

    pItem = Menu_GetItem(pMenu, id);
    if (pItem != NULL && pItem->type == 2)
        pItem->pSubMenu = pSubMenu;
}

// FUNCTION: CMR2 0x004a0410
void Menu_SelectItem(Menu *pMenu, int id)
{
    pMenu->cursor = (char)Menu_FindItem(pMenu, id);
}

// FUNCTION: CMR2 0x004a0430
void Menu_SetCallbacks(Menu *pMenu, MenuCallback pfn0, MenuCallback pfn1, MenuCallback pfn2, MenuCallback pfn3)
{
    pMenu->pfnCallback0 = pfn0;
    pMenu->pfnCallback1 = pfn1;
    pMenu->pfnCallback2 = pfn2;
    pMenu->pfnCallback3 = pfn3;
}

// FUNCTION: CMR2 0x004a0460
void Menu_SetFlags(Menu *pMenu, BYTE bit0, BYTE bit1, BYTE bit2, BYTE bit3)
{
    pMenu->flag0 = bit0;
    pMenu->flag1 = bit1;
    pMenu->flag2 = bit2;
    pMenu->flag3 = bit3;
}

// Moves the cursor to the next item that is enabled (flags bit0|bit1),
// starting at defaultCursor when it is valid.
// FUNCTION: CMR2 0x004a04a0
void Menu_ValidateCursor(Menu *pMenu, int unused)
{
    char def;
    int count;
    int tries;

    def = pMenu->defaultCursor;
    if (def >= 0 && def < pMenu->itemCount)
        pMenu->cursor = def;
    count = pMenu->itemCount;
    for (tries = 0; tries < count; tries++) {
        int i = pMenu->cursor + tries;
        int idx = i % count;
        if (pMenu->items[idx].enabled && pMenu->items[idx].visible) {
            pMenu->cursor = (char)idx;
            return;
        }
    }
}

// FUNCTION: CMR2 0x004a04f0
void Menu_CallCallback0(Menu *pMenu)
{
    MenuCallback pfn;
    int param;

    pfn = pMenu->pfnCallback0;
    if (pfn != NULL) {
        if (g_unk0x0059fa16 == 0 && g_unk0x0059fa17 == 0)
            param = 0;
        else
            param = 1;
        pfn(pMenu, param);
    }
}

// FUNCTION: CMR2 0x004a0530
void Menu_CallCallback3(Menu *pMenu)
{
    if (pMenu->pfnCallback3 != NULL)
        pMenu->pfnCallback3(pMenu, 0);
}

// FUNCTION: CMR2 0x004a0550
void Menu_CallCallback2(Menu *pMenu)
{
    if (pMenu != NULL && pMenu->pfnCallback2 != NULL)
        ((void (*)(Menu *))pMenu->pfnCallback2)(pMenu);
}

// Queues the action of the selected item; stops the current joystick
// effect first unless a menu transition is already pending.
// FUNCTION: CMR2 0x0049ff50
void Menu_PlaySound(int id)
{
    if (id >= 0)
        Sound_PlaySampleWithParameters(id, CInput::m_menuSoundVolume, 0x57e4, 0, 0, 0);
}

// FUNCTION: CMR2 0x004a0ad0
void Menu_SetNextAction(Menu *action)
{
    if (g_unk0x0059f8fc == 0 && g_unk0x0059fa14 != 0)
        Menu_PlaySound(CInput::m_menuSelectSound);
    g_unk0x0059fa16 = 0;
    g_menuNextAction = action;
}

#define ITEM_AT(pMenu, i) (&(pMenu)->items[(i)])
// The flag0..flag7 bits read as one byte; they follow layout (0xd).
#define MENU_FLAGS(pMenu) (*(&(pMenu)->layout + 1))

// Applies one frame of input to the menu: moves the cursor, changes the
// value of type 3/6 items, fires the item callbacks and returns the action
// queued by the selected item (0 = none). Input bits: 0-3 directions (the
// pairs swap with layout), 4 select, 5 back.
// match 86%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a0570
Menu *Menu_Update(Menu *pMenu, unsigned int input)
{
    BYTE bNext;
    BYTE bLeft;
    BYTE bRight;
    BYTE bBack;
    BYTE bSelect;
    BYTE bPrev;
    char count;
    int cursor;
    int i;
    int moved;
    int wrapped;
    MenuItem *pItem;
    Menu *action;
    BYTE flags;
    BYTE v;

    if (g_unk0x0059fa15 == 0 && (input & 0xc) != 0 && (input & 3) != 0)
        input = 0;
    g_menuLastInput = input;
    g_unk0x0059f8fc = 0;
    if (pMenu == NULL)
        return 0;
    if (g_menuActionPending != 0) {
        Menu_CallCallback0(pMenu);
        g_menuActionPending = 0;
        g_unk0x0059fa17 = 0;
    }
    if (g_menuNextAction == 0) {
        flags = MENU_FLAGS(pMenu);
        if (flags & 4)
            bSelect = (input & MENU_INPUT_SELECT) ? 1 : 0;
        else
            bSelect = 0;
        if (flags & 8)
            bBack = (input & MENU_INPUT_BACK) ? 1 : 0;
        else
            bBack = 0;
        // The two directional pairs swap roles with the layout (the bits are
        // physical arrows, see MENU_INPUT_*): layout 1 moves the cursor with
        // Up/Down and adjusts values with Left/Right; layout 0 is the
        // reverse. Items without a value (types other than 3/6) ignore value
        // adjustment, so their layout's adjust pair is dead.
        if (flags & 2) {
            if (pMenu->layout == 1) {
                bLeft = (input & MENU_INPUT_LEFT) ? 1 : 0;
                bRight = (input & MENU_INPUT_RIGHT) ? 1 : 0;
            } else {
                bLeft = (input & MENU_INPUT_UP) ? 1 : 0;
                bRight = (input & MENU_INPUT_DOWN) ? 1 : 0;
            }
            if (bLeft && bRight)
                bLeft = 0;
        } else {
            bLeft = 0;
            bRight = 0;
        }
        if ((flags & 1) && !bLeft && !bRight) {
            if (pMenu->layout == 1) {
                bPrev = (input & MENU_INPUT_UP) ? 1 : 0;
                bNext = (input & MENU_INPUT_DOWN) ? 1 : 0;
            } else {
                bPrev = (input & MENU_INPUT_LEFT) ? 1 : 0;
                bNext = (input & MENU_INPUT_RIGHT) ? 1 : 0;
            }
            if (bPrev && bNext)
                bPrev = 0;
        } else {
            bPrev = 0;
            bNext = 0;
        }
        if (pMenu->flag7 && (flags & 1)) {
            // Both-pairs navigation for menus with no adjustable values (the
            // quit confirmation opts in): either arrow pair moves the cursor.
            // Native reports 2026-11-01/02: Up/Down dead in the original
            // horizontal layout, only Left/Right in the vertical one - users
            // expect both pairs to work on a Yes/No dialog.
            bPrev = (input & (MENU_INPUT_LEFT | MENU_INPUT_UP)) ? 1 : 0;
            bNext = (input & (MENU_INPUT_RIGHT | MENU_INPUT_DOWN)) ? 1 : 0;
            if (bPrev && bNext)
                bPrev = 0;
            bLeft = 0;
            bRight = 0;
        }

        count = pMenu->itemCount;
        i = count;
        while (i > 0 && !ITEM_AT(pMenu, pMenu->cursor)->enabled) {
            cursor = pMenu->cursor + 1;
            i--;
            pMenu->cursor = cursor;
            if (cursor >= count)
                pMenu->cursor = 0;
        }

        if (bPrev) {
            cursor = pMenu->cursor;
            moved = 1;
            wrapped = 1;
            if (cursor > 0 || pMenu->flag4) {
                i = 0;
                do {
                    cursor--;
                    pMenu->cursor = cursor;
                    if (cursor < 0)
                        pMenu->cursor = cursor + count;
                    i++;
                    if (i >= count)
                        wrapped = 0;
                    cursor = pMenu->cursor;
                } while (!ITEM_AT(pMenu, cursor)->enabled || !ITEM_AT(pMenu, cursor)->visible);
                pMenu->moveFlags |= 1;
                if (wrapped && g_unk0x0059fa14 != 0)
                    Menu_PlaySound(CInput::m_menuMoveSound);
            } else {
                moved = 0;
            }
            if (pMenu->pItemCallbacks != NULL && pMenu->pItemCallbacks->pfnMove != NULL)
                pMenu->pItemCallbacks->pfnMove(pMenu, ITEM_AT(pMenu, pMenu->cursor), moved);
        } else if (bNext) {
            cursor = pMenu->cursor;
            moved = 1;
            wrapped = 1;
            if (cursor < count - 1 || (pMenu->flag4 && count != 0)) {
                i = 0;
                do {
                    cursor++;
                    pMenu->cursor = cursor;
                    if (cursor >= count)
                        pMenu->cursor = 0;
                    i++;
                    if (i >= count)
                        wrapped = 0;
                    cursor = pMenu->cursor;
                } while (!ITEM_AT(pMenu, cursor)->enabled || !ITEM_AT(pMenu, cursor)->visible);
                pMenu->moveFlags &= 0xfe;
                if (wrapped && g_unk0x0059fa14 != 0)
                    Menu_PlaySound(CInput::m_menuMoveSound);
            } else {
                moved = 0;
            }
            if (pMenu->pItemCallbacks != NULL && pMenu->pItemCallbacks->pfnMove != NULL)
                pMenu->pItemCallbacks->pfnMove(pMenu, ITEM_AT(pMenu, pMenu->cursor), moved);
        }

        pItem = ITEM_AT(pMenu, pMenu->cursor);
        if (bSelect && pItem->enabled)
            bSelect = 1;
        else
            bSelect = 0;
        if ((bPrev || bNext) && pItem->type == 6) {
            if ((pMenu->value & 0x7f) >= pItem->min)
                pMenu->value = ((pItem->min - 1) ^ pMenu->value) & 0x7f ^ pMenu->value;
            pItem->max = pMenu->value & 0x7f;
        }
        if (pItem->type == 3 || pItem->type == 6) {
            if (bLeft) {
                moved = 1;
                if (pItem->max > 0 || pItem->flag2) {
                    pItem->max--;
                    if (pItem->max == 0xff)
                        pItem->max = pItem->min - 1;
                    if (g_unk0x0059fa14 != 0)
                        Menu_PlaySound(CInput::m_menuChangeSound);
                    pMenu->moveFlags &= 0xfd;
                } else {
                    moved = 0;
                }
                if (pMenu->pItemCallbacks != NULL && pMenu->pItemCallbacks->pfnChange != NULL)
                    pMenu->pItemCallbacks->pfnChange(pMenu, ITEM_AT(pMenu, pMenu->cursor), moved);
            } else if (bRight || (bSelect && pItem->flag3)) {
                moved = 1;
                if (pItem->max < pItem->min - 1 || pItem->flag2 || (bSelect && pItem->flag3)) {
                    if (!bSelect || pItem->action == NULL) {
                        pItem->max++;
                        if (pItem->max >= pItem->min)
                            pItem->max = 0;
                    }
                    if (g_unk0x0059fa14 != 0)
                        Menu_PlaySound(CInput::m_menuChangeSound);
                    pMenu->moveFlags |= 2;
                } else {
                    moved = 0;
                }
                if (pMenu->pItemCallbacks != NULL && pMenu->pItemCallbacks->pfnChange != NULL)
                    pMenu->pItemCallbacks->pfnChange(pMenu, ITEM_AT(pMenu, pMenu->cursor), moved);
            }
            pMenu->value = (pItem->max ^ pMenu->value) & 0x7f ^ pMenu->value;
        }

        if (bSelect == 0) {
            if (bBack) {
back:
                g_menuNextAction = pMenu->pParent;
                g_unk0x0059f8fc = 1;
                if (pMenu->pItemCallbacks != NULL && pMenu->pItemCallbacks->pfnBack != NULL)
                    pMenu->pItemCallbacks->pfnBack(pMenu, ITEM_AT(pMenu, pMenu->cursor), 1);
                if (g_unk0x0059fa14 != 0)
                    Menu_PlaySound(CInput::m_menuBackSound);
            }
        } else {
            if (pItem->action != NULL)
                pItem->action(pMenu, pItem);
            if (bBack || pItem->type == 1)
                goto back;
            if (pItem->type == 2) {
                g_menuNextAction = pItem->pSubMenu;
                if (pMenu->pItemCallbacks != NULL && pMenu->pItemCallbacks->pfnSelect != NULL)
                    pMenu->pItemCallbacks->pfnSelect(pMenu, ITEM_AT(pMenu, pMenu->cursor), 1);
                if (g_unk0x0059fa14 != 0)
                    Menu_PlaySound(CInput::m_menuSelectSound);
            }
        }
        if (pMenu->pfnCallback1 != NULL)
            ((void (*)(Menu *))pMenu->pfnCallback1)(pMenu);
        if (g_menuNextAction == 0)
            goto done;
        if (bBack) {
            v = 1;
            goto notify;
        }
    }
    if (ITEM_AT(pMenu, pMenu->cursor)->type == 1)
        v = 1;
    else
        v = 0;
notify:
    g_unk0x0059fa16 = v;
    if (pMenu->pfnCallback3 != NULL)
        pMenu->pfnCallback3(pMenu, v);
    if (g_menuNextAction != 0)
        g_menuActionPending = 1;
done:
    action = g_menuNextAction;
    g_menuNextAction = 0;
    return action;
}

// Goes back to the parent menu (with the back sound unless muted).
// FUNCTION: CMR2 0x004a0b30
void Menu_GoBack(Menu *pMenu)
{
    if (g_unk0x0059f8fc == 0 && g_unk0x0059fa14 != 0)
        Menu_PlaySound(CInput::m_menuSelectSound);
    g_menuNextAction = pMenu->pParent;
    g_unk0x0059fa17 = 1;
}

// FUNCTION: CMR2 0x004a0ba0
void Menu_QueueDefaultAction(void)
{
    g_menuActionPending = 1;
    g_unk0x0059fa15 = 0;
    g_unk0x0059fa16 = 0;
    g_menuNextAction = 0;
}

// FUNCTION: CMR2 0x004a0bc0
void Menu_PlaySoundId(int id)
{
    int snd;

    switch (id) {
    case 0:
        snd = CInput::m_menuMoveSound;
        break;
    case 1:
        snd = CInput::m_menuSelectSound;
        break;
    case 2:
        snd = CInput::m_menuBackSound;
        break;
    case 3:
        snd = CInput::m_menuErrorSound;
        break;
    case 4:
        snd = CInput::m_menuChangeSound;
        break;
    }
    Menu_PlaySound(snd);
}

// FUNCTION: CMR2 0x004a0c40
void Menu_SetInputStateFlag(char param1)
{
    g_unk0x0059fa14 = param1;
}

// FUNCTION: CMR2 0x004a0c50
void Menu_SetActionLatch(char param1)
{
    g_unk0x0059fa15 = param1;
}
