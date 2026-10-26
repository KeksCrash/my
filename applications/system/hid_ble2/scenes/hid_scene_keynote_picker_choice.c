#include "../hid.h"
#include "../views.h"

enum {
    HidKeynotePickerChoiceList,
    HidKeynotePickerChoiceKeyboard,
    HidKeynotePickerChoiceBadKb,
};

static void hid_scene_keynote_picker_choice_callback(void* context, uint32_t index) {
    Hid* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void hid_scene_keynote_picker_choice_on_enter(void* context) {
    Hid* app = context;

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Auswahl-Modus");
    submenu_add_item(
        app->submenu, "Liste", HidKeynotePickerChoiceList,
        hid_scene_keynote_picker_choice_callback, app);
    submenu_add_item(
        app->submenu, "Tastatur", HidKeynotePickerChoiceKeyboard,
        hid_scene_keynote_picker_choice_callback, app);
#ifdef HID_TRANSPORT_BLE
    submenu_add_item(
        app->submenu, "BadKB Skript", HidKeynotePickerChoiceBadKb,
        hid_scene_keynote_picker_choice_callback, app);
#endif

    view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
}

bool hid_scene_keynote_picker_choice_on_event(void* context, SceneManagerEvent event) {
    Hid* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        uint32_t button_index = scene_manager_get_scene_state(
            app->scene_manager, HidSceneKeynotePickerChoice);

        if(event.event == HidKeynotePickerChoiceList) {
            scene_manager_set_scene_state(
                app->scene_manager, HidSceneKeynoteActionList, button_index);
            scene_manager_next_scene(app->scene_manager, HidSceneKeynoteActionList);
            consumed = true;
        } else if(event.event == HidKeynotePickerChoiceKeyboard) {
            scene_manager_set_scene_state(
                app->scene_manager, HidSceneKeynoteKeyPicker, button_index);
            scene_manager_next_scene(app->scene_manager, HidSceneKeynoteKeyPicker);
            consumed = true;
        }
#ifdef HID_TRANSPORT_BLE
        else if(event.event == HidKeynotePickerChoiceBadKb) {
            scene_manager_set_scene_state(
                app->scene_manager, HidSceneKeynoteConfig, button_index);
            scene_manager_next_scene(app->scene_manager, HidSceneKeynoteBadkbBrowse);
            consumed = true;
        }
#endif
    }

    return consumed;
}

void hid_scene_keynote_picker_choice_on_exit(void* context) {
    Hid* app = context;
    submenu_reset(app->submenu);
}
