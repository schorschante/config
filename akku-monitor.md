# Akku Monitor

## Projekt-Übersicht

ESP32-basierter Akku-Monitor. Misst die Batteriespannung über einen festen Spannungsteiler (R1=220kΩ, R2=100kΩ). GPIO34 wird alle 10s gelesen und an Home Assistant gemeldet.

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
GPIO34 = Vbat × 0.3125  → bei 4.2V: GPIO34 = 1.31V

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
| **IP-Adresse** | Text Sensor | WLAN-IP |
| **Uptime** | Text Sensor | Laufzeit in h/min |

### Kalibrierung

```yaml
filters:
  - multiply: 3.2
  - calibrate_linear:
      - 0.0 -> 0.0
      - 10.06 -> 3.98   # Kalibriert 2026-05-15: ESP las 10.06V, Multimeter 3.98V
```

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

---

**Zuletzt aktualisiert:** 2026-05-15
