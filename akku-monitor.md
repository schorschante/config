# Akku Monitor

## Projekt-Übersicht

ESP32-basierter Akku-Monitor. Misst die Batteriespannung über einen schaltbaren Spannungsteiler. Der Transistor trennt den Teiler ab, um Querstrom zu verhindern. GPIO34 wird immer gelesen (alle 10s); ob der Transistor wirklich sperrt, erkennt man am ADC-Floaten (~Vbat statt ~Vbat/2).

## Hardware

### Komponenten

- **ESP32 Dev Board** (esp32dev)
- **Spannungsteiler:** R1 = R2 = 100 kΩ
- **Transistor:** 2N2222A (NPN)
- **Basiswiderstand:** 1 kΩ

### Schaltung

```
Akku+
  │
  R1 (100kΩ)
  │
  ├──── GPIO34 (ADC, misst Vbat/2)
  │
  R2 (100kΩ)
  │
Collector ─── 2N2222A
Emitter ───── GND

GPIO25 ── 1kΩ ── Basis
```

**GPIO25 HIGH** → Transistor leitet → Spannungsteiler aktiv → GPIO34 = Vbat/2  
**GPIO25 LOW** → Transistor sperrt → GPIO34 floatet → ADC liest Rauschwerte

### Messbereich

| Parameter | Wert |
|-----------|------|
| ADC-Eingang | GPIO34 (ADC1 Kanal 6) |
| Attenuation | 12 dB (0–3.9 V) |
| Teiler-Faktor | 0.5 (R1=R2) |
| Max. Akku-Spannung | ~4.9 V |
| Passend für | LiPo Einzelzelle (3.0–4.2 V) |

### Transistor-Zustand mit Multimeter prüfen

Messen an **GPIO34 gegen GND**:

| Transistor | Multimeter-Wert |
|------------|----------------|
| Switch ON  | ~Vbat/2 ≈ 1.95 V |
| Switch OFF | ~Vbat ≈ 3.9 V (Multimeter 10 MΩ als Last) |

## Software

### Entitäten

| Entität | Typ | Beschreibung |
|---------|-----|-------------|
| **Akku messen** | Switch | GPIO25 steuern — Transistor ein/aus |
| **Akku Spannung** | Sensor | Gemessene Spannung in Volt (alle 10s) |
| **Akku Ladung** | Sensor | Berechneter Ladestand in % (alle 10s) |
| **Letzter Reset-Grund** | Text Sensor | Diagnose |
| **IP-Adresse** | Text Sensor | WLAN-IP |
| **Uptime** | Text Sensor | Laufzeit in h/min |

### Messverhalten

GPIO34 wird alle 10s gelesen, unabhängig vom Switch-Zustand:
- Switch **ON**: korrekte Akkuspannung (Spannungsteiler aktiv)
- Switch **OFF**: ADC floatet → Rauschwerte → letzter gültiger HA-Wert bleibt stehen

### Kalibrierung

Nach Inbetriebnahme mit Multimeter kalibrieren:

```yaml
filters:
  - multiply: 2.0
  - calibrate_linear:
      - 0.0 -> 0.0
      - 6.29 -> 3.91   # Kalibriert 2026-05-14: ESP las 6.29V, Multimeter 3.91V
```

### Akku-Typ anpassen

In `substitutions`:
```yaml
battery_min_v: "3.0"   # LiPo leer
battery_max_v: "4.2"   # LiPo voll
```

Für andere Akkus (z.B. 3× NiMH = 3.6V max) entsprechend anpassen.

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

**Zuletzt aktualisiert:** 2026-05-14
