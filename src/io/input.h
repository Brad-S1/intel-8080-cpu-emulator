// In src/io/input.h

#ifndef INPUT_H
#define INPUT_H

#include "machine_io.h" // Include the I/O hardware state definitions

// Initializes the I/O system (SDL, window)
int io_init(void);

// Handles all user input and system events
int io_handle_input(MachineState* machine);

// Cleans up I/O resources
void io_cleanup(void);

#endif // INPUT_H