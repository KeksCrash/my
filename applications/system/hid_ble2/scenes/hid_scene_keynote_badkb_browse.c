#include "../hid.h"

#define BADKB_BROWSE_DIR EXT_PATH("badkb")
#define BADKB_BROWSE_EXT ".txt"

static bool badkb_file_path_set;
static FuriString* badkb_file_path;

void hid_scene_keynote_badkb_browse_on_enter(void* context) {
    Hid* app = context;

    if(!badkb_file_path) {
        badkb_file_path = furi_string_alloc_set_str(BADKB_BROWSE_DIR);
    }
    badkb_file_path_set = false;

    DialogsFileBrowserOptions opts;
    dialog_file_browser_set_basic_options(&opts, BADKB_BROWSE_EXT, NULL);
    opts.base_path = BADKB_BROWSE_DIR;
    opts.hide_ext = false;

    if(dialog_file_browser_show(app->dialogs, badkb_file_path, badkb_file_path, &opts)) {
        badkb_file_path_set = true;
    }

    scene_manager_previous_scene(app->scene_manager);
}

bool hid_scene_keynote_badkb_browse_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void hid_scene_keynote_badkb_browse_on_exit(void* context) {
    Hid* app = context;

    if(badkb_file_path_set) {
        uint8_t button = scene_manager_get_scene_state(
            app->scene_manager, HidSceneKeynoteConfig);
        if(button < HidKeynoteButtonCount) {
            strlcpy(
                app->keynote_badkb_script[button],
                furi_string_get_cstr(badkb_file_path),
                sizeof(app->keynote_badkb_script[button]));
            app->keynote_actions[button] = HidKeynoteActionBadKbScript;
            bt_hid_save_cfg(app);
        }
    }
}
