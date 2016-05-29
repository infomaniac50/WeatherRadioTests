
#ifndef PROPERTY_COMMAND_H
#define PROPERTY_COMMAND_H

#include <SerialCommand.h>

struct prop_t {
    uint16_t address;
    uint16_t value;
};

class PropertyCommand {
private:
    SerialCommand* _serialCommand;

    uint16_t parseArg();
    void printArg(uint16_t arg);
public:
    PropertyCommand(SerialCommand* serialCommand);
    void setProperty(void);
    void getProperty(void);
};

#endif
