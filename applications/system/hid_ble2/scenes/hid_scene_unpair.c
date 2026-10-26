#include "../hid.h"
#include "../views.h"
#include "hid_icons.h"

#ifdef HID_TRANSPORT_BLE

#define HID_UNPAIR_IDX_UNPAIR_ALL        0xFF
#define HID_UNPAIR_EVENT_RENAME_DONE  0xFE
#define HID_UNPAIR_EVENT_USE_MAC      0xFD
#define HID_UNPAIR_EVENT_RENAME_START 0xFC
#define HID_UNPAIR_EVENT_UNPAIR_CONF  0xFB

typedef enum {
    HidUnpairStateDeviceList,
    HidUnpairStateDeviceOptions,
    HidUnpairStateDialog,
    HidUnpairStateRename,
    HidUnpairStateSwitching,
} HidUnpairState;

static void hid_scene_unpair_format_mac(const uint8_t addr[6], char out[18]) {
    snprintf(
        out,
        18,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        addr[5],
        addr[4],
        addr[3],
        addr[2],
        addr[1],
        addr[0]);
}

static void hid_scene_unpair_submenu_callback(void* context, uint32_t index) {
    Hid* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

static void hid_scene_unpair_rebuild_device_list(Hid* app) {
    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Bluetooth Devices");
    for(uint8_t i = 0; i < app->bonded_dev_count; i++) {
        submenu_add_item(
            app->submenu,
            app->bonded_dev_labels[i],
            i,
            hid_scene_unpair_submenu_callback,
            app);
    }
    submenu_add_item(
        app->submenu,
        "Unpair All",
        HID_UNPAIR_IDX_UNPAIR_ALL,
        hid_scene_unpair_submenu_callback,
        app);
}

static void hid_scene_unpair_rebuild_device_options(Hid* app) {
    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, app->bonded_dev_labels[app->selected_bonded_dev_idx]);
    submenu_add_item(
        app->submenu,
        "Use this MAC",
        HID_UNPAIR_EVENT_USE_MAC,
        hid_scene_unpair_submenu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "Rename",
        HID_UNPAIR_EVENT_RENAME_START,
        hid_scene_unpair_submenu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "Unpair",
        HID_UNPAIR_EVENT_UNPAIR_CONF,
        hid_scene_unpair_submenu_callback,
        app);
}

static void hid_scene_unpair_popup_callback(void* context) {
    Hid* app = context;
    uint32_t state = scene_manager_get_scene_state(app->scene_manager, HidSceneUnpair);

    if(state == HidUnpairStateSwitching) {
        // Labels already correct from pre-snapshot bt_hid_load_bonded_devices in USE_MAC handler.
        // Do NOT call bt_hid_load_bonded_devices here — BT profile just restarted via
        // bt_hid_apply_cfg and calling furi_hal_bt_get_bonded_devices or storage I/O
        // from the popup callback context causes an MPU fault / stack overflow.
        hid_scene_unpair_rebuild_device_list(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
        scene_manager_set_scene_state(
            app->scene_manager, HidSceneUnpair, HidUnpairStateDeviceList);
    } else if(
        app->selected_bonded_dev_idx != HID_UNPAIR_IDX_UNPAIR_ALL &&
        app->bonded_dev_count > 0) {
        hid_scene_unpair_rebuild_device_list(app);
        view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
        scene_manager_set_scene_state(
            app->scene_manager, HidSceneUnpair, HidUnpairStateDeviceList);
    } else {
        scene_manager_previous_scene(app->scene_manager);
    }
}

static void hid_scene_unpair_no_devs_callback(DialogExResult result, void* context) {
    Hid* app = context;
    UNUSED(result);
    scene_manager_previous_scene(app->scene_manager);
}

static void hid_scene_unpair_rename_callback(void* context) {
    Hid* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, HID_UNPAIR_EVENT_RENAME_DONE);
}

static void hid_scene_unpair_dialog_callback(DialogExResult result, void* context) {
    Hid* app = context;

    if(result == DialogExResultRight) {
        if(app->selected_bonded_dev_idx == HID_UNPAIR_IDX_UNPAIR_ALL) {
            bt_hid_remove_pairing(app);
            app->bonded_dev_count = 0;
        } else {
            bt_hid_remove_bonded_device_by_idx(app, app->selected_bonded_dev_idx);
            bt_hid_load_bonded_devices(app);
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, HidViewPopup);
    } else {
        if(app->selected_bonded_dev_idx == HID_UNPAIR_IDX_UNPAIR_ALL) {
            hid_scene_unpair_rebuild_device_list(app);
            view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
            scene_manager_set_scene_state(
                app->scene_manager, HidSceneUnpair, HidUnpairStateDeviceList);
        } else {
            hid_scene_unpair_rebuild_device_options(app);
            view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
            scene_manager_set_scene_state(
                app->scene_manager, HidSceneUnpair, HidUnpairStateDeviceOptions);
        }
    }
}

void hid_scene_unpair_on_enter(void* context) {
    Hid* app = context;

    bt_hid_load_bonded_devices(app);

    popup_set_icon(app->popup, 48, 6, &I_DolphinDone_80x58);
    popup_set_header(app->popup, "Done", 14, 15, AlignLeft, AlignTop);
    popup_set_timeout(app->popup, 1500);
    popup_set_context(app->popup, app);
    popup_set_callback(app->popup, hid_scene_unpair_popup_callback);
    popup_enable_timeout(app->popup);

    scene_manager_set_scene_state(
        app->scene_manager, HidSceneUnpair, HidUnpairStateDeviceList);

    if(app->bonded_dev_count == 0) {
        dialog_ex_reset(app->dialog);
        dialog_ex_set_header(app->dialog, "No Paired Devices", 64, 3, AlignCenter, AlignTop);
        dialog_ex_set_text(
            app->dialog, "No bonded BT devices\nfound.", 64, 22, AlignCenter, AlignTop);
        dialog_ex_set_left_button_text(app->dialog, "Back");
        dialog_ex_set_result_callback(app->dialog, hid_scene_unpair_no_devs_callback);
        dialog_ex_set_context(app->dialog, app);
        view_dispatcher_switch_to_view(app->view_dispatcher, HidViewDialog);
        return;
    }

    hid_scene_unpair_rebuild_device_list(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
}

bool hid_scene_unpair_on_event(void* context, SceneManagerEvent event) {
    Hid* app = context;
    bool consumed = false;

    uint32_t state = scene_manager_get_scene_state(app->scene_manager, HidSceneUnpair);

    if(event.type == SceneManagerEventTypeBack) {
        if(state == HidUnpairStateRename) {
            text_input_reset(app->text_input);
            hid_scene_unpair_rebuild_device_options(app);
            view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
            scene_manager_set_scene_state(
                app->scene_manager, HidSceneUnpair, HidUnpairStateDeviceOptions);
            consumed = true;
        } else if(state == HidUnpairStateDeviceOptions) {
            hid_scene_unpair_rebuild_device_list(app);
            view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
            scene_manager_set_scene_state(
                app->scene_manager, HidSceneUnpair, HidUnpairStateDeviceList);
            consumed = true;
        }
        // HidUnpairStateDeviceList → scene_manager handles (previous_scene)
    } else if(event.type == SceneManagerEventTypeCustom) {
        uint32_t idx = event.event;
        consumed = true;

        if(idx == HID_UNPAIR_EVENT_RENAME_DONE) {
            uint8_t dev_idx = app->selected_bonded_dev_idx;
            if(dev_idx < app->bonded_dev_count) {
                strlcpy(
                    app->bonded_dev_labels[dev_idx],
                    app->bonded_dev_rename_buf,
                    sizeof(app->bonded_dev_labels[dev_idx]));
                bt_hid_save_bonded_labels(app);
            }
            text_input_reset(app->text_input);
            hid_scene_unpair_rebuild_device_options(app);
            view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
            scene_manager_set_scene_state(
                app->scene_manager, HidSceneUnpair, HidUnpairStateDeviceOptions);

        } else if(state == HidUnpairStateDeviceList) {
            if(idx == HID_UNPAIR_IDX_UNPAIR_ALL) {
                dialog_ex_reset(app->dialog);
                dialog_ex_set_result_callback(app->dialog, hid_scene_unpair_dialog_callback);
                dialog_ex_set_context(app->dialog, app);
                dialog_ex_set_header(
                    app->dialog, "Unpair All?", 64, 3, AlignCenter, AlignTop);
                dialog_ex_set_text(
                    app->dialog,
                    "All pairings will\nbe lost!",
                    64,
                    22,
                    AlignCenter,
                    AlignTop);
                dialog_ex_set_left_button_text(app->dialog, "Back");
                dialog_ex_set_right_button_text(app->dialog, "Unpair");
                app->selected_bonded_dev_idx = HID_UNPAIR_IDX_UNPAIR_ALL;
                view_dispatcher_switch_to_view(app->view_dispatcher, HidViewDialog);
                scene_manager_set_scene_state(
                    app->scene_manager, HidSceneUnpair, HidUnpairStateDialog);
            } else if(idx < app->bonded_dev_count) {
                app->selected_bonded_dev_idx = (uint8_t)idx;
                hid_scene_unpair_rebuild_device_options(app);
                view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
                scene_manager_set_scene_state(
                    app->scene_manager, HidSceneUnpair, HidUnpairStateDeviceOptions);
            }

        } else if(state == HidUnpairStateDeviceOptions) {
            uint8_t dev_idx = app->selected_bonded_dev_idx;

            if(idx == HID_UNPAIR_EVENT_USE_MAC) {
                bt_hid_load_bonded_devices(app);
                memcpy(
                    app->ble_hid_cfg.mac_address,
                    app->bonded_dev_flipper_mac[dev_idx],
                    6);
                app->ble_hid_cfg.mac_address_set = true;
                strlcpy(
                    app->ble_hid_cfg.name,
                    app->bonded_dev_flipper_name[dev_idx],
                    sizeof(app->ble_hid_cfg.name));
                bt_hid_save_cfg(app);
                bt_hid_apply_cfg(app);
                popup_set_header(app->popup, "MAC Switched", 14, 15, AlignLeft, AlignTop);
                view_dispatcher_switch_to_view(app->view_dispatcher, HidViewPopup);
                scene_manager_set_scene_state(
                    app->scene_manager, HidSceneUnpair, HidUnpairStateSwitching);

            } else if(idx == HID_UNPAIR_EVENT_RENAME_START) {
                strlcpy(
                    app->bonded_dev_rename_buf,
                    app->bonded_dev_labels[dev_idx],
                    sizeof(app->bonded_dev_rename_buf));
                text_input_reset(app->text_input);
                text_input_set_header_text(app->text_input, "Rename Device");
                text_input_set_result_callback(
                    app->text_input,
                    hid_scene_unpair_rename_callback,
                    app,
                    app->bonded_dev_rename_buf,
                    sizeof(app->bonded_dev_rename_buf),
                    false);
                view_dispatcher_switch_to_view(app->view_dispatcher, HidViewTextInput);
                scene_manager_set_scene_state(
                    app->scene_manager, HidSceneUnpair, HidUnpairStateRename);

            } else if(idx == HID_UNPAIR_EVENT_UNPAIR_CONF) {
                char dev_mac_str[18];
                char flipper_mac_str[18];
                char details[96];
                hid_scene_unpair_format_mac(app->bonded_devs[dev_idx].addr, dev_mac_str);
                hid_scene_unpair_format_mac(
                    app->bonded_dev_flipper_mac[dev_idx], flipper_mac_str);
                snprintf(
                    details,
                    sizeof(details),
                    "Dev: %s\nFlipper: %s",
                    dev_mac_str,
                    flipper_mac_str);
                dialog_ex_reset(app->dialog);
                dialog_ex_set_result_callback(app->dialog, hid_scene_unpair_dialog_callback);
                dialog_ex_set_context(app->dialog, app);
                dialog_ex_set_header(
                    app->dialog, "Unpair Device?", 64, 3, AlignCenter, AlignTop);
                dialog_ex_set_text(app->dialog, details, 64, 20, AlignCenter, AlignTop);
                dialog_ex_set_left_button_text(app->dialog, "Back");
                dialog_ex_set_right_button_text(app->dialog, "Unpair");
                view_dispatcher_switch_to_view(app->view_dispatcher, HidViewDialog);
                scene_manager_set_scene_state(
                    app->scene_manager, HidSceneUnpair, HidUnpairStateDialog);
            }
        }
    }

    return consumed;
}

void hid_scene_unpair_on_exit(void* context) {
    Hid* app = context;
    submenu_reset(app->submenu);
    dialog_ex_reset(app->dialog);
    popup_reset(app->popup);
    text_input_reset(app->text_input);
}

#endif
