# LoRa Sender 1

## Projekt-Übersicht

ESP32-basierter LoRa-Sender der alle 120 Sekunden ein festes Datenpaket sendet. Kein Home Assistant erforderlich — läuft vollständig autonom.

## Hardware

### Komponenten

- **ESP32 Dev Board** (esp32dev)
- **SX127x LoRa Modul** (SX1276/SX1278, 868 MHz)

### Verkabelung (SPI)

```
SX127x      ESP32
├─ CLK  →   GPIO5
├─ MOSI →   GPIO27
├─ MISO →   GPIO19
├─ CS   →   GPIO18
├─ RST  →   GPIO14
└─ DIO0 →   GPIO26
```

## Software

### ESPHome Configuration

**Datei:** `lora-sender-1.yaml`

#### LoRa Parameter

| Parameter | Wert |
|-----------|------|
| Frequenz | 868.1 MHz (EU) |
| Bandbreite | 125 kHz |
| Spreading Factor | SF12 |
| Coding Rate | CR 4/8 |
| TX Power | 14 dBm (BOOST) |
| CRC | aktiviert |
| Sync Word | 0x12 |
| Preamble | 8 Symbole |

#### Sende-Interval

Alle **120 Sekunden** wird folgendes Test-Paket gesendet:

```
C5 51 78 82 B7 F9 9C 5C
```

#### Wichtige Parameter

- **Web Server:** Version 2 (Port 80)
- **API Reboot-Timeout:** 0s (kein Reboot wenn Home Assistant nicht erreichbar)
- **WiFi Reboot-Timeout:** 15min

## Funktionsweise

1. ESP32 verbindet sich mit WLAN
2. Alle 120s wird ein festes Datenpaket per LoRa gesendet
3. Status über Web-UI (`http://ESP_IP`) abrufbar
4. Bei WLAN-Ausfall: nach 15 Minuten automatischer Neustart

## Debugging

### Crash-Diagnose

**"Letzter Reset-Grund"** in der Web-UI:

| Wert | Bedeutung |
|------|-----------|
| Power On | Normaler Start |
| Software Reset (OTA) | OTA-Update |
| Panic/Crash | Software-Fehler → Logs prüfen |
| Software Watchdog | Hauptloop blockiert |

### OTA-Flash

```bash
docker exec esphome esphome run /config/lora-sender-1.yaml --no-logs
```

### Logs

```bash
docker exec esphome esphome logs /config/lora-sender-1.yaml
```

## Network Configuration

- **ESP32:** Via DHCP (192.168.178.36)
- **Web Server:** Port 80

## Technische Details

- **ESPHome:** 2026.1.0
- **Framework:** Arduino
- **Board:** esp32dev

## Changelog

### v1.0 (2026-05-08)
- Initiales Setup
- SX127x LoRa, SF12, 868.1 MHz, 14 dBm
- Sendet alle 120s ein festes Paket
- Web Server v2, Uptime, Reset-Grund, IP-Adresse
- API Reboot-Timeout deaktiviert (kein Home Assistant)

---

**Zuletzt aktualisiert:** 2026-05-08
