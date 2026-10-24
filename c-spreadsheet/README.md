# c-spreadsheet

Planilha em C para o console do Windows: grade de 20 linhas × 10 colunas, navegação por teclado, edição de células e renderização direta no buffer do console (sem flicker).

Recriação de um trabalho de Linguagem de Programação da FAAT.

## Compilar e rodar

Requer Windows (`conio.h` e a API de console do `windows.h`) e MinGW:

```bash
gcc -Wall -O2 -o spreadsheet.exe spreadsheet.c
./spreadsheet.exe
```
