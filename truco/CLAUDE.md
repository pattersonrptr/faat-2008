# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and run

Maven is **not in the system PATH**. Always use the wrapper:

```cmd
mvnw.cmd compile          # compile only
mvnw.cmd test             # run all tests
mvnw.cmd javafx:run       # launch the game
```

To run a single test class:
```cmd
mvnw.cmd test -Dtest=DeckTest
```

The wrapper sets `JAVA_HOME` (JDK 25, Eclipse Adoptium) and `MAVEN_HOME` automatically. Source/target is Java 21 in `pom.xml`, but the runtime JDK is 25 — Java 25 compatibility warnings from JavaFX/Maven internals are harmless noise.

## Architecture

Four layers, strictly separated:

```
core/      Pure game logic — no UI imports
variants/  TrucoVariant implementations
ai/        AI strategy — depends on core only
ui/        JavaFX — depends on everything above
```

### core/game — the engine

`GameController` is the single boundary between game logic and UI. The UI calls:
- `startRound()` — deal cards and begin a new hand
- `humanPlayCard(Card)` — play a card on the human's turn
- `humanRespondToBid(BidResponse)` — respond to an AI Truco call
- `humanCallTruco()` — human proactively calls Truco

`GameController` emits `GameEvent` objects to all registered `Consumer<GameEvent>` listeners. All events fire synchronously on the JavaFX Application Thread (game logic runs inside FX click handlers — no threading needed).

The bid-pause flow: when an AI calls Truco and the human must respond, `GameController` sets `waitingForHumanBidResponse = true` and emits `BID_DECISION_REQUIRED`. `advanceIfAI()` won't continue until `humanRespondToBid()` is called.

`Match → Round → Trick` is the state hierarchy. A `Round` tracks whose turn it is via `getCurrentPlayer()`, which uses `leadPlayerIndex` + `playsInCurrentTrick` to index into the ordered players list.

### Seating convention

Player list order is fixed throughout the match:
```
players[0] = Human      (bottom of table)
players[1] = OpponentA  (left)
players[2] = Partner    (top)
players[3] = OpponentB  (right)
```
`TrickArea` and `GameTableScreen` use this index directly — changing it breaks UI positioning.

### variants/ — adding a new Truco variant

Implement `TrucoVariant`:
- `cardStrength(Card, Card vira)` — returns an integer; higher = stronger. Pass `vira = null` if the variant has fixed manilhas (like Paulista). The UI highlights cards with strength ≥ 11 as manilhas (`CardView`).
- `getTrucoBidSequence()` — ordered list of `BidCall` records; each has `name()`, `pointsIfAccepted()`, `pointsIfRejected()`.
- `hasEnvido()` — Envido bidding is not yet implemented; return `true` once it is.

Register the new variant in `MenuScreen.buildVariantRow()` and add its description to `variantDescription()`.

### ai/

`AIStrategy` has three methods. `BasicAIStrategy` is purely rule-based (no look-ahead). The hand-strength thresholds for calling/accepting/running Truco are constants in that class — tweak them to change AI aggression.

`GameContext` is the read-only view the AI receives. It contains helpers like `isPartnerWinningCurrentTrick()` and `myHandStrength()` — add derived queries there rather than computing them inside strategy classes.

### ui/

`JTrucoApp` owns scene switching (Menu → Table). `GameTableScreen` listens to `GameController` events and refreshes components. The CSS (`src/main/resources/com/jtruco/ui/style.css`) drives all styling — no inline styles in Java code except layout geometry.

`CardView` renders face-up or face-down cards. Clickable cards (human's hand on their turn) are constructed with an `onClick` `Runnable`; non-clickable ones omit it.

## What is not yet implemented

- Truco Mineiro (needs vira-based `cardStrength`) and Truco Argentino (needs Envido)
- Trick count / hand history display (`trickCountLabels` array exists in `GameTableScreen` but is unused)
- Animations between tricks (currently clears instantly)
- Human bid-raise path when human calls Truco and AI raises back
