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
5. **Passo 5a (EM ANDAMENTO)** — camada de transporte (escrever byte "nos pinos"):
   - [x] defines do mapa de bits
   - [x] `i2c_bus_init(out_bus)` e `i2c_device_init(bus, out_dev)` via ponteiro de saída
   - [x] `pcf8574_write`, `lcd_pulse_enable`, `lcd_write_nibble`
   - [ ] **`lcd_send_byte`** (byte completo = nibble alto + nibble baixo) — AINDA FALTA
6. [ ] **Passo 5b** — sequência de inicialização do HD44780 (ritual de modo 4-bit) + comandos.
7. [ ] **Hello World** no LCD.

## Pendências imediatas (retomar AQUI)
No commit atual, faltam ajustes em `components/lcd_i2c/src/lcd_i2c.c` e `inc/lcd_i2c.h`:

1. **5 erros de sintaxe (typos)** introduzidos na última edição:
   - `.c` linha ~27: parêntese a mais em `i2c_new_master_bus(&bus_cfg, out_bus))` → tirar 1 `)`.
   - `.c` linha ~39: parêntese a mais em `i2c_master_bus_add_device(bus, &dev_cfg, out_dev))` → tirar 1 `)`.
   - `.c` linha ~40: `return err` → falta `;`.
   - `.h` linha ~7: declaração de `i2c_bus_init(...)` → falta `;` no fim.
   - `.h` linha ~11: declaração de `i2c_device_init(...)` → falta `;` no fim.
   (Lembrete: header termina com `;`; o `.c` termina com `{ ... }`.)
2. **Implementar `lcd_send_byte`** (a peça que falta do passo 5a):
   ```c
   static void lcd_send_byte(i2c_master_dev_handle_t dev, uint8_t value, uint8_t rs){
       lcd_write_nibble(dev, value >> 4,   rs);  // nibble alto (bits 7-4)
       lcd_write_nibble(dev, value & 0x0F, rs);  // nibble baixo (bits 3-0)
   }
   ```
3. Rodar `idf.py build` e confirmar compilação limpa.

## Perguntas conceituais em aberto (responder ao retomar)
1. Por que `lcd_write_nibble` desloca o nibble com `<< 4`?
2. Quando chamamos `lcd_send_byte` com `rs = 0` vs `rs = LCD_RS`?

## Conceitos-chave já aprendidos (para não reexplicar)
- Em C, **statements só dentro de funções**; escopo global só aceita declarações/inicializações.
- Handles do IDF saem por **ponteiro de saída** + retorno `esp_err_t` (padrão do `i2c_new_master_bus`).
- `| bit` liga um bit; `& ~bit` desliga um bit.
- O PCF8574 só espelha 1 byte → 8 pinos; toda a "inteligência" do LCD é software nosso.
- O HD44780 lê os pinos na **borda de descida do EN** (pulso de Enable).
- Modo 4-bit: 1 byte = 2 nibbles = 2 pulsos de EN.
- `i2c_master_probe` para scan; `i2c_master_transmit(dev, &buf, len, timeout_ms)` para escrever.
- `idf.py flash monitor` para gravar + ver logs; `vTaskDelay(pdMS_TO_TICKS(ms))` (não ticks crus).

## Próximo passo do professor ao retomar
Após os typos + `lcd_send_byte` compilarem: entregar a **sequência de init do HD44780**
(0x33 / 0x32 → modo 4-bit, function set 0x28, display on 0x0C, clear 0x01, entry mode 0x06,
com os delays corretos), depois `lcd_set_cursor` + `lcd_print` e finalmente o Hello World.
