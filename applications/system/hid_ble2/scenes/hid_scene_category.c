#include "../hid.h"
#include "../views.h"
#include <string.h>

/* Item indices per category – each category counts from 0 independently */

typedef enum {
    HidCatKeynoteItem_Keynote = 0,
    HidCatKeynoteItem_Vertical,
    HidCatKeynoteItem_Configured,
    HidCatKeynoteItem_ConfiguredVertical,
    HidCatKeynoteItem_Config,
} HidCatKeynoteItem;

typedef enum {
    HidCatKeyboardItem_Keyboard = 0,
    HidCatKeyboardItem_Layout,
    HidCatKeyboardItem_Numpad,
} HidCatKeyboardItem;

typedef enum {
    HidCatMouseItem_Mouse = 0,
    HidCatMouseItem_Clicker,
    HidCatMouseItem_Jiggler,
    HidCatMouseItem_JigglerStealth,
    HidCatMouseItem_PushToTalk,
} HidCatMouseItem;

typedef enum {
    HidCatMediasItem_Movie = 0,
    HidCatMediasItem_TikTok,
    HidCatMediasItem_AppleMusic,
} HidCatMediasItem;

typedef enum {
    HidCatBluetoothItem_Rename = 0,
    HidCatBluetoothItem_Mac,
    HidCatBluetoothItem_Pairing,
    HidCatBluetoothItem_Unpair,
} HidCatBluetoothItem;

/* Category IDs match HidStartIndex values from hid_scene_start.c */
#define CAT_KEYNOTES  1
#define CAT_KEYBOARD  2
#define CAT_MOUSE     3
#define CAT_MEDIAS    4
#define CAT_BLUETOOTH 5

static void hid_scene_category_submenu_callback(void* context, uint32_t index) {
    furi_assert(context);
    Hid* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

static const char* hid_scene_category_layout_name(const Hid* app) {
    if(app->keyboard_layout_path[0] == '\0') return "en-US.kl";
    const char* slash = strrchr(app->keyboard_layout_path, '/');
    return slash ? slash + 1 : app->keyboard_layout_path;
}

void hid_scene_category_on_enter(void* context) {
    Hid* app = context;
    uint32_t state = scene_manager_get_scene_state(app->scene_manager, HidSceneCategory);
    uint32_t category = state >> 16;
    uint32_t selected = state & 0xFFFF;

    switch(category) {
    case CAT_KEYNOTES:
        submenu_set_header(app->submenu, "Keynotes");
        submenu_add_item(
            app->submenu,
            "Keynote",
            HidCatKeynoteItem_Keynote,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "Keynote Vertical",
            HidCatKeynoteItem_Vertical,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "Keynote Configured",
            HidCatKeynoteItem_Configured,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "Keynote Configured Vertical",
            HidCatKeynoteItem_ConfiguredVertical,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "Keynote Config",
            HidCatKeynoteItem_Config,
            hid_scene_category_submenu_callback,
            app);
        break;

    case CAT_KEYBOARD: {
        char layout_item[64];
        snprintf(
            layout_item,
            sizeof(layout_item),
            "Keyboard Layout: %.46s",
            hid_scene_category_layout_name(app));
        submenu_set_header(app->submenu, "Keyboard");
        submenu_add_item(
            app->submenu,
            "Keyboard",
            HidCatKeyboardItem_Keyboard,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            layout_item,
            HidCatKeyboardItem_Layout,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "Numpad",
            HidCatKeyboardItem_Numpad,
            hid_scene_category_submenu_callback,
            app);
        break;
    }

    case CAT_MOUSE:
        submenu_set_header(app->submenu, "Mouse");
        submenu_add_item(
            app->submenu,
            "Mouse",
            HidCatMouseItem_Mouse,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "Mouse Clicker",
            HidCatMouseItem_Clicker,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "Mouse Jiggler",
            HidCatMouseItem_Jiggler,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "Mouse Jiggler Stealth",
            HidCatMouseItem_JigglerStealth,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "PushToTalk",
            HidCatMouseItem_PushToTalk,
            hid_scene_category_submenu_callback,
            app);
        break;

    case CAT_MEDIAS:
        submenu_set_header(app->submenu, "Medias");
        submenu_add_item(
            app->submenu,
            "Movie",
            HidCatMediasItem_Movie,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "TikTok / YT Shorts",
            HidCatMediasItem_TikTok,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "Apple Music macOS",
            HidCatMediasItem_AppleMusic,
            hid_scene_category_submenu_callback,
            app);
        break;

#ifdef HID_TRANSPORT_BLE
    case CAT_BLUETOOTH:
        submenu_set_header(app->submenu, "Bluetooth");
        submenu_add_item(
            app->submenu,
            "Bluetooth Remote Name",
            HidCatBluetoothItem_Rename,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "Bluetooth MAC Address",
            HidCatBluetoothItem_Mac,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "Bluetooth Pairing Mode",
            HidCatBluetoothItem_Pairing,
            hid_scene_category_submenu_callback,
            app);
        submenu_add_item(
            app->submenu,
            "Bluetooth Unpairing",
            HidCatBluetoothItem_Unpair,
            hid_scene_category_submenu_callback,
            app);
        break;
#endif

    default:
        furi_crash();
    }

    submenu_set_selected_item(app->submenu, selected);
    view_dispatcher_switch_to_view(app->view_dispatcher, HidViewSubmenu);
}

bool hid_scene_category_on_event(void* context, SceneManagerEvent event) {
    Hid* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        uint32_t state = scene_manager_get_scene_state(app->scene_manager, HidSceneCategory);
        uint32_t category = state >> 16;

        // persist cursor position within this category
        scene_manager_set_scene_state(
            app->scene_manager, HidSceneCategory, (category << 16) | event.event);

        switch(category) {
        case CAT_KEYNOTES:
            if(event.event == HidCatKeynoteItem_Config) {
                scene_manager_next_scene(app->scene_manager, HidSceneKeynoteConfig);
            } else {
                switch(event.event) {
                case HidCatKeynoteItem_Keynote:
                    hid_keynote_set_mode(app->hid_keynote, HidKeynoteModeDefault);
                    hid_keynote_set_orientation(app->hid_keynote, false);
                    break;
                case HidCatKeynoteItem_Vertical:
                    hid_keynote_set_mode(app->hid_keynote, HidKeynoteModeDefault);
                    hid_keynote_set_orientation(app->hid_keynote, true);
                    break;
                case HidCatKeynoteItem_Configured:
                    hid_keynote_set_mode(app->hid_keynote, HidKeynoteModeConfigured);
                    hid_keynote_set_orientation(app->hid_keynote, false);
                    break;
                case HidCatKeynoteItem_ConfiguredVertical:
                    hid_keynote_set_mode(app->hid_keynote, HidKeynoteModeConfigured);
                    hid_keynote_set_orientation(app->hid_keynote, true);
                    break;
                default:
                    furi_crash();
                }
                scene_manager_set_scene_state(app->scene_manager, HidSceneMain, HidViewKeynote);
                scene_manager_next_scene(app->scene_manager, HidSceneMain);
            }
            break;

        case CAT_KEYBOARD:
            if(event.event == HidCatKeyboardItem_Layout) {
                scene_manager_next_scene(app->scene_manager, HidSceneKeyboardLayout);
            } else {
                HidView view_id;
                switch(event.event) {
                case HidCatKeyboardItem_Keyboard:
                    view_id = HidViewKeyboard;
                    break;
                case HidCatKeyboardItem_Numpad:
                    view_id = HidViewNumpad;
                    break;
                default:
                    furi_crash();
                }
                scene_manager_set_scene_state(app->scene_manager, HidSceneMain, view_id);
                scene_manager_next_scene(app->scene_manager, HidSceneMain);
            }
            break;

        case CAT_MOUSE: {
            HidView view_id;
            switch(event.event) {
            case HidCatMouseItem_Mouse:
                view_id = HidViewMouse;
                break;
            case HidCatMouseItem_Clicker:
                view_id = HidViewMouseClicker;
                break;
            case HidCatMouseItem_Jiggler:
                view_id = HidViewMouseJiggler;
                break;
            case HidCatMouseItem_JigglerStealth:
                view_id = HidViewMouseJigglerStealth;
                break;
            case HidCatMouseItem_PushToTalk:
                view_id = HidViewPushToTalkMenu;
                break;
            default:
                furi_crash();
            }
            scene_manager_set_scene_state(app->scene_manager, HidSceneMain, view_id);
            scene_manager_next_scene(app->scene_manager, HidSceneMain);
            break;
        }

        case CAT_MEDIAS: {
            HidView view_id;
            switch(event.event) {
            case HidCatMediasItem_Movie:
                view_id = HidViewMovie;
                break;
            case HidCatMediasItem_TikTok:
                view_id = BtHidViewTikTok;
                break;
            case HidCatMediasItem_AppleMusic:
                view_id = HidViewMusicMacOs;
                break;
            default:
                furi_crash();
            }
            scene_manager_set_scene_state(app->scene_manager, HidSceneMain, view_id);
            scene_manager_next_scene(app->scene_manager, HidSceneMain);
            break;
        }

#ifdef HID_TRANSPORT_BLE
        case CAT_BLUETOOTH:
            switch(event.event) {
            case HidCatBluetoothItem_Rename:
                scene_manager_next_scene(app->scene_manager, HidSceneRename);
                break;
            case HidCatBluetoothItem_Mac:
                scene_manager_next_scene(app->scene_manager, HidSceneBleMac);
                break;
            case HidCatBluetoothItem_Pairing:
                scene_manager_next_scene(app->scene_manager, HidScenePairing);
                break;
            case HidCatBluetoothItem_Unpair:
                scene_manager_next_scene(app->scene_manager, HidSceneUnpair);
                break;
            default:
                furi_crash();
            }
            break;
#endif

        default:
            furi_crash();
        }
        consumed = true;
    }

    return consumed;
}

void hid_scene_category_on_exit(void* context) {
    Hid* app = context;
    submenu_reset(app->submenu);
}
