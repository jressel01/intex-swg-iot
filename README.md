# Intex Salt Water Chlorine Generators (SWG) 

For this project, original cable between display board and main board will now be from display board to ESP32 board. You'll need a new cable to connect ESP32 board to the main board. This way the ESP32 will be in the middle of every comunication. For the new cable I used a NEMA17 motor cable (the one that 3D printers use) as I had several ones at home, and one connector fits what I need, so only have to change the other connector from this cable.

## For the circuit you'll need:

- ESP32
- Relay module
- Power supply, I used 5V, but you can use 3.3V instead (wiring is different!)
- 2x Female and 1x Male XH2.54 4pin connectors (they are the most similar I've found that fits original cable)
- Level shifter as the logic from ESP32 is 3.3V and the SWG logic is 5V.
- Fuse for the power supply (optional)
- NEMA17 cable (I used this because I had several ones at home, but you can use whatever you have or you can buy)

## For the wiring, take into account the ESP32 pins:

- GPIO 19 -> SWG main board clock
- GPIO 18 -> SWG main board data
- GPIO 17 -> Display board clock
- GPIO 16 -> Display board data
- GPIO 2 -> Relay module control

PCB Layout https://github.com/jingsno/intex-swg-pcb-TM1650 (Change Relay pin from GPIO0 to GPIO2)

## RestAPI

For the API to control the system there are some endpoints that you could check in the code.
Basic API calls are the following:
- Control the machine
POST http://ip_addr:8080/api/v1/intex/swg
{
"data": {
"power": "{on|off|standby}"
}
}

- Reboot ESP
POST http://ip_addr:8080/api/v1/intex/swg/reboot
{
"data": {
"reboot": "yes"
}
}

- selfclean
POST http://ip_addr:8080/api/v1/intex/swg/self_clean
{
"data": {
"time": "{6|10|12}"
}
}

- Get current status
GET http://ip_addr:8080/api/v1/intex/swg/status

- Get current debug
GET http://ip_addr:8080/api/v1/intex/swg/debug

## OTA Update

- Enable OTA
POST http://ip_addr:8080/api/v1/intex/swg/enableota
{
"data": {
"enableota": "yes"
}
}

http://ip_addr:8080
