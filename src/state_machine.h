#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>

class NfcManager;
class LedController;
class BuzzerController;
class TimerManager;

class StateMachine {
public:
    enum State {
        IDLE,
        ACTIVE,
        COUNTING,
        WARNING,
        CONFIG
    };

    StateMachine();

    void begin(NfcManager* nfc, LedController* led, BuzzerController* buzzer, TimerManager* timer);
    void update();
    void enterConfigMode();
    void exitConfigMode();
    State getCurrentState() const;

private:
    NfcManager* _nfc;
    LedController* _led;
    BuzzerController* _buzzer;
    TimerManager* _timer;
    State _currentState;

    void transitionTo(State newState);
    void handleIdleState();
    void handleActiveState();
    void handleCountingState();
    void handleWarningState();
};

#endif