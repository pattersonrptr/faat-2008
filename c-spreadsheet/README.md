# c-spreadsheet

A spreadsheet for the Windows console, written in C: a 20 × 10 grid with keyboard navigation, cell editing and formulas with cell references, drawn straight into the console buffer so the screen never flickers.

Rebuild of a Programming Language course assignment at FAAT.

## How it works

- **Rendering.** Each frame is composed in a `CHAR_INFO` buffer in memory (character + color per position) and written to the console in a single `WriteConsoleOutputA` call. Since the console never shows a half-drawn screen, there is no flicker, unlike clearing with `system("cls")` and printing again. The grid borders use CP437 box-drawing characters.
- **Cells.** The grid is a `char ***` allocated with `malloc`, one string of up to 255 characters per cell. A cell holds either text or a number. Content that starts with `=` is a formula.
- **Formulas.** A recursive descent parser evaluates `+`, `-`, `*`, `/`, parentheses, numbers and cell references (`A1` to `J20`), respecting operator precedence:

  ```
  expression → term
  term       → factor (('+' | '-') factor)*
  factor     → primary (('*' | '/') primary)*
  primary    → number | cell reference | '(' expression ')'
  ```

  Referenced cells are evaluated recursively, so formulas can depend on other formulas. Empty cells count as `0`. An invalid expression or a division by zero shows `#ERR`. Whole results show without decimals, and the others with two.

## Controls

| Key | Navigation mode | Edit mode |
|---|---|---|
| Arrows | Move between cells | |
| `Enter` | Start editing the current cell | Confirm |
| `Esc` | Quit | Cancel |
| `Backspace` | Clear the cell | Delete a character |
| `Del` | Clear the cell | |

Example: type `10` in `A1`, `32` in `A2` and `=(A1+A2)*2` in `A3`. `A3` shows `84`.

## Building and running

Requires Windows (`conio.h` and the console API from `windows.h`) and GCC (MinGW):

```bash
gcc -Wall -O2 -o spreadsheet.exe spreadsheet.c
./spreadsheet.exe
```

## Notes

- Circular references (`A1` = `=A1`) are not detected and recurse until the stack overflows.
- Data lives only in memory. There is no saving or loading.

## License

[MIT](../LICENSE).
