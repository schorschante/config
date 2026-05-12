# LoRa Node 1 (Empfänger)

## Projekt-Übersicht

ESP32-basierter LoRa-Empfänger. Empfängt Pakete vom LoRa Sender 1, meldet RSSI, SNR und Zeit seit letztem Paket an Home Assistant. Watchdog neustartet den ESP wenn kein Paket in 6 Minuten kommt.

## Hardware

### Komponenten

- **ESP32 Dev Board** (esp32dev)
- **SX127x LoRa Modul** (SX1276/SX1278, 868 MHz)

> **Hinweis:** Das SH1106-OLED-Display wurde entfernt. Es blockierte den Main Loop via I2C (~150ms/Sek) und verursachte dadurch `sx127x marked as failed` Fehler nach einigen Stunden Betrieb.

### Verkabelung

```
SX127x      ESP32
├─ CLK  →   GPIO5  (Strapping-Pin — externer Pull nur wenn nötig)
├─ MOSI →   GPIO27
├─ MISO →   GPIO19
├─ CS   →   GPIO18
├─ RST  →   GPIO14
└─ DIO0 →   GPIO26
```

## Software

### ESPHome Configuration

**Datei:** `lora-node-1.yaml`

#### LoRa Parameter (müssen mit Sender übereinstimmen)

| Parameter | Wert |
|-----------|------|
| Frequenz | 868.1 MHz (EU) |
| Bandbreite | 125 kHz |
| Spreading Factor | SF12 |
| Coding Rate | CR 4/8 |
| Sync Word | 0x12 |
| Preamble | 8 Symbole |
| TX Power | 14 dBm (PA BOOST) |
| RX Mode | aktiv (`rx_start: true`) |

#### Wichtige Parameter

- **Framework:** ESP-IDF (nicht Arduino) — wegen `CONFIG_LWIP_MAX_SOCKETS=24`
- **Web Server:** Version 2 (Port 80)
- **Native API:** Port 6053 (Home Assistant Integration)
- **Fallback AP:** "LoRa Node 1", Passwort `lora1234`
- **API Reboot-Timeout:** 0s (kein Reboot wenn HA nicht erreichbar)
- **WiFi Reboot-Timeout:** 15min

#### Watchdog

Wenn `packet_count > 0` (d.h. mindestens ein Paket wurde empfangen) und das letzte Paket länger als 6 Minuten zurückliegt → automatischer Neustart. Verhindert, dass der Node dauerhaft stumm bleibt wenn der SX127x in einen Fehlerzustand gerät.

## Funktionsweise

1. ESP32 startet im LoRa RX-Modus (Continuous)
2. Bei Paketempfang über DIO0-Interrupt: RSSI, SNR und Hex-Inhalt werden geloggt und als HA-Sensoren publiziert
3. "LoRa Last Packet Age" zeigt wie lange das letzte Paket her ist (sekundengenau)
4. Button "LoRa: Testpaket senden" schickt das Standard-Testpaket (TX)

## Debugging

### Bekannte Fehler

#### SX127x `marked as failed`

**Ursache:** Der Main Loop wurde länger als ~100ms blockiert (war: OLED-Display via I2C). Das SX127x-Interrupt wird nicht abgearbeitet → Chip läuft aus dem RX-Modus. Nächster `loop()`-Aufruf schlägt fehl → `marked as failed` → Empfang dauerhaft tot bis Neustart.

**Fix:** Display entfernt. Watchdog neustartet wenn kein Paket in 6min.

#### Web Server nicht erreichbar (socket 23 Fehler)

**Ursache:** Arduino ESP32 hat nur ~10 lwIP-Sockets. Web Server + native API + mDNS + OTA + mehrere aktive Dashboard-Verbindungen erschöpfen den Pool.

**Fix:** ESP-IDF Framework mit `CONFIG_LWIP_MAX_SOCKETS=24`.

### Flash via USB (wenn OTA nicht funktioniert)

```bash
/home/schorsch/venvs/esphome/bin/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 write-flash 0x0 /home/schorsch/esphome/config/.esphome/build/lora-node-1/.pioenvs/lora-node-1/firmware.factory.bin
```

### Kompilieren

```bash
docker exec esphome esphome compile /config/lora-node-1.yaml
```

### Logs

```bash
docker exec esphome esphome logs /config/lora-node-1.yaml
```

> **Achtung:** Mehrere gleichzeitige `esphome logs`-Sessions belegen API-Verbindungen (max 8). ESPHome-Dashboard-Tabs schließen wenn API-Fehler auftreten.

## Network Configuration

- **ESP32:** Via DHCP (zuletzt 192.168.178.38)
- **Fallback AP:** "LoRa Node 1" / lora1234

## Technische Details

- **ESPHome:** 2026.1.0
- **Framework:** ESP-IDF (wegen Socket-Limit)
- **Board:** esp32dev
- **Flash:** 4MB

## Changelog

### v2.0 (2026-05-11)
- Display (SH1106) entfernt — war Ursache der SX127x-Ausfälle
- Framework Arduino → ESP-IDF (CONFIG_LWIP_MAX_SOCKETS=24)
- captive_portal entfernt (war extra Socket-Verbrauch)
- Restart-Watchdog: 6min kein Paket → Neustart
- LoRa RSSI/SNR/Last Packet als HA-Sensoren
- Home Assistant Integration via native API

### v1.0 (2026-05-08)
- Initiales Setup: SX127x LoRa Empfänger, SF12, 868.1 MHz
- SH1106 OLED Display (inzwischen entfernt)
- Web Server v2, Uptime, Reset-Grund, IP-Adresse

---

**Zuletzt aktualisiert:** 2026-05-11
