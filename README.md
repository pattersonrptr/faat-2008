# faat-sistemas-para-internet

Rebuilds of coursework from the **Internet Systems** degree (Sistemas para Internet, a technology degree) at **FAAT — Faculdades Atibaia**, 2008–2011.

The original code was lost. I wrote these projects by hand at the time, learning each language from scratch. What is here are reconstructions made in 2026, using AI to speed up the work instead of rewriting everything manually. The original constraints were kept: plain C and Java, terminal or desktop, no external libraries.

## Projects

| Folder | Description | Language | Course |
|---|---|---|---|
| [`cdns/`](cdns/) | DNS + web server for service discovery on a local network | C | Networks |
| [`cchat/`](cchat/) | Chat over TCP sockets (local or on a network) | C | Networks |
| [`hotelier/`](hotelier/) | Hotel management in the terminal (guests, rooms, check-in/out, finance, reports) | C | Programming Language |
| [`c-spreadsheet/`](c-spreadsheet/) | Spreadsheet for the Windows console, with formulas and flicker-free rendering | C | Programming Language |
| [`truco/`](truco/) | Truco Paulista in JavaFX, with AI opponents | Java | Object-Oriented Programming |
| [`pacman/`](pacman/) | Pac-Man in Java2D, with the arcade ghost AI and synthesized sound | Java | Object-Oriented Programming |

Each folder has its own README with build instructions.

## Roadmap

`truco/` and `pacman/` are the two I plan to keep developing:

- **truco**: the game is playable (Truco Paulista with a JavaFX table). Next: other variants (Mineiro, Argentino with Envido), trick history and card animations.
- **pacman**: a custom visual theme (soccer field, original sprites).

## History

`cdns`, `cchat`, `hotelier`, `truco` (formerly `jtruco`) and `pacman` (formerly `Java-Pac-Man`) used to be separate repositories and were brought here with `git subtree`, keeping their commits.

## License

Everything in this repository is under the [MIT License](LICENSE).

Exception: the Maven Wrapper scripts in `truco/` (`mvnw`, `mvnw.cmd`, `.mvn/`) are Apache Software
Foundation code under Apache-2.0, as stated in their headers.
