# Akku Monitor

## Projekt-Übersicht

ESP32-basierter Akku-Monitor. Misst die Batteriespannung über einen festen Spannungsteiler (R1=220kΩ, R2=100kΩ). GPIO35 wird alle 10s gelesen und an Home Assistant gemeldet.

## Hardware

### Komponenten

- **ESP32 Dev Board** (esp32dev)
- **R1:** 220 kΩ
- **R2:** 100 kΩ

### Schaltung

```
Akku+
  │
  R1 (220kΩ)
  │
  ├──── GPIO35 (ADC, misst Vbat × 0.3125)
  │
  R2 (100kΩ)
  │
 GND ──── ESP32 GND ──── Akku-/Netzteil GND
```

> **Wichtig:** GND des Akkus/Netzteils muss mit GND des ESP32 verbunden sein (gemeinsamer Masse-Bezug). Ohne das liefert der ADC keine verwertbaren Werte.

Teiler-Faktor: R2/(R1+R2) = 100/320 = **0.3125**  
GPIO35 = Vbat × 0.3125  → bei 4.2V: GPIO35 = 1.31V

### Messbereich

| Parameter | Wert |
|-----------|------|
| ADC-Eingang | GPIO35 (ADC1 Kanal 7) |
| Attenuation | 12 dB (0–3.9 V) |
| Teiler-Faktor | 100/320 = 0.3125 |
| Multiply-Filter | 3.2 (= 320/100) |
| Passend für | LiPo Einzelzelle (3.0–4.2 V) |

## Software

### Framework

**ESP-IDF** (nutzt `esp_adc_cal` für bessere ADC-Kalibrierung)

### Entitäten

| Entität | Typ | Beschreibung |
|---------|-----|-------------|
| **Akku Spannung** | Sensor | Gemessene Spannung in Volt (alle 10s) |
| **Akku Ladung** | Sensor | Berechneter Ladestand in % (alle 10s) |
| **Letzter Reset-Grund** | Text Sensor | Diagnose |
| **ESPHome Version** | Text Sensor | Installierte ESPHome-Version (Diagnose) |
| **IP-Adresse** | Text Sensor | WLAN-IP |
| **Uptime** | Text Sensor | Laufzeit in h/min |

### Kalibrierung

```yaml
filters:
  - multiply: 3.2
  - calibrate_linear:
      - 3.059 -> 3.0
      - 3.373 -> 3.3
      - 3.578 -> 3.5
      - 3.782 -> 3.7
      - 3.990 -> 3.9
      - 4.173 -> 4.1
      - 4.291 -> 4.2
```
7-Punkt-Kalibrierung durchgeführt 2026-05-15 mit Multimeter über den vollen Spannungsbereich.

Rohwert-Logging für Neukalibrierung (lambda zwischen multiply und calibrate_linear):
```yaml
- lambda: |-
    ESP_LOGI("cal", "Rohwert (pre-cal): %.3f V", x);
    return x;
```

### Akku-Typ anpassen

In `substitutions`:
```yaml
battery_min_v: "3.0"   # LiPo leer
battery_max_v: "4.2"   # LiPo voll
```

## Debugging

### Kompilieren und Flashen

```bash
docker exec esphome esphome run /config/akku-monitor.yaml --no-logs
```

### Logs

```bash
docker exec esphome esphome logs /config/akku-monitor.yaml
```

## Bekannte Probleme / Crash-Analyse

### Wiederholte exception/panic nach ~11h Laufzeit (2026-05-18)

**Symptom:** ESP crasht mehrfach nach langer Laufzeit, kein Backtrace verfugbar (Panic-Monitor lauft uber WiFi-API, nicht seriell).

**Kontext:** WiFi -81 dB zur Zeit des Panics, ESPHome 2026.1.0 / ESP-IDF 5.5.2.

**Wahrscheinlichste Ursache:** Heap-Fragmentierung oder Buffer-Leak im async_tcp/LWIP-Stack bei wiederholten TCP-Verbindungsabruchen unter schwachem WiFi-Signal. Bei -81 dB reissen Verbindungen (HA-API + Web-UI SSE) wiederholt ab und werden neu aufgebaut — das akkumuliert sich uber Stunden.

**Verstarkt durch:** `api: reboot_timeout: 0s` — der ESP heilt sich nicht selbst wenn der WiFi-Stack in einen Halbzustand gerat.

**Weitere Verdachtige (niedrigere Prioritat):**
- `component.update: battery_voltage` + `component.update: battery_percent` direkt hintereinander im selben Interval: battery_percent liest moglicherweise einen inkonsistenten Zwischenwert von battery_voltage (Timing-Konflikt, kein bewiesener Crash-Grund auf Single-Core)

**Diagnose-Empfehlung:** Nachsten Crash seriell loggen (USB-Kabel anlassen, `esphome logs` uber seriell statt WiFi). Backtrace zeigt ob Panic aus WiFi/TCP-Stack oder Sensor-Pfad kommt.

**Workaround bis Backtrace vorliegt:** WiFi-Abdeckung verbessern (Ziel: besser als -75 dB) oder `api: reboot_timeout: 5min` setzen damit der ESP sich bei Verbindungsproblemen selbst zurucksetzt.

---

**Zuletzt aktualisiert:** 2026-05-18
