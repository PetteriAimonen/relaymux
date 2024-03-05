// Main logic for SCPI command interface.
#pragma once

#include "scpi/scpi.h"
#include "board.h"

extern const scpi_command_t g_scpi_commands[];

#define SCPI_INPUT_BUFFER_LENGTH 128
#define SCPI_OUTPUT_BUFFER_LENGTH 128
#define SCPI_ERROR_QUEUE_SIZE 17
#define SCPI_IDN1 "devEmbedded"
#define SCPI_IDN2 "RelayMux"
#define SCPI_IDN3 board_serialnumber()
#define SCPI_IDN4 __DATE__
