# smoke_test.ps1 - Teste funcional completo do sistema
# Uso: cd hotel-management-system ; .\tests\smoke_test.ps1
#
# Fluxo: login -> quarto -> hospede -> reserva -> check-in -> servico -> checkout -> sair

param(
    [string]$ExePath = ".\hotel.exe"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Pass { param($msg) Write-Host "  [PASS] $msg" -ForegroundColor Green }
function Fail { param($msg) Write-Host "  [FAIL] $msg" -ForegroundColor Red ; $script:failed++ }

$script:failed = 0

if (-not (Test-Path $ExePath)) {
    Write-Error "hotel.exe nao encontrado em '$ExePath'. Compile: mingw32-make"
    exit 1
}

# Limpar dados anteriores
$dataDir = ".\data"
if (Test-Path $dataDir) {
    Remove-Item -Path "$dataDir\*" -Force -Recurse
}

# Datas para check-in/check-out (hoje e hoje+2)
$today    = (Get-Date).ToString("dd/MM/yyyy")
$checkout = (Get-Date).AddDays(2).ToString("dd/MM/yyyy")

Write-Host ""
Write-Host "====================================================" -ForegroundColor Cyan
Write-Host "  SMOKE TEST - Sistema de Gerenciamento Hoteleiro"   -ForegroundColor Cyan
Write-Host "  Check-in: $today   Check-out: $checkout"           -ForegroundColor Cyan
Write-Host "====================================================" -ForegroundColor Cyan
Write-Host ""

# Sequencia de entradas (uma por linha = um Enter)
$inputs = @(
    "admin",           # usuario
    "admin123",        # senha

    "1",               # main: Quartos
    "5",               # Cadastrar quarto
    "101",             # numero
    "1",               # andar
    "1",               # tipo (1=Duplo)
    "2",               # capacidade
    "150",             # diaria R$150
    "n",               # AC
    "n",               # Frigobar
    "n",               # Banheira
    "n",               # Varanda
    "n",               # Acessivel
    "",                # pause_screen
    "0",               # Voltar

    "2",               # main: Hospedes
    "3",               # Cadastrar hospede
    "Joao Silva",      # nome
    "1",               # tipo doc: 1=CPF
    "12345678909",     # CPF valido
    "01/01/1985",      # nascimento
    "M",               # sexo
    "Brasileiro",      # nacionalidade
    "99999-9999",      # telefone
    "joao@teste.com",  # email
    "Rua A, 1",        # endereco
    "0",               # tipo: 0=Regular
    "",                # pause_screen
    "0",               # Voltar

    "3",               # main: Reservas
    "5",               # Nova reserva
    "1",               # buscar por ID
    "1",               # ID do hospede
    $today,            # check-in
    $checkout,         # check-out (2 noites)
    "1",               # num quartos
    "101",             # quarto
    "2",               # adultos
    "0",               # criancas
    "",                # observacoes (pular)
    "",                # pause_screen
    "0",               # Voltar

    "4",               # main: Check-in/out
    "1",               # Realizar Check-in
    "1",               # ID da reserva
    "ok",              # documento (bypass)
    "n",               # caucao
    "n",               # acompanhantes extras
    "",                # pause_screen
    "0",               # Voltar

    "5",               # main: Servicos
    "7",               # Lancamento - Outro
    "101",             # numero do quarto
    "Cafe da manha",   # descricao
    "25",              # valor R$25
    "1",               # quantidade
    "5",               # categoria: 5=Outro
    "",                # pause_screen
    "0",               # Voltar

    "4",               # main: Check-in/out
    "2",               # Realizar Check-out
    "1",               # ID da reserva
    "0",               # pagamento: 0=Dinheiro
    "n",               # desconto
    "s",               # confirmar
    "",                # pause_screen
    "0",               # Voltar

    "0"                # main: Sair
)

# Executar hotel.exe com entradas via pipe
$inputStr = $inputs -join "`n"
$output   = $inputStr | & $ExePath 2>&1 | Out-String

# Verificacoes
Write-Host "  --- Resultados ---"

if ($output -match "Quartos|MENU PRINCIPAL|Hotel Sistema") {
    Pass "Login admin bem-sucedido"
} else {
    Fail "Login nao reconhecido"
}

if ($output -match "\[OK\].*101|Quarto 101 cadastrado") {
    Pass "Quarto 101 cadastrado"
} else {
    Fail "Quarto 101 nao cadastrado"
}

if ($output -match "\[OK\].*Hospede cadastrado|Hospede cadastrado") {
    Pass "Hospede Joao Silva cadastrado"
} else {
    Fail "Hospede nao cadastrado"
}

if ($output -match "300[,.]00|Reserva.*confirmada|\[OK\].*Reserva") {
    Pass "Reserva criada (R`$300,00)"
} else {
    Fail "Reserva nao criada"
}

if ($output -match "check-in.*concluido|\[OK\].*check-in|Check-in.*OK" -or $output -match "Check-in concluido") {
    Pass "Check-in realizado"
} else {
    Fail "Check-in nao realizado"
}

if ($output -match "25[,.]00|Cafe da manha|Servico registrado") {
    Pass "Servico 'Cafe da manha' R`$25,00 registrado"
} else {
    Fail "Servico nao registrado"
}

if ($output -match "325[,.]00") {
    Pass "Total checkout: R`$325,00"
} else {
    Fail "Total R`$325,00 nao encontrado"
}

if ($output -match "Obrigado pela hospedagem.*Joao Silva") {
    Pass "Mensagem de despedida correta"
} else {
    Fail "Mensagem 'Obrigado... Joao Silva' nao encontrada"
}

if ($output -match "Sistema encerrado|Ate logo") {
    Pass "Sistema encerrado normalmente"
} else {
    Fail "Saida normal nao detectada"
}

Write-Host ""
Write-Host "===================================================="
if ($script:failed -eq 0) {
    Write-Host "  RESULTADO: PASSOU - todos os testes OK" -ForegroundColor Green
    exit 0
} else {
    Write-Host "  RESULTADO: FALHOU ($($script:failed) verificacao(oes) falharam)" -ForegroundColor Red
    Write-Host ""
    Write-Host "  --- Saida completa (debug) ---"
    Write-Host $output
    exit 1
}
