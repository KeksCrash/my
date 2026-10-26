#include "badkb2_link.h"
#include <flipper_format/flipper_format.h>
#include <storage/storage.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>
#include <string.h>
#include <stdio.h>

// Generous parse-time cap: the labels file can accumulate more stale device
// entries over time than are currently bonded (unpairing a single device does
// not prune its labels), so this is intentionally larger than the small
// BADKB2_LINK_MAX_ENTRIES cap applied to the final output list.
#define BADKB2_LINK_PARSE_MAX 16

typedef struct {
    char suffix[13]; // 12 hex chars from the fmac_/fname_/label_ key + NUL
    uint8_t mac[GAP_MAC_ADDR_SIZE];
    bool has_mac;
    char name[FURI_HAL_BT_ADV_NAME_LENGTH];
    bool has_name;
    char label[24];
    bool has_label;
} Badkb2ParsedEntry;

static Badkb2ParsedEntry* badkb2_link_find_or_add(
    Badkb2ParsedEntry* entries,
    size_t* entry_count,
    const char* suffix) {
    for(size_t i = 0; i < *entry_count; i++) {
        if(strcmp(entries[i].suffix, suffix) == 0) return &entries[i];
    }
    if(*entry_count >= BADKB2_LINK_PARSE_MAX) return NULL;
    Badkb2ParsedEntry* e = &entries[(*entry_count)++];
    memset(e, 0, sizeof(*e));
    strlcpy(e->suffix, suffix, sizeof(e->suffix));
    return e;
}

bool badkb2_link_load_cfg(
    char name_out[FURI_HAL_BT_ADV_NAME_LENGTH],
    uint8_t mac_out[GAP_MAC_ADDR_SIZE]) {
    name_out[0] = '\0';
    memset(mac_out, 0, GAP_MAC_ADDR_SIZE);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);
    FuriString* temp_str = furi_string_alloc();
    bool got_mac = false;

    do {
        if(!flipper_format_file_open_existing(fff, BADKB2_SETTINGS_PATH)) break;

        // BadKB2's settings file may evolve - don't gate on header/version, just
        // best-effort read the two keys we care about.
        uint32_t version = 0;
        flipper_format_read_header(fff, temp_str, &version);

        flipper_format_rewind(fff);
        if(flipper_format_read_string(fff, "ble_name", temp_str)) {
            strlcpy(name_out, furi_string_get_cstr(temp_str), FURI_HAL_BT_ADV_NAME_LENGTH);
        }

        flipper_format_rewind(fff);
        got_mac = flipper_format_read_hex(fff, "ble_mac", mac_out, GAP_MAC_ADDR_SIZE);
    } while(0);

    furi_string_free(temp_str);
    flipper_format_free(fff);
    furi_record_close(RECORD_STORAGE);

    return got_mac;
}

size_t badkb2_link_load_saved_identities(Badkb2Identity* out, size_t max_count) {
    if(max_count == 0) return 0;

    // Heap-allocated: this struct array (~700 bytes) must not live on the caller's
    // stack. bt_hid_apply_cfg calls this from a 2-4KB main-thread stack that's
    // already deep into BT-stack + Storage/FlipperFormat call frames - adding a
    // large local array here caused an MPU fault / stack overflow in testing.
    Badkb2ParsedEntry* entries = malloc(sizeof(Badkb2ParsedEntry) * BADKB2_LINK_PARSE_MAX);
    if(!entries) return 0;
    size_t entry_count = 0;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);

    // Plain text-line parsing of BadKB2's labels file - deliberately avoids touching
    // the live BT stack (no furi_hal_bt_get_bonded_devices call), so this is safe to
    // call regardless of which app/keystore is currently active.
    if(file_stream_open(stream, BADKB2_LABELS_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FuriString* line = furi_string_alloc();

        while(stream_read_line(stream, line)) {
            const char* s = furi_string_get_cstr(line);
            bool is_fmac = strncmp(s, "fmac_", 5) == 0;
            bool is_fname = !is_fmac && strncmp(s, "fname_", 6) == 0;
            bool is_label = !is_fmac && !is_fname && strncmp(s, "label_", 6) == 0;
            if(!is_fmac && !is_fname && !is_label) continue;

            const char* suffix_start = s + (is_fmac ? 5 : 6);
            const char* colon = strchr(suffix_start, ':');
            if(!colon || (size_t)(colon - suffix_start) != 12) continue;

            char suffix[13];
            memcpy(suffix, suffix_start, 12);
            suffix[12] = '\0';

            const char* value = colon + 1;
            if(*value == ' ') value++;

            Badkb2ParsedEntry* entry = badkb2_link_find_or_add(entries, &entry_count, suffix);
            if(!entry) continue;

            if(is_fmac) {
                unsigned int b[GAP_MAC_ADDR_SIZE];
                if(sscanf(
                       value,
                       "%2X %2X %2X %2X %2X %2X",
                       &b[0],
                       &b[1],
                       &b[2],
                       &b[3],
                       &b[4],
                       &b[5]) == GAP_MAC_ADDR_SIZE) {
                    for(size_t k = 0; k < GAP_MAC_ADDR_SIZE; k++) entry->mac[k] = (uint8_t)b[k];
                    entry->has_mac = true;
                }
            } else if(is_fname) {
                strlcpy(entry->name, value, sizeof(entry->name));
                entry->has_name = true;
            } else {
                // label_XXXXXX only ever exists in the file for devices the user actually
                // renamed (bt_hid_save_bonded_labels skips writing it for auto-generated
                // "#N MAC" labels) - so its mere presence here means it's a genuine
                // custom label, safe to show as-is.
                strlcpy(entry->label, value, sizeof(entry->label));
                entry->has_label = true;
            }
        }

        furi_string_free(line);
    }

    file_stream_close(stream);
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);

    size_t result = 0;
    for(size_t i = 0; i < entry_count && result < max_count; i++) {
        if(!entries[i].has_mac) continue;
        memcpy(out[result].mac, entries[i].mac, GAP_MAC_ADDR_SIZE);
        out[result].has_name = entries[i].has_name;
        if(entries[i].has_name) {
            strlcpy(out[result].name, entries[i].name, sizeof(out[result].name));
        } else {
            out[result].name[0] = '\0';
        }
        out[result].has_label = entries[i].has_label;
        if(entries[i].has_label) {
            strlcpy(out[result].label, entries[i].label, sizeof(out[result].label));
        } else {
            out[result].label[0] = '\0';
        }
        result++;
    }
    free(entries);
    return result;
}

bool badkb2_link_identity_known(const char* name, const uint8_t mac[GAP_MAC_ADDR_SIZE]) {
    char cfg_name[FURI_HAL_BT_ADV_NAME_LENGTH];
    uint8_t cfg_mac[GAP_MAC_ADDR_SIZE];
    if(badkb2_link_load_cfg(cfg_name, cfg_mac) && memcmp(mac, cfg_mac, GAP_MAC_ADDR_SIZE) == 0 &&
       strncmp(name, cfg_name, FURI_HAL_BT_ADV_NAME_LENGTH) == 0) {
        return true;
    }

    Badkb2Identity* saved = malloc(sizeof(Badkb2Identity) * BADKB2_LINK_MAX_ENTRIES);
    if(!saved) return false;
    size_t count = badkb2_link_load_saved_identities(saved, BADKB2_LINK_MAX_ENTRIES);
    bool known = false;
    for(size_t i = 0; i < count; i++) {
        if(memcmp(mac, saved[i].mac, GAP_MAC_ADDR_SIZE) != 0) continue;
        if(saved[i].has_name && strncmp(name, saved[i].name, FURI_HAL_BT_ADV_NAME_LENGTH) != 0) {
            continue;
        }
        known = true;
        break;
    }
    free(saved);
    return known;
}
