LR11x0 GNSS Scan Example Sketch
===============================

This sketch exercises the LR11x0 GNSS functionality and prints information on the console.
It is intended to show how to use the GNSS functions and provide examples on how to interpret
some of the results.

It primarily loops performing GNSS scans but also checks the almanac status and asks the LR11x0
to receive almanac updates from satellite transmissions.

The sketch is configured to work as-is on a Seeed WIO Tracker 1110 dev board (red in color)
and using the sketch on different hardware requires small adaptations.
The LR11x0 almost always uses external RF switches that are controlled through some of its
pins and which pin controls which switch is board dependent.
On the WIO Tracker board this sketch uses the Seeed Arduino core.

Currently the sketch also requires `#define RADIOLIB_GODMODE 1` in `BuildOptUser.h` to access
otherwise private functions in the LR11x0 class.

### Sample output

```
[LR1110] Starting GNSS scan ... 
RLB_DBG: GNSS scan start 'medium effort'
RLB_DBG: Starting GNSS scan with effort=1 resMask=3c nbsvMax=8
RLB_DBG: GNSS scan done in 12598 ms
RLB_DBG: Demod status 'Word sync found' (2), info 01
[LR1110] Reading GNSS scan result ...
RLB_DBG: Last scan mode 'Assisted mode (Time and Assisted Position known)' (3)
RLB_DBG: Detected 8 SVs:
RLB_DBG:   SV 0: SNR 13 dB, Doppler -1221 Hz
RLB_DBG:   SV 1: SNR 10 dB, Doppler 2049 Hz
RLB_DBG:   SV 2: SNR 10 dB, Doppler 4481 Hz
RLB_DBG:   SV 3: SNR 7 dB, Doppler 3997 Hz
RLB_DBG:   SV 4: SNR 6 dB, Doppler 2906 Hz
RLB_DBG:   SV 5: SNR 5 dB, Doppler -1196 Hz
RLB_DBG:   SV 6: SNR 4 dB, Doppler 4275 Hz
RLB_DBG:   SV 7: SNR 2 dB, Doppler -1046 Hz
GPS time=1726180161 180720us accuracy=242256us unix=2042144943
Demodulated position (using 8 SVs):
  One-shot: 34.760742 -119.091797 | accuracy:42[?] xtal:974ppb | real err: 72538m
  Filtered: 34.497070 -119.707031 | accuracy:29[?] xtal:981ppb | real err: 10189m
Last scan timing, power, and % of power:
  GPS: total 4.3s/56.6mJ | P1:44.1% Adv:55.9% | capture:38.2% cpu:52.6% sleep:9.2%
  Beidou: total 0.0s/0.0mJ | P1:nan% Adv:nan% | capture:nan% cpu:nan% sleep:nan%
  GPS demod: total 8.3s/78.8mJ | capture:47.3% cpu:6.6% sleep:46.0%
  TOTAL: 12.6s/135.5mJ | GPS:41.8% Beidou:0.0% Demod:58.2% Init:0.0%
GPS almanac status: 'No satellite to update' (0)
  SVs needing update: 0 (SVs 0 0)
  Update in 0.0s, first subframe 0, 0 subframes
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
