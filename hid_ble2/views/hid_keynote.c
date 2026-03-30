#include "hid_keynote.h"
#include <gui/elements.h>
#include "../hid.h"

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
    HidKeynoteMode mode;
} HidKeynoteModel;

static const HidKeynoteAction hid_keynote_default_actions[HidKeynoteButtonCount] = {
    [HidKeynoteButtonUp] = HidKeynoteActionArrowUp,
    [HidKeynoteButtonDown] = HidKeynoteActionArrowDown,
    [HidKeynoteButtonLeft] = HidKeynoteActionArrowLeft,
    [HidKeynoteButtonRight] = HidKeynoteActionArrowRight,
    [HidKeynoteButtonOk] = HidKeynoteActionSpace,
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
} HidKeynoteActionType;

typedef struct {
    const char* name;
    const char* short_name;
    HidKeynoteActionType type;
    uint16_t code;
    int8_t x;
    int8_t y;
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
    [HidKeynoteActionCtrl] = {"Ctrl (Toggle)", "Ctrl", HidKeynoteActionTypeModifier, KEY_MOD_LEFT_CTRL, 0, 0},
    [HidKeynoteActionAlt] = {"Alt (Toggle)", "Alt", HidKeynoteActionTypeModifier, KEY_MOD_LEFT_ALT, 0, 0},
    [HidKeynoteActionShift] = {"Shift (Toggle)", "Shf", HidKeynoteActionTypeModifier, KEY_MOD_LEFT_SHIFT, 0, 0},
    [HidKeynoteActionGui] = {"GUI (Toggle)", "GUI", HidKeynoteActionTypeModifier, KEY_MOD_LEFT_GUI, 0, 0},
    [HidKeynoteActionMediaPlayPause] = {"Media Play/Pause", "Play", HidKeynoteActionTypeConsumer, HID_CONSUMER_PLAY_PAUSE, 0, 0},
    [HidKeynoteActionMediaPlay] = {"Media Play", "Play", HidKeynoteActionTypeConsumer, HID_CONSUMER_PLAY, 0, 0},
    [HidKeynoteActionMediaPause] = {"Media Pause", "Pause", HidKeynoteActionTypeConsumer, HID_CONSUMER_PAUSE, 0, 0},
    [HidKeynoteActionMediaStop] = {"Media Stop", "Stop", HidKeynoteActionTypeConsumer, HID_CONSUMER_STOP, 0, 0},
    [HidKeynoteActionMediaNext] = {"Media Next", "Next", HidKeynoteActionTypeConsumer, HID_CONSUMER_SCAN_NEXT_TRACK, 0, 0},
    [HidKeynoteActionMediaPrev] = {"Media Prev", "Prev", HidKeynoteActionTypeConsumer, HID_CONSUMER_SCAN_PREVIOUS_TRACK, 0, 0},
    [HidKeynoteActionMediaMute] = {"Media Mute", "Mute", HidKeynoteActionTypeConsumer, HID_CONSUMER_MUTE, 0, 0},
    [HidKeynoteActionMediaVolUp] = {"Media Vol+", "Vol+", HidKeynoteActionTypeConsumer, HID_CONSUMER_VOLUME_INCREMENT, 0, 0},
    [HidKeynoteActionMediaVolDown] = {"Media Vol-", "Vol-", HidKeynoteActionTypeConsumer, HID_CONSUMER_VOLUME_DECREMENT, 0, 0},
    [HidKeynoteActionCtrlC] = {"Ctrl+C", "C+C", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_C, 0, 0},
    [HidKeynoteActionCtrlV] = {"Ctrl+V", "C+V", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_V, 0, 0},
    [HidKeynoteActionCtrlX] = {"Ctrl+X", "C+X", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_X, 0, 0},
    [HidKeynoteActionCtrlA] = {"Ctrl+A", "C+A", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_A, 0, 0},
    [HidKeynoteActionCtrlZ] = {"Ctrl+Z", "C+Z", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_Z, 0, 0},
    [HidKeynoteActionCtrlY] = {"Ctrl+Y", "C+Y", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_Y, 0, 0},
    [HidKeynoteActionCtrlS] = {"Ctrl+S", "C+S", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_S, 0, 0},
    [HidKeynoteActionCtrlF] = {"Ctrl+F", "C+F", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_F, 0, 0},
    [HidKeynoteActionCtrlW] = {"Ctrl+W", "C+W", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_W, 0, 0},
    [HidKeynoteActionCtrlT] = {"Ctrl+T", "C+T", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_CTRL | HID_KEYBOARD_T, 0, 0},
    [HidKeynoteActionCtrlShiftT] = {"Ctrl+Shift+T", "CST", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_CTRL | KEY_MOD_LEFT_SHIFT | HID_KEYBOARD_T, 0, 0},
    [HidKeynoteActionAltF4] = {"Alt+F4", "A+4", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_ALT | HID_KEYBOARD_F4, 0, 0},
    [HidKeynoteActionAltTab] = {"Alt+Tab", "A+T", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_ALT | HID_KEYBOARD_TAB, 0, 0},
    [HidKeynoteActionCtrlAltDel] = {"Ctrl+Alt+Del", "CAD", HidKeynoteActionTypeKeyboard, KEY_MOD_LEFT_CTRL | KEY_MOD_LEFT_ALT | HID_KEYBOARD_DELETE_FORWARD, 0, 0},
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

static void hid_keynote_handle_action(
    HidKeynote* hid_keynote,
    HidKeynoteModel* model,
    HidKeynoteAction action,
    InputEvent* event,
    uint8_t acceleration) {
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
    case HidKeynoteActionTypeModifier:
        if(event->type == InputTypeShort) {
            bool* state = hid_keynote_get_modifier_state(model, def->code);
            if(state) {
                *state = !*state;
                if(*state) {
                    hid_hal_keyboard_press(hid_keynote->hid, def->code);
                } else {
                    hid_hal_keyboard_release(hid_keynote->hid, def->code);
                }
            }
        }
        break;
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
                    model->actions[HidKeynoteButtonUp],
                    event,
                    model->acceleration);
            } else if(event->key == InputKeyDown) {
                if(event->type == InputTypePress) model->down_pressed = true;
                if(event->type == InputTypeRelease) model->down_pressed = false;
                hid_keynote_handle_action(
                    hid_keynote,
                    model,
                    model->actions[HidKeynoteButtonDown],
                    event,
                    model->acceleration);
            } else if(event->key == InputKeyLeft) {
                if(event->type == InputTypePress) model->left_pressed = true;
                if(event->type == InputTypeRelease) model->left_pressed = false;
                hid_keynote_handle_action(
                    hid_keynote,
                    model,
                    model->actions[HidKeynoteButtonLeft],
                    event,
                    model->acceleration);
            } else if(event->key == InputKeyRight) {
                if(event->type == InputTypePress) model->right_pressed = true;
                if(event->type == InputTypeRelease) model->right_pressed = false;
                hid_keynote_handle_action(
                    hid_keynote,
                    model,
                    model->actions[HidKeynoteButtonRight],
                    event,
                    model->acceleration);
            } else if(event->key == InputKeyOk) {
                if(event->type == InputTypePress) model->ok_pressed = true;
                if(event->type == InputTypeRelease) model->ok_pressed = false;
                hid_keynote_handle_action(
                    hid_keynote,
                    model,
                    model->actions[HidKeynoteButtonOk],
                    event,
                    model->acceleration);
            } else if(event->key == InputKeyBack) {
                if(event->type == InputTypePress) model->back_pressed = true;
                if(event->type == InputTypeRelease) model->back_pressed = false;
                hid_keynote_handle_action(
                    hid_keynote,
                    model,
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
        {
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
