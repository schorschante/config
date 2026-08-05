# Akku Monitor

## Projekt-Übersicht

ESP32-basierter Akku-Monitor. Misst die Batteriespannung über einen festen Spannungsteiler (R1=220kΩ, R2=100kΩ). GPIO35 wird alle 10s gelesen und an Home Assistant gemeldet.

**Betriebsmodus:** Zeitbasierter Deep Sleep — wach von 08:00 bis 20:00 Uhr, schlaeft von 20:00 bis 08:00 Uhr (12h Schlaf). Deep Sleep-Dauer wird zur Laufzeit berechnet (Sekunden bis 08:00).

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

### Deep Sleep Zyklus (zeitbasiert)

| Phase | Zeit |
|-------|------|
| Wach | 08:00 – 20:00 Uhr |
| Schlaf | 20:00 – 08:00 Uhr (12h) |

**Ablauf:**
1. Boot → WiFi verbindet → `on_boot` (priority: -100) wartet 5s → erste Messung (Wert vor Schlaf aus NVS wiederherstellen, neue Messung als "nach Aufwachen")
2. Wachzeit: `interval: 10s` → laufende Messungen alle 10s
3. `time.on_time` schlägt genau um 20:00:00 an: letzte Messung, Wert in NVS speichern, dann `deep_sleep.enter` mit fester Schlafdauer 43200s (12h)
4. ESP wacht um ~08:00 auf → Boot von vorn

**Schlafdauer:** Fest 43200s (12h) — kein Berechnen von "Sekunden bis 08:00" mehr nötig, da `on_time` exakt um 20:00 triggert.

**Warum `on_time` statt `interval: 60s`:** Der `on_time` Trigger der SNTP-Komponente feuert exakt zur angegebenen Uhrzeit (auf die Sekunde genau), sobald die Zeit synchronisiert ist. Kein Polling jede Minute, keine Zeitberechnung, kein Edge-Case beim Boot in der Schlafzeit.

**Warum 5s Delay in `on_boot`:** WiFi-Verbindung + HA-API brauchen nach dem Boot etwas Zeit bis sie stabil sind. Ohne Delay wurde die erste Messung zwar im Log erscheinen, aber moglicherweise nicht korrekt an Home Assistant ubermittelt.

### Entitäten

| Entität | Typ | Beschreibung |
|---------|-----|-------------|
| **Akku Spannung** | Sensor | Gemessene Spannung in Volt (alle 10s, plus erste/letzte Messung) |
| **Akku Ladung** | Sensor | Berechneter Ladestand in % (alle 10s, plus erste/letzte Messung) |
| **Spannung vor Schlaf** | Sensor | Letzter Spannungswert direkt vor dem Deep Sleep (in `time.on_time` um 20:00) |
| **Spannung nach Aufwachen** | Sensor | Erster Spannungswert direkt nach Boot + 5s Delay + Messung (in `on_boot`) |
| **Letzter Reset-Grund** | Text Sensor | Diagnose — zeigt "Deep Sleep" nach normalem Sleep-Zyklus |
| **ESPHome Version** | Text Sensor | Installierte ESPHome-Version (Diagnose) |
| **IP-Adresse** | Text Sensor | WLAN-IP |
| **Uptime** | Text Sensor | Laufzeit in h/min (seit letztem Boot, max. ~1h) |

**Hinweis zu "Spannung vor/nach Schlaf":**
- `voltage_before_sleep` hat `restore_value: true` — ESPHome speichert den Wert in NVS (Flash), der Deep Sleep uberlebt. Nach dem Aufwachen wird der gespeicherte Wert sofort in `on_boot` via `publish_state()` in den Sensor geschrieben, bevor die neue Messung kommt. So zeigt "Spannung vor Schlaf" nach dem Aufwachen den letzten Wert von vor dem Einschlafen.
- `voltage_after_wake` hat `restore_value: false` — kein Persistieren gewollt, wird nach jedem Boot frisch gemessen.
- Beide Globals werden per `publish_state()` an Home Assistant ubermittelt.

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

## Deployment

### OTA flashen

```bash
docker exec esphome esphome run /config/akku-monitor.yaml --no-logs --device 192.168.178.146
```

**Wichtig bei Deep Sleep:** Der ESP ist nur zwischen 08:00 und 20:00 Uhr erreichbar. OTA muss in dieser Wachphase gestartet werden. Falls der ESP gerade schläft (20:00–08:00): warten bis 08:00 oder USB-Flash verwenden.

### USB-Flashen (wenn OTA nicht erreichbar)

```bash
/home/schorsch/venvs/esphome/bin/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 write-flash 0x0 /home/schorsch/esphome/config/.esphome/build/akku-monitor/.pioenvs/akku-monitor/firmware.factory.bin
```

### Logs

```bash
docker exec esphome esphome logs /config/akku-monitor.yaml
```

### Build-Cache löschen (nach ESPHome-Update)

```bash
docker exec esphome rm -rf /config/.esphome/build/akku-monitor
```

## Debugging

### Reset-Grund Tabelle

| Reset-Grund | Bedeutung |
|------------|-----------|
| Deep Sleep | Normaler Wake-up nach Sleep-Zyklus (Erwartungswert) |
| Power On | Erstmaliger Start oder manuelles Stromwegnehmen |
| Software Reset | OTA-Flash oder manueller Reboot |
| Watchdog | Crash — seriell debuggen |
| Brownout | Spannung zu niedrig — Stromversorgung prüfen |

### Deep Sleep OTA-Timing

Falls OTA wahrend Deep Sleep versucht wird:
- ESPHome meldet "Connection refused" oder Timeout
- Losung: Warten bis 08:00 Uhr (ESP wacht automatisch auf), dann OTA erneut starten
- Der Reset-Grund wechselt dann zu "Deep Sleep"

## Bekannte Probleme / Crash-Analyse

### Wiederholte exception/panic nach ~11h Laufzeit (2026-05-18)

**Symptom:** ESP crasht mehrfach nach langer Laufzeit, kein Backtrace verfügbar.

**Kontext:** WiFi -81 dB zur Zeit des Panics, ESPHome 2026.1.0 / ESP-IDF 5.5.2.

**Wahrscheinlichste Ursache:** Heap-Fragmentierung oder Buffer-Leak im async_tcp/LWIP-Stack bei wiederholten TCP-Verbindungsabrüchen unter schwachem WiFi-Signal.

**Durch Deep Sleep behoben:** Mit 1h run_duration / 3h sleep_duration kann sich der Heap nicht mehr über Stunden ansammeln. Jeder Boot startet mit sauberem Heap.

### `on_sleep` ist kein valides Key für `deep_sleep`

ESPHome's `deep_sleep` Komponente hat keine eigene `on_sleep` Automation. Stattdessen `esphome.on_shutdown` verwenden (wird beim Deep Sleep aufgerufen).

---

## Changelog

| Version | Datum | Änderung |
|---------|-------|----------|
| v1.0 | 2026-05-15 | Erstversion: ADC-Messung, Kalibrierung, Deep Sleep 1h/1h (run/sleep) |
| v1.1 | 2026-05-19 | Zwei neue Sensoren: "Spannung vor Schlaf" und "Spannung nach Aufwachen" via globals + publish_state |
| v1.2 | 2026-05-19 | `voltage_before_sleep` Global auf `restore_value: true` — Wert uberlebt Deep Sleep via NVS; `on_boot` publishes gespeicherten Wert sofort nach Aufwachen |
| v1.3 | 2026-05-19 | `sleep_duration` von 1h auf 3h erhöht — weniger Wach-Phasen, geringerer Energieverbrauch |
| v1.4 | 2026-05-19 | Zeitbasierter Deep Sleep: wach 08:00–20:00, schlaeft 20:00–08:00; `time: homeassistant` hinzugefuegt; Schlafdauer wird zur Laufzeit in Sekunden bis 08:00 berechnet; `on_boot` prüft sofort ob Schlafzeit; `interval: 60s` uberwacht Uhrzeit kontinuierlich |
| v1.5 | 2026-05-19 | Sleep-Logik vereinfacht: `interval: 60s` (Zeitcheck) und `on_boot`-Zeitcheck entfernt; stattdessen `time.on_time` um 20:00 → letzte Messung + `deep_sleep.enter` mit fester Schlafdauer 43200s; `on_shutdown` entfernt (Messung jetzt im `on_time`) |

| v1.6 | 2026-08-04 | OTA-Flash mit ESPHome 2026.7.3 / ESP-IDF 5.5.5 — Build-Cache geleert, kein Compile-Fehler, Flash OK |

**Zuletzt aktualisiert:** 2026-08-04
