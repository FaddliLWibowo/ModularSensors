/*
 *VariableArray.cpp
 *This file is part of the EnviroDIY modular sensors library for Arduino
 *
 *Initial library developement done by Sara Damiano (sdamiano@stroudcenter.org).
 *
 *This file is for the variable array class.
*/

#include "VariableArray.h"

    // Initialization - cannot do this in constructor arduino has issues creating
    // instances of classes with non-empty constructors
    void VariableArray::init(int variableCount, Variable *variableList[])
    {
        MS_DBG(F("Initializing variable array with "), variableCount, F(" variables...\n"));
        _variableCount = variableCount;
        _variableList = variableList;
        MS_DBG(F("   ... Success!\n"));
    }

    // Functions to return information about the list

    // This counts and returns the number of sensors
    int VariableArray::getSensorCount(void)
    {
        int numSensors = 1;
        // Check for unique sensors
        for (int i = 0; i < _variableCount; i++)
        {
            if (isLastVarFromSensor(i)) numSensors++;
        }
        return numSensors;
    }

    // Public functions for interfacing with a list of sensors
    // This sets up all of the sensors in the list
    bool VariableArray::setupSensors(void)
    {
        bool success = true;

        MS_DBG(F("Beginning setup for sensors and variables..."));

        // First wake up all of the sensors
        MS_DBG(F("Waking sensors for setup.\n"));
        for (int i = 0; i < _variableCount; i++)
            _variableList[i]->parentSensor->wake();

        // Now run all the set-up functions
        MS_DBG(F("Running setup functions.\n"));
        for (int i = 0; i < _variableCount; i++)
        {
            // Make 5 attempts to contact the sensor before giving up
            bool sensorSuccess = false;
            int setupTries = 0;
            while(setupTries < 5 and !sensorSuccess)
            {
                // Setting up the sensors for all variables whether they are repeats
                // or not.  This means setting up some sensors multiple times, but
                // this should be OK because setup is only run in the setup, not
                // repeatedly. It is not possible to check for repeated sensors in
                // the variable list until after the sensors have all been
                // setup and then all of the variables attached.
                delay(10);
                sensorSuccess = _variableList[i]->parentSensor->setup();
                setupTries++;
            }
            if (!sensorSuccess) MS_DBG(F("   ... Set up of "), _variableList[i]->getVarCode(), F(" failed!\n"));
            else MS_DBG(F("   ... Set up of "), _variableList[i]->getVarCode(), F(" succeeded.\n"));
            success &= sensorSuccess;
        }

        // Put all the sensors back to sleep
        MS_DBG(F("Putting sensors to sleep after setup.\n"));
        for (int i = 0; i < _variableCount; i++)
            _variableList[i]->parentSensor->sleep();

        // Now attach all of the variables to their parents
        for (int i = 0; i < _variableCount; i++){
            success &= _variableList[i]->setup();
        }

        if (success)
            MS_DBG(F("   ... Success!\n"));
        return success;
    }

    // This puts sensors to sleep (ie, cuts power)
    bool VariableArray::sensorsSleep(void)
    {
        MS_DBG(F("Putting sensors to sleep.\n"));
        bool success = true;
        for (int i = 0; i < _variableCount; i++)
        {
            if (isLastVarFromSensor(i))
                success &= _variableList[i]->parentSensor->sleep();
        }
        return success;
    }

    // This wakes sensors (ie, gives power)
    bool VariableArray::sensorsWake(void)
    {
        MS_DBG(F("Waking sensors.\n"));
        bool success = true;
        for (int i = 0; i < _variableCount; i++)
        {
            if (isLastVarFromSensor(i))
                success &= _variableList[i]->parentSensor->wake();
        }
        return success;
    }

    // This function updates the values for any connected sensors.
    bool VariableArray::updateAllSensors(void)
    {
        bool success = true;
        bool update_success = true;
        for (uint8_t i = 0; i < _variableCount; i++)
        {
            if (isLastVarFromSensor(i))
            {
                // Prints for debugging
                MS_DBG(F("--- Going to update "));
                MS_DBG(_variableList[i]->parentSensor->getSensorName());
                MS_DBG(F(" ---\n"));

                update_success = _variableList[i]->parentSensor->update();

                // Prints for debugging
                MS_DBG(F("--- Updated "));
                MS_DBG(_variableList[i]->parentSensor->getSensorName());
                MS_DBG(F(" ---\n"));
            }
        }
        success &= update_success;
        return success;
    }

    // This function prints out the results for any connected sensors to a stream
    void VariableArray::printSensorData(Stream *stream)
    {
        for (int i = 0; i < _variableCount; i++)
        {
            stream->print(_variableList[i]->parentSensor->getSensorName());
            stream->print(F(" at "));
            stream->print(_variableList[i]->parentSensor->getSensorLocation());
            // stream->print(F(" has status "));
            // stream->print(Sensor::printStatus(_variableList[i]->parentSensor->getStatus()));
            // stream->print(F(" and reports "));
            stream->print(F(" reports "));
            stream->print(_variableList[i]->getVarName());
            stream->print(F(" is "));
            stream->print(_variableList[i]->getValueString());
            stream->print(F(" "));
            stream->print(_variableList[i]->getVarUnit());
            stream->println();
        }
    }

    // This generates a comma separated list of sensor values WITHOUT TIME STAMP
    String VariableArray::generateSensorDataCSV(void)
    {
        String csvString = F("");

        for (uint8_t i = 0; i < _variableCount; i++)
        {
            csvString += _variableList[i]->getValueString();
            if (i + 1 != _variableCount)
            {
                csvString += F(",");
            }
        }

        return csvString;
    }

    bool VariableArray::isLastVarFromSensor(int arrayIndex)
    {
        // Check for unique sensors
        String sensName = _variableList[arrayIndex]->parentSensor->getSensorName();
        String sensLoc = _variableList[arrayIndex]->parentSensor->getSensorLocation();
        bool unique = true;
        for (int j = arrayIndex + 1; j < _variableCount; j++)
        {
            if (sensName == _variableList[j]->parentSensor->getSensorName() &&
                sensLoc == _variableList[j]->parentSensor->getSensorLocation())
            {
                unique = false;
                break;
            }
        }
        // Prints for debugging
        // if (unique){
        //     MS_DBG(_variableList[arrayIndex]->getVarName());
        //     MS_DBG(F(" from "));
        //     MS_DBG(sensName);
        //     MS_DBG(F(" at "));
        //     MS_DBG(sensLoc);
        //     MS_DBG(F(" will be used for sensor references.\n"));
        // }
        // else{
        //     MS_DBG(_variableList[arrayIndex]->getVarName());
        //     MS_DBG(F(" from "));
        //     MS_DBG(sensName);
        //     MS_DBG(F(" at "));
        //     MS_DBG(sensLoc);
        //     MS_DBG(F(" will be ignored.\n"));
        // }
        return unique;
    }
