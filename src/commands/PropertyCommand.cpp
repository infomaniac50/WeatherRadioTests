
#include "PropertyCommand.h"
#include <SI4707.h>

// private
uint16_t PropertyCommand::parseArg() {
    char* arg;

    arg = _serialCommand->next();

    if (arg == NULL) {
        return 0;
    }

    return strtoul(arg, 0, 16);
}

void PropertyCommand::printArg(uint16_t arg) {
    Serial.print("0x");
    Serial.print(arg, HEX);
    Serial.write(" 0b");
    Serial.print(arg, BIN);
    Serial.write(' ');
    Serial.println(arg);
}

// public
PropertyCommand::PropertyCommand(SerialCommand* serialCommand) {
    _serialCommand = serialCommand;
}

void PropertyCommand::setProperty(void) {
    prop_t property;

    property.address = parseArg();

    Serial.print("Address: ");
    printArg(property.address);

    property.value = parseArg();

    Serial.print("Value: ");
    printArg(property.value);

    Radio.setProperty(property.address, property.value);
}

void PropertyCommand::getProperty(void) {
    prop_t property;

    property.address = parseArg();

    Serial.print("Address: ");
    printArg(property.address);

    property.value = Radio.getProperty(property.address);

    Serial.print("Value: ");
    printArg(property.value);
}
