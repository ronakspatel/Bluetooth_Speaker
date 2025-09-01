# Bluetooth Speaker Project
Hello, my name is Ronak Patel. This repository holds the files for a bluetooth speaker with a metadata display. For this project, I used an ESP32 to receive the bluetooth audio. Then a PCM5102A converts this data stream into analog audio. A TPA3116 based amplifier board designed by me amplifies the audio signal to suitable levels for 4-8Ω speakers at 2x50W max output. Overall, this project has primarily been an opportunity for me to learn the electrical engineering process through manual coding of the ESP32, design of the amplifier board using KiCAD, and of course much iteration. At the end of this README, I will discuss the current status of the speaker as well as some improvements I would make in the future.
# Design and Considerations
### A2DP sink and metadata callback is controlled via an ESP32-WROOM-32 and the pschatzmann library
- It is crucial to use a microcontroller that supports Bluetooth Classic as A2DP is only compatible with such devices
- The pschatzmann library offers a simple implementation of A2DP protocol allowing for bluetooth playback and metadata callback
### PCM5102A DAC converts I2S data stream into analog audio output
- While the ESP32 has an internal DAC, it is only 8-bit, yielding poor audio quality, so an external DAC will work better
### Metadata and an Audio Visualizer are displayed on a 1.8" TFT display (128px x 160px)
- Metadata such as Song title, Artist, and album title are displayed to the upper half of the display
- Audio signals are tapped at the amplifier and fed back to the ESP32 for FFT bin allocation and visualizer display
