# Câblage — ESP32-S3 Super Mini + GC9A01 1.28" + pad tactile

| Écran GC9A01 | ESP32-S3 Super Mini | Note |
| ------------ | ------------------- | ---- |
| VCC          | 3V3                 | pas 5V |
| GND          | GND                 | |
| SCL (CLK)    | GPIO 5              | SPI SCK |
| SDA (MOSI)   | GPIO 6              | SPI MOSI |
| DC           | GPIO 8              | |
| CS           | GPIO 7              | |
| RST          | GPIO 9              | |
| BLK          | GPIO 10             | rétroéclairage, PWM (variateur de luminosité) |

| Périphérique            | Broche    | Note |
| ----------------------- | --------- | ---- |
| Bouton A (module tactile TTP223) | GPIO 4 | VCC→3V3, GND→GND, OUT→GPIO 4 |
| Bouton B (module tactile TTP223) | GPIO 2 | VCC→3V3, GND→GND, OUT→GPIO 2 |
| Buzzer passif (optionnel)| GPIO 1   | + décommenter `-DBUDDY_BUZZER_PIN=1` dans `platformio.ini` |

Seules les broches GPIO 1 à 10 sont utilisées. Évitées : 0, 3, 45, 46 (strapping), 19 et 20 (USB natif).
Carte requise : **ESP32-S3** (le C3 n'a pas de capteur tactile). Vérifie sur la sérigraphie que
les GPIO 1 à 10 sont bien sorties sur ta carte.

## Boutons

| Bouton | Action |
| ------ | ------ |
| A (tap) | approuver la demande / écran suivant / option suivante dans les menus |
| A (appui long 0,6 s) | ouvrir/fermer le menu |
| B | refuser la demande / valider dans les menus / page suivante |

Mode 1 seul bouton (si B absent) : retirer `-DBUDDY_BTNB_PIN=2` de `platformio.ini` ; tap = A,
double tap = B, appui long = menu.

## Flash

```bash
python -m platformio run -t upload
```

L'upload utilise l'esptool récent (`pip install -U esptool`), qui gère le reset automatique sur l'USB natif
de l'S3 : pas besoin de BOOT. En cas de souci, maintenir BOOT, rebrancher l'USB, relâcher.
Le moniteur série est en USB natif (`python -m platformio device monitor`).

## Réglages si besoin

Dans `platformio.ini` :
- `-DBUDDY_LCD_INVERT=1` si les couleurs sont inversées.
- `-DBUDDY_TOUCH_DEBUG=1` pour afficher `raw/base/thr` du capteur tactile sur le port série
  (ajuster `BUDDY_TOUCH_PCT` / `BUDDY_TOUCH_MIN_DELTA` si le pad est trop/pas assez sensible).

## Limites connues de ce premier port

- Pas testé sur le matériel : le build passe, le rendu et la sensibilité tactile restent à valider.
- L'UI d'origine est un portrait 135×240 centré sur l'écran rond : le haut et le bas (~20 px) sont
  rognés par le cercle (ligne « tap: approve / 2x: deny » en bas, notamment).
- Pas d'IMU : pas de « dizzy » (secousse), pas de sieste face cachée, horloge en portrait uniquement.
- Pas de batterie : les écrans d'info affichent 0 %. L'écran ne s'éteint jamais seul (alimentation USB).
