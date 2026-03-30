#include "../hid.h"
#include "../views.h"

enum HidKeynoteConfigIndex {
    HidKeynoteConfigIndexUp,
    HidKeynoteConfigIndexDown,
    HidKeynoteConfigIndexLeft,
    HidKeynoteConfigIndexRight,
    HidKeynoteConfigIndexOk,
    HidKeynoteConfigIndexBack,
    HidKeynoteConfigIndexResetDefaults,
};

static const char* const hid_keynote_config_button_labels[HidKeynoteButtonCount] = {
    "Up",
    "Down",
    "Left",
    "Right",
    "OK",
    "Back",
};

static bool hid_scene_keynote_config_index_to_button(
    uint32_t index,
    HidKeynoteButton* button,
    const char** label) {
    if(index >= HidKeynoteButtonCount) {
        return false;
    }

    *button = index;
    *label = hid_keynote_config_button_labels[index];
    return true;
}

static void hid_scene_keynote_config_update_items(Hid* app) {
    VariableItem* item;
    VariableItemList* list = app->variable_item_list;

    variable_item_list_reset(list);
    variable_item_list_set_header(list, "Keynote Config");

    for(uint32_t i = 0; i < HidKeynoteButtonCount; i++) {
        HidKeynoteAction action = hid_keynote_get_action(app->hid_keynote, i);
        item = variable_item_list_add(list, hid_keynote_config_button_labels[i], 1, NULL, app);
        variable_item_set_current_value_index(item, 0);
        variable_item_set_current_value_text(item, hid_keynote_action_get_name(action));
    }

    item = variable_item_list_add(list, "Restore Defaults", 1, NULL, app);
    variable_item_set_current_value_index(item, 0);
    variable_item_set_current_value_text(item, "");
}

static void hid_scene_keynote_config_action_selected(void* context, uint32_t index) {
    Hid* app = context;
    HidKeynoteButton button;
    const char* label;
    uint32_t selected_item = scene_manager_get_scene_state(app->scene_manager, HidSceneKeynoteConfig);

    if(!hid_scene_keynote_config_index_to_button(selected_item, &button, &label)) {
        return;
    }

    if(index >= HidKeynoteActionCount) {
        return;
    }

    hid_keynote_set_action(app->hid_keynote, button, index);
    bt_hid_save_cfg(app);
    hid_scene_keynote_config_update_items(app);
    variable_item_list_set_selected_item(app->variable_item_list, selected_item);
    view_dispatcher_switch_to_view(app->view_dispatcher, HidViewVariableItemList);
}

static void hid_scene_keynote_config_show_action_picker(Hid* app, uint32_t index) {
    HidKeynoteButton button;
    const char* label;

    if(!hid_scene_keynote_config_index_to_button(index, &button, &label)) {
        return;
    }

    submenu_reset(app->keynote_action_submenu);
    submenu_set_header(app->keynote_action_submenu, label);
    for(uint32_t action = 0; action < HidKeynoteActionCount; action++) {
        submenu_add_item(
            app->keynote_action_submenu,
            hid_keynote_action_get_name(action),
            action,
            hid_scene_keynote_config_action_selected,
            app);
    }
    submenu_set_selected_item(
        app->keynote_action_submenu,
        hid_keynote_get_action(app->hid_keynote, button));
    view_dispatcher_switch_to_view(app->view_dispatcher, HidViewKeynoteActionSubmenu);
}

static void hid_scene_keynote_config_select_callback(void* context, uint32_t index) {
    Hid* app = context;

    scene_manager_set_scene_state(app->scene_manager, HidSceneKeynoteConfig, index);

    if(index == HidKeynoteConfigIndexResetDefaults) {
        hid_keynote_reset_actions(app->hid_keynote);
        bt_hid_save_cfg(app);
        hid_scene_keynote_config_update_items(app);
        variable_item_list_set_selected_item(app->variable_item_list, index);
        return;
    }

    hid_scene_keynote_config_show_action_picker(app, index);
}

void hid_scene_keynote_config_on_enter(void* context) {
    Hid* app = context;

    variable_item_list_set_enter_callback(
        app->variable_item_list,
        hid_scene_keynote_config_select_callback,
        app);
    hid_scene_keynote_config_update_items(app);
    variable_item_list_set_selected_item(
        app->variable_item_list,
        scene_manager_get_scene_state(app->scene_manager, HidSceneKeynoteConfig));
    view_dispatcher_switch_to_view(app->view_dispatcher, HidViewVariableItemList);
}

bool hid_scene_keynote_config_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void hid_scene_keynote_config_on_exit(void* context) {
    Hid* app = context;
    variable_item_list_reset(app->variable_item_list);
    submenu_reset(app->keynote_action_submenu);
}