#include "../hid.h"

#define HID_LAYOUT_BASE_PATH EXT_PATH("badusb/assets/layouts")
#define HID_LAYOUT_EXT       ".kl"

void hid_scene_keyboard_layout_on_enter(void* context) {
    Hid* app = context;

    FuriString* path = furi_string_alloc();
    if(app->keyboard_layout_path[0] != '\0') {
        furi_string_set_str(path, app->keyboard_layout_path);
    } else {
        furi_string_set_str(path, HID_LAYOUT_BASE_PATH "/en-US.kl");
    }

    DialogsFileBrowserOptions options;
    dialog_file_browser_set_basic_options(&options, HID_LAYOUT_EXT, NULL);
    options.hide_ext = false;
    options.base_path = HID_LAYOUT_BASE_PATH;

    if(dialog_file_browser_show(app->dialogs, path, path, &options)) {
        strlcpy(
            app->keyboard_layout_path,
            furi_string_get_cstr(path),
            sizeof(app->keyboard_layout_path));
        hid_keyboard_set_layout_path(app->hid_keyboard, app->keyboard_layout_path);
        bt_hid_save_cfg(app);
    }

    furi_string_free(path);
    scene_manager_previous_scene(app->scene_manager);
}

bool hid_scene_keyboard_layout_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void hid_scene_keyboard_layout_on_exit(void* context) {
    UNUSED(context);
}
