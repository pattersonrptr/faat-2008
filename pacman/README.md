# Pac-Man Java2D

Um clone do Pac-Man clássico desenvolvido em Java puro, usando **Java2D** para todos os gráficos — sem imagens externas, sem bibliotecas de terceiros.

![Java](https://img.shields.io/badge/Java-17%2B-orange) ![License](https://img.shields.io/badge/license-MIT-blue)

## Funcionalidades

- **Pac-Man** animado (boca abre/fecha) com rotação por direção via `Graphics2D`
- **4 fantasmas** com IA fiel ao arcade original:
  - 🔴 **Blinky** — persegue diretamente
  - 🩷 **Pinky** — mira 4 tiles à frente do Pac-Man
  - 🩵 **Inky** — triangula com a posição do Blinky
  - 🟠 **Clyde** — persegue quando longe, foge quando perto
- Modo **Frightened** com contagem regressiva e efeito piscando
- **Frutas bônus** (cereja, morango e mais) desenhadas com Java2D
- **Som sintetizado** com `javax.sound.sampled` — sem arquivos de áudio externos
- **Menu de pausa** com controle individual de volume e mute para música e efeitos
- HUD com score, vidas e pontuação flutuante ao comer fantasmas

## Pré-requisitos

- Java 17 ou superior (testado com OpenJDK 25)

## Como rodar

### Compilar e executar diretamente

```bash
# Compilar
javac -d out src/main/java/com/pacman/*.java

# Executar
java -cp out com.pacman.Main
```

### Com Maven

```bash
mvn exec:java
```

## Controles

| Tecla | Ação |
|---|---|
| `W` / `↑` | Mover para cima |
| `S` / `↓` | Mover para baixo |
| `A` / `←` | Mover para esquerda |
| `D` / `→` | Mover para direita |
| `P` / `Esc` | Pausar / Retomar |
| `R` | Reiniciar (na tela de Game Over) |

### No menu de pausa

| Tecla | Ação |
|---|---|
| `↑` / `↓` | Navegar entre opções |
| `←` / `→` | Ajustar volume |
| `M` | Mutar/desmutar música |
| `E` | Mutar/desmutar efeitos |
| `Enter` | Confirmar |

## Estrutura do projeto

```
src/main/java/com/pacman/
├── Main.java          # Ponto de entrada
├── Game.java          # JFrame
├── GamePanel.java     # Game loop, renderização, colisões
├── Direction.java     # Enum de direções com dx/dy e ângulo
├── PacMan.java        # Sprite e lógica do Pac-Man
├── Ghost.java         # Sprite e IA dos fantasmas (4 personalidades)
├── Maze.java          # Labirinto 19×21, pastilhas e power pellets
├── BonusFruit.java    # Frutas bônus com sprites Java2D
├── SoundManager.java  # Síntese de som com javax.sound.sampled
└── PauseMenu.java     # Menu de pausa com controle de volume
```

## Pontuação

| Item | Pontos |
|---|---|
| Pastilha comum | 10 |
| Power pellet | 50 |
| Fantasma (1º) | 200 |
| Fantasma (2º) | 400 |
| Fantasma (3º) | 800 |
| Fantasma (4º) | 1600 |
| Cereja | 100 |
| Morango | 300 |

## Licença

MIT
