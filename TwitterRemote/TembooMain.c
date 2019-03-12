// TI RTOS and Simplelink includes
#include <stdio.h>
#include <stdint.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/gpio.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/gpio/GPIOCC32XX.h>
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/sysbios/BIOS.h>
#include "uart_term.h"
#include "unistd.h"

// Temboo includes
#include "temboo/Temboo.h"
#include "temboo/TembooUtil.h"
#include "temboo/http/TembooHttpSession.h"
#include "temboo/TembooTimer.h"
#include "device/DeviceGPIO.h"
#include "device/DeviceLog.h"
#include "device/DeviceGetMillis.h"

// Local project includes
#include "network_if.h"
#include "Board.h"
#include "TembooAccount.h"

// Temboo Session, holds static values for Temboo functions
TembooHttpSession session;

// Set timeout and interval values
const int CHOREO_TIMEOUT = 60;
const uint64_t choreoRunIntervalMillis = 30000;

// Array of Pin configurations for the TI driver
GPIO_PinConfig gpioPinConfigs[] = {
    // Push buttons for the launchpad
    GPIOCC32XX_GPIO_22 | GPIO_CFG_INPUT | GPIO_CFG_IN_INT_RISING,
    GPIOCC32XX_GPIO_13 | GPIO_CFG_INPUT | GPIO_CFG_IN_INT_RISING,
    // Pins used with Temboo program
};

// Declaring constants to be used when using GPIO pins
typedef enum CC3220_LAUNCHXL_GPIOName {
    CC3220_LAUNCHXL_GPIO_SW2 = 0,
    CC3220_LAUNCHXL_GPIO_SW3,
    CC3220_LAUNCHXL_GPIOCOUNT
} CC3220_LAUNCHXL_GPIOName;

GPIO_CallbackFxn gpioCallbackFunctions[] = {
    NULL,  // GPIO SW2 
    NULL   // GPIO SW3 
};

const GPIOCC32XX_Config GPIOCC32XX_config = {
    .pinConfigs = (GPIO_PinConfig *)gpioPinConfigs,
    .callbacks = (GPIO_CallbackFxn *)gpioCallbackFunctions,
    .numberOfPinConfigs = sizeof(gpioPinConfigs)/sizeof(GPIO_PinConfig),
    .numberOfCallbacks = sizeof(gpioCallbackFunctions)/sizeof(GPIO_CallbackFxn),
    .intPriority = (~0)
};


TembooError sessionSetup(TembooHttpSession* session) {
    // If you're running multiple Choreos in separate threads,
    // set this value equal to the maximum number of concurrent Choreos
    initTemboo(1);

    // Uncomment the line below for extra debug statements
    // Statements will be printed to the debug console
    // enableTembooTrace(true);

    // Initialize Temboo session with account details
    TembooError rc = initTembooHttpSession(session, 
            TEMBOO_ACCOUNT, 
            TEMBOO_APP_KEY_NAME, 
            TEMBOO_APP_KEY, 
            TEMBOO_DEVICE_TYPE);

    return rc;
}

int sendChoreoRequest(TembooHttpSession* session, TembooChoreo* choreo) {
    UART_PRINT("Running Choreo\n");

    // Set Choreo Inputs
    ChoreoInput QueryInput;
    QueryInput.name = "Query";
    QueryInput.value = "Donald J Trump";
    addChoreoInput(choreo, &QueryInput);

    ChoreoInput AccessTokenInput;
    AccessTokenInput.name = "AccessToken";
    AccessTokenInput.value = "2688594210-2WaD5jutr4tngU5mHRXJ3dMJUirT8Qvp7SChWLp";
    addChoreoInput(choreo, &AccessTokenInput);

    ChoreoInput ConsumerKeyInput;
    ConsumerKeyInput.name = "ConsumerKey";
    ConsumerKeyInput.value = "m5y5ceHu2Vz6WN31uXWDoVdkb";
    addChoreoInput(choreo, &ConsumerKeyInput);

    ChoreoInput ConsumerSecretInput;
    ConsumerSecretInput.name = "ConsumerSecret";
    ConsumerSecretInput.value = "qhPV2HVrO5WlzEAOu1YbuJVPAffUdBNAk50CajboNGX7umAyIC";
    addChoreoInput(choreo, &ConsumerSecretInput);

    ChoreoInput AccessTokenSecretInput;
    AccessTokenSecretInput.name = "AccessTokenSecret";
    AccessTokenSecretInput.value = "Xds4lv61VJi48SEPj9Ord60RCGkikBiUiT5pRlABPCNs2";
    addChoreoInput(choreo, &AccessTokenSecretInput);

    // runChoreoAsync will return a non-zero return code if it
    // can not send the Choreo request
    int rc = runChoreoAsync(choreo, session);

    if (rc != TEMBOO_SUCCESS) {
        UART_PRINT("Choreo error: %i\n\r", rc);
    }

    return rc;
}

int receiveChoreoResponse(TembooHttpSession* session, TembooChoreo* choreo) {
    // Check if Choreo is finished or has timed out
    // Poll controller in the mean time
    int rc = choreoIsFinished(choreo, session);

    if (rc != TEMBOO_SUCCESS && rc != TEMBOO_HTTP_ERROR_HTTP_ERROR) {
        return rc;
    }
    if (rc != TEMBOO_SUCCESS) {
        UART_PRINT("Choreo error: %i\n\r", rc);
    }

    // Parse Choreo response and print
    char* saveptr = NULL;
    char* nameToken = temboo_strtok_r((char*)(((TembooHttpProtocol*)(choreo->protocolData))->rxBuffer), "\x1F", &saveptr);
    char* dataToken = temboo_strtok_r(NULL, "\x1E", &saveptr);

    while (nameToken != NULL && dataToken != NULL) {
        UART_PRINT(nameToken);
        UART_PRINT(dataToken);
        nameToken = temboo_strtok_r(NULL, "\x1F", &saveptr);
        dataToken = temboo_strtok_r(NULL, "\x1E", &saveptr);
    }
    UART_PRINT("\n\r");

    return rc;
}

void* mainThread(void* args) {
    // Connect to the WiFi router
    while (0 != wifiConnect(WIFI_SSID, WPA_PASSWORD)) {
        UART_PRINT("Connection to router failed, trying again");
        sleep(2);
    }
    
    // Write the Root CA needed to connect to Temboo
    writeTembooRootCA();

    // Update the device's time to the current time
    while (updateCurrentTime("time-c.nist.gov") < 0) {
        // NIST requires a delay of at least 4 seconds between each query 
        sleep(4);
    }
    
    // Initialize Temboo session
    if (sessionSetup(&session) != TEMBOO_SUCCESS) {
        UART_PRINT("Temboo initialization failed");
        while (1);
    }

    // Setup time variable for when to run the Choreo
    uint64_t lastChoreoRunTime = deviceGetMillis() - choreoRunIntervalMillis;
    
    // Call Choreo every interval trigger
    while (1) {
        uint64_t now = deviceGetMillis();
        if (now - lastChoreoRunTime >= choreoRunIntervalMillis) {
            lastChoreoRunTime = now;
            TembooChoreo choreo;
            // Setting up send and receive buffers
            // Change size of buffers as needed
            uint8_t rxBuff[1024] = {0};
            uint8_t txBuff[1024] = {0};

            // Initialize the protocol
            TembooHttpProtocol protocol;
            initHttpProtocol(&protocol, rxBuff, sizeof(rxBuff), txBuff, sizeof(txBuff));

            const char choreoName[] = "/Library/Twitter/Search/LatestTweet";
            initHttpChoreo(&choreo, choreoName, &protocol);
            int rc = sendChoreoRequest(&session, &choreo);
            if (rc == TEMBOO_SUCCESS) {
                TembooTimer timer;
                tembooTimerStartSec(&timer, CHOREO_TIMEOUT);
                // Continue checking for a Choreo response while there is no error and the timer has not expired
                while (TEMBOO_ERROR_RESPONSE_CHECK_AGAIN == receiveChoreoResponse(&session, &choreo) && !tembooTimerExpired(&timer)) {
                    /*
                     *  Add any other code you want to run while waiting for the Choreo response within this while loop
                     */
                    usleep(100);
                }
            }
            // Stop the Choreo and close the socket
            stopTembooChoreo(&choreo);
        }        
        usleep(100);
    }
}