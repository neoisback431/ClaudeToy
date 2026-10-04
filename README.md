# ClaudeToy

Un petit compagnon physique pour Claude Desktop : un écran rond avec **Clawd**, la mascotte pixel-art de
Claude Code, qui réagit à ce que fait Claude (au travail, en attente de validation, terminé…) et te laisse
**approuver ou refuser** les demandes de permission depuis un bouton tactile.

*A tiny physical Claude Desktop companion on an ESP32-S3 + round GC9A01 display, with touch buttons to approve or deny
permission prompts. Port of the official [claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy)
(M5StickC Plus) to other hardware, with a 3D-printable case.*

<table>
  <tr>
    <td align="center"><img src="images/3dFront.png" alt="Vue de face du boîtier" width="360"><br><sub>Face avant : l'écran rond sur la face inclinée</sub></td>
    <td align="center"><img src="images/3dBack.png" alt="Vue en coupe du boîtier" width="440"><br><sub>Vue arrière (trappe retirée) : ESP32-S3 et modules dans le corps</sub></td>
  </tr>
</table>

> Projet communautaire non officiel, sans lien avec Anthropic. La fonction « Hardware Buddy » de Claude Desktop est
> réservée au mode développeur et n'est pas une fonctionnalité officiellement supportée.

## Ce que ça fait

<p align="center"><img src="images/clawd_states.png" alt="Les 7 états de Clawd" width="720"><br><sub>Les sept états de Clawd (rendu illustratif de l'animation du firmware)</sub></p>

- **Clawd réagit à Claude** : il dort, attend, travaille, s'inquiète quand une permission est demandée, fête une réussite…
- **Approbation physique** : tap pour approuver, un autre geste (ou un 2e bouton) pour refuser.
- **Écran de veille** : heure et date, tokens du jour, niveau et progression, sessions en cours/en attente,
  état du Bluetooth, dernier message de Claude.
- **Bluetooth LE chiffré** avec appairage par code à 6 chiffres affiché à l'écran.

## Nomenclature (BOM)

| Qté | Composant | Remarque |
| --- | --------- | -------- |
| 1 | ESP32-S3 Super Mini | USB natif et BLE 5. Ne convient pas : ESP32-C3 (pas de tactile natif, moins à l'aise) ni ESP8266 (pas de BLE) |
| 1 | Écran rond GC9A01 1,28" SPI, 240×240 | Versions à 7 ou 8 broches ; la broche BLK (rétroéclairage) est facultative |
| 1 à 2 | Module tactile capacitif TTP223 | 1 en mode 1 bouton, 2 en mode 2 boutons |
| 1 | Câble USB-C (données) | Alimentation et flash |
| — | Fils souples ou Dupont, barrettes à souder | 7 à 8 fils pour l'écran, 3 par module tactile |
| — | Fixations (vis, colle) | Selon ton montage |
| 1 | Corps principal imprimé en 3D | [`3dParts/Corps principal.stl`](3dParts/Corps%20principal.stl) |
| 1 | Trappe imprimée en 3D | [`3dParts/Trappe.stl`](3dParts/Trappe.stl) |

Optionnel, **non géré par le firmware** : accu Li-ion rechargeable et chargeur, schéma dans [BATTERY.md](BATTERY.md).

## Câblage

<p align="center"><img src="docs/wiring.svg" alt="Schéma de câblage" width="760"></p>

| Écran GC9A01 | ESP32-S3 |   | Module tactile | ESP32-S3 |
| ------------ | -------- | - | -------------- | -------- |
| VCC | 3V3 | | A : OUT | GPIO 4 |
| GND | GND | | B : OUT (optionnel) | GPIO 2 |
| SCL | GPIO 5 | | VCC (A et B) | 3V3 |
| SDA | GPIO 6 | | GND (A et B) | GND |
| CS | GPIO 7 | | | |
| DC | GPIO 8 | | | |
| RST | GPIO 9 | | | |
| BLK (si présent) | GPIO 10 | | | |

Détails et conseils : [WIRING.md](WIRING.md).

## Boîtier 3D

| Fichier | Contenu |
| ------- | ------- |
| [`3dParts/Corps principal.stl`](3dParts/Corps%20principal.stl) | Corps cylindrique, environ 60 × 60 × 74 mm, face supérieure inclinée pour l'écran |
| [`3dParts/Trappe.stl`](3dParts/Trappe.stl) | Trappe d'accès, environ 22 × 58 × 71 mm |
| [`3dParts/claude_toy.f3z`](3dParts/claude_toy.f3z) | Projet complet Fusion (archive) |
| [`3dParts/claud_toy.f3d`](3dParts/claud_toy.f3d) | Modèle Fusion |

Les dimensions ci-dessus sont lues dans les fichiers STL. Les fichiers Fusion permettent d'adapter le boîtier
(emplacement des modules, taille de la carte).

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
