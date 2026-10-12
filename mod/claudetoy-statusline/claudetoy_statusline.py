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
                  "command": "python C:/chemin/vers/mod/claudetoy-statusline/claudetoy_statusline.py",
                  "refreshInterval": 5 }
"""
import json
import os
import re
import sys
import tempfile
import time

ESPRESSIF_VID = 0x303A


def pct(value):
    """Pourcentage entier 0..100, ou -1 si inconnu."""
    try:
        return max(0, min(100, int(round(float(value)))))
    except (TypeError, ValueError):
        return -1


EFFORT_SHORT = {"medium": "med"}   # l'écran affiche 5 caractères au plus


def short_model(name):
    """« Claude Opus 4.8 (1M context) » -> « Opus 4.8 » (l'écran affiche 10 caractères)."""
    name = (name or "?").strip()
    if name.lower().startswith("claude "):
        name = name[7:]
    name = re.sub(r"\s*\([^)]*\)", "", name).strip()
    return (name or "?")[:15]


def find_port():
    from serial.tools import list_ports
    for p in list_ports.comports():
        if p.vid == ESPRESSIF_VID:
            return p.device
    return None


LAST = os.path.join(tempfile.gettempdir(), "claudetoy_statusline.last")


LOG = os.path.join(tempfile.gettempdir(), "claudetoy_statusline.log")


def note(text):
    """Dernier passage du script + journal des 100 derniers (pour vérifier quand Claude Code l'exécute)."""
    line = "%s %s\n" % (time.strftime("%Y-%m-%d %H:%M:%S"), text)
    try:
        with open(LAST, "w", encoding="utf-8") as f:
            f.write(line)
        try:
            with open(LOG, "r", encoding="utf-8") as f:
                lines = f.readlines()[-99:]
        except Exception:
            lines = []
        lines.append(line)
        with open(LOG, "w", encoding="utf-8") as f:
            f.writelines(lines)
    except Exception:
        pass


def push(payload, raw_info=""):
    try:
        import serial
        port = find_port()
        if not port:
            note("aucune carte Espressif détectée")
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
            note("envoyé sur %s : %s%s" % (port, json.dumps(payload, separators=(",", ":")), raw_info))
        finally:
            s.close()
    except Exception as exc:
        note("échec : %s: %s" % (type(exc).__name__, exc))  # carte absente, port occupé, pyserial manquant...


def main():
    try:
        data = json.load(sys.stdin)
    except Exception:
        data = {}

    model = short_model((data.get("model") or {}).get("display_name"))
    effort = (data.get("effort") or {}).get("level") or ""
    effort = EFFORT_SHORT.get(effort, effort)[:7]
    ctx = pct((data.get("context_window") or {}).get("used_percentage"))
    limits = data.get("rate_limits") or {}
    h5 = pct((limits.get("five_hour") or {}).get("used_percentage"))
    d7 = pct((limits.get("seven_day") or {}).get("used_percentage"))

    print("[%s] effort:%s ctx:%s%%" % (model, effort or "-", ctx if ctx >= 0 else "-"))

    # Trace de diagnostic : noms des champs reçus et contenu de context_window (uniquement des nombres).
    cw = data.get("context_window")
    sid = str(data.get("session_id") or "")[:8]
    raw_info = " | session=" + sid + " | champs=%s | context_window=%s | effort=%s | rate_limits=%s" % (
        ",".join(sorted(data.keys())),
        json.dumps(cw, separators=(",", ":")) if isinstance(cw, dict) else cw,
        data.get("effort"),
        "oui" if limits else "non",
    )
    push({"cc": {"m": model, "e": effort, "c": ctx, "h": h5, "d": d7}}, raw_info)


if __name__ == "__main__":
    main()
