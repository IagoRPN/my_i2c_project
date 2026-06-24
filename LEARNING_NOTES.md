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

## STATUS: ✅ "Hello World!" na tela. Objetivo inicial concluído.

## Pilha de funções construída (arquitetura em camadas)
```
app_main → i2c_bus_init / i2c_device_init / lcd_init / lcd_print   (API pública, no header)
lcd_print → lcd_send_data ─┐
lcd_init  → lcd_send_command, lcd_set_4_bit_mode ┘
            lcd_send_data/command → lcd_send_byte → lcd_write_nibble → lcd_pulse_enable → pcf8574_write → I2C
```

## Ideias para continuar (próximos passos opcionais)
- [ ] `lcd_set_cursor(dev, col, row)` — comando 0x80 | endereço. Linha 0 começa em 0x00, linha 1 em 0x40.
      (ex.: escrever na 2ª linha do 16x2.)
- [ ] `lcd_clear(dev)` exposto na API (comando 0x01 + delay >= 2ms).
- [ ] Caracteres customizados via CGRAM (comando 0x40 + 8 bytes de bitmap) — ex.: desenhar um ícone.
- [ ] Refatorar: agrupar os comandos em #defines com nome (LCD_CMD_CLEAR 0x01, etc.) em vez de números crus.
- [ ] (avançado) Empacotar como handle/struct (`lcd_handle_t`) guardando dev + estado do backlight,
      em vez de passar `dev` em toda função.

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

## Próximo passo do professor ao retomar
Objetivo inicial (Hello World) CONCLUÍDO. Se o aluno quiser continuar, escolher um item da
seção "Ideias para continuar" acima — sugestão natural: `lcd_set_cursor` para escrever na
2ª linha, depois caracteres customizados (CGRAM).
