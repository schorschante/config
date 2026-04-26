# HLK-LD2450 ESP32 PC Suspend System

## Projekt-Übersicht

Automatisches PC-Suspend-System basierend auf mmWave-Radarsensor. Der ESP32 überwacht mit dem HLK-LD2450 Sensor die Anwesenheit am Schreibtisch und suspendiert den PC automatisch wenn niemand näher als 2m ist.

## Hardware

### Komponenten
- **ESP32 Dev Board** (esp32dev)
- **HLK-LD2450** mmWave Radar Sensor (24GHz)
  - 1T2R (One-Transmitter-Two-Receiver)
  - Tracking bis zu 3 Personen gleichzeitig
  - Max. Reichweite: 6 Meter
  - Kommunikation: UART (256000 Baud) + Bluetooth

### Verkabelung

```
HLK-LD2450          ESP32
├─ 5V        →      5V (oder VIN)
├─ GND       →      GND
├─ TX        →      GPIO16 (RX)
└─ RX        →      GPIO17 (TX)
```

**Stecker:** ZH1.5mm 4-Pin Connector

### Sensor-Montage

- **Position:** Vertikal an der Wand montiert (hochkant)
- **Höhe:** 1,5-2m über dem Boden
- **Ausrichtung:** 
  - USB-C Buchse unten
  - Pins/Lötpads oben
  - Schaut horizontal in den Raum
- **Orientierung:** 
  - Y-Achse zeigt nach vorne (in den Raum)
  - X-Achse ist vertikal (hoch/runter durch 90° Drehung)

## Software

### ESPHome Configuration

**Datei:** `hlk-ld2450-esp32.yaml`

#### Wichtige Parameter

- **UART:** GPIO16 (RX), GPIO17 (TX), 256000 Baud, RX Buffer 2048
- **Distanz-Schwelle:** konfigurierbar per Web-UI ("Schreibtisch Entfernung", 500–6000mm), Standard: 2000mm (2 Meter)
- **Suspend Timeout:** konfigurierbar per Web-UI ("PC Suspend Timeout", 0–60 min, 0 = deaktiviert), Standard: 5 min
- **Safety Delay:** 3 Sekunden (zusätzlicher Delay vor Suspend)
- **Throttle:** 500ms (Sensor-Update-Rate)
- **Status-Log:** alle 30s — zeigt Countdown wenn niemand erkannt aber `desk_occupied` noch ON

#### Datenformat

Der LD2450 sendet Werte in **Millimetern**.

**UART Frame Format:**
```
AA FF 03 00 = Header
XX XX       = Target 1 X (mm, Little Endian)
YY YY       = Target 1 Y (mm, Little Endian)
SS SS       = Target 1 Speed (mm/s)
...         = Target 2, Target 3
55 CC       = Footer
```

#### Filter-Logik

```yaml
filters:
  - lambda: |-
      if (x < 0 || x > 6000) return {};  # Phantom-Targets rausfiltern (>6m)
      return x;
  - throttle: 500ms
```

### PC Suspend Service

**Standort:** `~/services/suspend-server.py`

#### Python Service

```python
#!/usr/bin/env python3
from flask import Flask
import subprocess

app = Flask(__name__)

@app.route('/suspend', methods=['GET', 'POST'])
def suspend():
    print("💤 PC wird suspendiert...")
    subprocess.run(['systemctl', 'suspend'])
    return 'Suspending system...', 200

@app.route('/status')
def status():
    return 'Server running', 200

if __name__ == '__main__':
    print("🚀 PC Suspend Server läuft auf Port 5000")
    app.run(host='0.0.0.0', port=5000)
```

#### Systemd Service

**Datei:** `/etc/systemd/system/pc-suspend.service`

```ini
[Unit]
Description=PC Suspend Server
After=network.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 /home/schorsch/services/suspend-server.py
Restart=always
User=root

[Install]
WantedBy=multi-user.target
```

#### Installation (Arch Linux)

```bash
# Flask installieren
sudo pacman -S python-flask

# Service aktivieren
sudo systemctl enable pc-suspend
sudo systemctl start pc-suspend

# Status prüfen
sudo systemctl status pc-suspend
curl http://localhost:5000/status
```

## Funktionsweise

### Logik-Flow

1. **Sensor trackt Personen** im Raum (bis zu 3 gleichzeitig)
2. **Distanz-Check:** Ist jemand näher als 2000mm (2m)?
3. **Wenn JA:**
   - Binary Sensor `desk_occupied` = ON
   - PC bleibt an
   - Log: "✅ Jemand am Schreibtisch - PC bleibt an"

4. **Wenn NEIN:**
   - `no_presence_since_ms` wird gesetzt (Startzeit der Abwesenheit)
   - Status-Log alle 30s: "AN - aber seit Xs niemand erkannt! PC suspend in Ys"
   - Nach konfiguriertem Timeout (Web-UI): `suspend_sent` = true
   - Warte weitere 3 Sekunden (Safety Delay)
   - Sende HTTP POST zu `http://<PC-IP>:5000/suspend`
   - PC suspendiert
   - Log: "⚠️ NIEMAND am Schreibtisch - PC wird suspendiert..."

### Szenarien

| Szenario | Person 1 | Person 2 | PC Status |
|----------|----------|----------|-----------|
| Du am Schreibtisch | 1500mm | - | AN ✅ |
| Frau hinten | 3500mm | - | SUSPEND nach 5min ⚠️ |
| Beide, du vorne | 1500mm | 3500mm | AN ✅ |
| Beide hinten | 3500mm | 4000mm | SUSPEND nach 5min ⚠️ |
| Raum leer | - | - | SUSPEND nach 5min ⚠️ |

## Konfiguration

### HLKRadarTool App (Bluetooth)

**Kritisch für zuverlässige Erkennung!**

1. **App installieren:**
   - Android: "HLKRadarTool" (Play Store)
   - iOS: "HLKRadarTool" (App Store)

2. **Mit Sensor verbinden** (Bluetooth)

3. **Einstellungen anpassen:**
   - **Sensitivity:** High oder Max (wichtig für Still-Target-Detection!)
   - **Max Detection Range:** 6 Meter
   - **Min Detection Range:** 0,5 Meter
   - **Detection Zones:** Bereich um den Schreibtisch einzeichnen
   - **Ignore Zones:** Bereiche außerhalb des Raums ausblenden
   - **Multi-Target Mode:** ON (für 2 Personen)

4. **Firmware-Update:** Falls verfügbar durchführen

### ESPHome Anpassungen

#### Distanz-Schwelle ändern

**Über Web-UI konfigurierbar** (kein Reflash nötig):
- Web-UI öffnen: `http://ESP32_IP`
- "Schreibtisch Entfernung" auf gewünschten Wert setzen (500–6000mm, Schritt: 100mm)
- Wert wird auf dem Gerät gespeichert (überlebt Neustart)

#### Verzögerung ändern

**Über Web-UI konfigurierbar** (kein Reflash nötig):
- Web-UI öffnen: `http://ESP32_IP`
- "PC Suspend Timeout" auf gewünschten Wert setzen (0–60 min)
- **0 = Suspend deaktiviert** (PC geht nie aus)
- Wert wird auf dem Gerät gespeichert (überlebt Neustart)

#### PC-IP ändern

**Über Web-UI konfigurierbar** (kein Reflash nötig):
- Web-UI öffnen: `http://ESP32_IP`
- "PC IP-Adresse" auf die neue IP setzen
- Wert wird auf dem Gerät gespeichert (überlebt Neustart)

## Debugging

### Crash-Diagnose

**"Letzter Reset-Grund"** in der Web-UI zeigt den Grund des letzten Reboots:

| Wert | Bedeutung |
|------|-----------|
| Power On | Normaler Start oder USB-Flash (esptool RTS-Reset) |
| Software Reset (OTA) | OTA-Update erfolgreich |
| Panic/Crash | Software-Fehler → Logs prüfen |
| Task Watchdog | Task hat zu lange geblockt |
| Interrupt Watchdog | Interrupt-Handler zu langsam |
| Brownout | Stromversorgung zu schwach |

**Wichtig:** Nach einem USB-Flash steht immer "Power On" (esptool macht Hardware-Reset). Der echte Crash-Grund ist nur sichtbar wenn der ESP32 **von selbst** neu bootet. Stecker ziehen also nur wenn der ESP gar nicht mehr reagiert — nicht zur Diagnose.

### ESP32 Logs

```bash
# Live Logs
esphome logs hlk-ld2450-esp32.yaml

# Nur wichtige Events
esphome logs hlk-ld2450-esp32.yaml | grep -E "(pc_control|status|Person)"

# Nur PC-Control
esphome logs hlk-ld2450-esp32.yaml | grep "pc_control"
```

### PC Service Logs

```bash
# Service Status
sudo systemctl status pc-suspend

# Live Logs
sudo journalctl -u pc-suspend -f

# Letzte 50 Zeilen
sudo journalctl -u pc-suspend -n 50 --no-pager
```

### Häufige Probleme

#### Sensor erkennt niemanden

**Symptom:** UART sendet nur `00:00:00:00...` obwohl Person im Raum

**Ursachen:**
1. **Empfindlichkeit zu niedrig** → HLKRadarTool: Sensitivity erhöhen
2. **Detection Zone falsch** → Zone nicht auf Schreibtisch fokussiert
3. **Montage ungünstig** → Sensor schaut am Schreibtisch vorbei
4. **Person zu still** → Bewege dich (Arme schwenken) zum Testen

**Lösung:** HLKRadarTool App nutzen und Live-View prüfen!

#### Extreme Distanz-Werte (>10m)

**Symptom:** `Person 1 Entfernung >> 6535 cm` (65 Meter!)

**Ursache:** Phantom-Targets durch Reflexionen (Wände, Fenster, Metall)

**Lösung:**
1. **Ignore Zones** in HLKRadarTool setzen
2. **Max Range** auf 6m begrenzen
3. **Filter prüfen:** `if (x < 0 || x > 600) return {};`

#### PC suspendiert nicht

**Check 1: Service läuft?**
```bash
sudo systemctl status pc-suspend
curl http://localhost:5000/status
```

**Check 2: Firewall blockiert?**
```bash
sudo firewall-cmd --add-port=5000/tcp --permanent
sudo firewall-cmd --reload
```

**Check 3: ESP32 erreicht PC?**
- Web-UI öffnen: `http://ESP32_IP`
- Button "Test PC Verbindung" klicken
- Logs prüfen

#### Sensor "hängt" scheinbar

**Symptom:** Lange Pausen ohne Sensor-Updates

**Realität:** Sensor hängt nicht! Er sendet nur keine Updates wenn:
- Keine Targets erkannt werden (alle Werte = 0)
- Werte sich nicht ändern
- `throttle` Filter aktiv ist

**UART sendet weiterhin kontinuierlich Daten!**

## Network Configuration

### IP-Adressen

- **PC:** `192.168.178.110` (konfigurierbar per Web-UI — "PC IP-Adresse")
- **ESP32:** Via DHCP (in Web-UI oder Logs sichtbar)
- **Service Port:** `5000` (HTTP)

### Firewall

Port 5000 muss erreichbar sein für ESP32. **ufw** ist aktiv — Regel einmalig setzen:

```bash
sudo ufw allow 5000/tcp
```

Ohne diese Regel schlägt der HTTP-Request vom ESP32 mit `ESP_ERR_HTTP_CONNECT` fehl, obwohl der Service läuft.

## Technische Details

### ESPHome Version

- **ESPHome:** 2026.1.0
- **Framework:** ESP-IDF
- **Board:** esp32dev

### Sensor Spezifikationen

- **Typ:** HLK-LD2450
- **Frequenz:** 24 GHz (FMCW)
- **Targets:** Max. 3 gleichzeitig
- **Reichweite:** 0,5 - 6 Meter
- **Genauigkeit:** ±10cm
- **Update-Rate:** ~2,5 Hz (alle ~400ms)
- **Stromverbrauch:** ~300-500mA @ 5V

### Koordinatensystem

```
Sensor-Sicht (nach Rotation):

         ↑ +X (vertikal hoch)
         │
         │
    ─────┼───── Y → (horizontal in den Raum)
         │
         ↓ -X (vertikal runter)
```

**Wichtig:** Durch vertikale Montage ist X-Achse vertikal (nicht horizontal links/rechts)!

## Erweiterungsmöglichkeiten

### Wake-on-LAN (WOL)

PC automatisch aufwecken wenn Person <2m:

```yaml
# WOL hinzufügen
button:
  - platform: wake_on_lan
    name: "PC Aufwecken"
    target_mac_address: "XX:XX:XX:XX:XX:XX"
```

### MQTT Integration

Statt HTTP direkt MQTT nutzen:

```yaml
mqtt:
  broker: 192.168.178.1
  
binary_sensor:
  - platform: template
    id: desk_occupied
    on_state:
      then:
        - mqtt.publish:
            topic: "home/office/desk/occupied"
            payload: !lambda 'return x ? "ON" : "OFF";'
```

### Home Assistant Integration

Über ESPHome API automatisch in HA verfügbar:
- `binary_sensor.person_am_schreibtisch`
- `sensor.person_1_entfernung`
- `button.test_pc_verbindung`

### Zonen-basierte Erkennung

Unterschiedliche Bereiche definieren:

```yaml
binary_sensor:
  - platform: template
    name: "Schreibtisch Zone"
    lambda: |-
      # Nur vorderer Bereich
      return (id(y1).state > 50 && id(y1).state < 250 &&
              id(x1).state > -100 && id(x1).state < 100);
```

## Datenschutz & Sicherheit

### Bluetooth-Sicherheit

**WARNUNG:** Der LD2450 hat **keine** Bluetooth-Authentifizierung!

- Jeder in BT-Reichweite (~10m) kann sich verbinden
- Kann Sensor-Einstellungen ändern
- Kann Live-Tracking-Daten sehen

**Empfehlung:** Nach Konfiguration nicht mehr per BT verbinden.

### Daten-Privacy

- Sensor speichert **keine** Daten
- Nur Live-Koordinaten (X/Y in cm)
- Keine Kamera, keine Bilder
- Anonyme Positions-Daten

### Service-Sicherheit

- Service läuft als `root` (für `systemctl suspend`)
- Keine Authentifizierung (localhost only empfohlen)
- Für externe Zugriffe: Firewall-Regel setzen

## Wartung

### Firmware Updates

**ESP32:**
```bash
esphome run hlk-ld2450-esp32.yaml
```

**LD2450 Sensor:** Via HLKRadarTool App

### Service Neustart

```bash
sudo systemctl restart pc-suspend
```

### Logs rotieren

```bash
# Journald automatisch (systemd)
sudo journalctl --vacuum-time=7d
```

## Lizenz & Credits

- **ESPHome:** MIT License
- **HLK-LD2450:** Hi-Link Electronic (Shenzhen)
- **Flask:** BSD License

## Changelog

### v1.0 (2026-04-20)
- Initiales Setup
- ESP32 + HLK-LD2450 Integration
- PC Suspend Service (Flask)
- 2m Distanz-Trigger
- Multi-Target Support
- Systemd Service Integration

## Support & Weiterentwicklung

### Bekannte Issues

1. **Still-Target-Detection** nicht perfekt → Empfindlichkeit in App anpassen
2. **Phantom-Targets** bei Reflexionen → Ignore Zones setzen
3. **Boot Counter sehr hoch** (392) → Frühere Stromprobleme, aktuell stabil

### TODO

- [ ] Wake-on-LAN Integration
- [ ] MQTT Support
- [ ] Zonen-basierte Automatisierung
- [ ] Grafana Dashboard
- [ ] Presence-History Logging
- [ ] Bluetooth-Sicherheit verbessern

## Kontakt & Dokumentation

- **ESPHome Docs:** https://esphome.io
- **LD2450 Datasheet:** http://www.hlktech.net/
- **HLKRadarTool:** Play Store / App Store
- **ESPHome LD2450 Component:** https://esphome.io/components/sensor/ld2450.html

---

**Zuletzt aktualisiert:** 2026-04-20  
**System läuft auf:** Arch Linux (Kernel 6.x)  
**Hostname:** DASNEST  
**User:** schorsch