/*
 *Yosemitech.cpp
 *This file is part of the EnviroDIY modular sensors library for Arduino
 *
 *Initial library developement done by Sara Damiano (sdamiano@stroudcenter.org).
 *
 *This file is for all Yosemitech sensors that communicate via Modbus and are
 *set up in the EnviroDIY Yosemitech library.
 *
 *Documentation for the Yosemitech Protocol commands and responses, along with
 *information about the various variables, can be found
 *in the EnviroDIY Yosemitech library at:
 * https://github.com/EnviroDIY/Yosemitech
*/

#include "YosemitechParent.h"

// The constructor - need the sensor type, modbus address, power pin, stream for data, and number of readings to average
YosemitechParent::YosemitechParent(byte modbusAddress, int powerPin,
                                   Stream* stream, int enablePin, int numReadings,
                                   String sensName, int numMeasurements,
                                   yosemitechModel model, int WarmUpTime_ms,
                                   int StabilizationTime_ms, int remeasurementTime_ms)
    : Sensor(powerPin, -1, sensName, numMeasurements, WarmUpTime_ms)
{
    _model = model;
    _modbusAddress = modbusAddress;
    _stream = stream;
    _enablePin = enablePin;
    _numReadings = numReadings;
    _StabilizationTime_ms = StabilizationTime_ms;
}
YosemitechParent::YosemitechParent(byte modbusAddress, int powerPin,
                                   Stream& stream, int enablePin, int numReadings,
                                   String sensName, int numMeasurements,
                                   yosemitechModel model, int WarmUpTime_ms,
                                   int StabilizationTime_ms, int remeasurementTime_ms)
    : Sensor(powerPin, -1, sensName, numMeasurements, WarmUpTime_ms)
{
    _model = model;
    _modbusAddress = modbusAddress;
    _stream = &stream;
    _enablePin = enablePin;
    _numReadings = numReadings;
    _StabilizationTime_ms = StabilizationTime_ms;
    _remeasurementTime_ms = remeasurementTime_ms;
}


SENSOR_STATUS YosemitechParent::setup(void)
{
    if (_powerPin > 0) pinMode(_powerPin, OUTPUT);
    if (_enablePin > 0) pinMode(_enablePin, OUTPUT);

    #if defined(DEBUGGING_SERIAL_OUTPUT)
        sensor.setDebugStream(&DEBUGGING_SERIAL_OUTPUT);
    #endif

    bool isSet = sensor.begin(_model, _modbusAddress, _stream, _enablePin);

    if (isSet)
    {
        MS_DBG(F("Set up Yosemitech sensor at "), getSensorLocation(), F("\n"));
        return SENSOR_READY;
    }
    else return SENSOR_ERROR;
}


// The sensor installation location on the Mayfly
String YosemitechParent::getSensorLocation(void)
{
    String sensorLocation = F("modbus_0x");
    if (_modbusAddress< 16) sensorLocation += "0";
    sensorLocation += String(_modbusAddress, HEX);
    return sensorLocation;
}

// Uses the YosemitechModbus library to communicate with the sensor
bool YosemitechParent::update()
{
    // Check if the power is on, turn it on if not
    bool wasOn = checkPowerOn();
    if(!wasOn){powerUp();}
    // Wait until the sensor is warmed up
    waitForWarmUp();

    // Clear values before starting loop
    clearValues();

    // Send the command to begin taking readings, trying up to 5 times
    bool success = false;
    int ntries = 0;
    while (!success && ntries < 5)
    {
        success = sensor.startMeasurement();
        ntries++;
    }

    if (success)
    {
        MS_DBG(F("Measurements started.\n"));

        // Wait until the sensor is ready to give readings
        delay(_StabilizationTime_ms);

        // averages x readings in this one loop
        for (int j = 0; j < _numReadings; j++)
        {
            MS_DBG(F("Taking reading #"), j, F("\n"));

            // Initialize float variables
            float parmValue, tempValue, thirdValue;
            // Get Values
            sensor.getValues(parmValue, tempValue, thirdValue);
            // Put values into the array
            // All sensors but pH and DO will have -9999 as the third value
            sensorValues[0] += parmValue;
            MS_DBG(F("Parm: "), parmValue, F("\n"));
            sensorValues[1] += tempValue;
            MS_DBG(F("Temp: "), tempValue, F("\n"));
            sensorValues[2] += thirdValue;
            MS_DBG(F("Third: "), thirdValue, F("\n"));

            if (j < _numReadings - 1)
            {
                MS_DBG(F("Waiting until sensor is ready for the next reading.\n"));
                delay(_remeasurementTime_ms);
            }
        }

        // Average over the number of readings
        MS_DBG(F("Averaging over "), _numReadings, F(" readings\n"));
        for (int i = 0; i < _numReturnedVars; i++)
        {
            sensorValues[i] /=  _numReadings;
            MS_DBG(F("Result #"), i, F(": "), sensorValues[i], F("\n"));
        }
    }


    else MS_DBG(F("Failed to start measuring!\n"));
    // Turn the power back off it it had been turned on
    if(!wasOn){powerDown();}

    // Update the registered variables with the new values
    notifyVariables();

    // Return true when finished
    return true;
}
