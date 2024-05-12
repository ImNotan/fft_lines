# Windows Audio FFT Visualizer

## Overview

This Windows application allows users to visualize the Fast Fourier Transform (FFT) of computer audio in real-time. The application captures audio input from the computer using WASAPI and generates a graphical representation of its frequency components using FFT. This can be useful for visualizing the frequency content of music, speech, or any other audio playing on the system.

## Features

- Real-time FFT visualization of computer audio.
- Adjustable settings for:
    -  frequency range
    -  window size
    -  color Gradients
    -  3D-mode
    -  Circle-mode
    -  Wave-mode
- Additional serial output for led-strip [Arduino Code](https://github.com/ImNotan/fft_lines_arduino)

## Screenshots

![fft_lines](https://github.com/ImNotan/fft_lines/assets/66617967/552ea3a5-3716-414e-bc48-687abf54e4ee)
*Main application window with FFT visualization. And 3D-mode*

![fft_lines2](https://github.com/ImNotan/fft_lines/assets/66617967/b32c7eab-31f9-43f4-a829-b15fa971294f)
*Main application window with FFT visualization. And 2D-barmode*

## Video



https://github.com/ImNotan/fft_lines/assets/66617967/ef2ba0ff-73c2-4931-b475-d4ec296d0a0f



https://github.com/ImNotan/fft_lines/assets/66617967/dd493760-969b-4025-bb07-b6bdc99a37ae



https://github.com/ImNotan/fft_lines/assets/66617967/42f4f58e-a3d3-4853-8c53-45818732c51c



https://github.com/ImNotan/fft_lines/assets/66617967/e742f60e-dcfa-42d7-ac87-91bf54102aca



https://github.com/ImNotan/fft_lines/assets/66617967/a5881c2a-9bcf-417a-8fcc-29e32eadf2e5



https://github.com/ImNotan/fft_lines/assets/66617967/e95fb227-578a-46c1-9e23-4dc1470eb598



https://github.com/ImNotan/fft_lines/assets/66617967/a109ae5b-9acd-42b4-a76b-abf2490be130



https://github.com/ImNotan/fft_lines/assets/66617967/67265163-a260-49b1-a256-830093cc929f



## Installation
Download from [Release](https://github.com/ImNotan/fft_lines/releases/tag/v1.1.0)
Unzip and start fft_lines.exe


## Usage

### Start
1. Open the application.
2. Adjust FFT settings as needed.
3. Play music on you Device
If no bars show up when sound is played see Change Device

### Change Device
1. Click settings in the menu of the application
2. Click Devices
3. Select your speaker or microphone you want to use
4. !!Click Change Device!!

### Settings

Click settings in the menu of the application
- **Number of Bars (Frequency Range):** Adjust the number of Bars.
- **Size of Bars:** Set the size of Bars if volume is too low.
- **Color Scheme:** Select a Color Scheme from the Dropwdown.
- **Additional:** 3D-background, Color Gradient, Circle-mode, wave-mode
- **Serial:** Click Connect Serial if Serial didn#t connect on start

To permanently apply settings Click OK
Settings are saved for next use

## Dependencies

This application relies on the following libraries:

- [FFTW](https://www.fftw.org/): fftwf is used to calculate fft
- [Savitzky–Golay filter](https://github.com/thatchristoph/vmd-cvs-github/blob/master/plugins/signalproc/src/sgsmooth.C): Used to smooth fft output and create more beautiful peaks

## Building from Source

If you want to build the application from source, follow these steps:

1. Clone the repository: `git clone https://github.com/ImNotan/fft_lines/new/master`
2. Open the project in your preferred development environment (Visual Studio recommended for this project).
3. Build the project.

## Credits
Inspiration from https://github.com/aiXander/Realtime_PyAudio_FFT

## Contributing

Contributions are welcome! If you have ideas for improvements, new features, or bug fixes, please open an issue or submit a pull request.
