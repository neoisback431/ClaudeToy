#!/usr/bin/env python3
"""Status line Claude Code -> ClaudeToy.

Claude Code lance ce script et lui envoie un JSON sur stdin (modèle, niveau d'effort,
contexte, quotas...). Le script :
  1. affiche une ligne de statut dans Claude Code (stdout), comme une status line classique ;
  2. envoie en plus quelques champs à la carte ClaudeToy, sur son port série USB.

Seuls le modèle, l'effort et des pourcentages sont envoyés (pas de dossier, pas de dépôt Git).
Si la carte est absente ou si le port est occupé (flash, moniteur série), l'envoi est ignoré.

Installation : pip install pyserial, puis dans ~/.claude/settings.json :
  "statusLine": { "type": "command",
                  "command": "python C:/chemin/vers/tools/claudetoy_statusline.py",
                  "refreshInterval": 5 }
"""
import json
import sys

ESPRESSIF_VID = 0x303A


def pct(value):
    """Pourcentage entier 0..100, ou -1 si inconnu."""
    try:
        return max(0, min(100, int(round(float(value)))))
    except (TypeError, ValueError):
        return -1


def short_model(name):
    name = (name or "?").strip()
    if name.lower().startswith("claude "):
        name = name[7:]
    return name[:15]


def find_port():
    from serial.tools import list_ports
    for p in list_ports.comports():
        if p.vid == ESPRESSIF_VID:
            return p.device
    return None


def push(payload):
    try:
        import serial
        port = find_port()
        if not port:
            return
        s = serial.Serial()
        s.port = port
        s.baudrate = 115200
        s.timeout = 0.2
        s.write_timeout = 0.3
        # Lignes de contrôle au repos AVANT l'ouverture : évite de redémarrer l'ESP32-S3.
        s.dtr = False
        s.rts = False
        s.open()
        try:
            s.write((json.dumps(payload, separators=(",", ":")) + "\n").encode("utf-8"))
            s.flush()
        finally:
            s.close()
    except Exception:
        pass  # carte absente, port occupé, pyserial manquant : sans importance


def main():
    try:
        data = json.load(sys.stdin)
    except Exception:
        data = {}

    model = short_model((data.get("model") or {}).get("display_name"))
    effort = ((data.get("effort") or {}).get("level") or "")[:7]
    ctx = pct((data.get("context_window") or {}).get("used_percentage"))
    limits = data.get("rate_limits") or {}
    h5 = pct((limits.get("five_hour") or {}).get("used_percentage"))
    d7 = pct((limits.get("seven_day") or {}).get("used_percentage"))

    print("[%s] effort:%s ctx:%s%%" % (model, effort or "-", ctx if ctx >= 0 else "-"))

    push({"cc": {"m": model, "e": effort, "c": ctx, "h": h5, "d": d7}})


if __name__ == "__main__":
    main()
