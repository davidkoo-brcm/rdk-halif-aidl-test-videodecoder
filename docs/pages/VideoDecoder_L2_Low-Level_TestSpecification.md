# VIDEODECODER L2 Low Level Test Specification and Procedure Documentation

## Table of Contents

    -[VIDEODECODER L2 Low Level Test Specification and Procedure Documentation](#videodecoder-l2-low-level-test-specification-and-procedure-documentation)

    -[Table of Contents](#table-of-contents)
         - [Overview](#overview)
           - [Acronyms, Terms and Abbreviations](#acronyms-terms-and-abbreviations)
           - [Definitions](#definitions)
           - [References](#references)
         - [Level 2 Test Procedure](#level-2-test-procedure)

## Overview

    This document describes the level 2 testing suite for the VIDEODECODER module.

### Acronyms, Terms and Abbreviations

    -`HAL` \- Hardware Abstraction Layer, may include some common components
       - `UT`  \- Unit Test(s)
       - `OEM`  \- Original Equipment Manufacture
       - `SoC`  \- System on a Chip

### Definitions

    -`ut-core` \- Common Testing Framework [https://github.com/rdkcentral/ut-core](https://github.com/rdkcentral/ut-core), which wraps a open-source framework that can be expanded to the requirements for future framework.

### References

- `High Level Test Specification` - [VideoDecoder_High-Level_TestSpec.md](VideoDecoder_High-Level_TestSpec.md)

## Level 2 Test Procedure

    The following functions are expecting to test the module operates correctly.

### Test 1

| Title         | Details                                                                                                                                                                                                                                                                                                                                                                                                   |
| ------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Function Name | `TestGetAndSetProperties`                                                                                                                                                                                                                                                                                                                                                                               |
| Description   | Get Supported Video Decoders IVideoDecoderManager class by calling getVideoDecoderIds(). Verify that the returned array of IVideoDecoder.Id objects is not null and contains valid IDs. Loop through each supported video decoder ID. Get Video Decoder Interface using getVideoDecoder(videoDecoderId). Verify that the returned IVideoDecoder object is not null. Set and Verify Read-Write Properties. |
| Test Group    | Module : 02                                                                                                                                                                                                                                                                                                                                                                                               |
| Test Case ID  | 001                                                                                                                                                                                                                                                                                                                                                                                                       |
| Priority      | High                                                                                                                                                                                                                                                                                                                                                                                                      |

**Pre-Conditions :**
None

**Dependencies :**
None

**User Interaction :**
If user chose to run the test in interactive mode, then the test case has to be selected via console.

#### Test Procedure :

| Variation / Steps | Description                                                                                                        | Test Data                                                                                | Expected Result                                      | Notes                |
| ----------------- | ------------------------------------------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------- | ---------------------------------------------------- | -------------------- |
| 01                | Get list of Video Decoder IDs using getVideoDecoderIds()                                                           | OUT:decoderIds:[]                                                                        | status.isOk() = true                                 | Should be successful |
| 02                | Verify that the returned array of IVideoDecoder.Id objects is not null and contains valid IDs                      | decoderIds.size() > 0                                                                    | status.isOk() = true                                 | Should be successful |
| 03                | Loop through each supported video decoder ID and get Video Decoder Interface using getVideoDecoder(videoDecoderId) | IN🆔[value], OUT:decoder[]                                                               | status.isOk() = true, decoder != nullptr             | Should be successful |
| 04                | Register event listener using registerEventListener()                                                              | IN:listener[0xX], OUT:registered[]                                                       | status.isOk() = true, registered = true              | Should be successful |
| 05                | Open decoder using open() with codecName and secure flag                                                           | IN:codecName[codecName], IN:secure[secure], IN:controllerListener[0xX], OUT:controller[] | status.isOk() = true, controller != nullptr          | Should be successful |
| 06                | Set property using setProperty() for each property                                                                 | IN:property:[property], IN:setValue:[1], OUT:setResult[]                                 | setStatus.isOk() = true, setResult = true            | Should be successful |
| 07                | Get property using getProperty() for each property                                                                 | IN:property:[property], OUT:getValue[]                                                   | getStatus.isOk() = true, getValue.has_value() = true | Should be successful |
| 08                | Close decoder using close()                                                                                        | IN:controller, OUT:closed[]                                                              | closestatus.isOk() = true, closeResult = true        | Should be successful |
| 09                | Unregister event listener using unregisterEventListener()                                                          | IN:decoderEventListeners[0xX], OUT:unregistered[]                                        | status.isOk() = true, unregistered = true            | Should be successful |
| 10                | Clear controllerListeners, controllers decoderEventListeners and decoders                                      |                                                                                          |                                                      | Should be successful |

### Test 2

| Title         | Details                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| ------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Function Name | `TestGetAndSetProperties_multipleInstance`                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| Description   | Get Supported Video Decoders IVideoDecoderManager class by calling getVideoDecoderIds(). Verify that the returned array of IVideoDecoder.Id objects is not null and contains valid IDs. Check how many decoders are supported by counting the elements in the array. Loop through each supported video decoder ID. Get Video Decoder Interface using getVideoDecoder(videoDecoderId). Verify that the returned IVideoDecoder object is not null. Open and Set Properties for Multiple Decoders. |
| Test Group    | Module : 02                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| Test Case ID  | 002                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| Priority      | High                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |

**Pre-Conditions :**
None

**Dependencies :**
None

**User Interaction :**
If user chose to run the test in interactive mode, then the test case has to be selected via console.

#### Test Procedure :

| Variation / Steps | Description                                                             | Test Data                                                                  | Expected Result                                                        | Notes                |
| ----------------- | ----------------------------------------------------------------------- | -------------------------------------------------------------------------- | ---------------------------------------------------------------------- | -------------------- |
| 01                | Get supported video decoder IDs using getVideoDecoderIds()              | None                                                                       | Array of valid IVideoDecoder.Id objects                                | Should be successful |
| 02                | Count the number of supported decoders                                  | None                                                                       | Number of elements in the array                                        | Should be successful |
| 03                | Loop through each supported video decoder ID                            | videoDecoderId = valid ID                                                  | None                                                                   | Should be successful |
| 04                | Get video decoder interface using getVideoDecoder(videoDecoderId)       | videoDecoderId = valid ID                                                  | IVideoDecoder object is not null                                       | Should be successful |
| 05                | Open decoder using open(codec, secure, controllerListener, &controller) | codec = valid codec, secure = boolean, controllerListener = valid listener | Status is OK, controller is not null                                   | Should be successful |
| 06                | Set property using setProperty(property, setValue, &setResult)          | property = valid property, setValue = 1                                    | Status is OK, setResult is true                                        | Should be successful |
| 07                | Get property using getProperty(property, &getValue)                     | property = valid property                                                  | Status is OK, getValue has value, getValue->value->getTag() = intValue | Should be successful |

### Test 3

| Title         | Details                                                                                                                                                                                                                                         |
| ------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Function Name | `TestDecoderStateTransitions`                                                                                                                                                                                                                 |
| Description   | Get Supported Video Decoders using the IVideoDecoderManager class by calling getVideoDecoderIds(). Verify that the returned array of IVideoDecoder.Id objects is not null and contains valid IDs. Loop through each supported video decoder ID. |
| Test Group    | Module : 02                                                                                                                                                                                                                                     |
| Test Case ID  | 003                                                                                                                                                                                                                                             |
| Priority      | High                                                                                                                                                                                                                                            |

**Pre-Conditions :**
None

**Dependencies :**
None

**User Interaction :**
If user chose to run the test in interactive mode, then the test case has to be selected via console.

#### Test Procedure :

| Variation / Steps | Description                                                | Test Data                                                                                            | Expected Result                             | Notes                |
| ----------------- | ---------------------------------------------------------- | ---------------------------------------------------------------------------------------------------- | ------------------------------------------- | -------------------- |
| 01                | Get list of Video Decoder IDs using `getVideoDecoderIds` | decoderIds = empty vector                                                                            | status.isOk() = true, decoderIds not empty  | Should be successful |
| 02                | Retrieve each Video Decoder using `getVideoDecoder`      | id = valid ID, decoder = nullptr                                                                     | status.isOk() = true, decoder != nullptr    | Should be successful |
| 03                | Register event listener using `registerEventListener`    | listener = valid listener, registered = false                                                        | registered = true                           | Should be successful |
| 04                | Open decoder with codec using `open`                     | codecName = valid codec, secure = boolean, controllerListener = valid listener, controller = nullptr | status.isOk() = true, controller != nullptr | Should be successful |
| 05                | Verify state transition from CLOSED to OPENING             | oldState = CLOSED, newState = OPENING                                                                | foundClosedToOpening = true                 | Should be successful |
| 06                | Wait for state transition from OPENING to READY            | state = READY                                                                                        | listener.waitForState() = true              | Should be successful |
| 07                | Get current state using `getState`                       | currentState = valid state                                                                           | currentState = READY                        | Should be successful |
| 08                | Close decoder using `close`                              | controller = valid controller, result = false                                                        | result = true                               | Should be successful |
| 09                | Verify state transition from READY to CLOSING              | oldState = READY, newState = CLOSING                                                                 | foundReadyToClosing = true                  | Should be successful |
| 10                | Wait for state transition from CLOSING to READY            | state = READY                                                                                        | listener.waitForState() = true              | Should be successful |
| 11                | Get current state using `getState`                       | currentState = valid state                                                                           | currentState = READY                        | Should be successful |
| 12                | Close decoder using `close`                              | controller = valid controller, closeResult = false                                                   | closeResult = true                          | Should be successful |
