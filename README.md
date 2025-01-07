By Willy Gardiol, provided under the GPLv3 License. https://www.gnu.org/licenses/gpl-3.0.html
Publicly available at: https://github.com/gardiol/pithermo
You can contact me at willy@gardiol.org

# PiThermo

Is a daemon written to manage a home heating system from a Pi board (Raspberry, Orange, etc...).

There is a backend that has all the logics and a web frontend to interact with it through a CGI interface and a shared memory.

There are three operating modes:
- Manual mode: you start / stop your furnaces manually, you can set a minimum and a maximum temperature and the system will act appropriately to keep the temperature in range.
- Program (or auto) mode: you choose when you want the "comfort" or the "eco" temperatures and which furnace to use, at 30min intervals. Everything then will be automatic.
- External mode: MQTT topics will be used to receive external activation (like from Home Assistant) and start / stop furnaces. In this mode, no logic will be applied, only external commands will be executed.

Except in "external" mode, there are lots of fine tunes:
- anti-ice protection
- pellet furnace missed start detection
- overtemperature protection
- a 1wire temperature sensor is required (via GPIO)
- GPIO relays are also required to operate the furnaces
- a GPIO sensor to detect if the furnace is sending hot water is also required for the wood pellet furnace to work properly

In all modes, the system support two heat generators (or "furnaces"), one wood pellet furnace and one gas furnace.

The wood pellet furnace can be turned on or off, and will be switched from "minimum" to "modulation" while the gas furnace can only be turned on or off.

MQTT is required for external mode and will be used to send current status (and 1wire temperature sensor readings) and to receive on/off commands. The topics can be configured.

## Configuration

The default config file is created upon first start, here is a full example:

``
# Set to FALSE to disable all operations
activated = true
# Mode can be "auto" "manual" "external"
mode = external
# Seconds to wait for pellet to become hot before giving up on pellet furnace
pellet_startup_delay = 2700
# Max (comfort) and Min (eco) temperature and hysteresis (°C)
max_temp = 19.6
hysteresis_max = 0.2
min_temp = 18
hysteresis_min = 0.2
# Smart temp calculates the max temperature with some additional logic
smart_temp = false
# How many degrees is too much over the Max temp. After this threesohld, turn OFF pellet in any case
excessive_overtemp_ts = 16.2
# Delta temp to fix 1wire sensor drift
temp_correction = 1.4
# Print lots of debug info in events_debug.txt file
debug = false
# Increase verbosity a lot (beware of disk fill)
debug_updates = false

# Define the GPIOs. Set to -1 to disable the GPIO 
gas_gpio_onoff = 0     # GPIO to turn on/off gas furnace
gas_gpio_status = -1   # GPIO to read furnace status (not used for gas)
gas_gpio_power = -1    # GPIO to change furnace power level (not used for gas)
pellet_gpio_onoff = 6  # GPIO to turn on/off pellet furnace
pellet_gpio_status = 7 # GPIO to read furnace status (dedicated switch sensor needed)
pellet_gpio_power = 5  # GPIO to change furnace power level (minimum to modulation)
temp_sensor_gpio = 1   # GPIO to use for communicating with 1wire temp/humidity sensor

# MQTT broker host and credentials:
mqtt_host = 10.0.0.1
mqtt_username = mqtt_username
mqtt_password = mqtt_password
external_request_topic = heating/feedback
external_usegas_topic = heating/usegas
external_usepellet_topic = heating/usepellet

# Sample templates and program (day by day)
# o = dont heat
# g = heat with gas
# p = heat with pellet
# x = heat with both (start with gas, disable gas as soon as pellet is hot)
[program]
template0 = oooooooooooooooooooooooooooooooooooooooooooooooo
template0_name =
template1 = oooooooooooooooooooooooooooooooooooooooooooooooo
template1_name =
template2 = oooooooooooooooooooooooooooooooooooooooooooooooo
template2_name =
template3 = oooooooooooooooooooooooooooooooooooooooooooooooo
template3_name =
template4 = oooooooooooooooooooooooooooooooooooooooooooooooo
template4_name =
day0 = oooooooooooxxxppppppppppppppppppppppppppoooooooo
day1 = oooooooooooxxxppppppppppppppppppppppppppoooooooo
day2 = oooooooooooxxxppppppppppppppppppppppppppoooooooo
day3 = oooooooooooxxxppppppppppppppppppppppppppoooooooo
day4 = oooooooooooxxxppppppppppppppppppppppppppoooooooo
day5 = oooooooooooxxxppppppppppppppppppppppppppoooooooo
day6 = oooooooooooxxxppppppppppppppppppppppppppoooooooo
``



## Backend

The backend is a C++ application provided under the backend folder. It should compile on any reasonably recent C++ compiler. It doesn't use any special
extension or fancy stuff, but it's most probably linux-bound as it uses some linux-specific headers. I believe it can be adapted for Windows or Mac easily,
proven that you can use WiringPi and Mosquitto libraries on such systems.

As external dependencies that you **need** to have installed:
- WiringPi [[https://github.com/WiringPi/WiringPi]] to access the GPIO of the board
- Mosquitto [[https://mosquitto.org/]] to subscribe and publish topics

Currently it's possible to compile without WiringPi setting DEMO=1 during build (es: make DEMO=1) but Mosquitto is still needed.

## CGIs

The communication between backend and frontend is done using a few CGIs which are compiled under cgi-bin folder. You need to setup your web server
to allow CGI access to /cgi-bin/. An example Apache config file is provided.

## Frontend

It's located under frontend folder. It's been developed a long time ago using a now mostly defunct framework called Dojo, sorry guys, it should
be rewritten in something else by now, but it works for me and i had no time.

Basically just point your web server document root fo the frontend folder.

An example Apache config file is provided. I choose Apache because it's much easier to manage the CGIs with Apache.
