#pragma once

#include <ble_profile/extra_profiles/hid_profile.h>

/** 
 * Optional arguments to pass along with profile template as 
 * FuriHalBleProfileParams for tuning profile behavior 
 **/
typedef struct {
    char name[FURI_HAL_BT_ADV_NAME_LENGTH]; /**< Full device name */
    uint8_t mac_address[6]; /**< BLE MAC address */
    bool mac_address_set; /**< Apply custom BLE MAC address */
} BleProfileHidExtParams;

/** Hid Keyboard Profile descriptor */
extern const FuriHalBleProfileTemplate* ble_profile_hid_ext;
