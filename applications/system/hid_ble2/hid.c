#include "hid.h"
#ifdef HID_TRANSPORT_BLE
#include "helpers/ducky_script.h"
#endif
#include <extra_profiles/hid_profile.h>
#include <profiles/serial_profile.h>
#include "views.h"
#include <notification/notification_messages.h>
#include <dolphin/dolphin.h>
#include <flipper_format/flipper_format.h>
#include <bt/bt_service/bt_i.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>
#include <input/input.h>

#define TAG "HidApp"

#define HID_BT_CFG_PATH      APP_DATA_PATH(".bt_hid2.cfg")
#define HID_BT_CFG_FILE_TYPE "Flipper BT Remote Settings File"
#define HID_BT_CFG_VERSION   7
#define HID_LAYOUT_BASE_PATH EXT_PATH("badusb/assets/layouts")
#define HID_LAYOUT_DEFAULT   HID_LAYOUT_BASE_PATH "/en-US.kl"

#define HID_BT_LABELS_PATH      APP_DATA_PATH(".bt_hid2.labels")
#define HID_BT_LABELS_FILE_TYPE "Flipper BT Remote Labels"
#define HID_BT_LABELS_VERSION   1

// Pending-import queue: written by BadKB2/BKB_Recorder (helpers/hid_ble2_link.c there)
// when they bond a new device via this app's shared identity/keystore. Consumed and
// deleted here on every load — those apps never write HID_BT_LABELS_PATH directly, so
// this stays the only path that adds "BadKB2 #N"-style entries for foreign pairings.
#define HID_BT_PENDING_PATH APP_DATA_PATH(".bt_hid2.pending")

static const char* const hid_keynote_cfg_keys[HidKeynoteButtonCount] = {
    "keynote_up",
    "keynote_down",
    "keynote_left",
    "keynote_right",
    "keynote_ok",
    "keynote_back",
};

// Custom-macro script per button (only meaningful when the matching keynote_cfg_keys
// entry above resolves to HidKeynoteActionCustomMacro) - added in cfg version 6.
static const char* const hid_keynote_macro_cfg_keys[HidKeynoteButtonCount] = {
    "keynote_macro_up",
    "keynote_macro_down",
    "keynote_macro_left",
    "keynote_macro_right",
    "keynote_macro_ok",
    "keynote_macro_back",
};

static const char* const hid_keynote_badkb_cfg_keys[HidKeynoteButtonCount] = {
    "keynote_badkb_up",
    "keynote_badkb_down",
    "keynote_badkb_left",
    "keynote_badkb_right",
    "keynote_badkb_ok",
    "keynote_badkb_back",
};

static void hid_keynote_set_default_actions(Hid* app) {
    for(size_t i = 0; i < HidKeynoteButtonCount; i++) {
        app->keynote_actions[i] = hid_keynote_get_default_action(i);
        app->keynote_macro[i][0] = '\0';
        app->keynote_badkb_script[i][0] = '\0';
    }
}

static void bt_hid_set_default_mac(Hid* app) {
#ifdef HID_BLE2_INSTANCE
    static const uint8_t default_mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    memcpy(app->ble_hid_cfg.mac_address, default_mac, sizeof(default_mac));
    app->ble_hid_cfg.mac_address_set = true;
#else
    UNUSED(app);
#endif
}

bool hid_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    Hid* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

bool hid_back_event_callback(void* context) {
    furi_assert(context);
    Hid* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

void bt_hid_remove_pairing(Hid* app) {
    Bt* bt = app->bt;
    bt_disconnect(bt);

    // Wait 2nd core to update nvm storage
    furi_delay_ms(200);

    furi_hal_bt_stop_advertising();

    bt_forget_bonded_devices(bt);

    // Delete labels file so re-paired devices get the current Flipper MAC,
    // not the stale pairing-time MAC from before the unpair.
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_simply_remove(storage, HID_BT_LABELS_PATH);
    furi_record_close(RECORD_STORAGE);

    furi_hal_bt_start_advertising();
}

static void bt_hid_dev_mac_to_label_key(const uint8_t addr[6], char key[20]) {
    snprintf(
        key,
        20,
        "label_%02X%02X%02X%02X%02X%02X",
        addr[0],
        addr[1],
        addr[2],
        addr[3],
        addr[4],
        addr[5]);
}

static void bt_hid_dev_mac_to_fmac_key(const uint8_t addr[6], char key[20]) {
    snprintf(
        key,
        20,
        "fmac_%02X%02X%02X%02X%02X%02X",
        addr[0],
        addr[1],
        addr[2],
        addr[3],
        addr[4],
        addr[5]);
}

static void bt_hid_dev_mac_to_fname_key(const uint8_t addr[6], char key[20]) {
    snprintf(
        key,
        20,
        "fname_%02X%02X%02X%02X%02X%02X",
        addr[0],
        addr[1],
        addr[2],
        addr[3],
        addr[4],
        addr[5]);
}

static void bt_hid_load_bonded_labels(Hid* app) {
    if(app->bonded_dev_count == 0) return;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);
    FuriString* temp_str = furi_string_alloc();
    uint32_t version = 0;

    do {
        if(!flipper_format_file_open_existing(fff, HID_BT_LABELS_PATH)) break;
        if(!flipper_format_read_header(fff, temp_str, &version)) break;
        if(strcmp(furi_string_get_cstr(temp_str), HID_BT_LABELS_FILE_TYPE) != 0) break;
        if(version != HID_BT_LABELS_VERSION) break;

        for(uint8_t i = 0; i < app->bonded_dev_count; i++) {
            char key[20];

            // Load stored Flipper MAC (pairing-time MAC) for this device
            bt_hid_dev_mac_to_fmac_key(app->bonded_devs[i].addr, key);
            flipper_format_rewind(fff);
            uint8_t stored_mac[6];
            if(flipper_format_read_hex(fff, key, stored_mac, 6)) {
                memcpy(app->bonded_dev_flipper_mac[i], stored_mac, 6);
                // Rebuild default label with the pairing-time MAC
                char mac_str[18];
                snprintf(
                    mac_str,
                    sizeof(mac_str),
                    "%02X:%02X:%02X:%02X:%02X:%02X",
                    stored_mac[5],
                    stored_mac[4],
                    stored_mac[3],
                    stored_mac[2],
                    stored_mac[1],
                    stored_mac[0]);
                snprintf(
                    app->bonded_dev_labels[i],
                    sizeof(app->bonded_dev_labels[i]),
                    "#%u %s",
                    (unsigned int)(i + 1),
                    mac_str);
            }

            // Load stored BT name (pairing-time name) for this device
            bt_hid_dev_mac_to_fname_key(app->bonded_devs[i].addr, key);
            flipper_format_rewind(fff);
            if(flipper_format_read_string(fff, key, temp_str)) {
                strlcpy(
                    app->bonded_dev_flipper_name[i],
                    furi_string_get_cstr(temp_str),
                    sizeof(app->bonded_dev_flipper_name[i]));
            }

            // Load custom label (overrides MAC-based default)
            // Skip auto-generated MAC labels ("#N XX:XX:XX:XX:XX:XX") — they are stale
            // after a MAC change. Only truly user-set labels (via Rename) are applied.
            bt_hid_dev_mac_to_label_key(app->bonded_devs[i].addr, key);
            flipper_format_rewind(fff);
            if(flipper_format_read_string(fff, key, temp_str)) {
                const char* loaded = furi_string_get_cstr(temp_str);
                unsigned int dummy_n;
                char dummy_mac[20] = {0};
                bool is_auto_label =
                    (sscanf(loaded, "#%u %17s", &dummy_n, dummy_mac) == 2) &&
                    (strlen(dummy_mac) == 17) && (dummy_mac[2] == ':') &&
                    (dummy_mac[5] == ':') && (dummy_mac[8] == ':') &&
                    (dummy_mac[11] == ':') && (dummy_mac[14] == ':');
                if(!is_auto_label) {
                    strlcpy(
                        app->bonded_dev_labels[i],
                        loaded,
                        sizeof(app->bonded_dev_labels[i]));
                }
            }

        }
    } while(0);

    furi_string_free(temp_str);
    flipper_format_free(fff);
    furi_record_close(RECORD_STORAGE);
}

// Consumes HID_BT_PENDING_PATH: for every "queued_XXXXXXXXXXXX" suffix that matches a
// currently live-bonded device without a pre-existing custom label, sets a "BadKB2 #N"
// label so it's recognizable as a foreign pairing. Deletes the file afterwards - entries
// are one-shot hints, the resulting label gets persisted normally via the caller's
// subsequent bt_hid_save_bonded_labels() call.
static void bt_hid_import_pending_badkb2(Hid* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);

    if(file_stream_open(stream, HID_BT_PENDING_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FuriString* line = furi_string_alloc();
        unsigned int imported_n = 0;
        while(stream_read_line(stream, line)) {
            const char* s = furi_string_get_cstr(line);
            if(strncmp(s, "queued_", 7) != 0 || strlen(s) < 19) continue;

            for(uint8_t i = 0; i < app->bonded_dev_count; i++) {
                char suffix[13];
                snprintf(
                    suffix,
                    sizeof(suffix),
                    "%02X%02X%02X%02X%02X%02X",
                    app->bonded_devs[i].addr[0],
                    app->bonded_devs[i].addr[1],
                    app->bonded_devs[i].addr[2],
                    app->bonded_devs[i].addr[3],
                    app->bonded_devs[i].addr[4],
                    app->bonded_devs[i].addr[5]);
                if(strncmp(s + 7, suffix, 12) != 0) continue;

                // Only override the still-default "#N MAC" label - never a custom Rename.
                char mac_str[18];
                snprintf(
                    mac_str,
                    sizeof(mac_str),
                    "%02X:%02X:%02X:%02X:%02X:%02X",
                    app->bonded_dev_flipper_mac[i][5],
                    app->bonded_dev_flipper_mac[i][4],
                    app->bonded_dev_flipper_mac[i][3],
                    app->bonded_dev_flipper_mac[i][2],
                    app->bonded_dev_flipper_mac[i][1],
                    app->bonded_dev_flipper_mac[i][0]);
                char auto_label[24];
                snprintf(
                    auto_label, sizeof(auto_label), "#%u %s", (unsigned int)(i + 1), mac_str);
                if(strcmp(app->bonded_dev_labels[i], auto_label) == 0) {
                    imported_n++;
                    snprintf(
                        app->bonded_dev_labels[i],
                        sizeof(app->bonded_dev_labels[i]),
                        "BadKB2 #%u",
                        imported_n);
                }
                break;
            }
        }
        furi_string_free(line);
    }

    file_stream_close(stream);
    stream_free(stream);
    storage_simply_remove(storage, HID_BT_PENDING_PATH);
    furi_record_close(RECORD_STORAGE);
}

void bt_hid_load_bonded_devices(Hid* app) {
    furi_hal_bt_get_bonded_devices(app->bonded_devs, &app->bonded_dev_count);

    // Default label: "#N Flipper-MAC"
    for(uint8_t i = 0; i < app->bonded_dev_count; i++) {
        memcpy(app->bonded_dev_flipper_mac[i], app->ble_hid_cfg.mac_address, 6);
        strlcpy(
            app->bonded_dev_flipper_name[i],
            app->ble_hid_cfg.name,
            sizeof(app->bonded_dev_flipper_name[i]));
        char mac_str[18];
        snprintf(
            mac_str,
            sizeof(mac_str),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            app->ble_hid_cfg.mac_address[5],
            app->ble_hid_cfg.mac_address[4],
            app->ble_hid_cfg.mac_address[3],
            app->ble_hid_cfg.mac_address[2],
            app->ble_hid_cfg.mac_address[1],
            app->ble_hid_cfg.mac_address[0]);
        snprintf(
            app->bonded_dev_labels[i],
            sizeof(app->bonded_dev_labels[i]),
            "#%u %s",
            (unsigned int)(i + 1),
            mac_str);
    }

    // Override with any saved pairing-time MACs and custom labels
    bt_hid_load_bonded_labels(app);

    // Import any devices bonded via BadKB2/BKB_Recorder's shared identity since our
    // last load, before persisting - see bt_hid_import_pending_badkb2 for details.
    bt_hid_import_pending_badkb2(app);

    // Auto-save so newly bonded devices get their current Flipper MAC persisted
    if(app->bonded_dev_count > 0) {
        bt_hid_save_bonded_labels(app);
    }
}

void bt_hid_save_bonded_labels(Hid* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);

    if(flipper_format_file_open_always(fff, HID_BT_LABELS_PATH)) {
        do {
            if(!flipper_format_write_header_cstr(
                   fff, HID_BT_LABELS_FILE_TYPE, HID_BT_LABELS_VERSION))
                break;
            for(uint8_t i = 0; i < app->bonded_dev_count; i++) {
                char key[20];
                // Save per-device Flipper MAC
                bt_hid_dev_mac_to_fmac_key(app->bonded_devs[i].addr, key);
                if(!flipper_format_write_hex(
                       fff, key, app->bonded_dev_flipper_mac[i], 6)) {
                    break;
                }
                // Save per-device BT name (pairing-time name)
                bt_hid_dev_mac_to_fname_key(app->bonded_devs[i].addr, key);
                if(!flipper_format_write_string_cstr(
                       fff, key, app->bonded_dev_flipper_name[i])) {
                    break;
                }
                // Save custom label — skip auto-generated MAC labels so they don't
                // get loaded as stale custom labels after a MAC change.
                bt_hid_dev_mac_to_label_key(app->bonded_devs[i].addr, key);
                {
                    char auto_mac[18];
                    char auto_label[24];
                    snprintf(
                        auto_mac,
                        sizeof(auto_mac),
                        "%02X:%02X:%02X:%02X:%02X:%02X",
                        app->bonded_dev_flipper_mac[i][5],
                        app->bonded_dev_flipper_mac[i][4],
                        app->bonded_dev_flipper_mac[i][3],
                        app->bonded_dev_flipper_mac[i][2],
                        app->bonded_dev_flipper_mac[i][1],
                        app->bonded_dev_flipper_mac[i][0]);
                    snprintf(
                        auto_label, sizeof(auto_label), "#%u %s", (unsigned int)(i + 1), auto_mac);
                    if(strcmp(app->bonded_dev_labels[i], auto_label) != 0) {
                        if(!flipper_format_write_string_cstr(fff, key, app->bonded_dev_labels[i]))
                            break;
                    }
                }
            }
        } while(0);
    }

    flipper_format_free(fff);
    furi_record_close(RECORD_STORAGE);
}

void bt_hid_remove_bonded_device_by_idx(Hid* app, uint8_t idx) {
    if(idx >= app->bonded_dev_count) return;
    bt_disconnect(app->bt);
    furi_delay_ms(200);
    furi_hal_bt_stop_advertising();
    furi_hal_bt_remove_bonded_device(
        app->bonded_devs[idx].addr_type, app->bonded_devs[idx].addr);
    furi_hal_bt_start_advertising();
}

static void bt_hid_load_cfg(Hid* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);
    bool loaded = false;

    app->ble_hid_cfg.name[0] = '\0';
    app->ble_hid_cfg.mac_address_set = false;
    app->ble_hid_cfg.pairing = GapPairingNone;
    strlcpy(app->keyboard_layout_path, HID_LAYOUT_DEFAULT, sizeof(app->keyboard_layout_path));
    hid_keynote_set_default_actions(app);

    FuriString* temp_str = furi_string_alloc();
    uint32_t temp_uint = 0;

    do {
        if(!flipper_format_file_open_existing(fff, HID_BT_CFG_PATH)) break;

        if(!flipper_format_read_header(fff, temp_str, &temp_uint)) break;
        if(strcmp(furi_string_get_cstr(temp_str), HID_BT_CFG_FILE_TYPE) != 0) break;
        if(temp_uint == 0 || temp_uint > HID_BT_CFG_VERSION)
            break;

        if(flipper_format_read_string(fff, "name", temp_str)) {
            strlcpy(
                app->ble_hid_cfg.name,
                furi_string_get_cstr(temp_str),
                sizeof(app->ble_hid_cfg.name));
        } else {
            flipper_format_rewind(fff);
        }

        if(flipper_format_read_hex(
               fff,
               "mac",
               app->ble_hid_cfg.mac_address,
               sizeof(app->ble_hid_cfg.mac_address))) {
            app->ble_hid_cfg.mac_address_set = true;
        }

        if(temp_uint >= 2) {
            for(size_t i = 0; i < HidKeynoteButtonCount; i++) {
                uint32_t action_value = 0;
                if(flipper_format_read_uint32(fff, hid_keynote_cfg_keys[i], &action_value, 1) &&
                   action_value < HidKeynoteActionCount) {
                    app->keynote_actions[i] = action_value;
                }
            }
        }

        if(temp_uint >= 4) {
            if(flipper_format_read_string(fff, "keyboard_layout", temp_str)) {
                strlcpy(
                    app->keyboard_layout_path,
                    furi_string_get_cstr(temp_str),
                    sizeof(app->keyboard_layout_path));
            }
        }

        if(temp_uint >= 5) {
            uint32_t pairing_val = 0;
            if(flipper_format_read_uint32(fff, "ble_pairing", &pairing_val, 1) &&
               pairing_val < GapPairingCount) {
                app->ble_hid_cfg.pairing = pairing_val;
            }
        }

        if(temp_uint >= 6) {
            for(size_t i = 0; i < HidKeynoteButtonCount; i++) {
                if(flipper_format_read_string(fff, hid_keynote_macro_cfg_keys[i], temp_str)) {
                    strlcpy(
                        app->keynote_macro[i],
                        furi_string_get_cstr(temp_str),
                        sizeof(app->keynote_macro[i]));
                }
            }
        }

        if(temp_uint >= 7) {
            for(size_t i = 0; i < HidKeynoteButtonCount; i++) {
                if(flipper_format_read_string(fff, hid_keynote_badkb_cfg_keys[i], temp_str)) {
                    strlcpy(
                        app->keynote_badkb_script[i],
                        furi_string_get_cstr(temp_str),
                        sizeof(app->keynote_badkb_script[i]));
                }
            }
        }

        loaded = true;
    } while(0);

    furi_string_free(temp_str);

    flipper_format_free(fff);
    furi_record_close(RECORD_STORAGE);

    if(!loaded) {
        app->ble_hid_cfg.name[0] = '\0';
        app->ble_hid_cfg.mac_address_set = false;
    }

    if(!app->ble_hid_cfg.mac_address_set) {
        bt_hid_set_default_mac(app);
    }

    if(app->hid_keynote) {
        hid_keynote_sync_actions(app->hid_keynote);
    }
    if(app->hid_keyboard) {
        hid_keyboard_set_layout_path(app->hid_keyboard, app->keyboard_layout_path);
    }
}

bool bt_hid_apply_cfg(Hid* app) {
    furi_hal_bt_stop_advertising();

    // hid_ble2 always uses its own keystore.  BadKB2 is the one that defers to hid_ble2
    // (via hid_ble2_link_identity_known in hid_ble_init) — not the other way around.
    // A reverse check here created a circular dependency: after hid_ble_adjust_config
    // was fixed to make BadKB2 inherit hid_ble2's MAC, BadKB2 bonds written at that MAC
    // would appear in .badkb2.labels, causing the Stage-2 check to fire and make hid_ble2
    // switch to BadKB2's (possibly empty) keystore on the next profile restart.
    bt_keys_storage_set_storage_path(app->bt, APP_DATA_PATH(HID_BT_KEYS_STORAGE_NAME));

    // PIN Display needs the pin screen to show the code; other modes auto-confirm
    app->bt->suppress_pin_screen = (app->ble_hid_cfg.pairing != GapPairingPinCodeShow);

    app->ble_hid_profile = bt_profile_start(app->bt, ble_profile_hid_ext, &app->ble_hid_cfg);
    if(!app->ble_hid_profile) {
        return false;
    }

    furi_hal_bt_start_advertising();
    return true;
}

void bt_hid_save_cfg(Hid* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);

    if(flipper_format_file_open_always(fff, HID_BT_CFG_PATH)) {
        do {
            if(!flipper_format_write_header_cstr(fff, HID_BT_CFG_FILE_TYPE, HID_BT_CFG_VERSION))
                break;
            if(!flipper_format_write_string_cstr(fff, "name", app->ble_hid_cfg.name)) break;
            if(app->ble_hid_cfg.mac_address_set &&
               !flipper_format_write_hex(
                   fff,
                   "mac",
                   app->ble_hid_cfg.mac_address,
                   sizeof(app->ble_hid_cfg.mac_address))) {
                break;
            }
            for(size_t i = 0; i < HidKeynoteButtonCount; i++) {
                uint32_t action_value = app->keynote_actions[i];
                if(!flipper_format_write_uint32(fff, hid_keynote_cfg_keys[i], &action_value, 1)) {
                    break;
                }
            }
            if(!flipper_format_write_string_cstr(fff, "keyboard_layout", app->keyboard_layout_path)) {
                break;
            }
            {
                uint32_t pairing_val = app->ble_hid_cfg.pairing;
                if(!flipper_format_write_uint32(fff, "ble_pairing", &pairing_val, 1)) break;
            }
            for(size_t i = 0; i < HidKeynoteButtonCount; i++) {
                if(!flipper_format_write_string_cstr(
                       fff, hid_keynote_macro_cfg_keys[i], app->keynote_macro[i])) {
                    break;
                }
            }
            for(size_t i = 0; i < HidKeynoteButtonCount; i++) {
                if(!flipper_format_write_string_cstr(
                       fff, hid_keynote_badkb_cfg_keys[i], app->keynote_badkb_script[i])) {
                    break;
                }
            }
        } while(0);
    }

    flipper_format_free(fff);
    furi_record_close(RECORD_STORAGE);
}

bool hid_handle_global_lock_input(Hid* hid, InputEvent* event) {
    // Back is handled exclusively by the central hid_input_pubsub_callback() below (hold =
    // lock, tap sequences = unlock/exit) for Media/Movie/Music_macos/TikTok ONLY (see
    // lock_feature_view_active in hid.h) - swallowed here whenever that's the active view, or
    // whenever already locked as a safety net (shouldn't normally happen outside those views,
    // but don't let a stray unlocked Back-swallow-less view leave the app stuck locked).
    // Reasoning for swallowing unconditionally within scope (not just while locked): each
    // view's own InputTypeLong+Back handling (release keys + navigate back) fires at the
    // platform's fixed ~350ms threshold, far shorter than any reasonable lock-hold duration -
    // without this, holding Back to lock always got preempted by the view's own
    // long-press-exit navigating away first, so the lock gesture could never actually
    // complete. 3x-quick-tap-Back (see hid_input_pubsub_callback()) is now the only way to
    // exit one of these 4 views via Back; this is what release-and-navigate-back on a genuine
    // long hold used to do there, so it isn't lost, just centralized. All other (non-lock-
    // feature) views are unaffected -- Back behaves exactly as before this feature existed.
    if(event->key == InputKeyBack) return hid->lock_feature_view_active || hid->app_locked;
    // All other keys: swallow only while locked, same as before.
    return hid->app_locked;
}

// Releases any HID keys/buttons the currently active view might be holding (unconditionally
// across all HID types -- harmless no-op for whichever types aren't in use by the active
// view) and pops back to the previous scene. Runs on its own generous-stack thread, same
// precaution as loader_menu.c's menu-rebuild fix: scene_manager_previous_scene() does more
// work than is safe to do inline on the tiny pubsub-callback stack. Not using toolbox's
// run_parallel() helper here since it isn't part of the FAP SDK's exported API surface
// (hid_ble2 is a FAP, unlike loader_menu.c which is built into the firmware) -- this is
// exactly what run_parallel() does internally, just inlined with APIs that are exported.
static void hid_quick_exit_view_thread_state(FuriThread* thread, FuriThreadState state, void* context) {
    UNUSED(context);
    if(state == FuriThreadStateStopped) furi_thread_free(thread);
}

static int32_t hid_quick_exit_view_worker(void* context) {
    Hid* hid = context;
    hid_hal_keyboard_release_all(hid);
    hid_hal_mouse_release_all(hid);
    hid_hal_consumer_key_release_all(hid);
    scene_manager_previous_scene(hid->scene_manager);
    return 0;
}

static void hid_quick_exit_view(Hid* hid) {
    FuriThread* thread = furi_thread_alloc_ex(NULL, 2048, hid_quick_exit_view_worker, hid);
    furi_thread_set_state_callback(thread, hid_quick_exit_view_thread_state);
    furi_thread_start(thread);
}

// Fired by back_hold_timer once Back has been held for HID_LOCK_HOLD_MS while unlocked --
// activates the lock (and vibrates) at that instant, while Back is still physically held,
// instead of only being noticed retroactively when the button comes back up.
static void hid_lock_hold_timer_callback(void* context) {
    Hid* hid = context;
    if(hid->app_locked) return; // race guard: already locked via some other path
    hid->app_locked = true;
    hid->back_click_count = 0;
    FURI_LOG_I(TAG, "lock: LOCKED (hold)");
    notification_message(hid->notifications, &sequence_single_vibro);
}

// Subscribed to the raw input pubsub for the app's whole lifetime (see hid_alloc()/hid_free()
// below), so tapping/holding Back locks/unlocks/exits regardless of which scene/view is
// currently shown - including the Submenu-based category-picker screens, which have no
// input_callback of their own to hook into.
static void hid_input_pubsub_callback(const void* message, void* context) {
    const InputEvent* event = message;
    Hid* hid = context;
    uint32_t held_ms;
    uint32_t now;

    if(event->key != InputKeyBack) return;
    // Scoped to Media/Movie/Music_macos/TikTok (see lock_feature_view_active in hid.h) -- but
    // keep evaluating if already locked, so a still-held lock (however it got into that
    // state) can always be unlocked regardless of which view ends up active.
    if(!hid->lock_feature_view_active && !hid->app_locked) return;

    FURI_LOG_I(TAG, "lock: back event type=%d locked=%d", (int)event->type, hid->app_locked);

    if(event->type == InputTypePress) {
        hid->back_press_tick = furi_get_tick();
        // Only relevant while unlocked -- while already locked, Back-hold has no meaning
        // (see the "Locked" branch below), so don't arm the lock-activation timer then.
        if(!hid->app_locked) furi_timer_start(hid->back_hold_timer, HID_LOCK_HOLD_MS);
        return;
    }
    if(event->type != InputTypeRelease) return;

    furi_timer_stop(hid->back_hold_timer);

    now = furi_get_tick();
    held_ms = (now - hid->back_press_tick) * 1000 / furi_kernel_get_tick_frequency();
    FURI_LOG_I(TAG, "lock: back release held_ms=%lu", (unsigned long)held_ms);

    if(hid->app_locked) {
        // Locked: only quick taps matter, HID_UNLOCK_CLICKS_NEEDED in a row unlocks. A long
        // hold while locked has no meaning here (a short tap while locked is swallowed by
        // hid_handle_global_lock_input() before any view sees it anyway).
        if(held_ms >= HID_LOCK_HOLD_MS) return;

        if(hid->back_click_count > 0 &&
           (now - hid->last_back_click_tick) * 1000 / furi_kernel_get_tick_frequency() >
               HID_BACK_CLICK_WINDOW_MS) {
            hid->back_click_count = 0;
        }
        hid->back_click_count++;
        hid->last_back_click_tick = now;

        if(hid->back_click_count >= HID_UNLOCK_CLICKS_NEEDED) {
            hid->app_locked = false;
            hid->back_click_count = 0;
            FURI_LOG_I(TAG, "lock: UNLOCKED (3x tap)");
            notification_message(hid->notifications, &sequence_double_vibro);
        }
        return;
    }

    // Unlocked, and the timer never fired (held_ms < HID_LOCK_HOLD_MS, else app_locked would
    // already be true and we'd have returned above) -- so this is a short tap. Counts toward
    // the 4-quick-taps exit-view gesture. Never
    // ambiguous with the 3-tap unlock gesture above since that one only ever evaluates while
    // app_locked -- the two tap-counting gestures never run in the same state.
    if(hid->back_click_count > 0 &&
       (now - hid->last_back_click_tick) * 1000 / furi_kernel_get_tick_frequency() >
           HID_BACK_CLICK_WINDOW_MS) {
        hid->back_click_count = 0;
    }
    hid->back_click_count++;
    hid->last_back_click_tick = now;

    if(hid->back_click_count >= HID_EXIT_CLICKS_NEEDED) {
        hid->back_click_count = 0;
        FURI_LOG_I(TAG, "lock: EXIT VIEW (4x tap)");
        hid_quick_exit_view(hid);
    }
}

static void bt_hid_connection_status_changed_callback(BtStatus status, void* context) {
    furi_assert(context);
    Hid* hid = context;
    const bool connected = (status == BtStatusConnected);
    hid->bt_connected = connected;
    notification_internal_message(
        hid->notifications, connected ? &sequence_set_blue_255 : &sequence_reset_blue);
    hid_keynote_set_connected_status(hid->hid_keynote, connected);
    hid_keyboard_set_connected_status(hid->hid_keyboard, connected);
    hid_numpad_set_connected_status(hid->hid_numpad, connected);
    hid_media_set_connected_status(hid->hid_media, connected);
    hid_music_macos_set_connected_status(hid->hid_music_macos, connected);
    hid_movie_set_connected_status(hid->hid_movie, connected);
    hid_mouse_set_connected_status(hid->hid_mouse, connected);
    hid_mouse_clicker_set_connected_status(hid->hid_mouse_clicker, connected);
    hid_mouse_jiggler_set_connected_status(hid->hid_mouse_jiggler, connected);
    hid_mouse_jiggler_stealth_set_connected_status(hid->hid_mouse_jiggler_stealth, connected);
    hid_ptt_set_connected_status(hid->hid_ptt, connected);
    hid_tiktok_set_connected_status(hid->hid_tiktok, connected);

}

static uint32_t hid_ptt_menu_view(void* context) {
    UNUSED(context);
    return HidViewPushToTalkMenu;
}

static uint32_t hid_keynote_config_view(void* context) {
    UNUSED(context);
    return HidViewVariableItemList;
}

Hid* hid_alloc() {
    Hid* app = malloc(sizeof(Hid));
    memset(app, 0, sizeof(Hid));

    // Gui
    app->gui = furi_record_open(RECORD_GUI);

    // Bt
    app->bt = furi_record_open(RECORD_BT);

    // Notifications
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    // Dialogs
    app->dialogs = furi_record_open(RECORD_DIALOGS);

    // View dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, hid_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, hid_back_event_callback);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Scene Manager
    app->scene_manager = scene_manager_alloc(&hid_scene_handlers, app);

    // Device Type Submenu view
    app->submenu = submenu_alloc();
    app->keynote_action_submenu = submenu_alloc();

    view_dispatcher_add_view(app->view_dispatcher, HidViewSubmenu, submenu_get_view(app->submenu));
    view_set_previous_callback(
        submenu_get_view(app->keynote_action_submenu), hid_keynote_config_view);
    view_dispatcher_add_view(
        app->view_dispatcher,
        HidViewKeynoteActionSubmenu,
        submenu_get_view(app->keynote_action_submenu));

    // Dialog view
    app->dialog = dialog_ex_alloc();
    view_dispatcher_add_view(app->view_dispatcher, HidViewDialog, dialog_ex_get_view(app->dialog));

    // Text input
    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, HidViewTextInput, text_input_get_view(app->text_input));

    // Byte input
    app->byte_input = byte_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, HidViewByteInput, byte_input_get_view(app->byte_input));

    hid_keynote_set_default_actions(app);

    // Variable item list
    app->variable_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        HidViewVariableItemList,
        variable_item_list_get_view(app->variable_item_list));

    // Popup view
    app->popup = popup_alloc();
    view_dispatcher_add_view(app->view_dispatcher, HidViewPopup, popup_get_view(app->popup));

    // Keynote view
    app->hid_keynote = hid_keynote_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher, HidViewKeynote, hid_keynote_get_view(app->hid_keynote));

    // Keynote Key Picker view
    app->hid_keynote_key_picker = hid_keynote_key_picker_alloc(app);
    view_set_previous_callback(
        hid_keynote_key_picker_get_view(app->hid_keynote_key_picker), hid_keynote_config_view);
    view_dispatcher_add_view(
        app->view_dispatcher,
        HidViewKeynoteKeyPicker,
        hid_keynote_key_picker_get_view(app->hid_keynote_key_picker));

    // Keyboard view
    app->hid_keyboard = hid_keyboard_alloc(app);
    hid_keyboard_set_layout_path(app->hid_keyboard, app->keyboard_layout_path);
    view_dispatcher_add_view(
        app->view_dispatcher, HidViewKeyboard, hid_keyboard_get_view(app->hid_keyboard));

    //Numpad keyboard view
    app->hid_numpad = hid_numpad_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher, HidViewNumpad, hid_numpad_get_view(app->hid_numpad));

    // Media view
    app->hid_media = hid_media_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher, HidViewMedia, hid_media_get_view(app->hid_media));

    // Music MacOs view
    app->hid_music_macos = hid_music_macos_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher, HidViewMusicMacOs, hid_music_macos_get_view(app->hid_music_macos));

    // Movie view
    app->hid_movie = hid_movie_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher, HidViewMovie, hid_movie_get_view(app->hid_movie));

    // TikTok view
    app->hid_tiktok = hid_tiktok_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher, BtHidViewTikTok, hid_tiktok_get_view(app->hid_tiktok));

    // Mouse view
    app->hid_mouse = hid_mouse_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher, HidViewMouse, hid_mouse_get_view(app->hid_mouse));

    // Mouse clicker view
    app->hid_mouse_clicker = hid_mouse_clicker_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher,
        HidViewMouseClicker,
        hid_mouse_clicker_get_view(app->hid_mouse_clicker));

    // Mouse jiggler view
    app->hid_mouse_jiggler = hid_mouse_jiggler_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher,
        HidViewMouseJiggler,
        hid_mouse_jiggler_get_view(app->hid_mouse_jiggler));
    // Mouse jiggler stealth view
    app->hid_mouse_jiggler_stealth = hid_mouse_jiggler_stealth_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher,
        HidViewMouseJigglerStealth,
        hid_mouse_jiggler_stealth_get_view(app->hid_mouse_jiggler_stealth));

    // PushToTalk view
    app->hid_ptt_menu = hid_ptt_menu_alloc(app);
    view_dispatcher_add_view(
        app->view_dispatcher, HidViewPushToTalkMenu, hid_ptt_menu_get_view(app->hid_ptt_menu));
    app->hid_ptt = hid_ptt_alloc(app);
    view_set_previous_callback(hid_ptt_get_view(app->hid_ptt), hid_ptt_menu_view);
    view_dispatcher_add_view(
        app->view_dispatcher, HidViewPushToTalk, hid_ptt_get_view(app->hid_ptt));

    // Key-lock: subscribe to raw input regardless of GUI focus, see hid_input_pubsub_callback().
    // lock_feature_view_active starts false (correct: HidSceneStart/HidSceneCategory are
    // shown before any HidView is switched to) -- hid_scene_main_on_enter() sets it from then
    // on for every subsequent view switch.
    app->lock_feature_view_active = false;
    app->input_pubsub = furi_record_open(RECORD_INPUT_EVENTS);
    app->input_subscription =
        furi_pubsub_subscribe(app->input_pubsub, hid_input_pubsub_callback, app);
    app->back_hold_timer =
        furi_timer_alloc(hid_lock_hold_timer_callback, FuriTimerTypeOnce, app);

    return app;
}

void hid_free(Hid* app) {
    furi_assert(app);

#ifdef HID_TRANSPORT_BLE
    if(app->badkb_active_script) {
        bad_usb_script_stop(app->badkb_active_script);
        bad_usb_script_close(app->badkb_active_script);
        app->badkb_active_script = NULL;
    }
#endif

    furi_timer_free(app->back_hold_timer);
    furi_pubsub_unsubscribe(app->input_pubsub, app->input_subscription);
    furi_record_close(RECORD_INPUT_EVENTS);

    // Reset notification
#ifdef HID_TRANSPORT_BLE
    notification_internal_message(app->notifications, &sequence_reset_blue);
#endif
    // Free views
    view_dispatcher_remove_view(app->view_dispatcher, HidViewSubmenu);
    submenu_free(app->submenu);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewKeynoteActionSubmenu);
    submenu_free(app->keynote_action_submenu);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewDialog);
    dialog_ex_free(app->dialog);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewTextInput);
    text_input_free(app->text_input);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewByteInput);
    byte_input_free(app->byte_input);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewVariableItemList);
    variable_item_list_free(app->variable_item_list);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewPopup);
    popup_free(app->popup);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewKeynote);
    hid_keynote_free(app->hid_keynote);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewKeynoteKeyPicker);
    hid_keynote_key_picker_free(app->hid_keynote_key_picker);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewKeyboard);
    hid_keyboard_free(app->hid_keyboard);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewNumpad);
    hid_numpad_free(app->hid_numpad);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewMedia);
    hid_media_free(app->hid_media);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewMusicMacOs);
    hid_music_macos_free(app->hid_music_macos);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewMovie);
    hid_movie_free(app->hid_movie);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewMouse);
    hid_mouse_free(app->hid_mouse);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewMouseClicker);
    hid_mouse_clicker_free(app->hid_mouse_clicker);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewMouseJiggler);
    hid_mouse_jiggler_free(app->hid_mouse_jiggler);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewMouseJigglerStealth);
    hid_mouse_jiggler_stealth_free(app->hid_mouse_jiggler_stealth);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewPushToTalkMenu);
    hid_ptt_menu_free(app->hid_ptt_menu);
    view_dispatcher_remove_view(app->view_dispatcher, HidViewPushToTalk);
    hid_ptt_free(app->hid_ptt);
    view_dispatcher_remove_view(app->view_dispatcher, BtHidViewTikTok);
    hid_tiktok_free(app->hid_tiktok);
    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);

    // Close records
    furi_record_close(RECORD_GUI);
    app->gui = NULL;
    furi_record_close(RECORD_NOTIFICATION);
    app->notifications = NULL;
    furi_record_close(RECORD_DIALOGS);
    app->dialogs = NULL;
    furi_record_close(RECORD_BT);
    app->bt = NULL;

    // Free rest
    free(app);
}

int32_t hid_usb_app(void* p) {
    UNUSED(p);
    Hid* app = hid_alloc();

    FURI_LOG_D("HID", "Starting as USB app");

    FuriHalUsbInterface* usb_mode_prev = furi_hal_usb_get_config();
    furi_hal_usb_unlock();
    furi_check(furi_hal_usb_set_config(&usb_hid, NULL) == true);

    dolphin_deed(DolphinDeedPluginStart);

    scene_manager_next_scene(app->scene_manager, HidSceneStart);

    view_dispatcher_run(app->view_dispatcher);

    furi_hal_usb_set_config(usb_mode_prev, NULL);

    hid_free(app);

    return 0;
}

int32_t hid_usb2_app(void* p) {
    UNUSED(p);
    Hid* app = hid_alloc();

    FURI_LOG_D("HID", "Starting as USB2 app");

    FuriHalUsbInterface* usb_mode_prev = furi_hal_usb_get_config();
    furi_hal_usb_unlock();
    furi_check(furi_hal_usb_set_config(&usb_hid, NULL) == true);

    dolphin_deed(DolphinDeedPluginStart);

    scene_manager_next_scene(app->scene_manager, HidSceneStart);

    view_dispatcher_run(app->view_dispatcher);

    furi_hal_usb_set_config(usb_mode_prev, NULL);

    hid_free(app);

    return 0;
}

int32_t hid_ble2_app(void* p) {
    UNUSED(p);
    Hid* app = hid_alloc();

    FURI_LOG_D("HID", "Starting as BLE app");

    bt_disconnect(app->bt);

    // Wait 2nd core to update nvm storage
    furi_delay_ms(200);

    // Migrate data from old sd-card folder
    Storage* storage = furi_record_open(RECORD_STORAGE);

    storage_common_migrate(
        storage,
        EXT_PATH("apps/Tools/" HID_BT_KEYS_STORAGE_NAME),
        APP_DATA_PATH(HID_BT_KEYS_STORAGE_NAME));

    bt_keys_storage_set_storage_path(app->bt, APP_DATA_PATH(HID_BT_KEYS_STORAGE_NAME));

    furi_record_close(RECORD_STORAGE);

    bt_hid_load_cfg(app);

    furi_check(bt_hid_apply_cfg(app));

    bt_set_status_changed_callback(app->bt, bt_hid_connection_status_changed_callback, app);

    dolphin_deed(DolphinDeedPluginStart);

    scene_manager_next_scene(app->scene_manager, HidSceneStart);

    view_dispatcher_run(app->view_dispatcher);

    bt_set_status_changed_callback(app->bt, NULL, NULL);

    app->bt->suppress_pin_screen = false;
    bt_disconnect(app->bt);

    // Wait 2nd core to update nvm storage
    furi_delay_ms(200);

    bt_keys_storage_set_default_path(app->bt);

    furi_check(bt_profile_restore_default(app->bt));

    hid_free(app);

    return 0;
}
