#include "../hid.h"
#include "../views.h"
#include "hid_icons.h"

enum HidSceneBleMacEvent {
    HidSceneBleMacEventByteInput,
    HidSceneBleMacEventPopup,
};

static void hid_scene_ble_mac_reverse_addr(uint8_t mac_addr[], size_t size) {
    uint8_t tmp;

    for(size_t i = 0; i < size / 2; i++) {
        tmp = mac_addr[i];
        mac_addr[i] = mac_addr[size - 1 - i];
        mac_addr[size - 1 - i] = tmp;
    }
}

static void hid_scene_ble_mac_byte_input_callback(void* context) {
    Hid* app = context;

    view_dispatcher_send_custom_event(app->view_dispatcher, HidSceneBleMacEventByteInput);
}

static void hid_scene_ble_mac_popup_callback(void* context) {
    Hid* app = context;

    view_dispatcher_send_custom_event(app->view_dispatcher, HidSceneBleMacEventPopup);
}

void hid_scene_ble_mac_on_enter(void* context) {
    Hid* app = context;

    memcpy(app->ble_mac_buf, app->ble_hid_cfg.mac_address, sizeof(app->ble_mac_buf));
    hid_scene_ble_mac_reverse_addr(app->ble_mac_buf, sizeof(app->ble_mac_buf));

    byte_input_set_header_text(app->byte_input, "Set BLE MAC address");
    byte_input_set_result_callback(
        app->byte_input,
        hid_scene_ble_mac_byte_input_callback,
        NULL,
        app,
        app->ble_mac_buf,
        sizeof(app->ble_mac_buf));

    popup_set_icon(app->popup, 48, 6, &I_DolphinDone_80x58);
    popup_set_header(app->popup, "Done", 14, 15, AlignLeft, AlignTop);
    popup_set_timeout(app->popup, 1500);
    popup_set_context(app->popup, app);
    popup_set_callback(app->popup, hid_scene_ble_mac_popup_callback);
    popup_enable_timeout(app->popup);

    view_dispatcher_switch_to_view(app->view_dispatcher, HidViewByteInput);
}

bool hid_scene_ble_mac_on_event(void* context, SceneManagerEvent event) {
    Hid* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        consumed = true;

        if(event.event == HidSceneBleMacEventByteInput) {
            hid_scene_ble_mac_reverse_addr(app->ble_mac_buf, sizeof(app->ble_mac_buf));
            memcpy(app->ble_hid_cfg.mac_address, app->ble_mac_buf, sizeof(app->ble_mac_buf));
            app->ble_hid_cfg.mac_address_set = true;

#ifdef HID_TRANSPORT_BLE
            furi_check(bt_hid_apply_cfg(app));
#endif

            bt_hid_save_cfg(app);
            view_dispatcher_switch_to_view(app->view_dispatcher, HidViewPopup);
        } else if(event.event == HidSceneBleMacEventPopup) {
            scene_manager_previous_scene(app->scene_manager);
        }
    }

    return consumed;
}

void hid_scene_ble_mac_on_exit(void* context) {
    Hid* app = context;

    byte_input_set_result_callback(app->byte_input, NULL, NULL, NULL, NULL, 0);
    byte_input_set_header_text(app->byte_input, NULL);
    popup_reset(app->popup);
}
