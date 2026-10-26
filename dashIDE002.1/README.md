# dashIDE002.1 — Arduino IDE para as quatro boards

Variante do `dash002.1` preparada para **Arduino IDE 2.x**, com os mesmos
backends e funcionalidades. Seleciona-se o hardware por script antes de
compilar. O script só altera ficheiros deste sketch: não muda
`User_Setup_Select.h`, não copia configurações para bibliotecas globais e não
altera os projetos anteriores.

Layouts e documentação JSON:
[CYD_Display — dashboards](https://github.com/MarsWalker/CYD_Display/tree/main/dashboards).
Não há cópias de JSONs nesta pasta. O layout inicial é `simple.v5.json`.

## 1. Instalar o ambiente

Instalar o core **esp32 by Espressif Systems 3.3.7** pelo Boards Manager.
Se necessário, adicionar em Preferences > Additional Boards Manager URLs:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Bibliotecas locais necessárias, nas versões usadas pelo projeto:

| Biblioteca | Versão | Perfis |
| --- | --- | --- |
| ArduinoJson | 7.4.3 | Todos |
| TFT_eSPI | 2.5.44 | esp32_ili9341_xpt2046, esp32_ili9341_cap_touch, esp32s3_axs15231b_qspi |
| XPT2046_Touchscreen | 1.4 | esp32_ili9341_xpt2046 |
| LovyanGFX | 1.2.0 | esp32_st7789_parallel_cst820 |

As bibliotecas já fornecidas em `../libraries` podem ser usadas diretamente:
em **File > Preferences > Sketchbook location**, escolher a pasta que contém
esse `libraries` e reabrir o IDE. Por exemplo:

```text
<sketchbook>/
  libraries/
    ArduinoJson/
    TFT_eSPI/
    XPT2046_Touchscreen/
    LovyanGFX/
  dashIDE002.1/
    dashIDE002.1.ino
```

Também é possível usar bibliotecas já instaladas no sketchbook habitual,
com as versões indicadas. Os setups específicos estão em
`hardware/User_Setups` dentro deste projeto e não precisam de ser copiados
para TFT_eSPI. LVGL não é necessário.

Instalar **Python 3** para executar o seletor. No Windows, o comando pode ser
`py -3` ou `python`; no Linux, `python3`. O seletor só usa a biblioteca padrão.

## 2. Criar a configuração privada

Copiar `secrets_template.h` para `secrets.h` e preencher Wi-Fi, URL/token do
Home Assistant e, se necessário, a origem/nome do layout. Não copiar o
`secrets.h` antigo que incluía também o carregador: usar este template.
O carregador de overrides `/secrets.json` está em `config.h`.

```sh
cp secrets_template.h secrets.h
```

No Windows: `copy secrets_template.h secrets.h`.

O template está incluído; `secrets.h` e `secrets.json` estão excluídos do Git.

## Seleção com interface gráfica — select_board_gui

A mesma seleção também está disponível numa janela nativa, com os nomes
públicos e os aliases pessoais lado a lado. Mostra o perfil guardado e as
opções que deves escolher manualmente em Arduino IDE > Tools.

Linux:

```sh
./select_board_gui
```

Windows: executar ou fazer duplo clique em **`select_board_gui.cmd`**.
Alternativa nas duas plataformas:

```sh
python3 select_board_gui.py
```

No Windows pode ser `py -3 select_board_gui.py` ou `python select_board_gui.py`.
Os atalhos funcionam independentemente da pasta atual do terminal.

A janela usa **Tkinter**. No Debian/Ubuntu, se o módulo não estiver instalado:

```sh
sudo apt install python3-tk
```

No Windows, usar Python 3 com suporte Tcl/Tk. Não é necessária uma biblioteca
GUI instalada por pip. É necessária uma sessão de desktop para abrir a janela;
num terminal sem ambiente gráfico continua a estar disponível `select_board.py`.

1. Guardar e fechar o sketch no Arduino IDE.
2. Escolher o perfil na lista; os nomes entre parênteses são os aliases pessoais.
3. Consultar as opções apresentadas e carregar em **Save and Execute IDE** para guardar a seleção e abrir `dashIDE002.1.ino` no Arduino IDE.
4. Configurar Tools com as opções apresentadas e usar Verify/Upload.

**Apply selection** permite guardar sem abrir o IDE. Ambos os botões avançam o
contador de preparação. Guardar e fechar previamente um sketch já aberto evita
que o editor mantenha uma versão antiga dos ficheiros gerados.

**Copy IDE settings** copia as opções para a área de transferência. **Close**
fecha a janela. Abrir a janela, pré-visualizar outros perfis ou copiar opções
não escreve ficheiros.
O atalho Ctrl+Enter também aplica a seleção e Escape fecha a janela.

O IDE é procurado no PATH e nas localizações habituais de instalação em
Windows, Linux e macOS. Em Linux também reconhece
`/opt/arduino-ide/arduino-ide.AppImage`. Se não o encontrar, pede para escolher
o executável. **Browse...** permite escolhê-lo antecipadamente; a
escolha vale para essa sessão. A variável de ambiente `ARDUINO_IDE` também pode
indicar o caminho do executável. Se a abertura falhar ou for cancelada, a seleção
permanece guardada.

A interface chama o mesmo `select_board.select()` do comando de terminal:
não deteta a placa por USB, não altera os menus do IDE, não compila nem faz
upload. Não modifica bibliotecas nem credenciais. Mensagens de erro de escrita
são apresentadas na janela, que continua aberta para corrigir o problema.

## 3. Selecionar a board

### Identificadores públicos e aliases pessoais

Os comandos aceitam os dois nomes; ambos escolhem exatamente o mesmo ID,
backend e pinagem. A documentação pública usa os identificadores técnicos.

| Identificador público | Aliases aceites | Hardware do perfil |
| --- | --- | --- |
| `esp32_ili9341_xpt2046` | `amarela2.8` | ESP32, ILI9341 e XPT2046 |
| `esp32_st7789_parallel_cst820` | `amarela2.2` | ESP32, ST7789 paralelo e CST820 |
| `esp32_ili9341_cap_touch` | `pink` | ESP32, ILI9341 e touch capacitivo compatível FT6x36 |
| `esp32s3_axs15231b_qspi` | `black`, `preta` | ESP32-S3, AXS15231B QSPI e touch AXS15231B |

Cada perfil inclui a **pinagem específica** definida nos respetivos setups.
Confirmar também essa pinagem ao usar uma placa com o mesmo MCU/controlador.
A seleção é manual; os nomes não representam deteção automática do hardware.
Os IDs numéricos de compilação continuam os mesmos da versão anterior.

Exemplos equivalentes no Linux:

```sh
./select_board.sh esp32_st7789_parallel_cst820
./select_board.sh amarela2.2
```

No Windows:

```bat
select_board.cmd esp32s3_axs15231b_qspi
select_board.cmd preta
```

Executar apenas um dos comandos equivalentes. Os aliases também funcionam com
`build.sh` e `build.cmd`. Maiúsculas/minúsculas são normalizadas. `board.txt`,
nomes apresentados pelo firmware, pastas `build/<perfil>` / `build_out/<perfil>`
e `board-build.json` usam sempre o identificador público. Binários antigos em
pastas com nomes pessoais devem ser recompilados para criar a estrutura nova.


Guardar/fechar o sketch no IDE antes de trocar de perfil. No terminal,
na pasta do projeto, executar **um** dos comandos:

Linux:

```sh
./select_board.sh esp32_ili9341_xpt2046
./select_board.sh esp32_st7789_parallel_cst820
./select_board.sh esp32_ili9341_cap_touch
./select_board.sh esp32s3_axs15231b_qspi
```

Windows:

```bat
select_board.cmd esp32_ili9341_xpt2046
select_board.cmd esp32_st7789_parallel_cst820
select_board.cmd esp32_ili9341_cap_touch
select_board.cmd esp32s3_axs15231b_qspi
```

Ou chamar diretamente `python3 select_board.py <perfil>` / `py -3 select_board.py <perfil>`.
O seletor imprime as opções do IDE e o FQBN correspondente.
A seleção inicial distribuída é `esp32_ili9341_xpt2046`.

O script atualiza `selected_board.h`, `build_opt.h`, `board.txt` e o contador
em `build.h`/`build_nr.txt`. Não compila, não instala bibliotecas, não cria
credenciais nem faz upload. O contador avança ao executar o seletor;
Verify/Upload repetidos no IDE mantêm esse número.

## 4. Abrir no IDE e escolher Tools

Reabrir **`dashIDE002.1.ino`** e configurar:

| Perfil | Tools > Board | Outras opções |
| --- | --- | --- |
| esp32_ili9341_xpt2046 | ESP32 Dev Module | Flash Size 4MB; PSRAM Disabled; Partition Scheme Default 4MB with spiffs |
| esp32_st7789_parallel_cst820 | ESP32 Dev Module | Flash Size 4MB; PSRAM Disabled; Partition Scheme Default 4MB with spiffs |
| esp32_ili9341_cap_touch | ESP32 Dev Module | Flash Size 4MB; PSRAM Disabled; Partition Scheme Default 4MB with spiffs |
| esp32s3_axs15231b_qspi | ESP32S3 Dev Module | Flash Size 16MB; PSRAM OPI PSRAM; USB CDC On Boot Enabled; Partition Scheme 16M Flash (3MB APP/9.9MB FATFS) |

O seletor escolhe o controlador/pinos do firmware, mas **não altera os menus
Tools do IDE**. Estes menus têm de corresponder à tabela. A compilação acusa
um MCU incompatível ou ausência de PSRAM no perfil esp32s3_axs15231b_qspi.

Depois usar **Verify** e **Upload** normalmente. Confirmar porta e MAC da
placa antes do upload; outros dispositivos USB série podem estar ligados.
Não existem flags extra para escrever manualmente no IDE.

### Hardware dos perfis

- esp32_ili9341_xpt2046: ILI9341 + XPT2046, TFT_eSPI.
- esp32_st7789_parallel_cst820: ST7789 paralelo + CST820, LovyanGFX.
- esp32_ili9341_cap_touch: ILI9341 + touch capacitivo I2C, TFT_eSPI.
- esp32s3_axs15231b_qspi: AXS15231B QSPI + touch AXS15231B, esp_lcd + sprite TFT_eSPI.

## Como a adaptação funciona

O core ESP32 3.3.7 lê o `build_opt.h` do sketch e aplica essas opções às
unidades C, C++ e assembly através de um response file do compilador.
Este projeto usa-o para incluir `ide_build.h` também ao compilar bibliotecas.
Assim, TFT_eSPI recebe os mesmos pinos/controlador que o sketch, sem alterar
a sua configuração global.

`ide_build.h` separa os contextos C/C++/assembly e carrega `tft_setup.h` apenas
em C++. `board_config.h` obtém o perfil de `selected_board.h`, salvo quando
um comando CLI fornece explicitamente DASH_BOARD. Uma flag distinta por
perfil no response file força a invalidação das caches ao trocar de board.

Os caminhos incluídos são relativos ao sketch; mover a pasta não grava nem
exige corrigir caminhos absolutos. É importante reabrir o sketch após trocar
de perfil para o editor não manter uma cópia antiga dos ficheiros gerados.

Referência técnica: [platform.txt do core 3.3.7](https://github.com/espressif/arduino-esp32/blob/3.3.7/platform.txt).
`build_opt.h` contém opções de compilação, apesar da extensão `.h`: **não**
o incluir com `#include` nem convertê-lo num cabeçalho C normal.

## Compilação por CLI continua disponível

Os scripts anteriores continuam a funcionar com as bibliotecas em
`../libraries`:

```sh
./build.sh esp32_ili9341_xpt2046
```

```bat
build.cmd esp32_ili9341_xpt2046
```

Com esses scripts, o perfil passado no comando tem prioridade sobre a seleção
IDE. Para regressar ao IDE, executar novamente o seletor pretendido.
Os scripts CLI continuam a incrementar o contador a cada tentativa.

Também é possível compilar sem flags especiais, usando o mesmo caminho de
opções que o IDE. Depois de configurar o sketchbook e executar o seletor:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 .
```

Para esp32s3_axs15231b_qspi, usar o FQBN completo impresso pelo seletor.

## Testes e limites

```sh
./tests/run.sh
python3 -B tests/test_selection.py
python3 -B tests/test_gui.py
python3 -B tests/test_ide_launcher.py
```

Os testes nativos não necessitam de boards ou credenciais. A compilação para
os quatro perfis é verificada com o template numa cópia temporária, sem
flags de build adicionais nem `--libraries`; isto exercita o mecanismo usado
pelo IDE. A interface gráfica e a validação física são verificações distintas.

Mantêm-se as limitações funcionais do dashboard: posições em píxeis sem
escalamento, consultas de rede bloqueantes na tarefa de dados, interface web
sem autenticação e HTTPS com `setInsecure()` no cliente atual. O gráfico guarda
151 amostras em RAM; não escreve histórico em SD/flash.

Não inclui JSONs, credenciais, backups, logs ou binários. Os comentários em
inglês, licenças e atribuições dos drivers foram preservados. Não foi atribuído
um novo licenciamento ao código herdado.

### Validação do mecanismo IDE

Resultados de referência da adaptação IDE anterior, mantendo os mesmos IDs e
backends. Compilação em core 3.3.7, com o template sem credenciais, seleção por script
e `arduino-cli compile` sem flags adicionais nem `--libraries`:

| Perfil | Programa | RAM estática |
| --- | --- | --- |
| esp32_ili9341_xpt2046 | 1246327 B (95%) | 98044 B (29%) |
| esp32_ili9341_cap_touch | 1266239 B (96%) | 98884 B (30%) |
| esp32_st7789_parallel_cst820 | 1275295 B (97%) | 98724 B (30%) |
| esp32s3_axs15231b_qspi | 1302426 B (41%) | 97556 B (29%) |

Passaram os testes nativos e os testes do seletor, incluindo caminhos com
espaços, perfis inválidos, MCU incompatível, PSRAM ausente e prioridade do
perfil CLI. A interface gráfica do IDE não foi operada nesta verificação.
Nenhuma board foi carregada durante a adaptação.

Também passou a regressão do build.sh: compilação de esp32_ili9341_xpt2046 com a seleção
IDE ainda em esp32s3_axs15231b_qspi, confirmando que o perfil explícito do comando CLI tem
prioridade. As dependências de TFT_eSPI.cpp confirmaram o setup local correto
para cada perfil; o User_Setup_Select.h global permaneceu inalterado.

A generalização dos nomes é verificada com testes de todos os identificadores
e aliases, normalização de nomes guardados, geração de metadados e
consistência das tabelas Python/Bash/PowerShell. Os testes dos scripts Bash
usam um compilador simulado; a execução do PowerShell não é testada neste
ambiente Linux.

A interface Tkinter foi verificada visualmente e em testes com um display
virtual temporário: leitura da seleção, quatro perfis, cópia das opções,
ausência de gravações na pré-visualização, aplicação pelo seletor real e erros
de escrita. Esses testes não usam hardware nem credenciais. Quando não existe
Tkinter/display, os testes de widgets são ignorados e os helpers continuam
a ser testados. A janela não foi testada num desktop Windows.

O seletor mostra agora o caminho detetado no campo editável **Arduino IDE executable**.
Pode ser alterado diretamente ou com **Browse...**, que abre na pasta do
executável atual. A alteração vale para a sessão e é usada por **Save and Execute IDE**.
