#include "../hid.h"
#include "../views.h"

void hid_scene_main_on_enter(void* context) {
    Hid* app = context;
    uint32_t view_id = scene_manager_get_scene_state(app->scene_manager, HidSceneMain);
    // Sole hook point for the key-lock feature's view scope (see hid.h's
    // lock_feature_view_active comment) -- every switch to a HidView flows through here.
    app->lock_feature_view_active = (view_id == HidViewMedia || view_id == HidViewMovie ||
                                      view_id == HidViewMusicMacOs || view_id == BtHidViewTikTok);
    view_dispatcher_switch_to_view(app->view_dispatcher, view_id);
}

bool hid_scene_main_on_event(void* context, SceneManagerEvent event) {
    Hid* app = context;
    bool consumed = false;
    UNUSED(app);
    UNUSED(event);

    return consumed;
}

void hid_scene_main_on_exit(void* context) {
    Hid* app = context;
    UNUSED(app);
}
