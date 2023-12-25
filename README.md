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
- Additional serial output for led-strip

## Screenshots

![fft_lines](https://github.com/ImNotan/fft_lines/assets/66617967/552ea3a5-3716-414e-bc48-687abf54e4ee)
*Main application window with FFT visualization. And 3D-mode*

![fft_lines2](https://github.com/ImNotan/fft_lines/assets/66617967/b32c7eab-31f9-43f4-a829-b15fa971294f)
*Main application window with FFT visualization. And 2D-barmode*

## Usage

1. Open the application.
2. Adjust FFT settings as needed.
3. Press the "Start" button to begin capturing and visualizing audio.
4. Explore and analyze the real-time FFT visualization.
5. To stop the visualization, press the "Stop" button.
6. Save or export the FFT visualization if desired.


If no bars show up when sound is played:
1. Click settings in the menu of the application
2. Click Devices
3. Select your speaker or microphone
4. !!Click Change Device!!

## Settings

Click settings in the menu of the application
- **Number of Bars (Frequency Range):** Adjust the number of Bars.
- **Size of Bars:** Set the size of Bars if volume is too low.
- **Color Scheme:** Select a Color Scheme from the Dropwdown.
- **Additional:** Border, 3D-background, Color Gradient
- **Serial:** Click Connect Serial if Serial didn#t connect on start

To permanently apply settings Click OK

## Dependencies

This application relies on the following libraries:

- [FFTW](https://www.fftw.org/): fftwf is used to calculate fft
- [Savitzky–Golay filter](https://github.com/thatchristoph/vmd-cvs-github/blob/master/plugins/signalproc/src/sgsmooth.C): Used to smooth fft output and create more beautiful peaks

## Building from Source

If you want to build the application from source, follow these steps:

1. Clone the repository: `git clone https://github.com/ImNotan/fft_lines/new/master`
2. Open the project in your preferred development environment (Visual Studio recommended for this project).
3. Build the project.

## Contributing

Contributions are welcome! If you have ideas for improvements, new features, or bug fixes, please open an issue or submit a pull request.
