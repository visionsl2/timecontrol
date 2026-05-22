#include "state_machine.h"
#include "nfc_manager.h"
#include "led_controller.h"
#include "buzzer_controller.h"
#include "timer_manager.h"
#include "config.h"

StateMachine::StateMachine()
    : _nfc(nullptr), _led(nullptr), _buzzer(nullptr), _timer(nullptr), _currentState(IDLE) {
}

void StateMachine::begin(NfcManager* nfc, LedController* led, BuzzerController* buzzer, TimerManager* timer) {
    _nfc = nfc;
    _led = led;
    _buzzer = buzzer;
    _timer = timer;

    // Initial state
    _led->setIdle();
}

void StateMachine::update() {
    switch (_currentState) {
        case IDLE:
            handleIdleState();
            break;
        case ACTIVE:
            handleActiveState();
            break;
        case COUNTING:
            handleCountingState();
            break;
        case WARNING:
            handleWarningState();
            break;
        case CONFIG:
            // Config mode handled by web interface
            break;
    }
}

void StateMachine::enterConfigMode() {
    if (_currentState != CONFIG) {
        transitionTo(CONFIG);
    }
}

void StateMachine::exitConfigMode() {
    if (_currentState == CONFIG) {
        transitionTo(IDLE);
    }
}

StateMachine::State StateMachine::getCurrentState() const {
    return _currentState;
}

void StateMachine::transitionTo(State newState) {
    if (_currentState == newState) {
        return;
    }

    _currentState = newState;

    switch (newState) {
        case IDLE:
            _led->setIdle();
            _buzzer->stop();
            _timer->reset();
            break;
        case ACTIVE:
            _led->setActive();
            _buzzer->stop();
            _timer->reset();
            break;
        case COUNTING:
            _led->setCounting();
            _buzzer->stop();
            _timer->startCounting();
            break;
        case WARNING:
            _led->setWarning();
            _buzzer->startAlarm();
            break;
        case CONFIG:
            _led->setConfig();
            _buzzer->stop();
            _timer->reset();
            break;
    }
}

void StateMachine::handleIdleState() {
    String uid = _nfc->detectTag();
    if (uid.length() > 0 && _nfc->isTagRegistered(uid)) {
        transitionTo(ACTIVE);
    }
}

void StateMachine::handleActiveState() {
    String uid = _nfc->detectTag();
    if (uid.length() == 0 || !_nfc->isTagRegistered(uid)) {
        transitionTo(COUNTING);
    }
}

void StateMachine::handleCountingState() {
    if (_timer->isTimeout()) {
        transitionTo(WARNING);
        return;
    }

    String uid = _nfc->detectTag();
    if (uid.length() > 0 && _nfc->isTagRegistered(uid)) {
        transitionTo(ACTIVE);
    }
}

void StateMachine::handleWarningState() {
    String uid = _nfc->detectTag();
    if (uid.length() > 0 && _nfc->isTagRegistered(uid)) {
        _buzzer->stop();
        transitionTo(IDLE);
    }
}