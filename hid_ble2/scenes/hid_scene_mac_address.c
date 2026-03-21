/*
 * hid_scene_mac_address.c
 *
 * This file handles the input of MAC addresses via a byte editor.
 */

#include <stdio.h>
#include <stdint.h>

// Function to convert a string to a MAC address.
void parse_mac_address(const char *mac_str, uint8_t *mac_out) {
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac_out[0], &mac_out[1], &mac_out[2],
           &mac_out[3], &mac_out[4], &mac_out[5]);
}

// Example usage
int main() {
    const char *mac_string = "00:1A:2B:3C:4D:5E";
    uint8_t mac_address[6];
    parse_mac_address(mac_string, mac_address);

    printf("Parsed MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", 
           mac_address[0], mac_address[1], mac_address[2],
           mac_address[3], mac_address[4], mac_address[5]);
    return 0;
}