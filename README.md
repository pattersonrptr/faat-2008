# JTruco

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Java](https://img.shields.io/badge/Java-21%2B-orange?logo=openjdk)](https://adoptium.net)
[![JavaFX](https://img.shields.io/badge/JavaFX-21.0.2-brightgreen?logo=java)](https://openjfx.io)
[![Build](https://img.shields.io/badge/Build-Maven-red?logo=apachemaven)](https://maven.apache.org)
[![Status](https://img.shields.io/badge/Status-In%20Development-yellow)]()

A Brazilian Truco card game built with JavaFX. Play Truco Paulista against three AI opponents with full bid/raise/run mechanics.

---

## Features

- **Truco Paulista** — fixed manilhas (Zap, Copas, Espadilha, Ouros), 12-point match
- **4-player partnership** — you and a partner vs. two opponents
- **Full bidding system** — Truco → Retruco → Vale Quatro, with accept / raise / run
- **Rule-based AI** — opponents call, raise, and fold based on hand strength
- **Dark-themed UI** — green felt table, card highlights for manilhas
- **Extensible variant system** — add new Truco variants by implementing one interface

---

## Card hierarchy (Paulista)

| Strength | Card | Name |
|---|---|---|
| 1st (highest) | 4♣ | Zap |
| 2nd | 7♥ | Copas |
| 3rd | A♠ | Espadilha |
| 4th | 7♦ | Ouros |
| 5th–14th | 3 · 2 · A · K · J · Q · 7 · 6 · 5 · 4 | — |

## Bid sequence

| Call | Points if accepted | Points if opponent runs |
|---|---|---|
| Truco | 3 | 1 |
| Retruco | 6 | 3 |
| Vale Quatro | 12 | 6 |

First team to **12 points** wins the match.

---

## Requirements

- JDK 21 or later (tested on JDK 25 / Eclipse Adoptium)
- No separate Maven installation needed — the wrapper is included

---

## Build and run

```cmd
mvnw.cmd javafx:run       # launch the game
mvnw.cmd compile          # compile only
mvnw.cmd test             # run all tests
mvnw.cmd test -Dtest=DeckTest  # run a single test class
```

> **Windows note:** use `mvnw.cmd`, not `mvn`. The wrapper sets `JAVA_HOME` and `MAVEN_HOME` automatically.

---

## Project structure

```
src/main/java/com/jtruco/
├── core/          # Pure game logic — no UI imports
│   ├── card/      # Card, Deck, Rank, Suit
│   ├── game/      # GameController, Round, Match, Trick, BidCall…
│   ├── player/    # Player, HumanPlayer, AIPlayer
│   └── team/      # Team
├── variants/
│   └── paulista/  # PaulistaVariant (TrucoVariant implementation)
├── ai/            # AIStrategy interface + BasicAIStrategy
└── ui/            # JavaFX screens and components
    ├── screens/   # MenuScreen, GameTableScreen
    └── components/# CardView, TrickArea, ActionBar, ScoreBar
```

Dependencies flow one way only: `ui → ai/variants → core`. Nothing in `core` or `ai` imports from `ui`.

---

## Adding a new variant

1. Create a class in `com.jtruco.variants.<name>` implementing `TrucoVariant`.
2. Implement `cardStrength(Card, Card vira)` — return an integer; higher = stronger. Cards with strength ≥ 11 are highlighted as manilhas.
3. Implement `getTrucoBidSequence()` — return an ordered `List<BidCall>`.
4. Register it in `MenuScreen.buildVariantRow()` and `variantDescription()`.

---

## Roadmap

- [ ] Truco Mineiro (vira-based manilhas)
- [ ] Truco Argentino (requires Envido bidding)
- [ ] Envido bidding system
- [ ] Trick history / hand score display
- [ ] Card play animations
- [ ] Human re-raise path (when AI raises back after human calls Truco)

---

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).
