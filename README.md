# NeuroplayPro-QtSDK

# Requirements

1. NeuroPlayPro installed - https://neuroplay.ru/ru/support/ and BCI device.
2. Qt5 installed.

# Starting Guide

1. Build `NeuroplayPro-QtSDK` in Qt.

2. Start NeuroplayPro.

3. Run `NeuroplayPro-QtSDK`, press buttons Connect and FilteredData.

You will see graphs with 625 measurements.
Pressing FilteredData button again gives shifts of the graphs - so this method is useful for spectrum analysis.

4. You may press Meditation button and meditation level will be printed at Qt's console.
Also you may send other commands to NeuroplayPro by typing them in the edit line and pressing Send button.

# API docs

- See `NeuroplayDevice::onResponse()` for variants of commands, but not all can be supported in the current SDK.

- Locally, when NeuroplayPro is running: http://127.0.0.1:2336/api

- Web page: https://neuroplay.ru/api-sdk/NeuroplayPro-API.html?r=0.35787390205751457
