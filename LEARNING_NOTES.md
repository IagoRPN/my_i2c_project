# LCD I2C (ESP32-S3 + PCF8574T) — Notas de Aprendizado / Handoff

> Registro da sessão de estudo guiada (modo "professor": **o aluno escreve todo o
> código**; o Claude apenas explica, revisa e guia — não edita arquivos de código).
> Use este arquivo para retomar de outra máquina, acompanhando os commits.

## Objetivo
Aprender a stack I2C do ESP32-S3 do zero até um "Hello World" num LCD de caracteres
(HD44780) acionado por um expansor I2C **PCF8574T**.

## Configuração confirmada
- Toolchain: **ESP-IDF v5.5.4** → usar o driver **NOVO** `driver/i2c_master.h` (não o legado `driver/i2c.h`).
- MCU: ESP32-S3.
- Fiação: **SDA = GPIO14**, **SCL = GPIO13**. VCC do módulo em **5V**, GND comum.
- Expansor: **PCF8574T**, endereço **0x27** (confirmado via scan `i2c_master_probe`).
- LCD: **HD44780 16x2**, modo **4-bit**.
- Mapa de bits do byte enviado ao PCF8574 (backpack padrão):
  ```
  bit:  7   6   5   4     3        2     1     0
       D7  D6  D5  D4  BACKLIGHT  EN    RW    RS
  ```
  Defines: RS=0x01, RW=0x02, EN=0x04, BL=0x08, dados no nibble alto (<<4).

## Roadmap (passos)
1. [x] Entender hardware (3 camadas: ESP32 mestre I2C → PCF8574 → HD44780).
2. [x] Definir fiação (GPIO14/GPIO13).
3. [x] Scanner I2C — achou device em 0x27. (validou fiação + driver)
4. [x] Criar o componente `lcd_i2c` (estrutura + CMake + build/link OK).
5. [x] Camada de transporte: `pcf8574_write`, `lcd_pulse_enable`, `lcd_write_nibble`, `lcd_send_byte`.
6. [x] Sequência de init do HD44780 (`lcd_set_4_bit_mode` + comandos) e helpers `lcd_send_command`/`lcd_send_data`.
7. [x] **Hello World no LCD — FUNCIONANDO! 🎉** (`lcd_print` + `app_main` montado).

## STATUS: ✅ Driver de LCD reutilizável COMPLETO e funcionando.
"Hello World!" + 2ª linha na tela. Componente `lcd_i2c` refatorado para um driver profissional
(handle opaco, estado encapsulado, erros propagados). Pronto para copiar em outros projetos.

## API pública do componente `lcd_i2c` (em `inc/lcd_i2c.h`)
```c
typedef struct lcd_i2c_t *lcd_i2c_handle_t;   // handle OPACO (corpo da struct só no .c)
typedef struct { i2c_master_bus_handle_t bus; uint8_t address, columns, rows; uint32_t scl_speed_hz; } lcd_i2c_config_t;

esp_err_t lcd_i2c_create(const lcd_i2c_config_t *config, lcd_i2c_handle_t *out_lcd);  // bus INJETADO
esp_err_t lcd_i2c_delete(lcd_i2c_handle_t lcd);
esp_err_t lcd_i2c_clear(lcd_i2c_handle_t lcd);
esp_err_t lcd_i2c_set_cursor(lcd_i2c_handle_t lcd, uint8_t col, uint8_t row);
esp_err_t lcd_i2c_print(lcd_i2c_handle_t lcd, const char *str);
esp_err_t lcd_i2c_write_char(lcd_i2c_handle_t lcd, char c);
esp_err_t lcd_i2c_set_backlight(lcd_i2c_handle_t lcd, bool on);
```
Uso: o `app_main` cria o bus (`i2c_new_master_bus`) e injeta no `lcd_i2c_create`.

## Pilha de funções (arquitetura em camadas) — internas são `static`
```
API pública (lcd_i2c_*) → lcd_send_command / lcd_send_data → lcd_send_byte
   → lcd_write_nibble (lê lcd->backlight) → lcd_pulse_enable → pcf8574_write → i2c_master_transmit
lcd_init (static) → lcd_set_4_bit_mode + comandos (LCD_CMD_*)
```

## Melhorias do refactor (todas FEITAS)
- [x] `lcd_i2c_set_cursor` (cmd 0x80 | endereço; linha 0 = 0x00, linha 1 = 0x40) + validação de `row`.
- [x] `lcd_i2c_clear` na API (cmd 0x01 + delay 2ms).
- [x] `#define`s nomeados pros comandos (LCD_CMD_CLEAR/ENTRY_MODE/DISPLAY_ON/FUNCTION_SET/SET_DDRAM).
- [x] Handle opaco (`lcd_i2c_handle_t`) com `calloc`/`free`; backlight virou ESTADO (`lcd->backlight`).
- [x] Bus DESACOPLADO: o componente recebe o bus, não o cria (dono = aplicação).
- [x] `esp_err_t` propagado em toda a cadeia via `ESP_RETURN_ON_ERROR` (fail-fast).
- [ ] (futuro, opcional) Caracteres customizados via CGRAM (cmd 0x40 + 8 bytes de bitmap).

## Perguntas conceituais (já respondidas durante a sessão)
1. Por que `<< 4` no nibble: os dados (D4-D7) ficam no nibble ALTO do byte do PCF8574; o nibble chega no baixo.
2. `rs = 0` (LCD_RS apagado) = comando/instrução; `rs = LCD_RS` = caractere/dado.
3. Não há "parse" char→byte: em C o `char` JÁ é o byte ASCII; o glifo é mapeado pela CGROM do LCD.

## Conceitos-chave já aprendidos (para não reexplicar)
- Em C, **statements só dentro de funções**; escopo global só aceita declarações/inicializações.
- Handles do IDF saem por **ponteiro de saída** + retorno `esp_err_t` (padrão do `i2c_new_master_bus`).
- `| bit` liga um bit; `& ~bit` desliga um bit.
- O PCF8574 só espelha 1 byte → 8 pinos; toda a "inteligência" do LCD é software nosso.
- O HD44780 lê os pinos na **borda de descida do EN** (pulso de Enable).
- Modo 4-bit: 1 byte = 2 nibbles = 2 pulsos de EN.
- `i2c_master_probe` para scan; `i2c_master_transmit(dev, &buf, len, timeout_ms)` para escrever.
- `idf.py flash monitor` para gravar + ver logs; `vTaskDelay(pdMS_TO_TICKS(ms))` (não ticks crus).
- **Padrão de driver I2C reutilizável** (vale pra QUALQUER chip I2C):
  handle opaco + config struct com bus injetado + `create`/`delete` (calloc/free) +
  internas `static` + API pública `esp_err_t` propagando erro. Bus pertence à aplicação.
- Inicialização vs atribuição de struct: `= { .campo = v, ... };` (vírgulas, só na declaração)
  vs `ptr->campo = v;` (statements com `;`). Não misturar.

---

# PRÓXIMO PROJETO: driver para o INA228 (monitor de corrente/potência I2C)

O INA228 é um monitor de energia de alta precisão (ADC sigma-delta de 20 bits) com interface I2C
— mede tensão de barramento, tensão de shunt, corrente, potência, energia, carga e temperatura.
Endereço configurável via pinos A0/A1 (faixa 0x40–0x4F).

## O que REAPROVEITA do que já aprendemos
- O MESMO padrão de driver reutilizável (handle opaco + bus injetado + create/delete + esp_err_t).
- O MESMO modelo de bus: app cria o bus uma vez, `ina228_create` recebe o bus (vários devices no mesmo barramento — o LCD e o INA228 podem coexistir).
- Scan I2C para confirmar endereço, ESP_RETURN_ON_ERROR para propagar erro.

## O que é NOVO (e será o foco do aprendizado)
- I2C de verdade com REGISTRADORES: ler/escrever registradores internos do chip.
  → usar `i2c_master_transmit_receive` (escreve o ponteiro de registrador, depois lê N bytes),
    não só `i2c_master_transmit`. Esse é o conceito central novo.
- Dados multi-byte big-endian e larguras incomuns (registradores de 16, 24, até 40 bits).
- Conversão de valor cru (raw) → unidade física (mV, A, W) com LSB/escala e a calibração do shunt
  (registrador SHUNT_CAL em função do resistor de shunt e da corrente máxima).
- Números com sinal (complemento de dois) para tensão/corrente que podem ser negativas.

## Primeiro passo sugerido ao iniciar
1. Confirmar endereço com o scan (provavelmente 0x40).
2. Ler o registrador de ID/MANUFACTURER_ID (0x3E, deve devolver 'TI') como "hello world" do INA228 —
   valida a leitura de registrador antes de qualquer cálculo.
3. Só então: config, calibração do shunt, e leitura de tensão/corrente.

## Convenção mantida
Modo "professor": o aluno escreve TODO o código; Claude só ensina/revisa. Ver memória local.
