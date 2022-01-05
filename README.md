# HouseHub

N-Channel video stream recorder software, especially targeted for usage on **Raspberry Pi-4** via **Esp32-Cam** modules.

- Cross platform, CMake + conan based (Currently Windows installer is missing)
- Easy to configure by a single ini file (variable chunk lengths, record directory size limit, video, output format/fps/resolution, local time for watermarking etc.).
- OpenCV + FFMpeg backend based, so supports codecs installed in your system, including h264
- Connection Recovery for dropped streams, suitable 7/24 surveliance
- Logging

## Roadmap
- Streaming
- Web UI for Record Playback
- Human/Motion Detection Tags for records

## Known Issues
- Record directory size limits grows around %10 more than the actual limit