#include "../hid.h"
#include "../views.h"

// Submenu indices match GapPairing enum values directly
// GapPairingNone = 0             -> "Just Works" (no PIN, iOS only)
// GapPairingPinCodeShow = 1      -> "PIN Display" (Flipper shows PIN, phone asks to type it)
// GapPairingPinCodeVerifyYesNo = 2 -> "Confirm Code" (both show code, both confirm)

static void hid_scene_pairing_submenu_callback(void* context, uint32_t index) {
    Hid* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void hid_scene_pairing_on_enter(void* context) {
    Hid* app = context;

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Pairing Mode");
    submenu_add_item(
        app->submenu,
        "Confirm Code",
        GapPairingPinCodeVerifyYesNo,
        hid_scene_pairing_submenu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "PIN Display",
        GapPairingPinCodeShow,
        hid_scene_pairing_submenu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "Just Works (iOS)",
        GapPairingNone,
        hid_scene_pairing_submenu_callback,
        app);

    // Highlight current selection
    uint32_t selected = 0;
    if(app->ble_hid_cfg.pairing == GapPairingPinCodeShow) {
        selected = 1;
    } else if(app->ble_hid_cfg.pairing == GapPairingNone) {
        selected = 2;
    }
    submenu_set_selected_item(app->submenu, selected);
    view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
}

bool hid_scene_pairing_on_event(void* context, SceneManagerEvent event) {
    Hid* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        app->ble_hid_cfg.pairing = (GapPairing)event.event;
        bt_hid_save_cfg(app);
        bt_disconnect(app->bt);
        furi_delay_ms(200);
        bt_hid_apply_cfg(app);
        scene_manager_previous_scene(app->scene_manager);
        consumed = true;
    }

    return consumed;
}

void hid_scene_pairing_on_exit(void* context) {
    Hid* app = context;
    submenu_reset(app->submenu);
}
