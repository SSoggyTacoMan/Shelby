# Shelby 

Shelby is a firmware designed specifically for the Sprig by hackclub that turns it into a mini desk companion

[Watch the demo video!](https://github.com/user-attachments/assets/a62c12d5-ae85-4c19-9b54-42e6baf9ffeb)
---

# What actually is this??

Shelby turns your Sprig from a gaming console into a device with fully working operating system that has all the important things on your finger tips! Your github contributions, they are there, your hackatime stats they are also there! and many other things on that small device
---

# Features

- **Dynamic background clock** - It syncs with your timezone which you put in `secrets.py` and changes the background accordingly every minute
- **Github Contributions Graph** - You can see your github contributions of last 18 weeks just by entering your username and github PAT (Personal Access Token) into the `secrets.py`
- **Device Monitoring** - Monitor your device and check if it has been eating more ram or if the wifi is working correctly or is it frying
- **Tasks List** - You can also add some tasks via `tasks.json` (detailed working below) and don't forget them!
- **Hackatime Stats** - See the time you spent on coding today and the total time spent in the last 7 days and list your best featured projects with their time logged also!!
- **Overclocking** - Yes you read that right I actually overclocked that tiny RP2040 chip to 250 MHz, which is changable through settings
- **Volume Control** - You can control the volume of the device just going into the settings and changing it
- *12h/24h clock mode* - Switch between 12 hour format and 24 hour format for your clock!
- **Music** - Listen to certain music! You can add your own music in `songs.py` and listen to them just remember it shouldn't be too long as it will cook the board. By default, there are 4 songs already uploaded
---

# Hardware
Just get a Sprig from Hackclub which they send out for free, apply for one [here](https://sprig.hackclub.com)
---

# Setup

**1. Download the OS**
Download the latest `.zip` release from the [Releases Page](https://github.com/Swamstick911/Shelby/releases/tag/v1.0.0). 
Extract the `.zip` folder, open your terminal, and `cd` into that extracted folder.

2. Create your `secrets.py` file
Create a new file in the folder and name it `secrets.py` and copy the `secrets.py.example` and fill in all your credentials

3. Installing all the dependencies
```bash
pip install
```

4. Upload to your sprig
```bash
mpremote connect list
```
Then whatever your COM number for the sprig is example COM7, enter this
```bash
mpremote connect COM7 cp -r . :
```
And you're done! Enjoy your Sprig running Shelby!!

# Controls
| Button        | Action                             |
| ------------- | ---------------------------------- |
| W/A/S/D       | Navigation through menu            |
| J             | Go back                            |
| I             | Select / confirm                   |
| L             | Open Menu when at clock screen     |

Other app controls are written in the respecitve screens

---

# Native C++ (Pico SDK) - WIP

This repo currently contains the MicroPython version of Shelby. A native rewrite is being started using the Raspberry Pi Pico C/C++ SDK.

## Build (Step 1 skeleton)

1. Install/clone `pico-sdk` somewhere and export `PICO_SDK_PATH`.
2. (Optional) Create a local `secrets.hpp` (ignored by git) with:
   - `#define WIFI_SSID "..."`  
   - `#define WIFI_PASSWORD "..."`
3. Build:

```bash
mkdir -p build
cmake -S . -B build
cmake --build build
```
