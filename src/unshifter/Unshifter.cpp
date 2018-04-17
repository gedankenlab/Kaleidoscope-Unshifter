// -*- c++ -*-

// TODO: decide the file names
#include "unshifter/Unshifter.h"

#include <Arduino.h>

#include KALEIDOSCOPE_HARDWARE_H
#include KALEIDOSCOPE_KEYADDR_H
#include <kaleidoscope/Key.h>
#include <kaleidoscope/Plugin.h>
#include <kaleidoscope/KeyswitchState.h>
#include <kaleidoscope/KeyArray.h>
#include <kaleidoscope/KeyswitchEvent.h>
#include <kaleidoscope/KeyswitchState.h>
#include <kaleidoscope/hid/Report.h>


namespace kaleidoscope {
namespace unshifter {

// This is our best guess as to whether the pressed key was *intended* to be interpreted
// as an explicit `shift` key by the user
bool isRealShift(Key key) {
  if (Key::Keyboard::testType(key)) {

    Key::Keyboard keyboard_key{key};

    return keyboard_key.isRealShift();
    // if (byte modifiers = keyboard_key.keycodeModifier()) {
    //   modifiers |= keyboard_key.modifierFlags();
    //   return modifiers & Key::Keyboard::mod_shift_bits;
    // }
  }

  return false;
}

// Event handler
bool Plugin::keyswitchEventHook(KeyswitchEvent& event,
                                kaleidoscope::Plugin*& caller) {
  // If Unkeys has already processed this event:
  if (checkCaller(caller))
    return true;

  // If the key toggled on, set the value based on the "true" shift state, and if
  // necessary set the shift-reverse flag
  if (event.state.toggledOn()) {
    if (const Unkey* unptr = lookupUnkey(event.key)) {
      if (shift_held_count_ == 0) {
        event.key = unptr->lower;
      } else {
        event.key = unptr->upper;
        if (Key::Keyboard::testType(event.key)) {
          Key::Keyboard keyboard_key{event.key};
          if (!(keyboard_key.modifiers() & keyboard_key.mods_mask_shift))
            reverse_shift_state_ = true;
        }
      }
    }
  }

  return true;
}


// Check timeouts and send necessary key events
bool Plugin::preReportHook(hid::keyboard::Report& keyboard_report) {
  if (reverse_shift_state_) {
    // release both shifts in report
    keyboard_report.remove(cKey::LeftShift);
    keyboard_report.remove(cKey::RightShift);
    reverse_shift_state_ = false;
  }
  return true;
}


// Update the count of "true" shift keys held
void Plugin::postReportHook(KeyswitchEvent event) {
  // I'm a bit concerned about the possibility of the count getting out of sync here, but
  // I'm going to trust it for now, and see how it plays out. If it doesn't work, we can
  // drop this hook function and just iterate through the array in the preReportHook to
  // find real shift keys.
  if (isRealShift(event.key)) {
    if (event.state.toggledOn())
      ++shift_held_count_;
    if (event.state.toggledOff())
      --shift_held_count_;
  }
}


// Check to see if the `Key` is an Unshifter key and if so, return a pointer to the
// corresponding Unkey object.
inline
const Unkey* Plugin::lookupUnkey(Key key) {
  if (UnshifterKey::testType(key)) {
    byte unkey_index = UnshifterKey(key).index();
    if (unkey_index < unkey_count_)
      return &(unkeys_[unkey_index]);
  }
  return nullptr;
}

} // namespace unshifter {
} // namespace kaleidoscope {
