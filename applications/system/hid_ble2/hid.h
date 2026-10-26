#pragma once

#include <furi.h>
#include <furi_hal_bt.h>
#include <furi_hal_usb.h>
#include <furi_hal_usb_hid.h>

#include "helpers/ble_hid_ext_profile.h"
#include "helpers/bad_usb_hid.h"

typedef struct BadUsbScript BadUsbScript;

#define BADKB_SCRIPT_PATH_MAX 256

#include <bt/bt_service/bt.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <notification/notification.h>
#include <storage/storage.h>
#include <dialogs/dialogs.h>

#include <gui/modules/submenu.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/popup.h>
#include <gui/modules/text_input.h>
#include <gui/modules/byte_input.h>
#include <gui/modules/variable_item_list.h>
#include "views/hid_keynote.h"
#include "views/hid_keynote_key_picker.h"
#include "views/hid_keyboard.h"
#include "views/hid_numpad.h"
#include "views/hid_media.h"
#include "views/hid_music_macos.h"
#include "views/hid_movie.h"
#include "views/hid_mouse.h"
#include "views/hid_mouse_clicker.h"
#include "views/hid_mouse_jiggler.h"
#include "views/hid_mouse_jiggler_stealth.h"
#include "views/hid_tiktok.h"
#include "views/hid_ptt.h"
#include "views/hid_ptt_menu.h"

#include "scenes/hid_scene.h"

#define HID_BT_KEYS_STORAGE_NAME ".bt_hid2.keys"

typedef struct Hid Hid;

struct Hid {
    FuriHalBleProfileBase* ble_hid_profile;
    BleProfileHidExtParams ble_hid_cfg;
    Bt* bt;
    Gui* gui;
    NotificationApp* notifications;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    DialogsApp* dialogs;
    Submenu* submenu;
    Submenu* keynote_action_submenu;
    DialogEx* dialog;
    TextInput* text_input;
    ByteInput* byte_input;
    VariableItemList* variable_item_list;
    Popup* popup;
    HidKeynote* hid_keynote;
    HidKeynoteKeyPicker* hid_keynote_key_picker;
    HidKeyboard* hid_keyboard;
    HidNumpad* hid_numpad;
    HidMedia* hid_media;
    HidMusicMacos* hid_music_macos;
    HidMovie* hid_movie;
    HidMouse* hid_mouse;
    HidMouseClicker* hid_mouse_clicker;
    HidMouseJiggler* hid_mouse_jiggler;
    HidMouseJigglerStealth* hid_mouse_jiggler_stealth;
    HidTikTok* hid_tiktok;
    HidPushToTalk* hid_ptt;
    HidPushToTalkMenu* hid_ptt_menu;
    HidKeynoteAction keynote_actions[HidKeynoteButtonCount];
    // Per-button custom macro script, only meaningful when keynote_actions[i] ==
    // HidKeynoteActionCustomMacro - see hid_keynote_play_macro() for syntax.
    char keynote_macro[HidKeynoteButtonCount][KEYNOTE_MACRO_MAX_LEN];
    // Step-by-step macro builder state (scenes/hid_scene_keynote_macro_edit.c +
    // scenes/hid_scene_keynote_key_picker.c): while macro_builder_active, picking a key
    // in the keyboard key-picker appends a step to keynote_macro[button] instead of
    // assigning the whole button - modifier picks accumulate into pending_mods (combined
    // into the next non-modifier key picked) rather than committing immediately. Reset
    // whenever hid_scene_keynote_config is (re-)entered, regardless of how we got there.
    bool macro_builder_active;
    uint16_t macro_builder_pending_mods;
    // Scratch buffer for the "Add Text..."/"Add Delay..." text_input sub-steps of the
    // macro builder, before the entered value is formatted and appended to keynote_macro.
    char macro_builder_input[64];
    // BadKB script execution via hid_ble2 passthrough
    BadUsbScript* badkb_active_script;
    int8_t badkb_active_button;
    BadUsbHidInterface badkb_iface;
    BadUsbHidConfig badkb_hid_cfg;
    char keynote_badkb_script[HidKeynoteButtonCount][BADKB_SCRIPT_PATH_MAX];
    FuriString* badkb_file_path;

    char keyboard_layout_path[256];
    uint8_t ble_mac_buf[6];
    // Bonded device list for selective unpair scene
    FuriHalBtBondedDevice bonded_devs[FURI_HAL_BT_MAX_BONDED_DEVS];
    uint8_t bonded_dev_count;
    char bonded_dev_labels[FURI_HAL_BT_MAX_BONDED_DEVS][24];
    uint8_t bonded_dev_flipper_mac[FURI_HAL_BT_MAX_BONDED_DEVS][6];
    char bonded_dev_flipper_name[FURI_HAL_BT_MAX_BONDED_DEVS][FURI_HAL_BT_ADV_NAME_LENGTH];
    char bonded_dev_rename_buf[24];
    bool bt_connected;
    uint8_t selected_bonded_dev_idx; // 0xFF = Unpair All

    // Key-lock: holding Back for HID_LOCK_HOLD_MS locks the app (app_locked = true); while
    // locked, 3 short Back presses within HID_BACK_CLICK_WINDOW_MS of each other unlock
    // again. Deliberately asymmetric (hold vs. triple-tap) so the two gestures can't be
    // confused with each other. Detected centrally in hid_input_pubsub_callback() (hid.c),
    // which subscribes directly to the raw RECORD_INPUT_EVENTS pubsub - this fires
    // regardless of which scene/view currently has GUI focus (including the Submenu-based
    // category-picker screens, which aren't one of the per-mode views below and would
    // otherwise never see this). Independent of and additive to the existing
    // InputTypeLong+Back handling in each view (release held keys + navigate back), which
    // keeps firing unchanged as an alternative to the 4-quick-taps exit gesture below.
    //
    // No timer/disambiguation needed between the different Back gestures (hold-to-lock,
    // tap-to-unlock, tap-to-exit-view): hold-to-lock and the two tap-counting gestures are
    // told apart purely by held_ms vs. HID_LOCK_HOLD_MS (see hid_input_pubsub_callback()),
    // and the two tap-counting gestures (3x unlock, 4x exit-view) never run in the same
    // state -- 3x-unlock only evaluates while app_locked, 4x-exit-view only while !app_locked
    // -- so a run of quick taps is never ambiguous between them.
    bool                     app_locked;
    uint32_t                 back_press_tick;
    uint8_t                  back_click_count;
    uint32_t                 last_back_click_tick;
    FuriPubSub*              input_pubsub;
    FuriPubSubSubscription*  input_subscription;
    // One-shot timer started on Back InputTypePress (while unlocked) so the lock activates
    // -- and vibrates -- the instant HID_LOCK_HOLD_MS elapses, while Back is still held,
    // instead of only being noticed retroactively on release. Stopped on release if it
    // hasn't fired yet. See hid_input_pubsub_callback() / hid_lock_hold_timer_callback().
    FuriTimer*               back_hold_timer;

    // Scopes the lock feature above to only Media/Movie/Music_macos/TikTok -- set in the one
    // place every view switch flows through, scenes/hid_scene_main.c's on_enter(). false on
    // all other views: Back behaves exactly as it did before this feature existed there
    // (hid_handle_global_lock_input() only swallows Back when this is true, or when already
    // locked as a safety net).
    bool                     lock_feature_view_active;
};

// How long Back must be held (in ms), while unlocked, to lock. Comfortably above the
// platform's InputTypeLong threshold (~350ms) so a normal long-press-back (used throughout
// the views to release held HID keys and navigate back) is a distinct gesture, but otherwise
// kept short since this is now the primary/expected way to lock.
#define HID_LOCK_HOLD_MS 1000
// Max. gap (ms) between consecutive quick Back taps for them to count as the same sequence.
#define HID_BACK_CLICK_WINDOW_MS 1000
// Quick Back taps (while locked) needed to unlock.
#define HID_UNLOCK_CLICKS_NEEDED  3
// Quick Back taps (while unlocked) needed to release held HID keys and exit the current view.
#define HID_EXIT_CLICKS_NEEDED  3

// Call at the very top of every view's input_callback, before any of its own handling.
// Returns true (swallow the event, view does nothing else with it) while the app is locked.
// Returns false otherwise. Lock/unlock detection itself happens centrally, see above.
bool hid_handle_global_lock_input(Hid* hid, InputEvent* event);

void bt_hid_remove_pairing(Hid* app);
void bt_hid_load_bonded_devices(Hid* app);
void bt_hid_remove_bonded_device_by_idx(Hid* app, uint8_t idx);
void bt_hid_save_bonded_labels(Hid* app);
bool bt_hid_apply_cfg(Hid* app);
void bt_hid_save_cfg(Hid* app);

void hid_hal_keyboard_press(Hid* instance, uint16_t event);
void hid_hal_keyboard_release(Hid* instance, uint16_t event);
void hid_hal_keyboard_release_all(Hid* instance);

void hid_hal_consumer_key_press(Hid* instance, uint16_t event);
void hid_hal_consumer_key_release(Hid* instance, uint16_t event);
void hid_hal_consumer_key_release_all(Hid* instance);

void hid_hal_mouse_move(Hid* instance, int8_t dx, int8_t dy);
void hid_hal_mouse_scroll(Hid* instance, int8_t delta);
void hid_hal_mouse_press(Hid* instance, uint16_t event);
void hid_hal_mouse_release(Hid* instance, uint16_t event);
void hid_hal_mouse_release_all(Hid* instance);
