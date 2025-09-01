# Bluetooth Speaker Project
This repository holds the design files and considerations for a stereo output bluetooth speaker
# Design and Considerations
### A2DP sink and metadata callback is controlled via an ESP32-WROOM-32 and the pschatzmann library
- It is crucial to use a microcontroller that supports Bluetooth Classic as A2DP is only compatible with such devices
- The pschatzmann library offers a simple implementation of A2DP protocol allowing for bluetooth playback and metadata callback
### PCM5102A DAC converts I2S data stream into analog audio output
- While the ESP32 has an internal DAC, it is only 8-bit, yielding poor audio quality, so an external DAC will work better
### Metadata and an Audio Visualizer are displayed on a 1.8" TFT display (128px x 160px)
- Metadata such as Song title, Artist, and album title are displayed to the upper half of the display
- Audio signals are tapped at the amplifier and fed back to the ESP32 for FFT bin allocation and visualizer display
