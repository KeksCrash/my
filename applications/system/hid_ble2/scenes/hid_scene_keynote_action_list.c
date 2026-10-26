#include "../hid.h"
#include "../views.h"
#include "../views/hid_keynote_key_picker.h"

static void hid_scene_keynote_action_list_callback(void* context, uint32_t index) {
    Hid* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void hid_scene_keynote_action_list_on_enter(void* context) {
    Hid* app = context;
    Submenu* menu = app->keynote_action_submenu;
    uint32_t button_index = scene_manager_get_scene_state(
        app->scene_manager, HidSceneKeynoteActionList);

    submenu_reset(menu);
    submenu_set_header(menu, "Aktion waehlen");

    // Add only actions that are NOT on the keyboard layout
    for(uint32_t i = 0; i < HidKeynoteActionCount; i++) {
        if(hid_keynote_key_picker_action_on_keyboard((HidKeynoteAction)i)) continue;
        const char* name = hid_keynote_action_get_name((HidKeynoteAction)i);
        if(name != NULL) {
            submenu_add_item(menu, name, i, hid_scene_keynote_action_list_callback, app);
        }
    }

    // Pre-select current action
    if(button_index < HidKeynoteButtonCount) {
        HidKeynoteAction current = hid_keynote_get_action(app->hid_keynote, button_index);
        submenu_set_selected_item(menu, (uint32_t)current);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, HidViewKeynoteActionSubmenu);
}

bool hid_scene_keynote_action_list_on_event(void* context, SceneManagerEvent event) {
    Hid* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        uint32_t button_index = scene_manager_get_scene_state(
            app->scene_manager, HidSceneKeynoteActionList);
        HidKeynoteAction action = (HidKeynoteAction)event.event;

        if(button_index < HidKeynoteButtonCount && action < HidKeynoteActionCount) {
            hid_keynote_set_action(app->hid_keynote, button_index, action);

            if(action == HidKeynoteActionCustomMacro) {
                // Needs a macro string, not just the action id - detour through the
                // builder scene instead of saving+returning immediately like every
                // other action here. macro_builder_active makes the keyboard key-picker
                // append steps instead of assigning the whole button (see
                // hid_scene_keynote_key_picker.c); cleared again in
                // hid_scene_keynote_config_on_enter() once the builder flow is done.
                app->macro_builder_active = true;
                scene_manager_set_scene_state(
                    app->scene_manager, HidSceneKeynoteMacroEdit, button_index);
                scene_manager_next_scene(app->scene_manager, HidSceneKeynoteMacroEdit);
                consumed = true;
                return consumed;
            }

            bt_hid_save_cfg(app);
        }
        scene_manager_search_and_switch_to_previous_scene(
            app->scene_manager, HidSceneKeynoteConfig);
        consumed = true;
    }

    return consumed;
}

void hid_scene_keynote_action_list_on_exit(void* context) {
    Hid* app = context;
    submenu_reset(app->keynote_action_submenu);
}
