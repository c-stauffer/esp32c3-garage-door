# esp32c3-garage-door

Arduino sketch for controlling garage door via an esp32c3 board.

### Usage:
The ESP32 will use a static IP (see: `my_ip`) and listen on the specifed port (see: `LISTEN_PORT`). This is currently configured as `192.168.1.45:6262`.

| URI | Method | Description | Response Code | Content |
| ----------- | ----------- | ----------- | ----------- | ----------- |
| `/ping` | GET | Determine if the esp32 is available. | 200 | software version, i.e. "1.0.0" |
| `/actuate` | GET | Activate garage door to open or close it. | 200 | "OK" |
| `/state` | GET | Gets the current state of the door, | 200 | one of: "open", "closed", "opening", "closing", "unknown" |
| `/temperature` | GET | Gets the current temperature, in Celsius. | 200 | float, degrees Celsius -OR- "--" if sensor could not be read. |
| `/humidity` | GET | Gets the current humidity, in percent. | 200 | float, percent humidity -OR- "--" if sensor could not be read. |
| `/heatindex` | GET | Gets the current calculated heat index, in percent. | 200 | float, degrees Celsius -OR- "--" if sensor could not be read. |


