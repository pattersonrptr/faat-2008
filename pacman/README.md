# pacman

A clone of the classic Pac-Man in plain Java, with every graphic drawn in **Java2D**: no external images and no third-party libraries.

Rebuild of an Object-Oriented Programming course assignment at FAAT.

![Java](https://img.shields.io/badge/Java-17%2B-orange) [![License](https://img.shields.io/badge/license-MIT-blue)](../LICENSE)

## Features

- **Animated Pac-Man** (mouth opening and closing), rotated by direction with `Graphics2D`
- **4 ghosts** with the targeting rules of the original arcade:
  - 🔴 **Blinky** chases Pac-Man directly
  - 🩷 **Pinky** aims 4 tiles ahead of Pac-Man
  - 🩵 **Inky** triangulates from Blinky's position
  - 🟠 **Clyde** chases when far away and flees when close
- **Frightened mode** with a countdown and a blinking warning
- **Bonus fruits** (cherry, strawberry and more) drawn with Java2D
- **Synthesized sound** with `javax.sound.sampled`, with no audio files
- **Pause menu** with separate volume and mute controls for music and effects
- HUD with score, lives and floating points when a ghost is eaten

## Requirements

- Java 17 or newer (tested with OpenJDK 25)

## Running

### Compile and run directly

```bash
javac -d out src/main/java/com/pacman/*.java
java -cp out com.pacman.Main
```

### With Maven

```bash
mvn exec:java
```

## Controls

| Key | Action |
|---|---|
| `W` / `↑` | Move up |
| `S` / `↓` | Move down |
| `A` / `←` | Move left |
| `D` / `→` | Move right |
| `P` / `Esc` | Pause / resume |
| `R` | Restart (on the Game Over screen) |

### Pause menu

| Key | Action |
|---|---|
| `↑` / `↓` | Move between options |
| `←` / `→` | Adjust volume |
| `M` | Mute/unmute music |
| `E` | Mute/unmute effects |
| `Enter` | Confirm |

## Project structure

```
src/main/java/com/pacman/
├── Main.java          # entry point
├── Game.java          # JFrame
├── GamePanel.java     # game loop, rendering, collisions
├── Direction.java     # direction enum with dx/dy and angle
├── PacMan.java        # Pac-Man sprite and logic
├── Ghost.java         # ghost sprites and AI (4 personalities)
├── Maze.java          # 19×21 maze, pellets and power pellets
├── BonusFruit.java    # bonus fruits drawn with Java2D
├── SoundManager.java  # sound synthesis with javax.sound.sampled
└── PauseMenu.java     # pause menu with volume control
```

## Scoring

| Item | Points |
|---|---|
| Pellet | 10 |
| Power pellet | 50 |
| Ghost (1st) | 200 |
| Ghost (2nd) | 400 |
| Ghost (3rd) | 800 |
| Ghost (4th) | 1600 |
| Cherry | 100 |
| Strawberry | 300 |

## License

[MIT](../LICENSE).
