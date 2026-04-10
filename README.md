# NavitrolComms

Lightweight C library for Navitrol TCP command/response exchange over lwIP netconn.

## What this library provides

- Binary payload builders for Navitrol actions:
  - ACTION_INITIALIZE
  - ACTION_GET_STATUS
  - ACTION_GO_TO_POSITION
  - ACTION_RESTART_CAN
  - ACTION_REBOOT_NAVITROL
  - ACTION_REQUEST_ERRORS
  - ACTION_PAUSE_DRIVE
  - ACTION_ABORT_DRIVE
- Response parsing into a shared state structure (NavitrolState_t)
- Basic frame length checks while parsing incoming buffers

## Repository layout

- include/NavitrolComms.h
- src/NavitrolComms.c
- LICENSE

## Requirements

- C11-compatible compiler
- lwIP with netconn API enabled
- CMSIS/RTOS delay function available (osDelay is used in current implementation)

Include paths normally needed in STM32CubeIDE projects:

- include
- lwIP include directories used by your target project

## Public API

Header: include/NavitrolComms.h

Main types:

- ActionType
- NavitrolState_t

Main functions:

- err_t SendDataToServer(struct netconn *conn, ActionType action, NavitrolState_t *state)
- void ConvertTo4Bytes(uint32_t value, uint8_t bytes[4])

## Quick start

1. Add this library to your project include/source paths.
2. Create one persistent NavitrolState_t instance.
3. Before calling ACTION_GET_STATUS, populate state inputs (manual mode, estop, battery fields, etc.).
4. Call SendDataToServer with your connected netconn.
5. Read response outputs from NavitrolState_t.

Minimal usage example:

    #include "NavitrolComms.h"

    static NavitrolState_t navState = {0};

    // Before send
    navState.amrEstop = 0;
    navState.navigationManual = 1;
    navState.navigationManualDriveSpeed = 0.0f;
    navState.batterySOC = 80;
    navState.batteryVoltage = 520;
    navState.batteryCurrent = 15;

    // Send command + parse reply into navState
    err_t st = SendDataToServer(conn, ACTION_GET_STATUS, &navState);
    if (st == ERR_OK) {
        // Values updated by parser
        // navState.navigationPosConfidence
        // navState.navigationError
        // navState.navigationErrorCode1..5
    }

## State ownership model

NavitrolState_t is split conceptually into:

- Inputs (application to library):
  - navigation target/manual fields
  - estop
  - batterySOC/batteryVoltage/batteryCurrent
- Outputs (library to application):
  - position result/pending id
  - confidence/error fields
  - pause/abort result fields

Your application should:

- Write input fields before send
- Read output fields after successful receive/parse

## Notes and limitations

- Current transport is lwIP netconn only.
- Current implementation uses osDelay between write and read.
- Current implementation allocates send buffers dynamically with malloc/free.
- Logging uses printf for errors.

These can be abstracted later if you want a platform-agnostic version.

## Typical integration flow

- Create/connect TCP netconn
- Call SendDataToServer(conn, ACTION_GET_STATUS, &navState) periodically
- Trigger action commands as needed (GO_TO_POSITION, PAUSE, ABORT, etc.)
- Consume navState outputs in your state machine

## Versioning suggestion

Use semantic versioning tags in GitHub:

- v1.0.0 for first stable public release
- v1.0.1 for fixes
- v1.1.0 for backward-compatible features
- v2.0.0 for breaking API changes

## License

MIT. See LICENSE.
