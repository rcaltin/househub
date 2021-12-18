# HouseHub

N-Channel video stream recorder software, especially targeted for usage on **Raspberry Pi-4** via **Esp32-Cam** modules.

- Cross platform, CMake + conan based (Currently Windows installer is missing)
- Easy to configure by a single ini file (variable chunk lengths, record directory size limit, video, output format/fps/resolution, local time for watermarking etc.).
- OpenCV + FFMpeg backend based, so supports codecs installed in your system, including h264
- Connection Recovery for dropped streams, suitable 7/24 surveliance
- Logging

## Build & Install
- Install [Ubuntu Server ](https://ubuntu.com/download/raspberry-pi "ubuntu server ")(x64 recommended) on your Raspberry Pi-4

- Install required packages
```bash
sudo apt update
sudo apt upgrade
sudo apt install libgtk2.0-dev libva-dev libvdpau-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
```

- Install [conan](https://docs.conan.io/en/latest/installation.html "conan"), [cmake](https://cmake.org/install/ "cmake") and [git](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git "git")
- Clone the repo
```bash
git clone git@github.com:rcaltin/househub.git
```

- **Edit househub.ini accordingly for your system!**

- Create build directory 
```bash
mkdir build_release
cd build_release
```

- Install conan dependency packages
```bash
conan install .. -s build_type=Release --build=missing
```

- Generate make file
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

- Build, then install
```bash
make -j4
sudo make install
```

- Enable, start and check the service daemon
```bash
sudo systemctl enable househub
sudo systemctl start househub
sudo systemctl is-active househub
```
- (Optionally) Install ftp server to reach the file system remotely
```bash
sudo apt install vsftpd
```

Hereby your Pi turned into a 7/24 recorder. Congrats!

### Roadmap
- Streaming
- Web UI for Record Playback
- Human/Motion Detection Tags for records

### Known Issues
- Rarely video chunks are missing few frames, so few seconds less files are getting generated in uniform chunks mode
- Record directory size limits grows around %10 more than the actual limit
