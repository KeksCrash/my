#pragma once

#include <gui/view.h>
#include "hid_keynote.h"

typedef struct Hid Hid;
typedef struct HidKeynoteKeyPicker HidKeynoteKeyPicker;

typedef void (*HidKeynoteKeyPickerCallback)(HidKeynoteAction action, void* context);

HidKeynoteKeyPicker* hid_keynote_key_picker_alloc(Hid* hid);
void hid_keynote_key_picker_free(HidKeynoteKeyPicker* picker);
View* hid_keynote_key_picker_get_view(HidKeynoteKeyPicker* picker);
void hid_keynote_key_picker_set_callback(
    HidKeynoteKeyPicker* picker,
    HidKeynoteKeyPickerCallback callback,
    void* context);
void hid_keynote_key_picker_set_selected(HidKeynoteKeyPicker* picker, HidKeynoteAction action);
bool hid_keynote_key_picker_action_on_keyboard(HidKeynoteAction action);
