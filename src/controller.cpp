#include "controller.h"
#include <cstdio>

Controller::Controller() {}
Controller::~Controller() {}


enum Button {
    A = 0, B = 1, SELECT = 2, START = 3,
    UP = 4, DOWN = 5, LEFT = 6, RIGHT = 7,
};
void Controller::set_button_state(Button btn, bool pressed) {
    if (pressed) {
        buttons_state |= (1 << btn);
    } else {
        buttons_state &= ~(1 << btn);
    }
}

void Controller::write(uint8_t data) {
    strobe_mode = (data & 0x01);
    if (strobe_mode) {
        shifter_state = buttons_state;
    }
}

uint8_t Controller::read() {
    uint8_t result = 0x00;
     
     if (strobe_mode) {
         result = (buttons_state & 0x01);
     } else {
         result = (shifter_state & 0x01);
         shifter_state >>= 1;
     }
     
     return result;
}
