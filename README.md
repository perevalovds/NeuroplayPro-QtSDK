# NeuroplayPro-QtSDK

This is C++ code for working with BCI devices Neuroplay-6C and Neuroplay-8Cap, https://neuroplay.ru

![NeuroplayPro-QtSDK](https://raw.githubusercontent.com/perevalovds/NeuroplayPro-QtSDK/raw/master/doc/NPP_Qt_Graphs.png)


# Requirements

1. NeuroPlayPro installed - https://neuroplay.ru/ru/support/ and BCI device.
2. Qt5.XX installed.

# How to run

1. Build `NeuroplayPro-QtSDK` in Qt.

2. Start NeuroplayPro.

3. Run `NeuroplayPro-QtSDK`, press Connect button, and then FilteredData.

4. You may press Meditation button and meditation level will be printed at Qt's console.
Also you may send other commands to NeuroplayPro by typing them in the edit line and pressing Send button.

# Docs

- See `NeuroplayDevice::onResponse()` for variants of commands, but not all can be supported in the current SDK.

- Locally, when NeuroplayPro is running, API is accessible here: http://127.0.0.1:2336/api

- Web page: https://neuroplay.ru/api-sdk/NeuroplayPro-API.html
