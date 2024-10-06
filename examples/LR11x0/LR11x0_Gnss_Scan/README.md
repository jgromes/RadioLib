LR11x0 GNSS Scan Example Sketch
===============================

This sketch exercises the LR11x0 GNSS functionality and prints information on the console.
It is intended to show how to use the GNSS functions and provide examples on how to interpret
some of the results.

It primarily loops performing GNSS scans but also checks the almanac status and asks the LR11x0
to receive almanac updates from satellite transmissions.

The sketch is configured to work as-is on a Seeed WIO Tracker 1110 dev board (red PCB)
and using the sketch on different hardware requires small adaptations.
The LR11x0 almost always uses external RF switches that are controlled through some of its
pins and which pin controls which switch is board dependent.
On the WIO Tracker board this sketch uses the Seeed Arduino core.

Currently the sketch also requires `#define RADIOLIB_GODMODE 1` in `BuildOptUser.h` to access
otherwise private functions in the LR11x0 class.
In addition, you can set the location of your device in location.h so it prints the actual
error (expect tens of km since this it's based on almanac data only).

## LoRaCloud evaluation shell script

The sketch outputs NAV lines that can be sent to LoRaCloud in order to resolve the precise
location (this is what the LR1110 is actually about).
A relatively simple bash shell script `lora-cloud.sh` reads the sketch output from the serial port,
detects the NAV lines, sends the information to LoRaCloud, and prints the result interspersed
with the sketch output.

For the script to work you need to put your creds into `lora-cloud.auth`, see the sample file.
You should also place your device's coordinates there so it can display the actual position error.

### Sample output

```
[LR1110] Starting GNSS scan ... 

RLB_DBG: GNSS scan start 'medium effort'
RLB_DBG: Starting GNSS scan with effort=1 resMask=3c nbsvMax=8
RLB_DBG: Status: 5 CMD_OK, INTR, mode=1
RLB_DBG: IRQstatus=00080000 errors=0x0000
RLB_DBG:     IRQ bit 19 set
RLB_DBG: GNSS scan done in 10946 ms
RLB_DBG: Demod status 'Time Of Week found' (3), info 03
[LR1110] Reading GNSS scan result ...
RLB_DBG: Last scan mode 'Assisted mode (Time and Assisted Position known)' (3)
RLB_DBG: Detected 7 SVs:
RLB_DBG:   SV 0: SNR 12 dB, Doppler 300 Hz
RLB_DBG:   SV 1: SNR 12 dB, Doppler 3983 Hz
RLB_DBG:   SV 2: SNR 10 dB, Doppler 3959 Hz
RLB_DBG:   SV 3: SNR 5 dB, Doppler -931 Hz
RLB_DBG:   SV 4: SNR 5 dB, Doppler 2529 Hz
RLB_DBG:   SV 5: SNR 3 dB, Doppler 1815 Hz
RLB_DBG:   SV 6: SNR 3 dB, Doppler 2698 Hz
GPS time=1410390673 407205us accuracy=11397us unix=1726355455
NAV @1410390664: 631052c00000000c94e1b0000007dcc35d722e5565bbe78df146009
LoRaCloud:
  Position: 35.12345 -119.12345 alt: 560.39
  Real error: 26m
  Claimed accuracy: 110.5m
  GPS time: 1410390665.293538 -- delta=1s
  UTC time: 1726355447.293538 -- 2024-09-14T23:10:47+00:00
Demodulated position (using 7 SVs):
  One-shot: 35.999999 -119.999999 | accuracy:39[?] xtal:1046ppb | real err: 14721m
  Filtered: 35.999999 -119.888888 | accuracy:51[?] xtal:1035ppb | real err: 13937m
Last scan timing, power, and % of power:
  GPS: total 4.3s/55.8mJ | P1:45.5% Adv:54.5% | capture:38.7% cpu:51.2% sleep:10.0%
  Beidou: total 0.0s/0.0mJ | P1:nan% Adv:nan% | capture:nan% cpu:nan% sleep:nan%
  GPS demod: total 6.7s/56.2mJ | capture:46.0% cpu:8.2% sleep:45.8%
  TOTAL: 11.0s/112.0mJ | GPS:49.8% Beidou:0.0% Demod:50.1% Init:0.0%
GPS almanac status: 'At least 1 satellite must be updated' (1)
  SVs needing update: 1 (SVs 63 25)
  Update in 364.7s, first subframe 4, 2 subframes
BDU almanac status: 'No satellite to update' (0)
  SVs needing update: 0 (SVs 0 0)
  Update in 0.0s, first subframe 0, 0 subframes
```

Almanac status with update needed:
```
GPS almanac status: 'At least 1 satellite must be updated' (1)                            
  SVs needing update: 1 (SVs 63 25)                                                       
  Update in 52.7s, first subframe 4, 2 subframes                                          
```

Almanac update:
```
[LR1110] Updating almanac in ~52666ms                                                     
RLB_DBG: GPS almanac update done in 14912 ms                                              
Outcome: 'Almanac update command launched without flash at the end' (7)                   
Almanac update done                                                                       
GPS almanac status: 'No satellite to update' (0)                                          
  SVs needing update: 0 (SVs 0 0)                                                         
  Update in 0.0s, first subframe 0, 0 subframes                                           
BDU almanac status: 'No satellite to update' (0)                                          
  SVs needing update: 0 (SVs 0 0)                                                         
  Update in 0.0s, first subframe 0, 0 subframes                                           
Last scan timing, power, and % of power:                                                  
  GPS: total 0.5s/8.6mJ | P1:0.0% Adv:100.0% | capture:43.5% cpu:56.5% sleep:0.0%         
  Beidou: total 0.0s/0.0mJ | P1:nan% Adv:nan% | capture:nan% cpu:nan% sleep:nan%          
  GPS demod: total 14.5s/261.6mJ | capture:70.2% cpu:6.2% sleep:23.7%                     
  TOTAL: 15.0s/270.4mJ | GPS:3.2% Beidou:0.0% Demod:96.7% Init:0.1%                       
```
## Open questions

The following questions have come up during the development of the sample sketch and no support
has been received from Semtech. They all relate to LR1110 firmware v4.1

### About the LR1110

- In GnssScan what does "bit change" refer to and where is it used?
- In GnssScan what is the signficance of the dopplers config in the resultMask? Is this purely a
  message length vs. accuracy tradeoff, e.e. using the doppler information as a sort of triangulation
  complement to the PR-based trilateration? Experimentally, it does not seem that omitting the
  dopplers reduces the accuracy...
- In GnssScan if the resultmask is set to 0 the position returned by GnssReadDopplerSolverRes is
  often updated, how come?
- In GnssReadDopplerSolverRes what are the units for the returned accuracy?
- In GnssReadDopplerSolverRes what is the filtered position, i.e., how is it filtered?
  It doesn't seem to be a simple function of the last N positions...
- How can the almanac data be reset in an LR1110 device so one can test the cold start and initial
  almanac acquisition phase?

### About LoRaCloud

Questions about `POST /api/v1/solve/gnss_lora_edge_singleframe`:
- The description of `gnss_capture_time` states that local server time is used if `gnss_capture_time`
  is not specified, however `gnss_capture_time_accuracy` implies that the time encoded in the binary
  payload is used. Which is it?
- What exact point in time does the returned `capture_time_gps` refer to relative to the invocation of
  the `gnssScan` command in the LR1110? To the start of the command? The end of the command? Or some other point in time?
- Related to the last question, if one measures the exact time of invoking `GnssScan` on the LR1110
  and passes that into the API then the `capture_time_gps` tends to be 1-2 seconds later. This means
  that passing a `gnss_capture_time_accuracy` lower than 3 seconds seems self-defeting because it will cause perfectly valid locations to be filtered out. Is there any real value in passing `gnss_capture_time_accuracy` lower than 15? Or any real value in passing it at all since the binary
  payload already has the time? (Maybe this parameter is primarily useful for older LR1110 firmware?)
