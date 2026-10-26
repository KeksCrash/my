#pragma once

#include <furi.h>
#include <furi_hal_bt.h>

// BadKB2's own settings/labels/keys files on the SD card. Read-only from here.
#define BADKB2_SETTINGS_PATH "/ext/apps_data/badkb2/.badkb2.settings"
#define BADKB2_KEYS_PATH     "/ext/apps_data/badkb2/.bt_hid.keys"
#define BADKB2_LABELS_PATH   "/ext/apps_data/badkb2/.badkb2.labels"

#define BADKB2_LINK_MAX_ENTRIES FURI_HAL_BT_MAX_BONDED_DEVS

typedef struct {
    uint8_t mac[GAP_MAC_ADDR_SIZE]; // Flipper MAC that was active when this device paired
    char name[FURI_HAL_BT_ADV_NAME_LENGTH]; // Flipper BT name at that time, if recorded
    bool has_name;
    char label[24]; // BadKB2's own custom label for this device (from Rename), if any
    bool has_label;
} Badkb2Identity;

// Best-effort read of BadKB2's currently configured BLE name + MAC.
// Returns false if BadKB2 was never configured (no settings file, or no mac saved).
bool badkb2_link_load_cfg(
    char name_out[FURI_HAL_BT_ADV_NAME_LENGTH],
    uint8_t mac_out[GAP_MAC_ADDR_SIZE]);

// Reads BadKB2's per-device pairing-time identities from its .badkb2.labels file (one
// entry per bonded device that still has a recorded fmac_ entry). Does not touch the
// live BT stack - pure text parsing, safe regardless of what's currently active.
size_t badkb2_link_load_saved_identities(Badkb2Identity* out, size_t max_count);

// True if name+mac matches BadKB2's current settings, or any of its saved per-device
// identities (mac always compared; name only compared when the saved entry has one).
bool badkb2_link_identity_known(const char* name, const uint8_t mac[GAP_MAC_ADDR_SIZE]);
