#include "NavitrolComms.h"
#include "lwip/api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void ConvertTo4Bytes(uint32_t value, uint8_t bytes[4])
{
    bytes[0] = (uint8_t)(value & 0xFF);
    bytes[1] = (uint8_t)((value >> 8) & 0xFF);
    bytes[2] = (uint8_t)((value >> 16) & 0xFF);
    bytes[3] = (uint8_t)((value >> 24) & 0xFF);
}

static void ConvertFloatTo4Bytes(float value, uint8_t bytes[4])
{
    uint32_t rawValue;
    memcpy(&rawValue, &value, sizeof(rawValue));
    bytes[0] = (uint8_t)(rawValue & 0xFF);
    bytes[1] = (uint8_t)((rawValue >> 8) & 0xFF);
    bytes[2] = (uint8_t)((rawValue >> 16) & 0xFF);
    bytes[3] = (uint8_t)((rawValue >> 24) & 0xFF);
}

static uint8_t HasMinLength(uint16_t length, uint16_t required)
{
    return (length >= required) ? 1U : 0U;
}

// Parse a raw Navitrol response and write results into *state.
static void ProcessNavitrolResponse(uint8_t *recvBuffer, uint16_t length, NavitrolState_t *state)
{
    if ((recvBuffer == NULL) || (state == NULL))
    {
        return;
    }

    // Need at least bytes [0..3] before checking message IDs.
    if (!HasMinLength(length, 4U))
    {
        return;
    }

    // Check for GO_TO_POSITION Message ID
    if (recvBuffer[2] == 0x21 && recvBuffer[3] == 0x0C)
    {
        if (!HasMinLength(length, 13U))
        {
            return;
        }

        // navigationPositionResult is at byte 12
        state->navigationPendingPositionId = recvBuffer[8];
        state->navigationPositionResult = recvBuffer[12];
    }
    // Add more message ID checks for other message types if needed

    // Check for GET_STATUS Message ID
    if (recvBuffer[2] == 0x1E && recvBuffer[3] == 0x0C)
    {
        if (!HasMinLength(length, 41U))
        {
            return;
        }

        // Process GET_STATUS response
        state->navigationPosConfidence = recvBuffer[38];
        state->navigationError = recvBuffer[40];
    }

    // Check for ACTION_REQUEST_ERRORS Message ID
    if (recvBuffer[2] == 0xC9 && recvBuffer[3] == 0x00)
    {
        if (!HasMinLength(length, 9U))
        {
            return;
        }

        // Process REQUEST_ERRORS response
        state->navigationErrorCount = recvBuffer[8];

        if (state->navigationErrorCount == 1)
        {
            if (!HasMinLength(length, 13U))
            {
                return;
            }

            state->navigationErrorCode1 = recvBuffer[12];
            state->navigationErrorCode2 = 0;
            state->navigationErrorCode3 = 0;
            state->navigationErrorCode4 = 0;
            state->navigationErrorCode5 = 0;
        }
        else if (state->navigationErrorCount == 2)
        {
            if (!HasMinLength(length, 17U))
            {
                return;
            }

        	state->navigationErrorCode1 = recvBuffer[12];
        	state->navigationErrorCode2 = recvBuffer[16];
			state->navigationErrorCode3 = 0;
			state->navigationErrorCode4 = 0;
			state->navigationErrorCode5 = 0;

        }
        else if (state->navigationErrorCount == 3)
        {
            if (!HasMinLength(length, 21U))
            {
                return;
            }

        	state->navigationErrorCode1 = recvBuffer[12];
        	state->navigationErrorCode2 = recvBuffer[16];
            state->navigationErrorCode3 = recvBuffer[20];
            state->navigationErrorCode4 = 0;
            state->navigationErrorCode5 = 0;
        }
        else if (state->navigationErrorCount == 4)
        {
            if (!HasMinLength(length, 25U))
            {
                return;
            }

        	state->navigationErrorCode1 = recvBuffer[12];
        	state->navigationErrorCode2 = recvBuffer[16];
        	state->navigationErrorCode3 = recvBuffer[20];
            state->navigationErrorCode4 = recvBuffer[24];
            state->navigationErrorCode5 = 0;
        }
        else if (state->navigationErrorCount == 5)
        {
            if (!HasMinLength(length, 29U))
            {
                return;
            }

        	state->navigationErrorCode1 = recvBuffer[12];
			state->navigationErrorCode2 = recvBuffer[16];
			state->navigationErrorCode3 = recvBuffer[20];
        	state->navigationErrorCode4 = recvBuffer[24];
            state->navigationErrorCode5 = recvBuffer[28];
        }
        else
        {
        	state->navigationErrorCode1 = 0;
			state->navigationErrorCode2 = 0;
			state->navigationErrorCode3 = 0;
			state->navigationErrorCode4 = 0;
			state->navigationErrorCode5 = 0;
        }
    }

    //Check for Abort Drive Message ID
    if (recvBuffer[2] == 0xC0 && recvBuffer[3] == 0x0B)
    {
        if (!HasMinLength(length, 9U))
        {
            return;
        }

        // Process ABORT_DRIVE response
        state->navigationAbortDriveResult = recvBuffer[8];
    }

    //Check for Pause Drive Message ID
    if (recvBuffer[2] == 0xC0 && recvBuffer[3] == 0x22)
    {
	   if (!HasMinLength(length, 9U))
	   {
	       return;
	   }

	   // Process PAUSE_DRIVE response
	   state->navigationPauseDriveResult = recvBuffer[8];
    }
}

err_t SendDataToServer(struct netconn *conn, ActionType action, NavitrolState_t *state)
{
    char buffer[512];          // Buffer to store formatted strings
    uint8_t *Send_Data = NULL; // Pointer to store data to be sent
    size_t data_size = 0;      // Size of the data to be sent

    // Prepare data based on the action
    if (action == ACTION_INITIALIZE)
    {
        data_size = 24;
        Send_Data = (uint8_t *)malloc(data_size);
        if (Send_Data == NULL)
        {
            printf("Error: Failed to allocate memory for Send_Data\r\n");
            return ERR_MEM;
        }
        memset(Send_Data, 0, data_size);

        // Prepare data for initialization
        Send_Data[0] = 0x04;
        Send_Data[1] = 0x00;
        // Message ID
        Send_Data[2] = 0xE9;
        Send_Data[3] = 0x03;
        // Length
        Send_Data[4] = 0x18;
        Send_Data[5] = 0x00;
        Send_Data[6] = 0x00;
        Send_Data[7] = 0x00;
        // Data
        Send_Data[8] = 0x00;
        Send_Data[9] = 0x00;
        Send_Data[10] = 0x00;
        Send_Data[11] = 0x00;
        Send_Data[12] = 0x00;
        Send_Data[13] = 0x00;
        Send_Data[14] = 0x00;
        Send_Data[15] = 0x00;
        Send_Data[16] = 0x00;
        Send_Data[17] = 0x00;
        Send_Data[18] = 0x00;
        Send_Data[19] = 0x00;
        Send_Data[20] = 0x01;
        Send_Data[21] = 0x00;
        Send_Data[22] = 0x00;
        Send_Data[23] = 0x00;
    }
    else if (action == ACTION_GET_STATUS)
    {
        data_size = 84;
        Send_Data = (uint8_t *)malloc(data_size);
        if (Send_Data == NULL)
        {
            printf("Error: Failed to allocate memory for Send_Data\r\n");
            return ERR_MEM;
        }
        memset(Send_Data, 0, data_size);

        // Prepare data for getting status
        // Protocol Version
        Send_Data[0] = 0x04;
        Send_Data[1] = 0x00;
        // Message ID
        Send_Data[2] = 0xBA;
        Send_Data[3] = 0x0B;
        // Length
        Send_Data[4] = 0x54;
        Send_Data[5] = 0x00;
        Send_Data[6] = 0x00;
        Send_Data[7] = 0x00;
        // Status Data
        Send_Data[8] = 0x01; // 1 if OK, 0 otherwise. If status is set to 1 Navitrol can drive the AGV
        Send_Data[9] = 0x00;
        // Estop Data
        Send_Data[10] = state->amrEstop; // 1 when ESTOP active, 0 otherwise
        Send_Data[11] = 0x00;
        // Number of Sections requested
        Send_Data[12] = 0x03;
        Send_Data[13] = 0x00;
        // Section 1
        Send_Data[14] = 0x01;
        Send_Data[15] = 0x00;
        // Section 2
        Send_Data[16] = 0x02;
        Send_Data[17] = 0x00;
        // Section 8
        Send_Data[18] = 0x08;
        Send_Data[19] = 0x00;

        // Section 1 Send
        Send_Data[20] = 0x01;
        Send_Data[21] = 0x00;
        // Length
        Send_Data[22] = 0x0E;
        Send_Data[23] = 0x00;
        Send_Data[24] = 0x00;
        Send_Data[25] = 0x00;
        // Actuator Value
        Send_Data[26] = 0x00;
        Send_Data[27] = 0x00;
        Send_Data[28] = 0x00;
        Send_Data[29] = 0x00;
        // Inputs 1
        Send_Data[30] = 0x00; // Load Detected
        // Inputs 2
        Send_Data[31] = 0x00;
        // Inputs
        Send_Data[32] = 0x00;
        // Inputs 4
        Send_Data[33] = 0x00;
        // Section 2 Send
        Send_Data[34] = 0x02;
        Send_Data[35] = 0x00;
        // Lengt
        Send_Data[36] = 0x14;
        Send_Data[37] = 0x00;
        Send_Data[38] = 0x00;
        Send_Data[39] = 0x00;
        // Auto/Manual
        Send_Data[40] = state->navigationManual; // 1 Auto, 0 Manual
        Send_Data[41] = 0x00;
        // Manual Forward
        Send_Data[42] = state->navigationManualForward; // 1 when manual forward is active, 0 otherwise
        Send_Data[43] = 0x00;
        // Manual Reverse
        Send_Data[44] = state->navigationManualReverse; // 1 when manual reverse is active, 0 otherwise
        Send_Data[45] = 0x00;
        // Manual Left
        Send_Data[46] = state->navigationManualLeft; // 1 when manual left is active, 0 otherwise
        Send_Data[47] = 0x00;
        // Manual Right
        Send_Data[48] = state->navigationManualRight; // 1 when manual right is active, 0 otherwise
        Send_Data[49] = 0x00;
        // Manual Drive Speed [50-53]
        ConvertFloatTo4Bytes(state->navigationManualDriveSpeed, &Send_Data[50]);

        // Section 8 Send
        Send_Data[54] = 0x08; // Length
        Send_Data[55] = 0x00;
        // Length
        Send_Data[56] = 0x1E;
        Send_Data[57] = 0x00;
        Send_Data[58] = 0x00;
        Send_Data[59] = 0x00;
        // Battery State of Charge
        Send_Data[60] = state->batterySOC;
        Send_Data[61] = 0x00;
        // Battery Voltage
        Send_Data[62] = (uint8_t)(state->batteryVoltage & 0xFF);
        Send_Data[63] = (uint8_t)((state->batteryVoltage >> 8) & 0xFF);
        // Charge_Current
        Send_Data[64] = (uint8_t)(state->batteryCurrent & 0xFF);
        Send_Data[65] = (uint8_t)((state->batteryCurrent >> 8) & 0xFF);
        // Energy Draw
        Send_Data[66] = 0x00;
        Send_Data[67] = 0x00;
        // Time to Go
        Send_Data[68] = 0x00;
        Send_Data[69] = 0x00;
        // Battery State of Health
        Send_Data[70] = 0x00;
        Send_Data[71] = 0x00;
        // Battery Temperature
        Send_Data[72] = 0x00;
        Send_Data[73] = 0x00;
        // Battery Load Cycles
        Send_Data[74] = 0x00;
        Send_Data[75] = 0x00;
        // Battery perating Hours
        Send_Data[76] = 0x00;
        Send_Data[77] = 0x00;
        // Charger Voltage
        Send_Data[78] = 0x00;
        Send_Data[79] = 0x00;
        // Errors
        Send_Data[80] = 0x00;
        Send_Data[81] = 0x00;
        Send_Data[82] = 0x00;
        Send_Data[83] = 0x00;
    }

    else if (action == ACTION_GO_TO_POSITION)
    {
        data_size = 12;
        Send_Data = (uint8_t *)malloc(data_size);
        if (Send_Data == NULL)
        {
            printf("Error: Failed to allocate memory for Send_Data\r\n");
            return ERR_MEM;
        }
        memset(Send_Data, 0, data_size);
        // Prepare data for going to a position
        // To be implemented
        // Protocol Version
        Send_Data[0] = 0x04;
        Send_Data[1] = 0x00;
        // Message ID
        Send_Data[2] = 0xBD;
        Send_Data[3] = 0x0B;
        // Length
        Send_Data[4] = 0x0C;
        Send_Data[5] = 0x00;
        Send_Data[6] = 0x00;
        Send_Data[7] = 0x00;
        // Position ID
        uint8_t targetIdBytes[4];
        ConvertTo4Bytes(state->navigationTargetId, targetIdBytes);
        Send_Data[8] = targetIdBytes[0];
        Send_Data[9] = targetIdBytes[1];
        Send_Data[10] = targetIdBytes[2];
        Send_Data[11] = targetIdBytes[3];
    }

    else if (action == ACTION_RESTART_CAN)
    {
        data_size = 8;
        Send_Data = (uint8_t *)malloc(data_size);
        if (Send_Data == NULL)
        {
            printf("Error: Failed to allocate memory for Send_Data\r\n");
            return ERR_MEM;
        }
        memset(Send_Data, 0, data_size);
        // Prepare data for going to a position
        // To be implemented
        // Protocol Version
        Send_Data[0] = 0x04;
        Send_Data[1] = 0x00;
        // Message ID
        Send_Data[2] = 0xFD;
        Send_Data[3] = 0x03;
        // Length
        Send_Data[4] = 0x08;
        Send_Data[5] = 0x00;
        Send_Data[6] = 0x00;
        Send_Data[7] = 0x00;
    }
    
    else if (action == ACTION_ABORT_DRIVE)
    {
        data_size = 8;
        Send_Data = (uint8_t *)malloc(data_size);
        if (Send_Data == NULL)
        {
            printf("Error: Failed to allocate memory for Send_Data\r\n");
            return ERR_MEM;
        }
        memset(Send_Data, 0, data_size);
        // Prepare data for going to a position
        // To be implemented
        // Protocol Version
        Send_Data[0] = 0x04;
        Send_Data[1] = 0x00;
        // Message ID
        Send_Data[2] = 0xC0;
        Send_Data[3] = 0x0B;
        // Length
        Send_Data[4] = 0x08;
        Send_Data[5] = 0x00;
        Send_Data[6] = 0x00;
        Send_Data[7] = 0x00;
    }

    else if (action == ACTION_REBOOT_NAVITROL)
    {
        data_size = 8;
        Send_Data = (uint8_t *)malloc(data_size);
        if (Send_Data == NULL)
        {
            printf("Error: Failed to allocate memory for Send_Data\r\n");
            return ERR_MEM;
        }
        memset(Send_Data, 0, data_size);
        // Prepare data for going to a position
        // To be implemented
        // Protocol Version
        Send_Data[0] = 0x04;
        Send_Data[1] = 0x00;
        // Message ID
        Send_Data[2] = 0xF4;
        Send_Data[3] = 0x03;
        // Length
        Send_Data[4] = 0x08;
        Send_Data[5] = 0x00;
        Send_Data[6] = 0x00;
        Send_Data[7] = 0x00;
    }

    else if (action == ACTION_REQUEST_ERRORS)
    {
        data_size = 8;
        Send_Data = (uint8_t *)malloc(data_size);
        if (Send_Data == NULL)
        {
            printf("Error: Failed to allocate memory for Send_Data\r\n");
            return ERR_MEM;
        }
        memset(Send_Data, 0, data_size);
        // Prepare data for going to a position
        // To be implemented
        // Protocol Version
        Send_Data[0] = 0x04;
        Send_Data[1] = 0x00;
        // Message ID
        Send_Data[2] = 0xC1;
        Send_Data[3] = 0x0B;
        // Length
        Send_Data[4] = 0x08;
        Send_Data[5] = 0x00;
        Send_Data[6] = 0x00;
        Send_Data[7] = 0x00;
    }

    else if (action == ACTION_PAUSE_DRIVE)
    {
        data_size = 8;
        Send_Data = (uint8_t *)malloc(data_size);
        if (Send_Data == NULL)
        {
            printf("Error: Failed to allocate memory for Send_Data\r\n");
            return ERR_MEM;
        }
        memset(Send_Data, 0, data_size);
        // Prepare data for going to a position
        // To be implemented
        // Protocol Version
        Send_Data[0] = 0x04;
        Send_Data[1] = 0x00;
        // Message ID
        Send_Data[2] = 0xBF;
        Send_Data[3] = 0x0B;
        // Length
        Send_Data[4] = 0x08;
        Send_Data[5] = 0x00;
        Send_Data[6] = 0x00;
        Send_Data[7] = 0x00;
    }

    else
    {
        printf("Invalid action!\r\n");
        free(Send_Data); // Free allocated memory
        return ERR_VAL;
    }

    // Print data to be sent
    //    printf("Sending data: ");
    //    for (size_t i = 0; i < data_size; i++)
    //    {
    //        printf("%02X ", Send_Data[i]);
    //    }
    //    printf("\r\n");

    // Send data to server
    err_t status = netconn_write(conn, Send_Data, data_size, NETCONN_COPY);
    osDelay(10);

    if (status != ERR_OK)
    {
        snprintf(buffer, sizeof(buffer), "Failed to send data! Error code: %d\r\n", status);
        printf("%s", buffer);
        free(Send_Data); // Free allocated memory
        return status;
    }
    else
    {
        // For Debugging Reasons
        // snprintf(buffer, sizeof(buffer), "Data sent successfully!\r\n");
        // printf("%s", buffer);
    }

    // Receive data from the server
    struct netbuf *inbuf;
    status = netconn_recv(conn, &inbuf);
    osDelay(50);

    if (status == ERR_OK)
    {
        // Print received data to the serial terminal
        void *data;
        u16_t len;
        netbuf_data(inbuf, &data, &len);
        ProcessNavitrolResponse(data, len, state);

        // Print received data as hexadecimal values
        //        printf("Received data: ");
        //        for (int i = 0; i < len; i++)
        //        {
        //            printf("%02X ", ((uint8_t *)data)[i]);
        //        }
        //        printf("\r\n");
        //
        // Free the received buffer
        netbuf_delete(inbuf);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "Error receiving data! Error code: %d\r\n", status);
        printf("%s", buffer);
        free(Send_Data); // Free allocated memory
        return status;
    }

    free(Send_Data); // Free allocated memory
    return ERR_OK;
}
