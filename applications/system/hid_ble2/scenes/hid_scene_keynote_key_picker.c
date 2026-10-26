#include <string.h>

#include "../hid.h"
#include "../views.h"
#include "../views/hid_keynote_key_picker.h"

// Builds one "MOD+MOD+KEY" macro step from the currently accumulated modifier bits plus
// the just-picked non-modifier action, and appends it (";"-separated) to the button's
// in-progress macro script. Matches the syntax hid_keynote_play_macro() parses.
static void hid_scene_keynote_key_picker_commit_macro_step(
    Hid* app,
    uint32_t button_index,
    HidKeynoteAction action) {
    if(button_index >= HidKeynoteButtonCount) return;

    char mods_buf[24] = "";
    if(app->macro_builder_pending_mods & KEY_MOD_LEFT_CTRL) strlcat(mods_buf, "CTRL+", sizeof(mods_buf));
    if(app->macro_builder_pending_mods & KEY_MOD_LEFT_ALT) strlcat(mods_buf, "ALT+", sizeof(mods_buf));
    if(app->macro_builder_pending_mods & KEY_MOD_LEFT_SHIFT) strlcat(mods_buf, "SHIFT+", sizeof(mods_buf));
    if(app->macro_builder_pending_mods & KEY_MOD_LEFT_GUI) strlcat(mods_buf, "GUI+", sizeof(mods_buf));

    char step[40];
    snprintf(step, sizeof(step), "%s%s", mods_buf, hid_keynote_action_get_short_name(action));

    char* macro = app->keynote_macro[button_index];
    size_t max_len = sizeof(app->keynote_macro[button_index]);
    if(macro[0] != '\0') {
        strlcat(macro, ";", max_len);
    }
    strlcat(macro, step, max_len);

    app->macro_builder_pending_mods = 0;
}

static void hid_scene_keynote_key_picker_callback(HidKeynoteAction action, void* context) {
    Hid* app = context;
    uint32_t button_index =
        scene_manager_get_scene_state(app->scene_manager, HidSceneKeynoteKeyPicker);

    if(app->macro_builder_active) {
        uint16_t mod_bit;
        if(hid_keynote_action_is_modifier(action, &mod_bit)) {
            // Accumulate the modifier and stay in the key picker - the user picks the
            // actual key (or another modifier) next, no navigation happens here.
            app->macro_builder_pending_mods |= mod_bit;
            return;
        }

        hid_scene_keynote_key_picker_commit_macro_step(app, button_index, action);
        scene_manager_search_and_switch_to_previous_scene(
            app->scene_manager, HidSceneKeynoteMacroEdit);
        return;
    }

    if(button_index < HidKeynoteButtonCount) {
        hid_keynote_set_action(app->hid_keynote, button_index, action);
        bt_hid_save_cfg(app);
    }
    scene_manager_search_and_switch_to_previous_scene(
        app->scene_manager, HidSceneKeynoteConfig);
}

void hid_scene_keynote_key_picker_on_enter(void* context) {
    Hid* app = context;
    uint32_t button_index =
        scene_manager_get_scene_state(app->scene_manager, HidSceneKeynoteKeyPicker);

    hid_keynote_key_picker_set_callback(
        app->hid_keynote_key_picker, hid_scene_keynote_key_picker_callback, app);

    // Pre-select the currently assigned action - skip while builder-picking a step, since
    // "current action" for the button (e.g. CustomMacro itself) isn't a keyboard key.
    if(!app->macro_builder_active && button_index < HidKeynoteButtonCount) {
        HidKeynoteAction current = hid_keynote_get_action(app->hid_keynote, button_index);
        hid_keynote_key_picker_set_selected(app->hid_keynote_key_picker, current);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, HidViewKeynoteKeyPicker);
}

bool hid_scene_keynote_key_picker_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void hid_scene_keynote_key_picker_on_exit(void* context) {
    UNUSED(context);
}
