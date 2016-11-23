///////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include <stdio.h>
#include <stdint.h>
#include "azure_c_shared_utility/macro_utils.h"
//#include <pgmspace.h>
//#include <Arduino.h>
#include "sdk/schemalib.h"
#include <time.h>
#include "AzureIoTHub.h"
#include "commentmonkey_http.h"

// The Azure IoT Hub DeviceID and Connection String
static const char DeviceId[] = "commentmonkey";
static const char connectionString[] = "HostName=wpiothub.azure-devices.net;DeviceId=commentmonkey;SharedAccessKey=UnT06vcggH+mQhtazKnuriLMminKtW2OhjmJWvmQw9M=";

// How frequently the Azure IoT Hub library should poll to receive messages (if any) as well as how often the simple_run_http() method should send messages.
unsigned int receiveFrequencySecs = 1;   /* How frequently, in seconds, should it receive messages.  Because it can poll "after 9 seconds" polls will happen effectively at ~10 seconds*/
unsigned int sendFrequencyMillis = 30000; /* How frequently, in milliseconds, should it send messages */

// The pins that the led and the monkey relay are connected to
static int ledPin = 0;
static int monkeyPin = 13;

// The onboard LED on the Huzzah Feather turns ON when when pulled LOW.  I'll define some variables here
// to clarify if I am turning the led and monkey on or off.  
static int LED_ON = 1;
static int LED_OFF = 0;
static int MONKEY_ON = 1;
static int MONKEY_OFF = 0;


// Define the Model for monkey.  This is used to serialize the data sent to and received from the
// Azure IoT Hub
BEGIN_NAMESPACE(WordPressIoT);        //The namespace of the model is "WordPressIoT"

DECLARE_MODEL(CommentMonkey,          //The model is called "CommentMonkey"
WITH_DATA(ascii_char_ptr, DeviceId),  //It has a character field for the DeviceID
WITH_DATA(int, EventTime),            //It has an int for the EventTime
WITH_ACTION(DanceMonkey)              //It has a callable action named "DanceMonkey"
);

END_NAMESPACE(WordPressIoT);

//DEFINE_ENUM_STRINGS(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES)

//  DanceMonkey! 
//  This method is called when the ESP8266 Receives a messages from the Azure IoT Hub
//  The message to invoke this method is:
//  
//  {"Name":"DanceMonkey","Parameters":""}
//
//  When the above message is received, this method is called, and the monkey dances!

EXECUTE_COMMAND_RESULT DanceMonkey(CommentMonkey* device)
{
    (void)device;

    LogInfo("\r\n----------\r\nDance Monkey! Dance!\r\n----------\r\n");

//    digitalWrite(ledPin,LED_ON);
//    digitalWrite(monkeyPin, MONKEY_ON);
    ThreadAPI_Sleep(2000);
//    digitalWrite(ledPin,LED_OFF);
//    digitalWrite(monkeyPin, MONKEY_OFF);


    return EXECUTE_COMMAND_SUCCESS;
}


// This method is the callback that is invoked after an IoT Hub message is SENT by the library.
// Here we simply log that was SENT by the device, or "Received" by the IoT Hub
// And display the confirmation status result
void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    int messageTrackingId = (intptr_t)userContextCallback;

    LogInfo("Message Id: %d Received.\r\n", messageTrackingId);

    LogInfo("Result Call Back Called! Result is: %s \r\n", ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

// This method is used to send a message using the Azure IoT Hub library
static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* buffer, size_t size)
{
    static unsigned int messageTrackingId;
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);
    if (messageHandle == NULL)
    {
        LogInfo("unable to create a new IoTHubMessage\r\n");
    }
    else
    {
        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)(uintptr_t)messageTrackingId) != IOTHUB_CLIENT_OK)
        {
            LogInfo("failed to hand over the message to IoTHubClient");
        }
        else
        {
            LogInfo("IoTHubClient accepted the message for delivery\r\n");
        }
        IoTHubMessage_Destroy(messageHandle);
    }
    free((void*)buffer);
    messageTrackingId++;
}


// This method is called by the library when a message is RECEIVED fro the Azure IoT Hub.  
// It checks the message for the "Name", and then invokes the action method with that name. 
// In our case, this is how the "DanceMonkey" method get's called when the message
//
//  {"Name":"DanceMonkey","Parameters":""}
//
// is received.
static IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubMessage(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    LogInfo("Command Recieved\r\n");
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    const unsigned char* buffer;
    size_t size;
    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        LogInfo("unable to IoTHubMessage_GetByteArray\r\n");
        result = EXECUTE_COMMAND_ERROR;
    }
    else
    {
        /*buffer is not zero terminated*/
        char* temp = malloc(size + 1);
        if (temp == NULL)
        {
            LogInfo("failed to malloc\r\n");
            result = EXECUTE_COMMAND_ERROR;
        }
        else
        {
            memcpy(temp, buffer, size);
            temp[size] = '\0';
            //  The "EXECUTE_COMMAND" method is part of the Azure IoT Hub library, and is what
            //  parses the JSON message, and looks for the "Name" of the action to invoke, 
            //  and the "Parameters" if any.  Then it actually invokes the commend.  
            EXECUTE_COMMAND_RESULT executeCommandResult = EXECUTE_COMMAND(userContextCallback, temp);
            result =
                (executeCommandResult == EXECUTE_COMMAND_ERROR) ? IOTHUBMESSAGE_ABANDONED :
                (executeCommandResult == EXECUTE_COMMAND_SUCCESS) ? IOTHUBMESSAGE_ACCEPTED :
                IOTHUBMESSAGE_REJECTED;
            free(temp);
        }
    }
    return result;
}

// This method is called by the Setup method in the commentmonkey.ino sketch.
// It kicks off the Azure IoT Hub message send loop, and receive polling.
void simplesample_http_run(void)
{

    // Start with the led and Monkey off
//    pinMode(ledPin, OUTPUT);
//    digitalWrite(ledPin, LED_OFF);
    
//    pinMode(monkeyPin, OUTPUT);
//    digitalWrite(monkeyPin, MONKEY_OFF);

    if (serializer_init(NULL) != SERIALIZER_OK)
    {
        LogInfo("Failed on serializer_init\r\n");
    }
    else
    {
        // Establish the Azure IoT Hub connection.  It uses the connectionString from above to connect as the correct DeviceID with the right Key.
        IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol);
        srand((unsigned int)time(NULL));

        if (iotHubClientHandle == NULL)
        {
            LogInfo("Failed on IoTHubClient_LL_Create\r\n");
        }
        else
        {
            //  Tell the Azure IoT Hub library how often to poll for incoming messages.  I'm defaulting this to once a second for demos.  
            //  See the receiveFrequencySecs variable declaration at the top to change this.  
            if (IoTHubClient_LL_SetOption(iotHubClientHandle, "MinimumPollingTime", &receiveFrequencySecs) != IOTHUB_CLIENT_OK)
            {
                LogInfo("failure to set option \"MinimumPollingTime\"\r\n");
            }

            //  Create an instance of the model
            CommentMonkey* myMonkey = CREATE_MODEL_INSTANCE(WordPressIoT, CommentMonkey);
            if (myMonkey == NULL)
            {
                LogInfo("Failed on CREATE_MODEL_INSTANCE\r\n");
            }
            else
            {
                //  Tell the Azure IoT Hub library what method to call when messages are received.
                if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, IoTHubMessage, myMonkey) != IOTHUB_CLIENT_OK)
                {
                    LogInfo("unable to IoTHubClient_SetMessageCallback\r\n");
                }
                else
                {
                    
                    /* wait for commands */
                    long Prev_time_ms = 999998888;   //FIXME millis();
                    char buff[11];
                    int timeNow = 0;

                    // Loop continuosly
                    while (1)
                    {
                        long Curr_time_ms = 999999888; //FIXME millis();
                        if (Curr_time_ms >= Prev_time_ms + sendFrequencyMillis)  // If it has been "sendFrequencyMillis" milliseconds (30000) by default.  
                        {
                            Prev_time_ms = Curr_time_ms;
                            
                            timeNow = (int)time(NULL);
                            
                            // Build the message data by populating the Model fields
                            // If you had other sensors, etc. attached, you could add fields for them to the model and populate the fields here.
                            myMonkey->DeviceId = DeviceId;
                            myMonkey->EventTime = timeNow;

                            LogInfo("Result: %s | %d \r\n", myMonkey->DeviceId, myMonkey->EventTime);
                        
                            unsigned char* destination;
                            size_t destinationSize;
                            
                            if (SERIALIZE(&destination, &destinationSize, myMonkey->DeviceId, myMonkey->EventTime) != IOT_AGENT_OK)
                            {
                                LogInfo("Failed to serialize\r\n");
                            }
                            else
                            {
                                // Create an Azure IoT Hub library compatible message body
                                IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(destination, destinationSize);
                                if (messageHandle == NULL)
                                {
                                    LogInfo("unable to create a new IoTHubMessage\r\n");
                                }
                                else
                                {
                                    //  Send the message asynchronously.
                                    if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)1) != IOTHUB_CLIENT_OK)
                                    {
                                        LogInfo("failed to hand over the message to IoTHubClient\r\n");
                                    }
                                    else
                                    {
                                        LogInfo("IoTHubClient accepted the message for delivery\r\n");
                                    }
    
                                    IoTHubMessage_Destroy(messageHandle);
                                }
                                free(destination);
                            }
                            
                        }

                        //  Let the library take care of stuff it has to do
                        IoTHubClient_LL_DoWork(iotHubClientHandle);

                        //Pause for 100 millis before looping
                        ThreadAPI_Sleep(100);
                    }
                }

                // Clean up the model instance
                DESTROY_MODEL_INSTANCE(myMonkey);
            }

            //Clean up the IoT Hub client
            IoTHubClient_LL_Destroy(iotHubClientHandle);
        }

        //Clean up the serializer instance
        serializer_deinit();
    }
}


