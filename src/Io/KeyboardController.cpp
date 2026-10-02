#include "KeyboardController.h"

#include "Library/Platform/Interface/PlatformEvents.h"

static constexpr float AXIS_PRESS_THRESHOLD = 0.5f; // Same threshold as GameWindowHandler::gamepadAxisEvent.

/**
 * @param axis                          Axis key, as reported for positive axis values.
 * @return                              Key for the opposite direction of the same axis, or `KEY_NONE` for one-sided
 *                                      axes like triggers.
 */
static PlatformKey negativeAxisKey(PlatformKey axis) {
    switch (axis) {
    case PlatformKey::KEY_GAMEPAD_LEFTSTICK_RIGHT:  return PlatformKey::KEY_GAMEPAD_LEFTSTICK_LEFT;
    case PlatformKey::KEY_GAMEPAD_LEFTSTICK_DOWN:   return PlatformKey::KEY_GAMEPAD_LEFTSTICK_UP;
    case PlatformKey::KEY_GAMEPAD_RIGHTSTICK_RIGHT: return PlatformKey::KEY_GAMEPAD_RIGHTSTICK_LEFT;
    case PlatformKey::KEY_GAMEPAD_RIGHTSTICK_DOWN:  return PlatformKey::KEY_GAMEPAD_RIGHTSTICK_UP;
    default:                                        return PlatformKey::KEY_NONE;
    }
}

KeyboardController::KeyboardController() : PlatformEventFilter({EVENT_KEY_PRESS, EVENT_KEY_RELEASE, EVENT_GAMEPAD_KEY_PRESS,
                                                                 EVENT_GAMEPAD_KEY_RELEASE, EVENT_GAMEPAD_AXIS}) {}

bool KeyboardController::isKeyPressedThisFrame(PlatformKey key) const {
    if (key == PlatformKey::KEY_NONE)
        return false;

    return _isKeyPressedThisFrame[key];
}

bool KeyboardController::isKeyDownThisFrame(PlatformKey key) const {
    if (key == PlatformKey::KEY_NONE)
        return false;

    return _isKeyDown[key] || _isKeyPressedThisFrame[key];
}

bool KeyboardController::keyPressEvent(const PlatformKeyEvent *event) {
    setKeyDown(event->key, true);
    return false;
}

bool KeyboardController::keyReleaseEvent(const PlatformKeyEvent *event) {
    setKeyDown(event->key, false);
    return false;
}

bool KeyboardController::gamepadKeyPressEvent(const PlatformGamepadKeyEvent *event) {
    setKeyDown(event->key, true);
    return false;
}

bool KeyboardController::gamepadKeyReleaseEvent(const PlatformGamepadKeyEvent *event) {
    setKeyDown(event->key, false);
    return false;
}

bool KeyboardController::gamepadAxisEvent(const PlatformGamepadAxisEvent *event) {
    setKeyDown(event->axis, event->value >= AXIS_PRESS_THRESHOLD);

    PlatformKey negativeKey = negativeAxisKey(event->axis);
    if (negativeKey != PlatformKey::KEY_NONE)
        setKeyDown(negativeKey, event->value <= -AXIS_PRESS_THRESHOLD);
    return false;
}

void KeyboardController::processMessages(PlatformEventHandler *eventHandler) {
    _isKeyPressedThisFrame.fill(false);
    ProxyEventLoop::processMessages(eventHandler);
}

void KeyboardController::reset() {
    _isKeyDown.fill(false);
    _isKeyPressedThisFrame.fill(false);
}

void KeyboardController::setKeyDown(PlatformKey key, bool down) {
    if (key == PlatformKey::KEY_NONE)
        return;

    if (down && !_isKeyDown[key]) // Repeated presses of a held key are auto repeat.
        _isKeyPressedThisFrame[key] = true;
    _isKeyDown[key] = down;
}
