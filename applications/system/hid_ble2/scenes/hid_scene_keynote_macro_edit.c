#include <stdlib.h>
#include <string.h>

#include "../hid.h"
#include "../views.h"

// Step-by-step macro builder: everything except free-text ("Add Text...") is picked via
// buttons (keyboard key-picker, reused with macro_builder_active - see
// hid_scene_keynote_key_picker.c) rather than typed - see hid_keynote_play_macro() in
// views/hid_keynote.c for the underlying ";"-separated step syntax this assembles.

enum HidSceneKeynoteMacroEditIndex {
    HidSceneKeynoteMacroEditIndexPickKey,
    HidSceneKeynoteMacroEditIndexAddText,
    HidSceneKeynoteMacroEditIndexAddDelay,
    HidSceneKeynoteMacroEditIndexUndo,
    HidSceneKeynoteMacroEditIndexSave,
};

// Offset well above HidSceneKeynoteMacroEditIndex so both enums can share one switch below.
enum HidSceneKeynoteMacroEditEvent {
    HidSceneKeynoteMacroEditEventTextDone = 100,
    HidSceneKeynoteMacroEditEventDelayDone,
};

static void hid_scene_keynote_macro_edit_menu_callback(void* context, uint32_t index) {
    Hid* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

static void hid_scene_keynote_macro_edit_refresh(Hid* app) {
    uint32_t button_index =
        scene_manager_get_scene_state(app->scene_manager, HidSceneKeynoteMacroEdit);
    Submenu* menu = app->submenu;

    submenu_reset(menu);

    const char* macro = (button_index < HidKeynoteButtonCount) ?
                             app->keynote_macro[button_index] :
                             "";
    char header[32];
    if(macro[0] == '\0') {
        snprintf(header, sizeof(header), "Macro: (leer)");
    } else {
        snprintf(header, sizeof(header), "Macro: %s", macro);
    }
    submenu_set_header(menu, header);

    submenu_add_item(
        menu,
        "Taste waehlen...",
        HidSceneKeynoteMacroEditIndexPickKey,
        hid_scene_keynote_macro_edit_menu_callback,
        app);
    submenu_add_item(
        menu,
        "Text hinzufuegen...",
        HidSceneKeynoteMacroEditIndexAddText,
        hid_scene_keynote_macro_edit_menu_callback,
        app);
    submenu_add_item(
        menu,
        "Pause hinzufuegen...",
        HidSceneKeynoteMacroEditIndexAddDelay,
        hid_scene_keynote_macro_edit_menu_callback,
        app);
    submenu_add_item(
        menu,
        "Letzten Schritt entfernen",
        HidSceneKeynoteMacroEditIndexUndo,
        hid_scene_keynote_macro_edit_menu_callback,
        app);
    submenu_add_item(
        menu,
        "Speichern & Zurueck",
        HidSceneKeynoteMacroEditIndexSave,
        hid_scene_keynote_macro_edit_menu_callback,
        app);
}

static void hid_scene_keynote_macro_edit_text_done_callback(void* context) {
    Hid* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, HidSceneKeynoteMacroEditEventTextDone);
}

static void hid_scene_keynote_macro_edit_delay_done_callback(void* context) {
    Hid* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, HidSceneKeynoteMacroEditEventDelayDone);
}

static void hid_scene_keynote_macro_edit_append_step(Hid* app, const char* step) {
    uint32_t button_index =
        scene_manager_get_scene_state(app->scene_manager, HidSceneKeynoteMacroEdit);
    if(button_index >= HidKeynoteButtonCount) return;

    char* macro = app->keynote_macro[button_index];
    size_t max_len = sizeof(app->keynote_macro[button_index]);
    if(macro[0] != '\0') {
        strlcat(macro, ";", max_len);
    }
    strlcat(macro, step, max_len);
}

void hid_scene_keynote_macro_edit_on_enter(void* context) {
    Hid* app = context;

    hid_scene_keynote_macro_edit_refresh(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
}

bool hid_scene_keynote_macro_edit_on_event(void* context, SceneManagerEvent event) {
    Hid* app = context;
    uint32_t button_index =
        scene_manager_get_scene_state(app->scene_manager, HidSceneKeynoteMacroEdit);
    bool consumed = false;

    if(event.type != SceneManagerEventTypeCustom) return consumed;
    consumed = true;

    switch(event.event) {
    case HidSceneKeynoteMacroEditIndexPickKey:
        scene_manager_set_scene_state(app->scene_manager, HidSceneKeynoteKeyPicker, button_index);
        scene_manager_next_scene(app->scene_manager, HidSceneKeynoteKeyPicker);
        break;

    case HidSceneKeynoteMacroEditIndexAddText:
        app->macro_builder_input[0] = '\0';
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Text zum Tippen");
        text_input_set_result_callback(
            app->text_input,
            hid_scene_keynote_macro_edit_text_done_callback,
            app,
            app->macro_builder_input,
            sizeof(app->macro_builder_input),
            true);
        view_dispatcher_switch_to_view(app->view_dispatcher, HidViewTextInput);
        break;

    case HidSceneKeynoteMacroEditIndexAddDelay:
        app->macro_builder_input[0] = '\0';
        text_input_reset(app->text_input);
        text_input_set_header_text(app->text_input, "Pause in ms (z.B. 100)");
        text_input_set_result_callback(
            app->text_input,
            hid_scene_keynote_macro_edit_delay_done_callback,
            app,
            app->macro_builder_input,
            sizeof(app->macro_builder_input),
            true);
        view_dispatcher_switch_to_view(app->view_dispatcher, HidViewTextInput);
        break;

    case HidSceneKeynoteMacroEditIndexUndo:
        if(button_index < HidKeynoteButtonCount) {
            char* macro = app->keynote_macro[button_index];
            char* last_sep = strrchr(macro, ';');
            if(last_sep != NULL) {
                *last_sep = '\0';
            } else {
                macro[0] = '\0';
            }
        }
        hid_scene_keynote_macro_edit_refresh(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
        break;

    case HidSceneKeynoteMacroEditIndexSave:
        if(button_index < HidKeynoteButtonCount &&
           app->keynote_macro[button_index][0] != '\0' &&
           hid_keynote_macro_is_valid(app->keynote_macro[button_index])) {
            bt_hid_save_cfg(app);
        }
        scene_manager_search_and_switch_to_previous_scene(
            app->scene_manager, HidSceneKeynoteConfig);
        break;

    case HidSceneKeynoteMacroEditEventTextDone:
        if(app->macro_builder_input[0] != '\0') {
            char step[80];
            snprintf(step, sizeof(step), "STRING %s", app->macro_builder_input);
            hid_scene_keynote_macro_edit_append_step(app, step);
        }
        hid_scene_keynote_macro_edit_refresh(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
        break;

    case HidSceneKeynoteMacroEditEventDelayDone: {
        int ms = atoi(app->macro_builder_input);
        if(ms > 0) {
            char step[24];
            snprintf(step, sizeof(step), "DELAY %d", ms);
            hid_scene_keynote_macro_edit_append_step(app, step);
        }
        hid_scene_keynote_macro_edit_refresh(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
        break;
    }

    default:
        break;
    }

    return consumed;
}

void hid_scene_keynote_macro_edit_on_exit(void* context) {
    Hid* app = context;
    submenu_reset(app->submenu);
    text_input_reset(app->text_input);
}
