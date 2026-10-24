# c-spreadsheet

Mini-planilha em C para o console do Windows: grade de 20 linhas × 10 colunas, navegação pelo teclado, edição de células e interface retrô desenhada direto no buffer do console (sem flicker).

Recriação de um trabalho de Linguagem de Programação da FAAT (2º semestre de 2008 / início de 2009).

## Compilar e rodar

Precisa do Windows (usa `conio.h` e a API de console do `windows.h`) e do MinGW:

```bash
gcc -Wall -O2 -o spreadsheet.exe spreadsheet.c
./spreadsheet.exe
```
