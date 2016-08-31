# slideblast
Screencasts with high-res screenshots, around 4 second temporal resolution

A demo of itself using itself
https://youtu.be/x3ZMG_lHupg

## Installation and usage

Step 1. clone this repository and compile (Use Qt5)
```bash
> cd src
> qmake
> make
```

Step 2. Install shutter, ffmpeg, and imagemagick

Step 3. Important: Open shutter, and go to edit->preferences. Change file output type to .jpg with quality around 60 (reduce file size). Also check box to include the mouse cursor in the screen captures.

Step 4. Open the program

```bash
> bin/slideblast
```

Click "Start recording", then do your stuff. It will record audio and screenshots every 4 seconds, of the currently active window. Then click "Stop recording". That will be one segment. Repeat to create more segments. Finally click "Save recording". It will ask you to name the recording, and then you can browse into the newly created folder to see the movie, the movies of individual segments, and (useful) folder of the original (non-resized) screenshots.

Please provide your feedback, and let me know if you want to help make this easier to use.

