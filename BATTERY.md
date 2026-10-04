# Alimentation sur batterie — Li-ion 16340 + chargeur TP4057

## ⚠️ À vérifier avant de brancher quoi que ce soit

- **L'accu doit être un Li-ion RECHARGEABLE 3,7 V (4,2 V pleine charge).** Les piles marquées « LS… » chez
  Saft (ex. LS 17330) sont des lithium-thionyle **non rechargeables** (3,6 V) : les mettre sur un chargeur
  est dangereux (échauffement, fuite, incendie). Cherche « Li-ion », « rechargeable » ou « 4,2 V » sur l'étiquette.
- **Protection obligatoire** : le TP4057 charge, mais ne coupe pas la décharge ni les courts-circuits. Prends un
  accu **protégé** (petit circuit intégré en bout) ou ajoute un module de protection 1S (DW01A + FS8205A).
- **Courant de charge ≤ 0,5 C** : pour ~700 mAh, vise 300 à 350 mA. La plupart des modules TP405x se règlent
  avec la résistance PROG (I ≈ 1200 V / R : 3,3 kΩ ≈ 360 mA). Vérifie la valeur sur ton module (1,2 kΩ = 1 A, trop fort).

## Principe (chemin d'alimentation)

- **Sur USB** (câble branché sur le module chargeur) : l'ESP32 est alimenté par l'USB via D1 ; le MOSFET Q1
  isole l'accu du système, qui se recharge **seul** (la charge se termine correctement).
- **Sur batterie** (USB débranché) : Q1 conduit, l'accu alimente la broche 5V de la carte (chute quasi nulle,
  contrairement à une diode).
- La carte régule ensuite en 3,3 V avec son propre régulateur : l'accu 4,2 → 3,5 V donne un 3V3 stable ; en dessous
  la tension descend doucement. Le firmware coupera vers 3,3 V pour protéger l'accu.

## Schéma

```
                     ┌──────────────────────────┐
   USB-C ───────────►│ IN+        TP4057        │
                     │ (module     chargeur     │
                     │  complet)  BAT+  BAT-    │
                     └──┬───────────┬─────┬─────┘
                        │           │     │
              VUSB ─────┤           │     └──────────────────────────────── GND commun
                        │      ┌────┴─────┐
                        │      │ PROTECTION│  (inutile si accu protégé)
                        │      │ DW01+8205 │
                        │      └────┬─────┘
                        │           │ P+  / P− (vers GND commun)
                        │      ┌────┴─────┐
                        │      │  ACCU    │  Li-ion 16340, 3,7 V
                        │      │  (+) (−) │
                        │      └──────────┘
                        │           │
                        │         [SW] interrupteur à glissière sur le +
                        │           │
                        │         BATSW ──────────┬──── R3 100k ──┬── R4 100k ── GND
                        │           │             │               └─► GPIO 1  (ADC : tension accu ÷2)
                        │           │             │                   + C1 100 nF vers GND
                        │       D = Drain         │
                        │      ┌────┴────┐        │
                        │      │   Q1    │ P-MOSFET AO3401A (ou SI2301, IRLML6401)
                        ├──────┤ G       │
                        │      └────┬────┘
                        │       S = Source
                        │           │
                        │    ┌──────┴──────────────── SYS ───┬──► carte S3 : broche 5V
                        │    │                               │
                        └─►|─┘  D1 Schottky SS14 / 1N5819    ├─ C2 100 µF (près de la carte) vers GND
                      (anode = VUSB, cathode = SYS)          │
                                                             └─► (écran GC9A01 : VCC reste sur 3V3 de la carte)

   Grille de Q1 : reliée à VUSB, plus R1 100 kΩ de la grille vers GND (tire la grille à 0 V sans USB → Q1 passant).

   Détection USB :  VUSB ── R5 100k ──┬── R6 100k ── GND
                                       └─► GPIO 3   + C3 100 nF vers GND
```

Résumé des connexions :

| De | Vers |
|----|------|
| Chargeur IN+ (5 V USB) | D1 anode, grille de Q1, R1, R5 |
| D1 cathode | SYS |
| Chargeur BAT+ | accu + (via la protection) |
| Accu + | interrupteur SW → BATSW |
| BATSW | Q1 drain, R3 |
| Q1 source | SYS |
| SYS | broche **5V** de la carte S3, C2 |
| R1 (autre bout) | GND |
| R3 / R4 (point milieu) | **GPIO 1**, C1 vers GND |
| R5 / R6 (point milieu) | **GPIO 3**, C3 vers GND |
| GND chargeur, accu −, GND carte, GND écran | tous reliés ensemble |

Le buzzer optionnel (GPIO 1) n'est plus disponible : la broche sert à mesurer l'accu.

## Pièces

- Accu Li-ion rechargeable 16340 (idéalement protégé) + support
- Module chargeur TP4057 avec prise USB (courant réglé à ~350 mA)
- (Option) module de protection 1S DW01A + FS8205A si l'accu n'est pas protégé
- Q1 : MOSFET canal P, SOT-23, ex. AO3401A, SI2301 ou IRLML6401
- D1 : diode Schottky SS14 ou 1N5819
- R1, R3, R4, R5, R6 : 5 × 100 kΩ
- C1, C3 : 2 × 100 nF ; C2 : 100 µF
- Interrupteur à glissière

## Règles d'usage

1. **Interrupteur sur OFF avant de brancher l'USB-C de la carte S3** (flash, console). Sinon le 5 V de la carte
   peut repousser du courant dans Q1 et l'accu. Pour recharger, utilise l'USB du **module chargeur**.
2. Ne jamais court-circuiter l'accu ; ne jamais le charger sans la protection.
3. Autonomie estimée : ~6 à 8 h avec Bluetooth et rétroéclairage au maximum ; l'écran s'éteint seul après 30 s
   sans interaction sur batterie (c'est déjà dans le firmware d'origine).

## L'« indicateur de charge » que tu as

Un indicateur à LED branché directement sur l'accu (BAT+/BAT−) fonctionne tel quel, sans lien avec l'ESP32.
Le firmware, lui, affichera le pourcentage mesuré sur GPIO 1.
