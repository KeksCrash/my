#include "hid_keynote.h"
#include <gui/elements.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include "../hid.h"
#ifdef HID_TRANSPORT_BLE
#include "../helpers/ducky_script.h"
#include "../helpers/bad_usb_hid.h"
#endif

#include "hid_icons.h"

#define TAG "HidKeynote"

struct HidKeynote {
    View* view;
    Hid* hid;
};

typedef struct {
    bool left_pressed;
    bool up_pressed;
    bool right_pressed;
    bool down_pressed;
    bool ok_pressed;
    bool back_pressed;
    bool connected;
    bool mod_shift;
    bool mod_alt;
    bool mod_ctrl;
    bool mod_gui;
    uint8_t acceleration;
    HidKeynoteAction actions[HidKeynoteButtonCount];
    HidKeynoteMode mode;    Hid* hid;
} HidKeynoteModel;

static const HidKeynoteAction hid_keynote_default_actions[HidKeynoteButtonCount] = {
    [HidKeynoteButtonUp] = HidKeynoteActionArrowUp,
    [HidKeynoteButtonDown] = HidKeynoteActionArrowDown,
    [HidKeynoteButtonLeft] = HidKeynoteActionArrowLeft,
    [HidKeynoteButtonRight] = HidKeynoteActionArrowRight,
    [HidKeynoteButtonOk] = HidKeynoteActionEnter,
    [HidKeynoteButtonBack] = HidKeynoteActionBrowserBack,
};

static void hid_keynote_apply_mode(HidKeynote* hid_keynote, HidKeynoteModel* model) {
    for(size_t i = 0; i < HidKeynoteButtonCount; i++) {
        model->actions[i] = model->mode == HidKeynoteModeConfigured ?
                                hid_keynote->hid->keynote_actions[i] :
                                hid_keynote_default_actions[i];
    }
}

typedef enum {
    HidKeynoteActionTypeKeyboard,
    HidKeynoteActionTypeConsumer,
    HidKeynoteActionTypeModifier,
    HidKeynoteActionTypeMouseMove,
    HidKeynoteActionTypeMouseScroll,
    HidKeynoteActionTypeMouseButton,
    HidKeynoteActionTypeMacroBack,
    HidKeynoteActionTypeMacroSeq, // sequential steps: code→release, code2→release, code3→release
    HidKeynoteActionTypeCustomMacro, // user-authored script, see hid_keynote_play_macro()
    HidKeynoteActionTypeBadKbScript, // Duckyscript from /ext/badkb/, executed via active BLE profile
} HidKeynoteActionType;

typedef struct {
    const char* name;
    const char* short_name;
    HidKeynoteActionType type;
    uint16_t code;
    int8_t x;
    int8_t y;
    uint16_t code2; // step 2 for MacroSeq (0 = unused)
    uint16_t code3; // step 3 for MacroSeq (0 = unused)
} HidKeynoteActionDef;

static const HidKeynoteActionDef hid_keynote_action_defs[HidKeynoteActionCount] = {
    [HidKeynoteActionArrowUp] = {"Arrow Up", "Up", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_UP_ARROW, 0, 0},
    [HidKeynoteActionArrowDown] = {"Arrow Down", "Dn", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_DOWN_ARROW, 0, 0},
    [HidKeynoteActionArrowLeft] = {"Arrow Left", "Lf", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_LEFT_ARROW, 0, 0},
    [HidKeynoteActionArrowRight] = {"Arrow Right", "Rt", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_RIGHT_ARROW, 0, 0},
    [HidKeynoteActionW] = {"W", "W", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_W, 0, 0},
    [HidKeynoteActionA] = {"A", "A", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_A, 0, 0},
    [HidKeynoteActionS] = {"S", "S", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_S, 0, 0},
    [HidKeynoteActionD] = {"D", "D", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_D, 0, 0},
    [HidKeynoteActionQ] = {"Q", "Q", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_Q, 0, 0},
    [HidKeynoteActionE] = {"E", "E", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_E, 0, 0},
    [HidKeynoteActionR] = {"R", "R", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_R, 0, 0},
    [HidKeynoteActionF] = {"F", "F", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F, 0, 0},
    [HidKeynoteActionT] = {"T", "T", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_T, 0, 0},
    [HidKeynoteActionY] = {"Y", "Y", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_Y, 0, 0},
    [HidKeynoteActionU] = {"U", "U", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_U, 0, 0},
    [HidKeynoteActionI] = {"I", "I", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_I, 0, 0},
    [HidKeynoteActionO] = {"O", "O", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_O, 0, 0},
    [HidKeynoteActionP] = {"P", "P", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_P, 0, 0},
    [HidKeynoteActionG] = {"G", "G", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_G, 0, 0},
    [HidKeynoteActionH] = {"H", "H", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_H, 0, 0},
    [HidKeynoteActionJ] = {"J", "J", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_J, 0, 0},
    [HidKeynoteActionK] = {"K", "K", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_K, 0, 0},
    [HidKeynoteActionL] = {"L", "L", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_L, 0, 0},
    [HidKeynoteActionZ] = {"Z", "Z", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_Z, 0, 0},
    [HidKeynoteActionX] = {"X", "X", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_X, 0, 0},
    [HidKeynoteActionC] = {"C", "C", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_C, 0, 0},
    [HidKeynoteActionV] = {"V", "V", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_V, 0, 0},
    [HidKeynoteActionB] = {"B", "B", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_B, 0, 0},
    [HidKeynoteActionN] = {"N", "N", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_N, 0, 0},
    [HidKeynoteActionM] = {"M", "M", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_M, 0, 0},
    [HidKeynoteActionSpace] = {"Space", "Spc", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_SPACEBAR, 0, 0},
    [HidKeynoteActionEnter] = {"Enter", "Ent", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_RETURN, 0, 0},
    [HidKeynoteActionTab] = {"Tab", "Tab", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_TAB, 0, 0},
    [HidKeynoteActionEscape] = {"Escape", "Esc", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_ESCAPE, 0, 0},
    [HidKeynoteActionBackspace] = {"Backspace", "Bsp", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_DELETE, 0, 0},
    [HidKeynoteActionDelete] = {"Delete", "Del", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_DELETE_FORWARD, 0, 0},
    [HidKeynoteActionMouseUp] = {"Mouse Up", "MUp", HidKeynoteActionTypeMouseMove, 0, 0, -MOUSE_MOVE_SHORT},
    [HidKeynoteActionMouseDown] = {"Mouse Down", "MDn", HidKeynoteActionTypeMouseMove, 0, 0, MOUSE_MOVE_SHORT},
    [HidKeynoteActionMouseLeft] = {"Mouse Left", "MLf", HidKeynoteActionTypeMouseMove, 0, -MOUSE_MOVE_SHORT, 0},
    [HidKeynoteActionMouseRight] = {"Mouse Right", "MRt", HidKeynoteActionTypeMouseMove, 0, MOUSE_MOVE_SHORT, 0},
    [HidKeynoteActionMouseScrollUp] = {"Scroll Up", "Scr+", HidKeynoteActionTypeMouseScroll, 0, -1, 0},
    [HidKeynoteActionMouseScrollDown] = {"Scroll Down", "Scr-", HidKeynoteActionTypeMouseScroll, 0, 1, 0},
    [HidKeynoteActionMouseLeftClick] = {"Left Click", "LClk", HidKeynoteActionTypeMouseButton, HID_MOUSE_BTN_LEFT, 0, 0},
    [HidKeynoteActionMouseRightClick] = {"Right Click", "RClk", HidKeynoteActionTypeMouseButton, HID_MOUSE_BTN_RIGHT, 0, 0},
    [HidKeynoteActionBrowserBack] = {"Browser Back", "Back", HidKeynoteActionTypeMacroBack, 0, 0, 0},
    [HidKeynoteActionCtrl] = {"Ctrl (Hold)", "Ctrl", HidKeynoteActionTypeModifier, KEY_MOD_LEFT_CTRL, 0, 0},
    [HidKeynoteActionAlt] = {"Alt (Hold)", "Alt", HidKeynoteActionTypeModifier, KEY_MOD_LEFT_ALT, 0, 0},
    [HidKeynoteActionShift] = {"Shift (Hold)", "Shf", HidKeynoteActionTypeModifier, KEY_MOD_LEFT_SHIFT, 0, 0},
    [HidKeynoteActionGui] = {"GUI (Hold)", "GUI", HidKeynoteActionTypeModifier, KEY_MOD_LEFT_GUI, 0, 0},
    [HidKeynoteActionMediaPlayPause] = {"Media Play/Pause", "Play", HidKeynoteActionTypeConsumer, HID_CONSUMER_PLAY_PAUSE, 0, 0},
    [HidKeynoteActionMediaPlay] = {"Media Play", "Play", HidKeynoteActionTypeConsumer, HID_CONSUMER_PLAY, 0, 0},
    [HidKeynoteActionMediaPause] = {"Media Pause", "Pause", HidKeynoteActionTypeConsumer, HID_CONSUMER_PAUSE, 0, 0},
    [HidKeynoteActionMediaStop] = {"Media Stop", "Stop", HidKeynoteActionTypeConsumer, HID_CONSUMER_STOP, 0, 0},
    [HidKeynoteActionMediaNext] = {"Media Next", "Next", HidKeynoteActionTypeConsumer, HID_CONSUMER_SCAN_NEXT_TRACK, 0, 0},
    [HidKeynoteActionMediaPrev] = {"Media Prev", "Prev", HidKeynoteActionTypeConsumer, HID_CONSUMER_SCAN_PREVIOUS_TRACK, 0, 0},
    [HidKeynoteActionMediaMute] = {"Media Mute", "Mute", HidKeynoteActionTypeConsumer, HID_CONSUMER_MUTE, 0, 0},
    [HidKeynoteActionMediaVolUp] = {"Media Vol+", "Vol+", HidKeynoteActionTypeConsumer, HID_CONSUMER_VOLUME_INCREMENT, 0, 0},
    [HidKeynoteActionMediaVolDown] = {"Media Vol-", "Vol-", HidKeynoteActionTypeConsumer, HID_CONSUMER_VOLUME_DECREMENT, 0, 0},
    // tmux prefix-sequence combos: code=^B chord (step 1), code2=key (step 2)
    [HidKeynoteActionTmuxNewWin]  = {"New Win (^B,C)",  "NWin", HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, HID_KEYBOARD_C,                               0},
    [HidKeynoteActionTmuxNextWin] = {"Next Win (^B,N)", "NxtW", HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, HID_KEYBOARD_N,                               0},
    [HidKeynoteActionTmuxPrevWin] = {"Prev Win (^B,P)", "PrvW", HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, HID_KEYBOARD_P,                               0},
    [HidKeynoteActionTmuxSplitH]  = {"Split H (^B,\")", "SpH",  HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, KEY_MOD_LEFT_SHIFT | HID_KEYBOARD_APOSTROPHE, 0},
    [HidKeynoteActionTmuxSplitV]  = {"Split V (^B,%)",  "SpV",  HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, KEY_MOD_LEFT_SHIFT | HID_KEYBOARD_5,          0},
    [HidKeynoteActionTmuxPaneUp]  = {"Pane Up (^B,Up)", "PnUp", HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, HID_KEYBOARD_UP_ARROW,                        0},
    [HidKeynoteActionTmuxPaneDn]  = {"Pane Dn (^B,Dn)", "PnDn", HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, HID_KEYBOARD_DOWN_ARROW,                      0},
    [HidKeynoteActionTmuxPaneLt]  = {"Pane Lt (^B,Lt)", "PnLt", HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, HID_KEYBOARD_LEFT_ARROW,                      0},
    [HidKeynoteActionTmuxPaneRt]  = {"Pane Rt (^B,Rt)", "PnRt", HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, HID_KEYBOARD_RIGHT_ARROW,                     0},
    [HidKeynoteActionTmuxDetach]  = {"Detach (^B,D)",   "Det",  HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, HID_KEYBOARD_D,                               0},
    [HidKeynoteActionTmuxZoom]    = {"Zoom (^B,Z)",     "Zoom", HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, HID_KEYBOARD_Z,                               0},
    [HidKeynoteActionTmuxCopyMode]= {"CopyMode (^B,[)", "CpyM", HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, HID_KEYBOARD_OPEN_BRACKET,                    0},
    [HidKeynoteActionTmuxLastWin] = {"Last Win (^B,L)", "LstW", HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, HID_KEYBOARD_L,                               0},
    [HidKeynoteActionTmuxRename]  = {"Rename (^B,,)",   "Ren",  HidKeynoteActionTypeMacroSeq, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_B, 0, 0, HID_KEYBOARD_COMMA,                           0},
    // Symbol keys
    [HidKeynoteActionMinus] = {"-", "-", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_MINUS, 0, 0},
    [HidKeynoteActionEquals] = {"=", "=", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_EQUAL_SIGN, 0, 0},
    [HidKeynoteActionOpenBracket] = {"[", "[", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_OPEN_BRACKET, 0, 0},
    [HidKeynoteActionCloseBracket] = {"]", "]", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_CLOSE_BRACKET, 0, 0},
    [HidKeynoteActionBackslash] = {"\\\\", "\\\\", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_BACKSLASH, 0, 0},
    [HidKeynoteActionSemicolon] = {";", ";", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_SEMICOLON, 0, 0},
    [HidKeynoteActionApostrophe] = {"'", "'", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_APOSTROPHE, 0, 0},
    [HidKeynoteActionGraveAccent] = {"`", "`", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_GRAVE_ACCENT, 0, 0},
    [HidKeynoteActionComma] = {",", ",", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_COMMA, 0, 0},
    [HidKeynoteActionDot] = {".", ".", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_DOT, 0, 0},
    [HidKeynoteActionSlash] = {"/", "/", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_SLASH, 0, 0},
    // Function keys
    [HidKeynoteActionF1] = {"F1", "F1", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F1, 0, 0},
    [HidKeynoteActionF2] = {"F2", "F2", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F2, 0, 0},
    [HidKeynoteActionF3] = {"F3", "F3", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F3, 0, 0},
    [HidKeynoteActionF4] = {"F4", "F4", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F4, 0, 0},
    [HidKeynoteActionF5] = {"F5", "F5", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F5, 0, 0},
    [HidKeynoteActionF6] = {"F6", "F6", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F6, 0, 0},
    [HidKeynoteActionF7] = {"F7", "F7", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F7, 0, 0},
    [HidKeynoteActionF8] = {"F8", "F8", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F8, 0, 0},
    [HidKeynoteActionF9] = {"F9", "F9", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F9, 0, 0},
    [HidKeynoteActionF10] = {"F10", "F10", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F10, 0, 0},
    [HidKeynoteActionF11] = {"F11", "F11", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F11, 0, 0},
    [HidKeynoteActionF12] = {"F12", "F12", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_F12, 0, 0},
    // Navigation keys
    [HidKeynoteActionHome] = {"Home", "Hom", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_HOME, 0, 0},
    [HidKeynoteActionEnd] = {"End", "End", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_END, 0, 0},
    [HidKeynoteActionPageUp] = {"Page Up", "PUp", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_PAGE_UP, 0, 0},
    [HidKeynoteActionPageDown] = {"Page Down", "PDn", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_PAGE_DOWN, 0, 0},
    [HidKeynoteActionInsert] = {"Insert", "Ins", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_INSERT, 0, 0},
    // Extra keys not on touch keyboard
    [HidKeynoteActionCapsLock] = {"CapsLock", "Cap", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_CAPS_LOCK, 0, 0},
    [HidKeynoteActionPrintScreen] = {"PrintScreen", "PrSc", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_PRINT_SCREEN, 0, 0},
    [HidKeynoteActionScrollLock] = {"ScrollLock", "ScLk", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_SCROLL_LOCK, 0, 0},
    [HidKeynoteActionPause] = {"Pause", "Paus", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_PAUSE, 0, 0},
    [HidKeynoteActionContextMenu] = {"Context Menu", "Menu", HidKeynoteActionTypeKeyboard, HID_KEYBOARD_APPLICATION, 0, 0},
    [HidKeynoteActionCustomMacro] = {"Custom Macro...", "Macro", HidKeynoteActionTypeCustomMacro, 0, 0, 0},
    [HidKeynoteActionBadKbScript] = {"BadKB Skript...", "BdKB", HidKeynoteActionTypeBadKbScript, 0, 0, 0},
};

static uint16_t hid_keynote_get_modifier_mask(HidKeynoteModel* model) {
    uint16_t mask = 0;
    if(model->mod_shift) mask |= KEY_MOD_LEFT_SHIFT;
    if(model->mod_alt) mask |= KEY_MOD_LEFT_ALT;
    if(model->mod_ctrl) mask |= KEY_MOD_LEFT_CTRL;
    if(model->mod_gui) mask |= KEY_MOD_LEFT_GUI;
    return mask;
}

static bool* hid_keynote_get_modifier_state(HidKeynoteModel* model, uint16_t modifier_code) {
    if(modifier_code == KEY_MOD_LEFT_SHIFT) return &model->mod_shift;
    if(modifier_code == KEY_MOD_LEFT_ALT) return &model->mod_alt;
    if(modifier_code == KEY_MOD_LEFT_CTRL) return &model->mod_ctrl;
    if(modifier_code == KEY_MOD_LEFT_GUI) return &model->mod_gui;
    return NULL;
}

static void hid_keynote_draw_round_button(
    Canvas* canvas,
    uint8_t x,
    uint8_t y,
    bool pressed,
    const char* label) {
    canvas_draw_icon(canvas, x, y, &I_Button_18x18);
    if(pressed) {
        elements_slightly_rounded_box(canvas, x + 3, y + 2, 13, 13);
        canvas_set_color(canvas, ColorWhite);
    }
    canvas_draw_str_aligned(canvas, x + 9, y + 10, AlignCenter, AlignCenter, label);
    canvas_set_color(canvas, ColorBlack);
}

static void hid_keynote_draw_wide_button(
    Canvas* canvas,
    uint8_t x,
    uint8_t y,
    uint8_t text_x,
    bool pressed,
    const char* label) {
    canvas_draw_icon(canvas, x, y, &I_Space_65x18);
    if(pressed) {
        elements_slightly_rounded_box(canvas, x + 3, y + 2, 60, 13);
        canvas_set_color(canvas, ColorWhite);
    }
    canvas_draw_str_aligned(canvas, text_x, y + 9, AlignCenter, AlignCenter, label);
    canvas_set_color(canvas, ColorBlack);
}

static void hid_keynote_draw_wide_button_vertical(
    Canvas* canvas,
    uint8_t x,
    uint8_t y,
    uint8_t text_x,
    bool pressed,
    const char* label) {
    canvas_draw_icon(canvas, x, y, &I_Space_60x18);
    if(pressed) {
        elements_slightly_rounded_box(canvas, x + 3, y + 2, 55, 13);
        canvas_set_color(canvas, ColorWhite);
    }
    canvas_draw_str_aligned(canvas, text_x, y + 9, AlignCenter, AlignCenter, label);
    canvas_set_color(canvas, ColorBlack);
}

static void hid_keynote_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    HidKeynoteModel* model = context;

    // Header
#ifdef HID_TRANSPORT_BLE
    if(model->connected) {
        canvas_draw_icon(canvas, 0, 0, &I_Ble_connected_15x15);
    } else {
        canvas_draw_icon(canvas, 0, 0, &I_Ble_disconnected_15x15);
    }
#endif

    canvas_set_font(canvas, FontPrimary);
    elements_multiline_text_aligned(
        canvas,
        17,
        3,
        AlignLeft,
        AlignTop,
        model->mode == HidKeynoteModeConfigured ? "Keynote CFG" : "Keynote");

    canvas_draw_icon(canvas, 68, 2, &I_Pin_back_arrow_10x8);
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(canvas, 127, 3, AlignRight, AlignTop, "Hold to exit");

    hid_keynote_draw_round_button(
        canvas,
        21,
        24,
        model->up_pressed,
        hid_keynote_action_get_short_name(model->actions[HidKeynoteButtonUp]));
    hid_keynote_draw_round_button(
        canvas,
        21,
        45,
        model->down_pressed,
        hid_keynote_action_get_short_name(model->actions[HidKeynoteButtonDown]));
    hid_keynote_draw_round_button(
        canvas,
        0,
        45,
        model->left_pressed,
        hid_keynote_action_get_short_name(model->actions[HidKeynoteButtonLeft]));
    hid_keynote_draw_round_button(
        canvas,
        42,
        45,
        model->right_pressed,
        hid_keynote_action_get_short_name(model->actions[HidKeynoteButtonRight]));
    hid_keynote_draw_wide_button(
        canvas,
        63,
        24,
        95,
        model->ok_pressed,
        hid_keynote_action_get_short_name(model->actions[HidKeynoteButtonOk]));
    hid_keynote_draw_wide_button(
        canvas,
        63,
        45,
        95,
        model->back_pressed,
        hid_keynote_action_get_short_name(model->actions[HidKeynoteButtonBack]));

#ifdef HID_TRANSPORT_BLE
    if(model->hid && model->hid->badkb_active_script) {
        BadUsbState* st = bad_usb_script_get_state(model->hid->badkb_active_script);
        if(st && st->state != BadUsbStateIdle) {
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_box(canvas, 0, 54, 128, 10);
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_frame(canvas, 0, 54, 128, 10);
            canvas_set_font(canvas, FontSecondary);
            char prog[32];
            if(st->state == BadUsbStateDone) {
                snprintf(prog, sizeof(prog), "BadKB fertig");
            } else if(st->line_nb > 0) {
                uint8_t pct = (st->line_cur * 100) / st->line_nb;
                snprintf(
                    prog, sizeof(prog), "BadKB %lu/%lu %u%%",
                    (unsigned long)st->line_cur, (unsigned long)st->line_nb, pct);
                uint8_t bar_w = (pct * 126) / 100;
                canvas_draw_box(canvas, 1, 55, bar_w, 8);
                canvas_set_color(canvas, ColorWhite);
            } else {
                snprintf(prog, sizeof(prog), "BadKB...");
            }
            canvas_draw_str_aligned(canvas, 64, 58, AlignCenter, AlignTop, prog);
        }
    }
#endif

    if(model->hid->app_locked) {
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_icon(canvas, 119, 2, &I_Lock_7x8);
    }
}

static void hid_keynote_draw_vertical_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    HidKeynoteModel* model = context;

    // Header
#ifdef HID_TRANSPORT_BLE
    if(model->connected) {
        canvas_draw_icon(canvas, 0, 0, &I_Ble_connected_15x15);
    } else {
        canvas_draw_icon(canvas, 0, 0, &I_Ble_disconnected_15x15);
    }
    canvas_set_font(canvas, FontPrimary);
    elements_multiline_text_aligned(
        canvas,
        20,
        3,
        AlignLeft,
        AlignTop,
        model->mode == HidKeynoteModeConfigured ? "Keynote CFG" : "Keynote");
#else
    canvas_set_font(canvas, FontPrimary);
    elements_multiline_text_aligned(
        canvas,
        12,
        3,
        AlignLeft,
        AlignTop,
        model->mode == HidKeynoteModeConfigured ? "Keynote CFG" : "Keynote");
#endif

    canvas_draw_icon(canvas, 2, 18, &I_Pin_back_arrow_10x8);
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(canvas, 15, 19, AlignLeft, AlignTop, "Hold to exit");

    const uint8_t x_2 = 23;
    const uint8_t x_1 = 2;
    const uint8_t x_3 = 44;

    const uint8_t y_1 = 44;
    const uint8_t y_2 = 65;

    hid_keynote_draw_round_button(
        canvas,
        x_2,
        y_1,
        model->up_pressed,
        hid_keynote_action_get_short_name(model->actions[HidKeynoteButtonUp]));
    hid_keynote_draw_round_button(
        canvas,
        x_2,
        y_2,
        model->down_pressed,
        hid_keynote_action_get_short_name(model->actions[HidKeynoteButtonDown]));
    hid_keynote_draw_round_button(
        canvas,
        x_1,
        y_2,
        model->left_pressed,
        hid_keynote_action_get_short_name(model->actions[HidKeynoteButtonLeft]));
    hid_keynote_draw_round_button(
        canvas,
        x_3,
        y_2,
        model->right_pressed,
        hid_keynote_action_get_short_name(model->actions[HidKeynoteButtonRight]));
    hid_keynote_draw_wide_button_vertical(
        canvas,
        2,
        86,
        31,
        model->ok_pressed,
        hid_keynote_action_get_short_name(model->actions[HidKeynoteButtonOk]));
    hid_keynote_draw_wide_button_vertical(
        canvas,
        2,
        107,
        31,
        model->back_pressed,
        hid_keynote_action_get_short_name(model->actions[HidKeynoteButtonBack]));
}

// ---- Custom macro parsing/playback ----
// Syntax (';'-separated steps, each one of):
//   MOD[+MOD...]+KEYNAME   e.g. "CTRL+b", "CTRL+SHIFT+TAB"  (KEYNAME matched case-insensitively
//                          against any HidKeynoteActionTypeKeyboard action's name/short_name)
//   STRING <text>          types <text> (rest of the step) char by char via the ASCII->HID map
//   DELAY <ms>             pause
// e.g. "CTRL+b;STRING new-window;DELAY 100"

static bool hid_keynote_macro_find_modifier(const char* token, uint16_t* out_bit) {
    if(strcasecmp(token, "CTRL") == 0) {
        *out_bit = KEY_MOD_LEFT_CTRL;
        return true;
    }
    if(strcasecmp(token, "ALT") == 0) {
        *out_bit = KEY_MOD_LEFT_ALT;
        return true;
    }
    if(strcasecmp(token, "SHIFT") == 0) {
        *out_bit = KEY_MOD_LEFT_SHIFT;
        return true;
    }
    if(strcasecmp(token, "GUI") == 0) {
        *out_bit = KEY_MOD_LEFT_GUI;
        return true;
    }
    return false;
}

static bool hid_keynote_macro_find_keycode(const char* token, uint16_t* out_code) {
    for(size_t i = 0; i < HidKeynoteActionCount; i++) {
        const HidKeynoteActionDef* def = &hid_keynote_action_defs[i];
        if(def->type != HidKeynoteActionTypeKeyboard) continue;
        if(strcasecmp(token, def->name) == 0 || strcasecmp(token, def->short_name) == 0) {
            *out_code = def->code;
            return true;
        }
    }
    return false;
}

static char* hid_keynote_macro_trim(char* s) {
    while(*s == ' ') s++;
    size_t len = strlen(s);
    while(len > 0 && s[len - 1] == ' ') s[--len] = '\0';
    return s;
}

static bool hid_keynote_macro_parse_combo(char* step, uint16_t* out_code) {
    uint16_t mods = 0;
    uint16_t key_code = 0;
    bool got_key = false;

    char* token = strtok(step, "+");
    while(token != NULL) {
        token = hid_keynote_macro_trim(token);
        uint16_t mod_bit;
        uint16_t key_code_candidate;
        if(hid_keynote_macro_find_modifier(token, &mod_bit)) {
            mods |= mod_bit;
        } else if(hid_keynote_macro_find_keycode(token, &key_code_candidate)) {
            key_code = key_code_candidate;
            got_key = true;
        } else {
            return false;
        }
        token = strtok(NULL, "+");
    }

    if(!got_key) return false;
    *out_code = key_code | mods;
    return true;
}

// Shared by hid_keynote_play_macro() (dry_run=false) and hid_keynote_macro_is_valid()
// (dry_run=true, no hid needed / no HID traffic sent - just checks every step parses).
static bool hid_keynote_macro_run(Hid* hid, const char* macro, bool dry_run) {
    if(macro == NULL || macro[0] == '\0') return false;

    char buf[KEYNOTE_MACRO_MAX_LEN];
    strncpy(buf, macro, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    bool any_step = false;
    char* step = strtok(buf, ";");
    while(step != NULL) {
        step = hid_keynote_macro_trim(step);
        if(step[0] == '\0') {
            step = strtok(NULL, ";");
            continue;
        }
        any_step = true;

        if(strncasecmp(step, "STRING ", 7) == 0) {
            if(!dry_run) {
                for(const char* c = step + 7; *c != '\0'; c++) {
                    uint16_t code = HID_ASCII_TO_KEY(*c);
                    if(code == HID_KEYBOARD_NONE) continue;
                    hid_hal_keyboard_press(hid, code);
                    furi_delay_ms(10);
                    hid_hal_keyboard_release(hid, code);
                    furi_delay_ms(10);
                }
            }
        } else if(strncasecmp(step, "DELAY", 5) == 0 && (step[5] == ' ' || step[5] == '\0')) {
            int ms = atoi(step + 5);
            if(ms <= 0) return false;
            if(!dry_run) furi_delay_ms((uint32_t)ms);
        } else {
            uint16_t code;
            if(!hid_keynote_macro_parse_combo(step, &code)) return false;
            if(!dry_run) {
                hid_hal_keyboard_press(hid, code);
                furi_delay_ms(30);
                hid_hal_keyboard_release(hid, code);
            }
        }

        if(!dry_run) furi_delay_ms(30);
        step = strtok(NULL, ";");
    }

    return any_step;
}

void hid_keynote_play_macro(Hid* hid, const char* macro) {
    hid_keynote_macro_run(hid, macro, false);
}

bool hid_keynote_macro_is_valid(const char* macro) {
    return hid_keynote_macro_run(NULL, macro, true);
}

#ifdef HID_TRANSPORT_BLE
static void hid_keynote_badkb_cleanup_if_done(Hid* hid) {
    if(!hid->badkb_active_script) return;
    BadUsbState* st = bad_usb_script_get_state(hid->badkb_active_script);
    if(st && (st->state == BadUsbStateDone || st->state == BadUsbStateIdle ||
              st->state == BadUsbStateScriptError || st->state == BadUsbStateFileError)) {
        bad_usb_script_close(hid->badkb_active_script);
        hid->badkb_active_script = NULL;
        hid->badkb_active_button = -1;
    }
}
#endif

bool hid_keynote_action_is_modifier(HidKeynoteAction action, uint16_t* out_mod_bit) {
    if(action >= HidKeynoteActionCount) return false;
    const HidKeynoteActionDef* def = &hid_keynote_action_defs[action];
    if(def->type != HidKeynoteActionTypeModifier) return false;
    if(out_mod_bit) *out_mod_bit = def->code;
    return true;
}

static void hid_keynote_handle_action(
    HidKeynote* hid_keynote,
    HidKeynoteModel* model,
    HidKeynoteButton button,
    HidKeynoteAction action,
    InputEvent* event,
    uint8_t acceleration) {
#ifdef HID_TRANSPORT_BLE
    hid_keynote_badkb_cleanup_if_done(hid_keynote->hid);
#endif

    const HidKeynoteActionDef* def = &hid_keynote_action_defs[action];

    switch(def->type) {
    case HidKeynoteActionTypeKeyboard:
        if(event->type == InputTypePress) {
            hid_hal_keyboard_press(hid_keynote->hid, def->code | hid_keynote_get_modifier_mask(model));
        } else if(event->type == InputTypeRelease) {
            hid_hal_keyboard_release(hid_keynote->hid, def->code | hid_keynote_get_modifier_mask(model));
        }
        break;
    case HidKeynoteActionTypeConsumer:
        if(event->type == InputTypePress) {
            hid_hal_consumer_key_press(hid_keynote->hid, def->code);
        } else if(event->type == InputTypeRelease) {
            hid_hal_consumer_key_release(hid_keynote->hid, def->code);
        }
        break;
    case HidKeynoteActionTypeModifier: {
        bool* state = hid_keynote_get_modifier_state(model, def->code);
        if(state) {
            if(event->type == InputTypePress) {
                *state = true;
                hid_hal_keyboard_press(hid_keynote->hid, def->code);
            } else if(event->type == InputTypeRelease) {
                *state = false;
                hid_hal_keyboard_release(hid_keynote->hid, def->code);
            }
        }
        break;
    }
    case HidKeynoteActionTypeMouseButton:
        if(event->type == InputTypePress) {
            hid_hal_mouse_press(hid_keynote->hid, def->code);
        } else if(event->type == InputTypeRelease) {
            hid_hal_mouse_release(hid_keynote->hid, def->code);
        }
        break;
    case HidKeynoteActionTypeMouseMove:
        if(event->type == InputTypePress) {
            hid_hal_mouse_move(hid_keynote->hid, def->x, def->y);
        } else if(event->type == InputTypeRepeat) {
            for(uint8_t i = acceleration; i > 1; i -= 2) {
                hid_hal_mouse_move(
                    hid_keynote->hid,
                    def->x < 0 ? -MOUSE_MOVE_LONG : def->x > 0 ? MOUSE_MOVE_LONG : 0,
                    def->y < 0 ? -MOUSE_MOVE_LONG : def->y > 0 ? MOUSE_MOVE_LONG : 0);
            }
        }
        break;
    case HidKeynoteActionTypeMouseScroll:
        if(event->type == InputTypePress || event->type == InputTypeRepeat) {
            hid_hal_mouse_scroll(hid_keynote->hid, def->x);
        }
        break;
    case HidKeynoteActionTypeMacroBack:
        if(event->type == InputTypeShort) {
            hid_hal_keyboard_press(hid_keynote->hid, HID_KEYBOARD_DELETE);
            hid_hal_keyboard_release(hid_keynote->hid, HID_KEYBOARD_DELETE);
            hid_hal_consumer_key_press(hid_keynote->hid, HID_CONSUMER_AC_BACK);
            hid_hal_consumer_key_release(hid_keynote->hid, HID_CONSUMER_AC_BACK);
        }
        break;
    case HidKeynoteActionTypeMacroSeq:
        if(event->type == InputTypeShort) {
            hid_hal_keyboard_press(hid_keynote->hid, def->code);
            furi_delay_ms(30);
            hid_hal_keyboard_release(hid_keynote->hid, def->code);
            if(def->code2) {
                furi_delay_ms(30);
                hid_hal_keyboard_press(hid_keynote->hid, def->code2);
                furi_delay_ms(30);
                hid_hal_keyboard_release(hid_keynote->hid, def->code2);
            }
            if(def->code3) {
                furi_delay_ms(30);
                hid_hal_keyboard_press(hid_keynote->hid, def->code3);
                furi_delay_ms(30);
                hid_hal_keyboard_release(hid_keynote->hid, def->code3);
            }
        }
        break;
    case HidKeynoteActionTypeCustomMacro:
        if(event->type == InputTypeShort) {
            hid_keynote_play_macro(hid_keynote->hid, hid_keynote->hid->keynote_macro[button]);
        }
        break;
#ifdef HID_TRANSPORT_BLE
    case HidKeynoteActionTypeBadKbScript:
        if(event->type == InputTypeShort) {
            Hid* hid = hid_keynote->hid;
            const char* script_path = hid->keynote_badkb_script[button];
            if(script_path[0] == '\0') break;

            if(hid->badkb_active_script) {
                bad_usb_script_stop(hid->badkb_active_script);
                bad_usb_script_close(hid->badkb_active_script);
                hid->badkb_active_script = NULL;
                if(hid->badkb_active_button == (int8_t)button) {
                    hid->badkb_active_button = -1;
                    break;
                }
            }

            hid->badkb_iface = BadUsbHidInterfaceBlePassthrough;
            memset(&hid->badkb_hid_cfg, 0, sizeof(hid->badkb_hid_cfg));
            FuriString* fp = furi_string_alloc_set_str(script_path);
            hid->badkb_active_script = bad_usb_script_open(
                fp, &hid->badkb_iface, &hid->badkb_hid_cfg, false, hid->ble_hid_profile);
            furi_string_free(fp);

            if(hid->badkb_active_script) {
                bad_usb_script_start(hid->badkb_active_script);
                hid->badkb_active_button = (int8_t)button;
            }
        }
        break;
#else
    case HidKeynoteActionTypeBadKbScript:
        break;
#endif
    }
}

static void hid_keynote_process(HidKeynote* hid_keynote, InputEvent* event) {
    with_view_model(
        hid_keynote->view,
        HidKeynoteModel * model,
        {
            model->acceleration = (event->type == InputTypePress)   ? 1 :
                                  (event->type == InputTypeRelease) ? 0 :
                                  (model->acceleration >= 20)       ? 20 :
                                                                      model->acceleration + 1;

            if(event->key == InputKeyUp) {
                if(event->type == InputTypePress) model->up_pressed = true;
                if(event->type == InputTypeRelease) model->up_pressed = false;
                hid_keynote_handle_action(
                    hid_keynote,
                    model,
                    HidKeynoteButtonUp,
                    model->actions[HidKeynoteButtonUp],
                    event,
                    model->acceleration);
            } else if(event->key == InputKeyDown) {
                if(event->type == InputTypePress) model->down_pressed = true;
                if(event->type == InputTypeRelease) model->down_pressed = false;
                hid_keynote_handle_action(
                    hid_keynote,
                    model,
                    HidKeynoteButtonDown,
                    model->actions[HidKeynoteButtonDown],
                    event,
                    model->acceleration);
            } else if(event->key == InputKeyLeft) {
                if(event->type == InputTypePress) model->left_pressed = true;
                if(event->type == InputTypeRelease) model->left_pressed = false;
                hid_keynote_handle_action(
                    hid_keynote,
                    model,
                    HidKeynoteButtonLeft,
                    model->actions[HidKeynoteButtonLeft],
                    event,
                    model->acceleration);
            } else if(event->key == InputKeyRight) {
                if(event->type == InputTypePress) model->right_pressed = true;
                if(event->type == InputTypeRelease) model->right_pressed = false;
                hid_keynote_handle_action(
                    hid_keynote,
                    model,
                    HidKeynoteButtonRight,
                    model->actions[HidKeynoteButtonRight],
                    event,
                    model->acceleration);
            } else if(event->key == InputKeyOk) {
                if(event->type == InputTypePress) model->ok_pressed = true;
                if(event->type == InputTypeRelease) model->ok_pressed = false;
                hid_keynote_handle_action(
                    hid_keynote,
                    model,
                    HidKeynoteButtonOk,
                    model->actions[HidKeynoteButtonOk],
                    event,
                    model->acceleration);
            } else if(event->key == InputKeyBack) {
                if(event->type == InputTypePress) model->back_pressed = true;
                if(event->type == InputTypeRelease) model->back_pressed = false;
                hid_keynote_handle_action(
                    hid_keynote,
                    model,
                    HidKeynoteButtonBack,
                    model->actions[HidKeynoteButtonBack],
                    event,
                    model->acceleration);
            }
        },
        true);
}

static bool hid_keynote_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    HidKeynote* hid_keynote = context;
    if(hid_handle_global_lock_input(hid_keynote->hid, event)) return true;
    bool consumed = false;

    if(event->type == InputTypeLong && event->key == InputKeyBack) {
        hid_hal_keyboard_release_all(hid_keynote->hid);
        with_view_model(
            hid_keynote->view,
            HidKeynoteModel * model,
            {
                model->mod_shift = false;
                model->mod_alt = false;
                model->mod_ctrl = false;
                model->mod_gui = false;
            },
            true);
    } else {
        hid_keynote_process(hid_keynote, event);
        consumed = true;
    }

    return consumed;
}

HidKeynote* hid_keynote_alloc(Hid* hid) {
    HidKeynote* hid_keynote = malloc(sizeof(HidKeynote));
    hid_keynote->view = view_alloc();
    hid_keynote->hid = hid;
    view_set_context(hid_keynote->view, hid_keynote);
    view_allocate_model(hid_keynote->view, ViewModelTypeLocking, sizeof(HidKeynoteModel));
    view_set_draw_callback(hid_keynote->view, hid_keynote_draw_callback);
    view_set_input_callback(hid_keynote->view, hid_keynote_input_callback);
    with_view_model(
        hid_keynote->view,
        HidKeynoteModel * model,
        { model->hid = hid;
            model->mode = HidKeynoteModeDefault;
            hid_keynote_apply_mode(hid_keynote, model);
        },
        true);
    return hid_keynote;
}

void hid_keynote_free(HidKeynote* hid_keynote) {
    furi_assert(hid_keynote);
    view_free(hid_keynote->view);
    free(hid_keynote);
}

View* hid_keynote_get_view(HidKeynote* hid_keynote) {
    furi_assert(hid_keynote);
    return hid_keynote->view;
}

void hid_keynote_set_connected_status(HidKeynote* hid_keynote, bool connected) {
    furi_assert(hid_keynote);
    with_view_model(
        hid_keynote->view, HidKeynoteModel * model, { model->connected = connected; }, true);
}

void hid_keynote_set_orientation(HidKeynote* hid_keynote, bool vertical) {
    furi_assert(hid_keynote);

    if(vertical) {
        view_set_draw_callback(hid_keynote->view, hid_keynote_draw_vertical_callback);
        view_set_orientation(hid_keynote->view, ViewOrientationVerticalFlip);

    } else {
        view_set_draw_callback(hid_keynote->view, hid_keynote_draw_callback);
        view_set_orientation(hid_keynote->view, ViewOrientationHorizontal);
    }
}

const char* hid_keynote_action_get_name(HidKeynoteAction action) {
    if(action >= HidKeynoteActionCount) {
        return "?";
    }

    return hid_keynote_action_defs[action].name;
}

const char* hid_keynote_action_get_short_name(HidKeynoteAction action) {
    if(action >= HidKeynoteActionCount) {
        return "?";
    }

    return hid_keynote_action_defs[action].short_name;
}

HidKeynoteAction hid_keynote_get_default_action(HidKeynoteButton button) {
    if(button >= HidKeynoteButtonCount) {
        return HidKeynoteActionArrowUp;
    }

    return hid_keynote_default_actions[button];
}

HidKeynoteAction hid_keynote_get_action(HidKeynote* hid_keynote, HidKeynoteButton button) {
    furi_assert(hid_keynote);
    furi_assert(button < HidKeynoteButtonCount);
    return hid_keynote->hid->keynote_actions[button];
}

void hid_keynote_set_action(
    HidKeynote* hid_keynote,
    HidKeynoteButton button,
    HidKeynoteAction action) {
    furi_assert(hid_keynote);
    furi_assert(button < HidKeynoteButtonCount);
    furi_assert(action < HidKeynoteActionCount);

    hid_keynote->hid->keynote_actions[button] = action;
    with_view_model(
        hid_keynote->view,
        HidKeynoteModel * model,
        {
            if(model->mode == HidKeynoteModeConfigured) {
                model->actions[button] = action;
            }
        },
        true);
}

void hid_keynote_reset_actions(HidKeynote* hid_keynote) {
    furi_assert(hid_keynote);
    for(size_t i = 0; i < HidKeynoteButtonCount; i++) {
        hid_keynote_set_action(hid_keynote, i, hid_keynote_get_default_action(i));
    }
}

void hid_keynote_sync_actions(HidKeynote* hid_keynote) {
    furi_assert(hid_keynote);
    with_view_model(
        hid_keynote->view,
        HidKeynoteModel * model,
        { hid_keynote_apply_mode(hid_keynote, model); },
        true);
}

void hid_keynote_set_mode(HidKeynote* hid_keynote, HidKeynoteMode mode) {
    furi_assert(hid_keynote);
    with_view_model(
        hid_keynote->view,
        HidKeynoteModel * model,
        {
            model->mode = mode;
            hid_keynote_apply_mode(hid_keynote, model);
        },
        true);
}
