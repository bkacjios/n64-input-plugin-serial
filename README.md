# n64-input-plugin-serial

Using this in conjunction with https://github.com/stump/n64io and an Arduino will allow you to use an N64 controller nativly with mupen64plus or Project64, along with all of its peripherals. Direct communication means rumble paks, controller paks, and transfer paks will just work without the need of some stupid expensive adapter.

![Transfer Pak working in mupen](https://i.imgur.com/da6Fy4e.png)

# Setup / Configuration

## mupen64plus

The settings for the mupen64plus version can be found in mupen64plus.cfg in `~/.config/mupen64plus/mupen64plus.cfg` for Linux and `CSIDL_APPDATA\Mupen64Plus\mupen64plus.cfg` for Windows, under `[Input-Serial]`

![mupen64plus config](https://i.imgur.com/YfLY2kS.png)

Assign your serial device to a controller, and you're done.

## Project64

The settings for the project64 version can be found under your project64 `Config` directory, usually in `C:\Program Files (x86)\Project64 2.3\Config`

The file will be named `Serial-Input.ini`

![Project64 config](https://i.imgur.com/uNqvm8N.png)

Assign your serial device to a controller, and you're done.
