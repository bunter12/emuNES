#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <cstdint>

class Controller {
public:
    Controller();
    ~Controller();
    
    uint8_t read();
    void write(uint8_t data);
    
    enum Button {
        A = 0, B = 1, SELECT = 2, START = 3,
        UP = 4, DOWN = 5, LEFT = 6, RIGHT = 7,
    };
    void set_button_state(Button btn, bool pressed);

private:
    uint8_t buttons_state = 0;
    uint8_t shifter_state = 0;
    bool strobe_mode = false;
};

#endif //CONTROLLER_H
