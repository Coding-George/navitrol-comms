#ifndef NAVITROL_COMMS_H
#define NAVITROL_COMMS_H

#include <stdint.h>
#include "lwip/api.h"

struct netconn;

// Define action types
typedef enum
{
	ACTION_INITIALIZE,
	ACTION_GET_STATUS,
	ACTION_GO_TO_POSITION,
	ACTION_RESTART_CAN,
	ACTION_REBOOT_NAVITROL,
	ACTION_REQUEST_ERRORS,
	ACTION_PAUSE_DRIVE,
	ACTION_ABORT_DRIVE
} ActionType;

// Function prototypes
// Navitrol communication state.
// Inputs  – written by the application before calling SendDataToServer.
// Outputs – written by the library after each response is parsed.
typedef struct NavitrolState
{
	/* --- Inputs (application → library) --- */
	uint16_t         navigationTargetId;
	volatile uint8_t amrEstop;
	uint8_t          navigationManual;
	uint8_t          navigationManualForward;
	uint8_t          navigationManualReverse;
	uint8_t          navigationManualLeft;
	uint8_t          navigationManualRight;
	float            navigationManualDriveSpeed;
	uint8_t          batterySOC;
	uint16_t         batteryVoltage;
	uint16_t         batteryCurrent;

	/* --- Outputs (library → application) --- */
	uint8_t          navigationPositionResult;
	uint32_t         navigationPendingPositionId;
	uint8_t          navigationPosConfidence;
	uint8_t          navigationError;
	uint8_t          navigationErrorCount;
	uint8_t          navigationErrorCode1;
	uint8_t          navigationErrorCode2;
	uint8_t          navigationErrorCode3;
	uint8_t          navigationErrorCode4;
	uint8_t          navigationErrorCode5;
	uint8_t          navigationAbortDriveResult;
	uint8_t          navigationPauseDriveResult;
} NavitrolState_t;

// Function prototypes
err_t SendDataToServer(struct netconn *conn, ActionType action, NavitrolState_t *state);
void ConvertTo4Bytes(uint32_t value, uint8_t bytes[4]);

#endif // NAVITROL_COMMS_H
