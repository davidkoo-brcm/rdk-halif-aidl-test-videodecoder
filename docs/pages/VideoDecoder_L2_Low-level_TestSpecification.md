# VIDEODECODER L2 Low Level Test Specification and Procedure Documentation

## Table of Contents

[VIDEODECODER L2 Low Level Test Specification and Procedure Documentation](#videodecoder-l2-low-level-test-specification-and-procedure-documentation)
- [Table of Contents](#table-of-contents)
- [Overview](#overview)
- [Acronyms, Terms and Abbreviations](#acronyms-terms-and-abbreviations)
- [Definitions](#definitions)
- [References](#references)
- [Level 2 Test Procedure](#level-2-test-procedure)

## Overview
This document describes the level 2 testing suite for the VIDEODECODER module.
### Acronyms, Terms and Abbreviations
 - `HAL` \- Hardware Abstraction Layer, may include some common components
 - `UT`  \- Unit Test(s)
 - `OEM`  \- Original Equipment Manufacture
 - `SoC`  \- System on a Chip
### Definitions
`ut-core` \- Common Testing Framework [https://github.com/rdkcentral/ut-core](https://github.com/rdkcentral/ut-core), which wraps a open-source framework that can be expanded to the requirements for future framework.

### References
- `High Level Test Specification` - [VideoDecoder_High-Level_TestSpec.md](VideoDecoder_High-Level_TestSpec.md)

## Level 2 Test Procedure
The following functions are expecting to test the module operates correctly.
### Test 1
| Title | Details |
| - |- |
| Function Name | `TestGetAndSetProperties` |
| Description   | Get Supported Video Decoders IVideoDecoderManager class by calling getVideoDecoderIds(). Verify that the returned array of IVideoDecoder.Id objects is not null and contains valid IDs. Loop through each supported video decoder ID. Get Video Decoder Interface using getVideoDecoder(videoDecoderId). Verify that the returned IVideoDecoder object is not null. Set and Verify Read-Write Properties using setProperty(property, propertyValue) and Retrieve the property value using getProperty(property). Close the decoder using close. |
| Test Group | Module : 02 |
| Test Case ID  | 001  |
| Priority  | High |

**Pre-Conditions :**
None

**Dependencies :**
None

**User Interaction :**
If user chose to run the test in interactive mode, then the test case has to be selected via console.

#### Test Procedure :

| Variation / Steps | Description | Test Data | Expected Result | Notes |
| - | - | - | - | - |
| 01| Get list of Video Decoder IDs using getVideoDecoderIds()| OUT:decoderIds:[] | status.isOk() = true | Should be successful |
| 02 | Verify that the returned array of IVideoDecoder.Id objects is not null and contains valid IDs | decoderIds.size() > 0 | status.isOk() = true | Should be successful |
| 03 | Loop through each supported video decoder ID and get Video Decoder Interface using getVideoDecoder(videoDecoderId) | IN🆔[value], OUT:decoder[] | status.isOk() = true, decoder != nullptr | Should be successful |
| 04 | Register event listener using registerEventListener() | IN:listener[0xX], OUT:registered[] | status.isOk() = true, registered = true | Should be successful |
| 05 | Open decoder using open() with codecName and secure flag | IN:codecName[codecName], IN:secure[secure], IN:controllerListener[0xX], OUT:controller[] | status.isOk() = true, controller != nullptr | Should be successful |
| 06 | Set property using setProperty() for each property | IN:property:[property], IN:setValue:[1], OUT:setResult[] | setStatus.isOk() = true, setResult = true | Should be successful |
| 07 | Get property using getProperty() for each property | IN:property:[property], OUT:getValue[] | getStatus.isOk() = true, getValue.has_value() = true | Should be successful |
| 08 | Close decoder using close() | IN:controller, OUT:closed[] | closestatus.isOk() = true, closeResult = true | Should be successful |
| 09 | Unregister event listener using unregisterEventListener() | IN:decoderEventListeners[0xX], OUT:unregistered[] | status.isOk() = true, unregistered = true | Should be successful |
| 10 | Clear controllerListeners, controllers decoderEventListeners and decoders | | | Should be successful |

### Test 2

| Title | Details |
| ----- | ------- |
| Function Name | `TestGetAndSetProperties_multipleInstance` |
| Description   | Get Supported Video Decoders IVideoDecoderManager class by calling getVideoDecoderIds(). Verify that the returned array of IVideoDecoder.Id objects is not null and contains valid IDs. Check how many decoders are supported by counting the elements in the array. Loop through each supported video decoder ID. Get Video Decoder Interface using getVideoDecoder(videoDecoderId). Verify that the returned IVideoDecoder object is not null. Open and Set Properties for Multiple Decoders. For each supported video decoder ID: Open the decoder using open(). For each read-write property in the Property enum. Set the property using setProperty(property, propertyValue). Retrieve the property value using getProperty(property). Verify that the retrieved value matches the set value. Close the decoder using close(). |
| Test Group    | Module : 02 |
| Test Case ID  | 002 |
| Priority      | High |

**Pre-Conditions :**
None

**Dependencies :**
None

**User Interaction :**
If user chose to run the test in interactive mode, then the test case has to be selected via console.

#### Test Procedure :

| Variation / Steps | Description | Test Data | Expected Result | Notes |
| - | - | - | - | - |
| 01 | Get supported video decoder IDs using getVideoDecoderIds() | None | Array of valid IVideoDecoder.Id objects | Should be successful |
| 02 | Count the number of supported decoders | None | Number of elements in the array | Should be successful |
| 03 | Loop through each supported video decoder ID | videoDecoderId = valid ID | None | Should be successful |
| 04 | Get video decoder interface using getVideoDecoder(videoDecoderId) | videoDecoderId = valid ID | IVideoDecoder object is not null | Should be successful |
| 05 | Open all the decoder instances using open(codec, secure, controllerListener, &controller) | codec = valid codec, secure = boolean, controllerListener = valid listener | Status is OK, controller is not null | Should be successful |
| 06 | Set property using setProperty(property, setValue, &setResult) | property = valid property, setValue = 1 | Status is OK, setResult is true | Should be successful |
| 07 | Get property using getProperty(property, &getValue) | property = valid property | Status is OK, getValue has value, getValue->value->getTag() = intValue | Should be successful |
| 08 | Close decoder using close() | IN:controller, OUT:closed[] | closestatus.isOk() = true, closeResult = true | Should be successful |
| 09 | Unregister event listener using unregisterEventListener() | IN:decoderEventListeners[0xX], OUT:unregistered[] | status.isOk() = true, unregistered = true | Should be successful |
| 10 | Clear controllerListeners, controllers decoderEventListeners and decoders | | | Should be successful |

### Test 3

| Title | Details |
| - | - |
| Function Name | `TestDecoderStateTransitions` |
| Description   | Get Supported Video Decoders using the IVideoDecoderManager class by calling getVideoDecoderIds(). Verify that the returned array of IVideoDecoder.Id objects is not null and contains valid IDs. Loop through each supported video decoder ID. Get the Video Decoder Interface using getVideoDecoder(videoDecoderId). Verify that the returned IVideoDecoder object is not null. Register event listeners using registerEventListener. Open the decoder using open(). Ensure each state(i.e. in the enum State) transitions(open -> start -> flush -> stop -> close) work as expected using getState(). Validate event listener notifications using onStateChanged. Close the decoder using close(). |
| Test Group    | Module : 02 |
| Test Case ID  | 003 |
| Priority      | High |

**Pre-Conditions :**
None

**Dependencies :**
None

**User Interaction :**
If user chose to run the test in interactive mode, then the test case has to be selected via console.

#### Test Procedure :

| Variation / Steps | Description | Test Data | Expected Result | Notes |
| - | - | - | - | - |
| 01 | Get list of Video Decoder IDs using `getVideoDecoderIds` | OUT: tmpIds = [] | status.isOk() = true, tmpIds not empty      | Should be successful |
| 02 | Get Video Decoder using `getVideoDecoder` for each ID    | IN: id = [current ID], OUT: decoder = [] | status.isOk() = true, decoder != nullptr    | Should be successful |
| 03 | Register event listener using `registerEventListener`    | IN: listener = [new listener], OUT: registered = [] | status.isOk() = true, registered = true     | Should be successful |
| 04 | Open decoder using `open` with codec and secure options  | IN: codec = [current codec], secure = [secure option], controllerListener = [new listener], OUT: controller = [] | status.isOk() = true, controller != nullptr | Should be successful |
| 05 | Wait for state transition from CLOSED to OPENING | State transition | State = OPENING | Should be successful |
| 06 | Wait for state transition from OPENING to READY | State transition | State = READY | Should be successful |
| 07 | Get current state using `getState` | OUT: currentState = [] | currentState = READY | Should be successful |
| 08 | Start decoder using `start` | No input | status.isOk() = true | Should be successful |
| 09 | Wait for state transition from READY to STARTING | State transition | State = STARTING | Should be successful |
| 10 | Wait for state transition from STARTING to STARTED | State transition | State = STARTED | Should be successful |
| 11 | Get current state using `getState` | OUT: currentState = [] | currentState = STARTED | Should be successful |
| 12 | Flush decoder using `flush` | IN: reset = false | status.isOk() = true | Should be successful |
| 13 | Wait for state transition from STARTED to FLUSHING | State transition | State = FLUSHING | Should be successful |
| 14 | Wait for state transition from FLUSHING to STARTED | State transition | State = STARTED | Should be successful |
| 15 | Get current state using `getState` | OUT: currentState = [] | currentState = STARTED | Should be successful |
| 16 | Stop decoder using `stop` | No input | status.isOk() = true | Should be successful |
| 17 | Wait for state transition from STARTED to STOPPING | State transition | State = STOPPING | Should be successful |
| 18 | Wait for state transition from STOPPING to READY | State transition | State = READY | Should be successful |
| 19 | Get current state using `getState` | OUT: currentState = [] | currentState = READY | Should be successful |
| 20 | Close decoder using `close` | IN: controller = [current controller], OUT: result = [] | status.isOk() = true, result = true | Should be successful |
| 21 | Wait for state transition from READY to CLOSING | State transition | State = CLOSING | Should be successful |
| 22 | Wait for state transition from CLOSING to CLOSED | State transition | State = CLOSED | Should be successful |
| 23 | Get current state using `getState` | OUT: currentState = [] | currentState = CLOSED | Should be successful |
| 24 | Tear down the test environment and clear the lists | | | Should be successful |

### Test 4

| Title | Details |
| - | - |
| Function Name | `TestDecoderStateTransitions_multipleInstance` |
| Description   | Get Supported Video Decoders using the IVideoDecoderManager class by calling getVideoDecoderIds(). Verify that the returned array of IVideoDecoder.Id objects is not null and contains valid IDs. Loop through each supported video decoder ID.<br />Get the Video Decoder Interface using getVideoDecoder(videoDecoderId). Verify that the returned IVideoDecoder object is not null. Register event listeners using registerEventListener. If platform supports multiple decoder, open all the instances of the decoders using open(). Ensure each state(i.e. in the enum State) transitions(open -> start -> flush -> stop -> close) work as expected using getState(). Validate event listener notifications using onStateChanged. Close the decoder using close(). |
| Test Group    | Module : 02 |
| Test Case ID  | 004 |
| Priority      | High |

**Pre-Conditions :**
None

**Dependencies :**
None

**User Interaction :**
If user chose to run the test in interactive mode, then the test case has to be selected via console.

#### Test Procedure :

| Variation / Steps | Description | Test Data | Expected Result | Notes |
| - | - | - | - | - |
| 01 | Get list of Video Decoder IDs using `getVideoDecoderIds` | OUT: tmpIds = [] | status.isOk() = true | Should be successful |
| 02 | Verify that the list of Video Decoder IDs is not empty | tmpIds.size() > 0 | status.isOk() = true | Should be successful |
| 03 | For each Video Decoder ID, get the Video Decoder using `getVideoDecoder` | IN: id = decoderId, OUT: decoder = [] | status.isOk() = true | Should be successful |
| 04 | Register an event listener for each Video Decoder using `registerEventListener` | IN: listener = new MyVideoDecoderEventListener(), OUT: registered = [] | status.isOk() = true, registered = true     | Should be successful |
| 05 | Open all the instances of Video Decoder with a specific codec using `open`      | IN: codec = Codec, secure = bool, controllerListener = new MyVideoDecoderControllerListener(), OUT: controller = [] | status.isOk() = true, controller != nullptr | Should be successful |
| 06 | Verify state transition from CLOSED to OPENING to READY | State transitions | State = READY | Should be successful |
| 07 | Start the Video Decoder using `start` | No input data | status.isOk() = true | Should be successful |
| 08 | Verify state transition from READY to STARTING to STARTED | State transitions | State = STARTED | Should be successful |
| 09 | Flush the Video Decoder using `flush` | IN: reset = false | status.isOk() = true | Should be successful |
| 10 | Verify state transition from STARTED to FLUSHING to STARTED | State transitions | State = STARTED | Should be successful |
| 11 | Stop the Video Decoder using `stop` | No input data | status.isOk() = true | Should be successful |
| 12 | Verify state transition from STARTED to STOPPING to READY | State transitions | State = READY | Should be successful |
| 13 | Close all the instances of  Video Decoder using `close` | IN: controller = controller, OUT: result = [] | status.isOk() = true, result = true | Should be successful |
| 14 | Verify state transition from READY to CLOSING to CLOSED | State transitions | State = CLOSED | Should be successful |