#include "hid_keynote_key_picker.h"
#include <furi.h>
#include <string.h>
#include <gui/elements.h>
#include <gui/icon_i.h>
#include "../hid.h"
#include "hid_icons.h"

#define TAG "HidKeynoteKeyPicker"

// ---- Keyboard page: 1:1 copy of hid_keyboard layout ----
#define KP_MARGIN_TOP   0
#define KP_MARGIN_LEFT  3
#define KP_KEY_WIDTH    11
#define KP_KEY_HEIGHT   13
#define KP_KEY_PADDING  -1
#define KP_ROW_COUNT    7
#define KP_COLUMN_COUNT 12

typedef struct {
    uint8_t width;
    char key;
    char shift_key;
    const Icon* icon;
    const Icon* icon_shift;
    const Icon* icon_toggled;
    uint8_t value;
} KpKey;

// Exact same keyset as hid_keyboard_keyset
static const KpKey kp_keyset[KP_ROW_COUNT][KP_COLUMN_COUNT] = {
    {
        {.width = 1, .icon = &I_ButtonF1_5x8, .value = HID_KEYBOARD_F1},
        {.width = 1, .icon = &I_ButtonF2_5x8, .value = HID_KEYBOARD_F2},
        {.width = 1, .icon = &I_ButtonF3_5x8, .value = HID_KEYBOARD_F3},
        {.width = 1, .icon = &I_ButtonF4_5x8, .value = HID_KEYBOARD_F4},
        {.width = 1, .icon = &I_ButtonF5_5x8, .value = HID_KEYBOARD_F5},
        {.width = 1, .icon = &I_ButtonF6_5x8, .value = HID_KEYBOARD_F6},
        {.width = 1, .icon = &I_ButtonF7_5x8, .value = HID_KEYBOARD_F7},
        {.width = 1, .icon = &I_ButtonF8_5x8, .value = HID_KEYBOARD_F8},
        {.width = 1, .icon = &I_ButtonF9_5x8, .value = HID_KEYBOARD_F9},
        {.width = 1, .icon = &I_ButtonF10_5x8, .value = HID_KEYBOARD_F10},
        {.width = 1, .icon = &I_ButtonF11_5x8, .value = HID_KEYBOARD_F11},
        {.width = 1, .icon = &I_ButtonF12_5x8, .value = HID_KEYBOARD_F12},
    },
    {
        {.width = 1, .icon = NULL, .key = '1', .shift_key = '!', .value = HID_KEYBOARD_1},
        {.width = 1, .icon = NULL, .key = '2', .shift_key = '@', .value = HID_KEYBOARD_2},
        {.width = 1,
         .icon = NULL,
         .icon_shift = &I_hash_button_9x11,
         .key = '3',
         .value = HID_KEYBOARD_3},
        {.width = 1, .icon = NULL, .key = '4', .shift_key = '$', .value = HID_KEYBOARD_4},
        {.width = 1,
         .icon = NULL,
         .icon_shift = &I_percent_button_9x11,
         .key = '5',
         .value = HID_KEYBOARD_5},
        {.width = 1, .icon = NULL, .key = '6', .shift_key = '^', .value = HID_KEYBOARD_6},
        {.width = 1, .icon = NULL, .key = '7', .shift_key = '&', .value = HID_KEYBOARD_7},
        {.width = 1, .icon = NULL, .key = '8', .shift_key = '*', .value = HID_KEYBOARD_8},
        {.width = 1, .icon = NULL, .key = '9', .shift_key = '(', .value = HID_KEYBOARD_9},
        {.width = 1, .icon = NULL, .key = '0', .shift_key = ')', .value = HID_KEYBOARD_0},
        {.width = 2, .icon = &I_backspace_19x11, .value = HID_KEYBOARD_DELETE},
        {.width = 0, .value = HID_KEYBOARD_DELETE},
    },
    {
        {.width = 1, .icon = NULL, .key = 'q', .shift_key = 'Q', .value = HID_KEYBOARD_Q},
        {.width = 1, .icon = NULL, .key = 'w', .shift_key = 'W', .value = HID_KEYBOARD_W},
        {.width = 1, .icon = NULL, .key = 'e', .shift_key = 'E', .value = HID_KEYBOARD_E},
        {.width = 1, .icon = NULL, .key = 'r', .shift_key = 'R', .value = HID_KEYBOARD_R},
        {.width = 1, .icon = NULL, .key = 't', .shift_key = 'T', .value = HID_KEYBOARD_T},
        {.width = 1, .icon = NULL, .key = 'y', .shift_key = 'Y', .value = HID_KEYBOARD_Y},
        {.width = 1, .icon = NULL, .key = 'u', .shift_key = 'U', .value = HID_KEYBOARD_U},
        {.width = 1, .icon = NULL, .key = 'i', .shift_key = 'I', .value = HID_KEYBOARD_I},
        {.width = 1, .icon = NULL, .key = 'o', .shift_key = 'O', .value = HID_KEYBOARD_O},
        {.width = 1, .icon = NULL, .key = 'p', .shift_key = 'P', .value = HID_KEYBOARD_P},
        {.width = 1,
         .icon = &I_sq_bracket_left_button_9x11,
         .icon_shift = &I_brace_left_button_9x11,
         .value = HID_KEYBOARD_OPEN_BRACKET},
        {.width = 1,
         .icon = &I_sq_bracket_right_button_9x11,
         .icon_shift = &I_brace_right_button_9x11,
         .value = HID_KEYBOARD_CLOSE_BRACKET},
    },
    {
        {.width = 1, .icon = NULL, .key = 'a', .shift_key = 'A', .value = HID_KEYBOARD_A},
        {.width = 1, .icon = NULL, .key = 's', .shift_key = 'S', .value = HID_KEYBOARD_S},
        {.width = 1, .icon = NULL, .key = 'd', .shift_key = 'D', .value = HID_KEYBOARD_D},
        {.width = 1, .icon = NULL, .key = 'f', .shift_key = 'F', .value = HID_KEYBOARD_F},
        {.width = 1, .icon = NULL, .key = 'g', .shift_key = 'G', .value = HID_KEYBOARD_G},
        {.width = 1, .icon = NULL, .key = 'h', .shift_key = 'H', .value = HID_KEYBOARD_H},
        {.width = 1, .icon = NULL, .key = 'j', .shift_key = 'J', .value = HID_KEYBOARD_J},
        {.width = 1, .icon = NULL, .key = 'k', .shift_key = 'K', .value = HID_KEYBOARD_K},
        {.width = 1, .icon = NULL, .key = 'l', .shift_key = 'L', .value = HID_KEYBOARD_L},
        {.width = 1, .icon = NULL, .key = ';', .shift_key = ':', .value = HID_KEYBOARD_SEMICOLON},
        {.width = 2, .icon = &I_Return_10x7, .value = HID_KEYBOARD_RETURN},
        {.width = 0, .value = HID_KEYBOARD_RETURN},
    },
    {
        {.width = 1, .icon = NULL, .key = 'z', .shift_key = 'Z', .value = HID_KEYBOARD_Z},
        {.width = 1, .icon = NULL, .key = 'x', .shift_key = 'X', .value = HID_KEYBOARD_X},
        {.width = 1, .icon = NULL, .key = 'c', .shift_key = 'C', .value = HID_KEYBOARD_C},
        {.width = 1, .icon = NULL, .key = 'v', .shift_key = 'V', .value = HID_KEYBOARD_V},
        {.width = 1, .icon = NULL, .key = 'b', .shift_key = 'B', .value = HID_KEYBOARD_B},
        {.width = 1, .icon = NULL, .key = 'n', .shift_key = 'N', .value = HID_KEYBOARD_N},
        {.width = 1, .icon = NULL, .key = 'm', .shift_key = 'M', .value = HID_KEYBOARD_M},
        {.width = 1, .icon = &I_slash_button_9x11, .shift_key = '?', .value = HID_KEYBOARD_SLASH},
        {.width = 1,
         .icon = &I_backslash_button_9x11,
         .shift_key = '|',
         .value = HID_KEYBOARD_BACKSLASH},
        {.width = 1,
         .icon = &I_backtick_button_9x11,
         .shift_key = '~',
         .value = HID_KEYBOARD_GRAVE_ACCENT},
        {.width = 1, .icon = &I_ButtonUp_7x4, .value = HID_KEYBOARD_UP_ARROW},
        {.width = 1,
         .icon = NULL,
         .icon_shift = &I_underscore_button_9x11,
         .key = '-',
         .value = HID_KEYBOARD_MINUS},
    },
    {
        {.width = 1,
         .icon = &I_Shift_inactive_7x9,
         .icon_toggled = &I_Shift_active_7x9,
         .value = HID_KEYBOARD_L_SHIFT},
        {.width = 1, .icon = NULL, .key = ',', .shift_key = '<', .value = HID_KEYBOARD_COMMA},
        {.width = 1, .icon = NULL, .key = '.', .shift_key = '>', .value = HID_KEYBOARD_DOT},
        {.width = 4, .icon = NULL, .key = ' ', .value = HID_KEYBOARD_SPACEBAR},
        {.width = 0, .value = HID_KEYBOARD_SPACEBAR},
        {.width = 0, .value = HID_KEYBOARD_SPACEBAR},
        {.width = 0, .value = HID_KEYBOARD_SPACEBAR},
        {.width = 1,
         .icon = &I_apostrophe_button_9x11,
         .icon_shift = &I_quote_button_9x11,
         .value = HID_KEYBOARD_APOSTROPHE},
        {.width = 1,
         .icon = &I_equals_button_9x11,
         .shift_key = '+',
         .value = HID_KEYBOARD_EQUAL_SIGN},
        {.width = 1, .icon = &I_ButtonLeft_4x7, .value = HID_KEYBOARD_LEFT_ARROW},
        {.width = 1, .icon = &I_ButtonDown_7x4, .value = HID_KEYBOARD_DOWN_ARROW},
        {.width = 1, .icon = &I_ButtonRight_4x7, .value = HID_KEYBOARD_RIGHT_ARROW},
    },
    {
        {.width = 2,
         .icon = &I_Ctrl_17x10,
         .icon_toggled = &I_Ctrl_active_17x9,
         .value = HID_KEYBOARD_L_CTRL},
        {.width = 0, .value = HID_KEYBOARD_L_CTRL},
        {.width = 2,
         .icon = &I_Alt_17x10,
         .icon_toggled = &I_Alt_active_17x9,
         .value = HID_KEYBOARD_L_ALT},
        {.width = 0, .value = HID_KEYBOARD_L_ALT},
        {.width = 2,
         .icon = &I_Cmd_17x10,
         .icon_toggled = &I_Cmd_active_17x9,
         .value = HID_KEYBOARD_L_GUI},
        {.width = 0, .value = HID_KEYBOARD_L_GUI},
        {.width = 2, .icon = &I_Tab_17x10, .value = HID_KEYBOARD_TAB},
        {.width = 0, .value = HID_KEYBOARD_TAB},
        {.width = 2, .icon = &I_Esc_17x10, .value = HID_KEYBOARD_ESCAPE},
        {.width = 0, .value = HID_KEYBOARD_ESCAPE},
        {.width = 2, .icon = &I_Del_17x10, .value = HID_KEYBOARD_DELETE_FORWARD},
        {.width = 0, .value = HID_KEYBOARD_DELETE_FORWARD},
    },
};

// ---- HID value to HidKeynoteAction mapping ----
static HidKeynoteAction kp_hid_to_action(uint8_t hid_value) {
    switch(hid_value) {
    case HID_KEYBOARD_UP_ARROW:
        return HidKeynoteActionArrowUp;
    case HID_KEYBOARD_DOWN_ARROW:
        return HidKeynoteActionArrowDown;
    case HID_KEYBOARD_LEFT_ARROW:
        return HidKeynoteActionArrowLeft;
    case HID_KEYBOARD_RIGHT_ARROW:
        return HidKeynoteActionArrowRight;
    case HID_KEYBOARD_Q:
        return HidKeynoteActionQ;
    case HID_KEYBOARD_W:
        return HidKeynoteActionW;
    case HID_KEYBOARD_E:
        return HidKeynoteActionE;
    case HID_KEYBOARD_R:
        return HidKeynoteActionR;
    case HID_KEYBOARD_T:
        return HidKeynoteActionT;
    case HID_KEYBOARD_Y:
        return HidKeynoteActionY;
    case HID_KEYBOARD_U:
        return HidKeynoteActionU;
    case HID_KEYBOARD_I:
        return HidKeynoteActionI;
    case HID_KEYBOARD_O:
        return HidKeynoteActionO;
    case HID_KEYBOARD_P:
        return HidKeynoteActionP;
    case HID_KEYBOARD_A:
        return HidKeynoteActionA;
    case HID_KEYBOARD_S:
        return HidKeynoteActionS;
    case HID_KEYBOARD_D:
        return HidKeynoteActionD;
    case HID_KEYBOARD_F:
        return HidKeynoteActionF;
    case HID_KEYBOARD_G:
        return HidKeynoteActionG;
    case HID_KEYBOARD_H:
        return HidKeynoteActionH;
    case HID_KEYBOARD_J:
        return HidKeynoteActionJ;
    case HID_KEYBOARD_K:
        return HidKeynoteActionK;
    case HID_KEYBOARD_L:
        return HidKeynoteActionL;
    case HID_KEYBOARD_Z:
        return HidKeynoteActionZ;
    case HID_KEYBOARD_X:
        return HidKeynoteActionX;
    case HID_KEYBOARD_C:
        return HidKeynoteActionC;
    case HID_KEYBOARD_V:
        return HidKeynoteActionV;
    case HID_KEYBOARD_B:
        return HidKeynoteActionB;
    case HID_KEYBOARD_N:
        return HidKeynoteActionN;
    case HID_KEYBOARD_M:
        return HidKeynoteActionM;
    case HID_KEYBOARD_SPACEBAR:
        return HidKeynoteActionSpace;
    case HID_KEYBOARD_RETURN:
        return HidKeynoteActionEnter;
    case HID_KEYBOARD_TAB:
        return HidKeynoteActionTab;
    case HID_KEYBOARD_ESCAPE:
        return HidKeynoteActionEscape;
    case HID_KEYBOARD_DELETE:
        return HidKeynoteActionBackspace;
    case HID_KEYBOARD_DELETE_FORWARD:
        return HidKeynoteActionDelete;
    case HID_KEYBOARD_L_SHIFT:
        return HidKeynoteActionShift;
    case HID_KEYBOARD_L_CTRL:
        return HidKeynoteActionCtrl;
    case HID_KEYBOARD_L_ALT:
        return HidKeynoteActionAlt;
    case HID_KEYBOARD_L_GUI:
        return HidKeynoteActionGui;
    case HID_KEYBOARD_MINUS:
        return HidKeynoteActionMinus;
    case HID_KEYBOARD_EQUAL_SIGN:
        return HidKeynoteActionEquals;
    case HID_KEYBOARD_OPEN_BRACKET:
        return HidKeynoteActionOpenBracket;
    case HID_KEYBOARD_CLOSE_BRACKET:
        return HidKeynoteActionCloseBracket;
    case HID_KEYBOARD_BACKSLASH:
        return HidKeynoteActionBackslash;
    case HID_KEYBOARD_SEMICOLON:
        return HidKeynoteActionSemicolon;
    case HID_KEYBOARD_APOSTROPHE:
        return HidKeynoteActionApostrophe;
    case HID_KEYBOARD_GRAVE_ACCENT:
        return HidKeynoteActionGraveAccent;
    case HID_KEYBOARD_COMMA:
        return HidKeynoteActionComma;
    case HID_KEYBOARD_DOT:
        return HidKeynoteActionDot;
    case HID_KEYBOARD_SLASH:
        return HidKeynoteActionSlash;
    case HID_KEYBOARD_F1:
        return HidKeynoteActionF1;
    case HID_KEYBOARD_F2:
        return HidKeynoteActionF2;
    case HID_KEYBOARD_F3:
        return HidKeynoteActionF3;
    case HID_KEYBOARD_F4:
        return HidKeynoteActionF4;
    case HID_KEYBOARD_F5:
        return HidKeynoteActionF5;
    case HID_KEYBOARD_F6:
        return HidKeynoteActionF6;
    case HID_KEYBOARD_F7:
        return HidKeynoteActionF7;
    case HID_KEYBOARD_F8:
        return HidKeynoteActionF8;
    case HID_KEYBOARD_F9:
        return HidKeynoteActionF9;
    case HID_KEYBOARD_F10:
        return HidKeynoteActionF10;
    case HID_KEYBOARD_F11:
        return HidKeynoteActionF11;
    case HID_KEYBOARD_F12:
        return HidKeynoteActionF12;
    default:
        return HidKeynoteActionCount; // Invalid / not mapped (e.g. number keys)
    }
}

// ---- Check if action is on the keyboard layout ----
bool hid_keynote_key_picker_action_on_keyboard(HidKeynoteAction action) {
    if(action >= HidKeynoteActionCount) return false;
    for(uint8_t row = 0; row < KP_ROW_COUNT; row++) {
        for(uint8_t col = 0; col < KP_COLUMN_COUNT; col++) {
            if(kp_keyset[row][col].width == 0) continue;
            if(kp_hid_to_action(kp_keyset[row][col].value) == action) return true;
        }
    }
    return false;
}

// Non-keyboard actions (mouse/media/navigation/tmux combos) are NOT handled by this
// view anymore - they used to have their own hand-drawn "page 2/2" grid here, which had
// a layout bug (last row + page indicator overlapped, unreadable) and was redundant with
// the already-existing hid_scene_keynote_action_list.c Submenu, which lists every single
// one of those same actions (see hid_keynote_key_picker_action_on_keyboard() below - the
// filter that Submenu uses) via a safe, scrollable, properly text-rendered widget instead
// of a custom canvas grid. Reached via the "Liste" choice in hid_scene_keynote_picker_choice.c.

// ---- Picker struct and model ----
struct HidKeynoteKeyPicker {
    View* view;
    Hid* hid;
    HidKeynoteKeyPickerCallback callback;
    void* callback_context;
};

typedef struct {
    uint8_t x;
    uint8_t y;
    HidKeynoteAction current_action;
    Hid* hid;
} HidKeynoteKeyPickerModel;

// ---- Keyboard page drawing (1:1 from hid_keyboard) ----
static void kp_draw_key(
    Canvas* canvas,
    uint8_t x,
    uint8_t y,
    KpKey key,
    bool selected,
    bool is_current) {
    if(!key.width) return;

    canvas_set_color(canvas, ColorBlack);
    uint8_t keyWidth = KP_KEY_WIDTH * key.width + KP_KEY_PADDING * (key.width - 1);
    uint8_t px = KP_MARGIN_LEFT + x * (KP_KEY_WIDTH + KP_KEY_PADDING);
    uint8_t py = KP_MARGIN_TOP + y * (KP_KEY_HEIGHT + KP_KEY_PADDING);

    if(selected) {
        elements_slightly_rounded_box(canvas, px, py, keyWidth, KP_KEY_HEIGHT);
        canvas_set_color(canvas, ColorWhite);
    } else if(is_current) {
        elements_slightly_rounded_box(canvas, px, py, keyWidth, KP_KEY_HEIGHT);
        canvas_set_color(canvas, ColorWhite);
        // Corner dots to distinguish from cursor
        canvas_draw_dot(canvas, px + 1, py + 1);
        canvas_draw_dot(canvas, px + keyWidth - 2, py + 1);
        canvas_draw_dot(canvas, px + 1, py + KP_KEY_HEIGHT - 2);
        canvas_draw_dot(canvas, px + keyWidth - 2, py + KP_KEY_HEIGHT - 2);
    } else {
        elements_slightly_rounded_frame(canvas, px, py, keyWidth, KP_KEY_HEIGHT);
    }

    if(key.icon != NULL) {
        const Icon* key_icon = key.icon;
        canvas_draw_icon(
            canvas,
            px + keyWidth / 2 - key_icon->width / 2,
            py + KP_KEY_HEIGHT / 2 - key_icon->height / 2,
            key_icon);
        return;
    }

    if(key.key != 0) {
        char key_str[2] = {key.key, '\0'};
        uint8_t key_offset = 0;
        if(key.value >= HID_KEYBOARD_1 && key.value <= HID_KEYBOARD_0) {
            key_offset = 1;
        }
        canvas_draw_str_aligned(
            canvas,
            px + keyWidth / 2 + 1,
            py + KP_KEY_HEIGHT / 2 + key_offset,
            AlignCenter,
            AlignCenter,
            key_str);
        return;
    }
}

static void kp_draw_keyboard_page(Canvas* canvas, HidKeynoteKeyPickerModel* model) {
    canvas_set_font(canvas, FontKeyboard);

    // Scrolling logic (same as hid_keyboard)
    uint8_t initY = model->y == 0 ? 0 : 1;
    if(model->y > 5) {
        initY = model->y - 4;
    }

    elements_scrollbar(canvas, initY, 3);

    for(uint8_t y = initY; y < KP_ROW_COUNT; y++) {
        const KpKey* row = kp_keyset[y];
        uint8_t x = 0;
        for(uint8_t i = 0; i < KP_COLUMN_COUNT; i++) {
            KpKey key = row[i];
            bool keySelected = (x <= model->x && model->x < (x + key.width)) && y == model->y;
            HidKeynoteAction action = kp_hid_to_action(key.value);
            bool is_current =
                (action < HidKeynoteActionCount) && (action == model->current_action);

            kp_draw_key(canvas, x, y - initY, key, keySelected, is_current && !keySelected);
            x += key.width;
        }
    }
}

// ---- Main draw callback ----
static void hid_keynote_key_picker_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    HidKeynoteKeyPickerModel* model = context;

    kp_draw_keyboard_page(canvas, model);

    // Macro builder feedback: show which modifiers are "held" so far, waiting for the
    // next key to complete a MOD+KEY step (see hid_scene_keynote_key_picker.c). Drawn as
    // a solid cleared box (not just text over the busy keyboard grid) so it's unmissable -
    // without this, pressing OK on e.g. Ctrl looked like nothing happened.
    if(model->hid->macro_builder_active && model->hid->macro_builder_pending_mods) {
        char mods[32] = "Hold: ";
        if(model->hid->macro_builder_pending_mods & KEY_MOD_LEFT_CTRL) strlcat(mods, "CTRL ", sizeof(mods));
        if(model->hid->macro_builder_pending_mods & KEY_MOD_LEFT_ALT) strlcat(mods, "ALT ", sizeof(mods));
        if(model->hid->macro_builder_pending_mods & KEY_MOD_LEFT_SHIFT) strlcat(mods, "SHIFT ", sizeof(mods));
        if(model->hid->macro_builder_pending_mods & KEY_MOD_LEFT_GUI) strlcat(mods, "GUI ", sizeof(mods));

        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 24, 26, 80, 12);
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_frame(canvas, 24, 26, 80, 12);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, mods);
    }

    if(model->hid->app_locked) {
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_icon(canvas, 119, 2, &I_Lock_7x8);
    }
}

// ---- Keyboard page navigation (same as hid_keyboard) ----
static void kp_navigate_keyboard(HidKeynoteKeyPickerModel* model, int8_t dx, int8_t dy) {
    if(dy != 0) {
        do {
            const int delta_sum = model->y + dy;
            model->y = delta_sum < 0 ? KP_ROW_COUNT - 1 : delta_sum % KP_ROW_COUNT;
        } while(kp_keyset[model->y][model->x].value == 0);
    }
    if(dx != 0) {
        do {
            const int delta_sum = model->x + dx;
            model->x = delta_sum < 0 ? KP_COLUMN_COUNT - 1 : delta_sum % KP_COLUMN_COUNT;
        } while(kp_keyset[model->y][model->x].width == 0);
    }
}

// ---- Input callback ----
static bool hid_keynote_key_picker_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    HidKeynoteKeyPicker* picker = context;
    if(hid_handle_global_lock_input(picker->hid, event)) return true;

    if(event->type == InputTypeLong && event->key == InputKeyBack) {
        return false;
    }

    if(event->type != InputTypeShort && event->type != InputTypeRepeat) {
        return true;
    }

    bool consumed = true;

    with_view_model(
        picker->view,
        HidKeynoteKeyPickerModel * model,
        {
            switch(event->key) {
            case InputKeyUp:
                kp_navigate_keyboard(model, 0, -1);
                break;
            case InputKeyDown:
                kp_navigate_keyboard(model, 0, 1);
                break;
            case InputKeyLeft:
                kp_navigate_keyboard(model, -1, 0);
                break;
            case InputKeyRight:
                kp_navigate_keyboard(model, 1, 0);
                break;
            case InputKeyOk: {
                HidKeynoteAction action = kp_hid_to_action(kp_keyset[model->y][model->x].value);
                if(action < HidKeynoteActionCount && picker->callback) {
                    picker->callback(action, picker->callback_context);
                }
                break;
            }
            case InputKeyBack:
                consumed = false;
                break;
            default:
                break;
            }
        },
        true);

    return consumed;
}

// ---- Public API ----
HidKeynoteKeyPicker* hid_keynote_key_picker_alloc(Hid* hid) {
    HidKeynoteKeyPicker* picker = malloc(sizeof(HidKeynoteKeyPicker));
    picker->hid = hid;
    picker->callback = NULL;
    picker->callback_context = NULL;
    picker->view = view_alloc();
    view_set_context(picker->view, picker);
    view_allocate_model(picker->view, ViewModelTypeLocking, sizeof(HidKeynoteKeyPickerModel));
    view_set_draw_callback(picker->view, hid_keynote_key_picker_draw_callback);
    view_set_input_callback(picker->view, hid_keynote_key_picker_input_callback);
    with_view_model(
        picker->view, HidKeynoteKeyPickerModel * model, { model->hid = hid; }, true);
    return picker;
}

void hid_keynote_key_picker_free(HidKeynoteKeyPicker* picker) {
    furi_assert(picker);
    view_free(picker->view);
    free(picker);
}

View* hid_keynote_key_picker_get_view(HidKeynoteKeyPicker* picker) {
    furi_assert(picker);
    return picker->view;
}

void hid_keynote_key_picker_set_callback(
    HidKeynoteKeyPicker* picker,
    HidKeynoteKeyPickerCallback callback,
    void* context) {
    furi_assert(picker);
    picker->callback = callback;
    picker->callback_context = context;
}

void hid_keynote_key_picker_set_selected(HidKeynoteKeyPicker* picker, HidKeynoteAction action) {
    furi_assert(picker);
    with_view_model(
        picker->view,
        HidKeynoteKeyPickerModel * model,
        { model->current_action = action;
            model->x = 0;
            model->y = 0;
            bool found = false;

            for(uint8_t row = 0; row < KP_ROW_COUNT && !found; row++) {
                uint8_t acc_x = 0;
                for(uint8_t col = 0; col < KP_COLUMN_COUNT && !found; col++) {
                    KpKey key = kp_keyset[row][col];
                    if(key.width > 0 && kp_hid_to_action(key.value) == action) {
                        model->x = acc_x;
                        model->y = row;
                        found = true;
                    }
                    acc_x += key.width;
                }
            }
        },
        true);
}
