# LoRa Node 1 (Empfänger)

## Projekt-Übersicht

ESP32-basierter LoRa-Empfänger mit OLED-Display. Empfängt Pakete vom LoRa Sender 1, zeigt RSSI, SNR und Zeit seit letztem Paket auf dem Display. Kein Home Assistant erforderlich.

## Hardware

### Komponenten

- **ESP32 Dev Board** (esp32dev)
- **SX127x LoRa Modul** (SX1276/SX1278, 868 MHz)
- **SH1106 OLED Display** 128×64, I2C (0x3C)

### Verkabelung

```
SX127x      ESP32
├─ CLK  →   GPIO5
├─ MOSI →   GPIO27
├─ MISO →   GPIO19
├─ CS   →   GPIO18
├─ RST  →   GPIO14
└─ DIO0 →   GPIO26

SH1106      ESP32
├─ SDA  →   GPIO21
└─ SCL  →   GPIO22
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
| RX Mode | aktiv (`rx_start: true`) |

#### Display-Anzeige

- **Kein Paket empfangen:** Wartezeit seit Start
- **Paket empfangen:** RSSI (groß), SNR + Zeit seit letztem Paket

#### Wichtige Parameter

- **Web Server:** Version 2 (Port 80)
- **Fallback AP:** "LoRa Node 1", Passwort `lora1234`
- **API Reboot-Timeout:** 0s (kein Reboot wenn HA nicht erreichbar)
- **WiFi Reboot-Timeout:** 15min

## Funktionsweise

1. ESP32 startet im LoRa RX-Modus
2. Bei Paketempfang: RSSI, SNR und Hex-Inhalt werden geloggt und als Sensoren publiziert
3. Display zeigt laufend den Status (RSSI, SNR, Zeit seit letztem Paket)
4. Button "LoRa: Testpaket senden" schickt das Standard-Testpaket zurück

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
docker exec esphome esphome run /config/lora-node-1.yaml --no-logs
```

### Logs

```bash
docker exec esphome esphome logs /config/lora-node-1.yaml
```

## Network Configuration

- **ESP32:** Via DHCP
- **Fallback AP:** "LoRa Node 1"

## Technische Details

- **ESPHome:** 2026.1.0
- **Framework:** Arduino
- **Board:** esp32dev

## Changelog

### v1.0 (2026-05-08)
- Initiales Setup dokumentiert
- SX127x LoRa Empfänger, SF12, 868.1 MHz
- SH1106 OLED Display (RSSI/SNR Anzeige)
- Web Server v2, Uptime, Reset-Grund, IP-Adresse
- API Reboot-Timeout deaktiviert (kein Home Assistant)

---

**Zuletzt aktualisiert:** 2026-05-08
