// In src/io/machine_io.h

#ifndef MACHINE_IO_H
#define MACHINE_IO_H

#include <stdint.h>

// Structure for the emulated machine hardware (ports and shift register)
// inspired by https://web.archive.org/web/20240118230907/http://www.emulator101.com/buttons-and-ports.html
typedef struct MachineState {
    uint8_t  port1;
    uint8_t  port2;
    uint16_t shift_register;
    uint8_t  shift_offset;
} MachineState;

#endif // MACHINE_IO_H