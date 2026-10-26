#include "../hid.h"
#include "../views.h"

typedef enum {
    HidStartIndexMedia,
    HidStartIndexCatKeynotes,
    HidStartIndexCatKeyboard,
    HidStartIndexCatMouse,
    HidStartIndexCatMedias,
    HidStartIndexCatBluetooth,
} HidStartIndex;

static void hid_scene_start_submenu_callback(void* context, uint32_t index) {
    furi_assert(context);
    Hid* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void hid_scene_start_on_enter(void* context) {
    Hid* app = context;
    submenu_add_item(
        app->submenu, "Media", HidStartIndexMedia, hid_scene_start_submenu_callback, app);
    submenu_add_item(
        app->submenu, "Keynotes", HidStartIndexCatKeynotes, hid_scene_start_submenu_callback, app);
    submenu_add_item(
        app->submenu, "Keyboard", HidStartIndexCatKeyboard, hid_scene_start_submenu_callback, app);
    submenu_add_item(
        app->submenu, "Mouse", HidStartIndexCatMouse, hid_scene_start_submenu_callback, app);
    submenu_add_item(
        app->submenu, "Medias", HidStartIndexCatMedias, hid_scene_start_submenu_callback, app);
#ifdef HID_TRANSPORT_BLE
    submenu_add_item(
        app->submenu,
        "Bluetooth",
        HidStartIndexCatBluetooth,
        hid_scene_start_submenu_callback,
        app);
#endif
    submenu_set_selected_item(
        app->submenu,
        scene_manager_get_scene_state(app->scene_manager, HidSceneStart));
    view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
}

bool hid_scene_start_on_event(void* context, SceneManagerEvent event) {
    Hid* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_set_scene_state(app->scene_manager, HidSceneStart, event.event);
        if(event.event == HidStartIndexMedia) {
            scene_manager_set_scene_state(app->scene_manager, HidSceneMain, HidViewMedia);
            scene_manager_next_scene(app->scene_manager, HidSceneMain);
        } else {
            // encode category in high 16 bits; selected item (0) in low 16 bits
            scene_manager_set_scene_state(
                app->scene_manager, HidSceneCategory, (uint32_t)event.event << 16);
            scene_manager_next_scene(app->scene_manager, HidSceneCategory);
        }
        consumed = true;
    }

    return consumed;
}

void hid_scene_start_on_exit(void* context) {
    Hid* app = context;
    submenu_reset(app->submenu);
}
