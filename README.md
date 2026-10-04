# ClaudeToy

Un petit compagnon physique pour Claude Desktop : un écran rond avec **Clawd**, la mascotte pixel-art de
Claude Code, qui réagit à ce que fait Claude (au travail, en attente de validation, terminé…) et te laisse
**approuver ou refuser** les demandes de permission depuis un bouton tactile.

*A tiny physical Claude Desktop companion on an ESP32-S3 + round GC9A01 display, with touch buttons to approve or deny
permission prompts. Port of the official [claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy)
(M5StickC Plus) to other hardware.*

> Projet communautaire non officiel, sans lien avec Anthropic. La fonction « Hardware Buddy » de Claude Desktop est
> réservée au mode développeur et n'est pas une fonctionnalité officiellement supportée.

## Matériel

- ESP32-S3 Super Mini (le C3 ne convient pas : pas de BLE 5 confortable ni de tactile natif)
- Écran rond GC9A01 1,28" SPI 240×240
- 1 ou 2 modules tactiles type TTP223
- (Optionnel, non implémenté dans le firmware) batterie Li-ion + chargeur : voir [BATTERY.md](BATTERY.md)

Câblage complet : [WIRING.md](WIRING.md).

## Compiler et flasher

```bash
pip install platformio esptool
python -m platformio run -t upload
```

L'upload passe par l'esptool récent (PlatformIO embarque une version qui rate le reset automatique sur l'USB natif
de l'S3). Si la carte n'est pas détectée : maintenir **BOOT**, rebrancher l'USB, relâcher après 2 s.

## Appairage avec Claude Desktop

1. Dans Claude Desktop : `Help → Troubleshooting → Enable Developer Mode`.
2. `Developer → Open Hardware Buddy…` puis **Connect** et choisir `Claude-XXXX`.
3. Saisir le code à 6 chiffres affiché sur l'écran.

Le protocole (Bluetooth LE, Nordic UART, JSON) est documenté dans le dépôt d'origine
([REFERENCE.md](https://github.com/anthropics/claude-desktop-buddy/blob/main/REFERENCE.md)).

## Boutons

Mode 2 boutons (par défaut, `-DBUDDY_BTNB_PIN=2`) :

| Bouton | Action |
| ------ | ------ |
| A, tap | approuver / écran suivant |
| A, appui long | menu |
| B | refuser / valider / page suivante |

Mode 1 bouton (commenter `BUDDY_BTNB_PIN`) : tap = A, appui long = B, double tap = menu.

## Ce qui change par rapport à l'original

- Couche de compatibilité `src/M5StickCPlus.h` + `src/m5shim.cpp` : l'API M5Stack est émulée (écran TFT_eSPI, rétroéclairage PWM,
  boutons tactiles, horloge logicielle), le reste du firmware est quasi inchangé.
- Nouvelle espèce `clawd` ([src/buddies/clawd.cpp](src/buddies/clawd.cpp)), dessinée en pixels, avec les 7 états animés.
- Tableau de bord de veille (heure, tokens du jour, niveau, sessions, Bluetooth, dernier message).
- Interface réduite à 135×208 px, centrée dans le cercle.

## État

- Testé sur le matériel : compilation, flash, affichage, mode 1 bouton, appairage Bluetooth avec Claude Desktop.
- Mode 2 boutons : implémenté, pas encore validé sur le matériel.
- Pas d'IMU : pas de secousse, de sieste face cachée ni d'horloge paysage. Pas de gestion de batterie dans le firmware.
- Les personnages GIF ne sont pas fournis (les GIF « bufo » du dépôt d'origine sont des œuvres de tiers) ; voir
  `tools/prep_character.py` et `tools/flash_character.py`.

## Licence et crédits

Code basé sur [anthropics/claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy)
(licence MIT, © 2026 Anthropic, PBC) : voir [LICENSE](LICENSE). Clawd est la mascotte de Claude Code ;
le dessin pixel-art de ce dépôt est une réinterprétation non officielle.
