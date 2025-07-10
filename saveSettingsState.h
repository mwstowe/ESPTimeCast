#ifndef SAVE_SETTINGS_STATE_H
#define SAVE_SETTINGS_STATE_H

// Enum to track the state of the settings save operation
enum SaveSettingsState {
  IDLE,
  PREPARE,
  WRITE_SETTINGS,
  FINALIZE
};

// Global variables for state management
extern SaveSettingsState saveSettingsState;
extern bool settingsSavePending;

#endif // SAVE_SETTINGS_STATE_H
