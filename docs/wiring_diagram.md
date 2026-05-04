# PCM5102A Wiring Reference — Raspberry Pi 4B

## PCM5102A Pin Description

| Pin | Name | Type | Description |
|-----|------|------|-------------|
| 1 | FLT | Input | Filter select: GND = Normal latency, VCC = Low latency |
| 2 | DEMP | Input | De-emphasis: GND = Disabled (recommended) |
| 3 | XSMT | Input | Soft mute: GND = Muted, VCC = **Unmuted (active)** |
| 4 | FMT | Input | Format: GND = I2S, VCC = Left-Justified |
| 5 | AGND | Power | Analog ground |
| 6 | VCOM | Output | Common voltage output — decouple with 1µF to AGND |
| 7 | OUTR+ | Output | Right channel differential output (+) |
| 8 | OUTR− | Output | Right channel differential output (−) |
| 9 | OUTL− | Output | Left channel differential output (−) |
| 10 | OUTL+ | Output | Left channel differential output (+) |
| 11 | AVDD | Power | Analog supply 3.3V |
| 12 | LRCK | Input | Left/Right Clock (Word Select) = sample rate |
| 13 | DATA | Input | Serial audio data (I2S SDATA) |
| 14 | BCK | Input | Bit Clock |
| 15 | SCKI | Input | System Clock Input (MCLK) — can be left unconnected if using SCK mode |
| 16 | DVDD | Power | Digital supply 3.3V |
| 17 | DGND | Power | Digital ground |

---

## Raspberry Pi 4B GPIO → PCM5102A Wiring

```
Raspberry Pi 4B                        PCM5102A Module
─────────────────                      ───────────────

Pin 1  (3.3V) ─────────────────────── VCC  (DVDD + AVDD)
Pin 6  (GND)  ─────────────────────── GND  (DGND + AGND)

Pin 12 (GPIO18, PCM_CLK)  ──────────── BCK  (Pin 14)
Pin 35 (GPIO19, PCM_FS)   ──────────── LRCK (Pin 12)
Pin 40 (GPIO21, PCM_DOUT) ──────────── DATA (Pin 13)

                           GND ──────── FLT  (Pin 1)   Normal latency
                           GND ──────── DEMP (Pin 2)   De-emphasis OFF
                           3.3V ─────── XSMT (Pin 3)   Unmuted
                           GND ──────── FMT  (Pin 4)   I2S format

PCM5102A OUTL+ ─── 100Ω ─────────────────────────── Left  (+) to amp/jack
PCM5102A OUTL− ─── 100Ω ─────────────────────────── Left  (−) to amp/jack
PCM5102A OUTR+ ─── 100Ω ─────────────────────────── Right (+) to amp/jack
PCM5102A OUTR− ─── 100Ω ─────────────────────────── Right (−) to amp/jack
```

> **Note:** The 100Ω resistors on the output are optional but recommended to protect against short circuits and reduce RF emissions.

---

## ASCII Schematic

```
                    ┌──────────────────┐
3.3V ───────────────┤ DVDD        BCK  ├──── GPIO18 (Pin 12)
3.3V ───────────────┤ AVDD       LRCK  ├──── GPIO19 (Pin 35)
GND  ───────────────┤ DGND       DATA  ├──── GPIO21 (Pin 40)
GND  ───────────────┤ AGND              │
                    │                  │
GND  ───────────────┤ FLT    OUTL+ ───┼──── L+ ──┐
GND  ───────────────┤ DEMP   OUTL− ───┼──── L- ──┤ 3.5mm
3.3V ───────────────┤ XSMT   OUTR+ ───┼──── R+ ──┤  Jack
GND  ───────────────┤ FMT    OUTR− ───┼──── R- ──┘
                    │                  │
             1µF ───┤ VCOM             │
             to GND └──────────────────┘
                         PCM5102A
```

---

## Raspberry Pi 4B GPIO Header Reference

```
                     3V3  (1) (2)  5V
                   GPIO2  (3) (4)  5V
                   GPIO3  (5) (6)  GND
                   GPIO4  (7) (8)  GPIO14
                     GND  (9) (10) GPIO15
                  GPIO17 (11) (12) GPIO18  ← BCK  (I2S CLK)
                  GPIO27 (13) (14) GND
                  GPIO22 (15) (16) GPIO23
                     3V3 (17) (18) GPIO24
                  GPIO10 (19) (20) GND
                   GPIO9 (21) (22) GPIO25
                  GPIO11 (23) (24) GPIO8
                     GND (25) (26) GPIO7
                   GPIO0 (27) (28) GPIO1
                   GPIO5 (29) (30) GND
                   GPIO6 (31) (32) GPIO12
                  GPIO13 (33) (34) GND
                  GPIO19 (35) (36) GPIO16  ← LRCK (I2S FS)
                  GPIO26 (37) (38) GPIO20
                     GND (39) (40) GPIO21  ← DATA (I2S DOUT)
```

**Summary of I2S pins used:**
| GPIO | Header Pin | I2S Function | PCM5102A Pin |
|------|-----------|--------------|--------------|
| GPIO18 | Pin 12 | PCM_CLK (BCK) | BCK (14) |
| GPIO19 | Pin 35 | PCM_FS (LRCK) | LRCK (12) |
| GPIO21 | Pin 40 | PCM_DOUT (DATA) | DATA (13) |

---

## Decoupling Capacitors (Recommended)

For a clean build on breadboard or custom PCB:

| Location | Value | Purpose |
|----------|-------|---------|
| DVDD to DGND | 100nF ceramic + 10µF electrolytic | Digital supply decoupling |
| AVDD to AGND | 100nF ceramic + 10µF electrolytic | Analog supply decoupling |
| VCOM to AGND | 1µF ceramic | Common voltage stabilisation |

Place capacitors as close to the PCM5102A as possible.

---

## Verification After Wiring

```bash
# After running setup.sh and rebooting:

# 1. Check GPIO alt function (should show ALT0 for I2S)
raspi-gpio get 18 19 21

# 2. Check card appears
aplay -l | grep -i i2s

# 3. Play test tone
speaker-test -D plughw:sndrpii2scard,0 -t sine -f 1000 -c 2 -l 1
```

If you hear a 1kHz sine wave on both channels — bringup is successful.
