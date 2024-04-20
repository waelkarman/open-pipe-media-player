## Open pipe media player [in development]

A media player built for [bunch-linux](https://waelkarman.github.io/bunch-linux-manifests/). <br>
The idea is to separate the audio from the video stream and introduce recording and stream forwarding features.<br><br>

<img src="doc/screen.gif">

- C
- gtk+3
- gstreamer
- Cmake
- POSIX thread

To be able to reproduce most of the audio/video format on debian based distros please install:
> sudo apt-get install gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav

# Build and run
please run: 
> cmake . ; make; ./open-pipe-media-player \</path/to/video\>
