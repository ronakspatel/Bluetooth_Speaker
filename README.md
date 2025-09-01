# Bluetooth Speaker Project
This repository holds the design files and considerations for a stereo output bluetooth speaker
# Design and Considerations
### A2DP sink and metadata callback is controlled via an ESP32-WROOM-32 and the pschatzmann library
- It is crucial to use a microcontroller that supports Bluetooth Classic as A2DP is only compatible with such devices
- The pschatzmann library offers a simple implementation of A2DP protocol allowing for bluetooth playback and metadata callback
