# AGENTS.md

Guidance for AI coding agents working in this repository.

## Build commands

Maven is **not in the system PATH**. Always use the wrapper script:

```cmd
mvnw.cmd compile          # compile only
mvnw.cmd test             # run all tests
mvnw.cmd test -Dtest=DeckTest  # run a single test class
mvnw.cmd javafx:run       # launch the game (JavaFX)
```

Runtime is JDK 25 (Eclipse Adoptium). `pom.xml` targets Java 21 — JavaFX/Maven compatibility warnings about Java 25 are harmless noise, not build failures.

After any non-trivial change, run `mvnw.cmd test` and confirm all tests pass before reporting the task complete. For UI changes, note that automated testing of the JavaFX layer is not set up — state this explicitly rather than claiming visual correctness.

## Layer rules — enforce strictly

```
core/      Pure game logic — zero UI imports allowed
variants/  TrucoVariant implementations — depends on core only
ai/        AI strategy — depends on core only
ui/        JavaFX — may depend on all layers above
```

Never introduce an import that violates this dependency direction. A `core/` class must never import from `ui/`, `ai/`, or `variants/`. An `ai/` class must never import from `ui/` or `variants/`.

## Key files

| File | Role |
|---|---|
| `GameController.java` | Only boundary between game logic and UI. The UI calls three methods; everything else is internal. |
| `GameContext.java` | Read-only game state view given to AI. Add derived queries here, not inside strategy classes. |
| `TrucoVariant.java` | Interface all variant implementations must satisfy. |
| `BasicAIStrategy.java` | Sole AI implementation; hand-strength thresholds are named constants at the top of the class. |
| `GameTableScreen.java` | Main play screen; subscribes to `GameEvent` and refreshes components. |
| `style.css` | All visual styling lives here (`src/main/resources/com/jtruco/ui/style.css`). No inline styles in Java except layout geometry (widths, spacing). |

## GameController API (what the UI calls)

```java
startRound()                        // deal cards, begin a new hand
humanPlayCard(Card)                 // play a card on the human's turn
humanRespondToBid(BidResponse)      // respond ACCEPT / RAISE / RUN to an AI Truco call
humanCallTruco()                    // human proactively calls Truco
```

All `GameEvent` callbacks fire synchronously on the JavaFX Application Thread. There is no background threading in the game loop.

## Bid-pause flow

When an AI calls Truco: `GameController` sets `waitingForHumanBidResponse = true` and emits `BID_DECISION_REQUIRED`. `advanceIfAI()` will not continue until `humanRespondToBid()` is called. Do not call `advanceIfAI()` directly from UI code.

## Seating convention — do not change

```
players[0] = Human      (bottom / south)
players[1] = OpponentA  (left  / west)
players[2] = Partner    (top   / north)
players[3] = OpponentB  (right / east)
```

`TrickArea` and `GameTableScreen` hard-code these indices for card positioning. Reordering them breaks the UI layout.

## State hierarchy

`Match → Round → Trick`. `Round.getCurrentPlayer()` derives the active player from `leadPlayerIndex + playsInCurrentTrick` — do not maintain a separate "current player" variable.

## Adding a new Truco variant

1. Create a class in `com.jtruco.variants.<name>/` implementing `TrucoVariant`.
2. Implement `cardStrength(Card, Card vira)` — return an integer; higher = stronger. Pass `vira = null` for fixed-manilha variants (like Paulista). Cards with strength ≥ 11 are highlighted as manilhas in `CardView`.
3. Implement `getTrucoBidSequence()` — return an ordered `List<BidCall>`; each `BidCall` carries `name()`, `pointsIfAccepted()`, `pointsIfRejected()`.
4. `hasEnvido()` — return `false` until Envido bidding is implemented.
5. Register the variant in `MenuScreen.buildVariantRow()` and add a description to `variantDescription()`.

## Adding or modifying AI behavior

Edit constants in `BasicAIStrategy` to adjust aggression thresholds. For richer logic, add helper queries to `GameContext` (keeping `BasicAIStrategy` clean). A new strategy class must implement `AIStrategy` and be wired up in `GameFactory`.

## Tests

Three test classes exist under `src/test/java/com/jtruco/core/`:
- `DeckTest` — card deck
- `BasicAIStrategyTest` — AI strategy rules
- `PaulistaVariantTest` — Paulista card-strength rules

When adding a new variant or AI strategy, add a corresponding test class. Tests do not use mocks for game state — use real `Round`/`Match` instances built via `GameFactory`.

## Not yet implemented — do not assume these exist

- **Truco Mineiro** — variant class does not exist yet; needs vira-based `cardStrength`.
- **Truco Argentino** — needs Envido bidding system.
- **Envido** — `hasEnvido()` is stubbed; no bidding logic exists.
- **Trick history display** — `trickCountLabels` array exists in `GameTableScreen` but is unpopulated.
- **Animations** — trick area clears instantly; no transition animations.
- **Human raise path** — when the human calls Truco and the AI raises back, the re-raise response UI is not wired up.

Do not add stubs, placeholders, or partial implementations for these features unless the task explicitly targets them.
