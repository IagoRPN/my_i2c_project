# LCD I2C (ESP32-S3 + PCF8574T) — Notas de Aprendizado / Handoff

> Registro da sessão de estudo guiada (modo "professor": **o aluno escreve todo o
> código**; o Claude apenas explica, revisa e guia — não edita arquivos de código).
> Use este arquivo para retomar de outra máquina, acompanhando os commits.

## Configuração confirmada
- Toolchain: **ESP-IDF v5.5.4** → driver **NOVO** `driver/i2c_master.h` (não o legado `driver/i2c.h`).
- MCU: ESP32-S3.
- Fiação: **SDA = GPIO14**, **SCL = GPIO13**. VCC do módulo em **5V**, GND comum.
- Expansor: **PCF8574T**, endereço **0x27**. LCD: **HD44780 16x2**, modo **4-bit**.

---

# ✅ CONCLUÍDO: driver `lcd_i2c` reutilizável (Hello World funcionando)

Componente `lcd_i2c` profissional: handle opaco, estado encapsulado, bus injetado, erros propagados.

## API pública (`components/lcd_i2c/inc/lcd_i2c.h`)
```c
typedef struct lcd_i2c_t *lcd_i2c_handle_t;   // handle OPACO
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
(O código original do Hello World está comentado no fim de `main/my_lcd_i2c_project.c`.)

## Conceitos-chave já aprendidos (não reexplicar)
- **Padrão de driver I2C reutilizável** (vale pra QUALQUER chip): handle opaco + config struct com bus
  injetado + `create`/`delete` (calloc/free) + internas `static` + API pública `esp_err_t`. Bus pertence à app.
- Em C, statements só dentro de funções; handles do IDF saem por ponteiro de saída + retorno `esp_err_t`.
- `| bit` liga; `& ~bit` desliga. PCF8574 = 1 byte → 8 pinos; HD44780 lê na borda de descida do EN.
- `i2c_master_probe` p/ scan; `i2c_master_transmit(dev,&buf,len,timeout)` p/ escrever.
- `idf.py flash monitor`; `vTaskDelay(pdMS_TO_TICKS(ms))`.

---

# 🟡 FOCO ATUAL #1: Relógio NTP no LCD (WiFi + SNTP + timezone SP)

**Objetivo da aplicação:** conectar no WiFi → sincronizar via NTP → mostrar a hora (timezone
São Paulo) no LCD. (O projeto INA228 foi ADIADO — sem o chip em mãos; ver fim do arquivo.)

## Conceito central NOVO: modelo ASSÍNCRONO (já destravado na sessão)
- Diferente do I2C (síncrono), conectar no WiFi é assíncrono: você "pede" e a resposta vem DEPOIS, por EVENTOS, em outra task.
- **Event loop** (`esp_event`) = central de mensagens (fila). Tasks de WiFi/IP postam eventos lá.
- **Handler/callback** = função que VOCÊ registra e o sistema chama (inversão de controle — você nunca a chama).
- **Event Group do FreeRTOS** = ponte de volta pro síncrono: `app_main` faz `xEventGroupWaitBits` (dorme), o handler faz `xEventGroupSetBits` (acende o bit), a `app_main` acorda.
- **Analogia do restaurante:** pedir = `esp_wifi_connect`; sentar e esperar = `WaitBits`; pager vibrar = evento;
  comida pronta = **`IP_EVENT_STA_GOT_IP`** (só aí tem IP — `WIFI_EVENT_STA_CONNECTED` é só "associou", SEM IP ainda).
- Eventos vêm de 2 famílias (event bases) diferentes: `WIFI_EVENT` (rádio) e `IP_EVENT` (esp_netif/lwIP) → registrar handler pras DUAS.

## Stack WiFi em camadas
```
app_main (espera no Event Group)
 → esp_event (event loop: WIFI_EVENT_* e IP_EVENT_* → handlers)
 → esp_wifi (driver do rádio: modo STA, SSID/senha, connect/start)
 → esp_netif (cola driver no TCP/IP; roda DHCP, dá o IP)
 → lwIP (TCP/IP) | NVS (flash; init ANTES do WiFi)
```

## Ordem de init canônica do WiFi
```
nvs_flash_init → esp_netif_init → esp_event_loop_create_default
→ esp_netif_create_default_wifi_sta → esp_wifi_init → registrar handlers (WIFI + IP)
→ esp_wifi_set_mode(WIFI_MODE_STA) → esp_wifi_set_config → esp_wifi_start
```

## ⏸️ STATUS: FOCO #1 EM PAUSA (priorizando o marquee, FOCO #2).
O `app_main` foi repurposado pro demo do marquee — a fundação WiFi (NVS/netif/event loop) foi
REMOVIDA do `app_main`. Ao voltar pro relógio NTP, recolocar a fundação (os includes wifi/netif/
event ainda estão lá, mas sem uso). Retomar pelo Roadmap abaixo.

## Roadmap
1. [ ] **Fundação de boot** (NVS + netif + event loop) — tinha sido escrita, mas SAIU do app_main; refazer.
2. [ ] Sequência WiFi + handlers (4 eventos: STA_START→connect, CONNECTED, GOT_IP, DISCONNECTED→retry).
3. [ ] Event group: `app_main` espera o IP de forma limpa.
4. [ ] SNTP (`esp_netif_sntp_*`, servidor `pool.ntp.org`) — também assíncrono (esperar o sync).
5. [ ] Timezone SP: `setenv("TZ", ...)` + `tzset()` + `localtime_r` + `strftime`. **SP não tem mais horário de verão** (abolido 2019) → TZ string simples, SEM regra de DST.
6. [ ] Juntar com o LCD: mostrar HH:MM:SS, atualizando.

## Decisões já tomadas
- SSID/senha via **Kconfig/menuconfig** (FEITO): `main/Kconfig.projbuild` define `CONFIG_WIFI_SSID` e `CONFIG_WIFI_PWD`. Default do PWD é placeholder (`Pwd@123`), senha real só no `sdkconfig` local.
- **`sdkconfig` NÃO é versionado** (está no `.gitignore`) — cada máquina roda `menuconfig` p/ pôr a senha. ⚠️ Ao retomar de outra máquina: rodar `idf.py menuconfig` e preencher SSID/senha.
- `main/CMakeLists.txt`: `REQUIRES lcd_i2c nvs_flash esp_wifi esp_netif esp_event` (FEITO).

## Conceitos novos aprendidos nesta etapa
- **Kconfig**: `main/Kconfig.projbuild` (sufixo `.projbuild` = aparece no topo do menuconfig). `config NOME` vira `CONFIG_NOME` no C via `sdkconfig.h` (auto-incluído). `menuconfig` salva no `sdkconfig`.
- **Resolução de headers no IDF**: C não tem "imports"; `#include` é só texto. Um header só entra no caminho se o componente dele estiver no `REQUIRES`/`PRIV_REQUIRES`. Componentes "comuns" (`log`, `freertos`, `heap`...) são auto-incluídos; o resto (`nvs_flash`, `esp_wifi`...) você declara. Confie no `idf.py build`, não nos squigglies do editor (regenerar índice: `idf.py reconfigure`).
- `nvs_flash_init`: tratar `ESP_ERR_NVS_NO_FREE_PAGES`/`ESP_ERR_NVS_NEW_VERSION_FOUND` → `nvs_flash_erase()` + re-init.

## ⚠️ Lições da Fundação (aplicar ao refazer o boot WiFi)
Quando a fundação existia no `app_main`, tinha estes problemas — corrigir na reescrita:
- "Loga o erro e continua" nos `if (err != ESP_OK)` — usar `ESP_ERROR_CHECK` (fail-fast, recomendado) OU dar `return` após o log. Não cair pro próximo passo com erro.
- Checar o 2º `nvs_flash_init()` (após erase).
- Não prender o log de sucesso no `else` de uma só chamada. Typo: "Fundations"→"Foundations".

---

# 🟡 FOCO ATUAL #2 (paralelo): feature `lcd_i2c_marquee` (texto rolando / "ticker")

Define uma região de UMA linha do LCD (row, col_start..col_end) e rola um texto de qualquer
tamanho dentro dela, em loop contínuo, a `n` casas/segundo. Mora no componente `lcd_i2c`
(`components/lcd_i2c/src/marquee.c` + protótipos no `lcd_i2c.h`).

## Arquitetura escolhida: Opção B — **task de fundo + mutex** (concorrência de verdade)
- `marquee_create` faz `xTaskCreate` de uma task que rola pra sempre e retorna na hora.
- **Mutex compartilhado**: o lock protege o RECURSO (o LCD), então NÃO pode ser privado do marquee —
  a app cria UM mutex e passa pro marquee E usa nas próprias escritas. A task dá `Take`/`Give`
  envolvendo o BLOCO inteiro (`set_cursor` + `print`), senão não é atômico.
- **Shutdown limpo** (Fase 3): `volatile bool running` + done-semaphore; `delete` para a task e ESPERA
  ela sair antes de `free` (senão use-after-free). Liberar na ordem inversa.

## Plano incremental
- **Fase 1 — ✅ FEITA:** `create` + `marquee_task` rolando pra sempre. Texto rola na tela.
- **Fase 2 — ✅ FEITA:** mutex compartilhado. Provada a corrida (sem lock corrompe; com lock limpo, mesmo
  com `vTaskDelay(10)` artificial entre `set_cursor` e `print` pra alargar a janela). Lição central:
  o mutex é uma CONVENÇÃO — só protege se TODOS pegarem o MESMO bastão (bug clássico: passar NULL pro
  marquee e criar o mutex depois → só um lado travava → corrompia).
- **Fase 3a — ✅ FEITA:** `delete` cooperativo (running=false → Take(done) → vSemaphoreDelete → free text+m).
  Testado: texto congela, tela limpa, sem panic. Shutdown limpo confirmado.
- **Fase 3b — 🟡 PRÓXIMA:** `set_text` (trocar o texto em tempo real, sob lock).

## Contrato de API (Opção B)
```c
typedef struct lcd_i2c_marquee_t *lcd_i2c_marquee_handle_t;   // handle OPACO (ponteiro!)
esp_err_t lcd_i2c_marquee_create(lcd_i2c_handle_t lcd, SemaphoreHandle_t lcd_mutex,
                                 uint8_t row, uint8_t col_start, uint8_t col_end,
                                 uint8_t chars_per_sec, const char *initial_text,
                                 lcd_i2c_marquee_handle_t *out);
esp_err_t lcd_i2c_marquee_set_text(lcd_i2c_marquee_handle_t m, const char *text);
esp_err_t lcd_i2c_marquee_delete(lcd_i2c_marquee_handle_t m);
```

## Algoritmo do scroll
- `W` = `col_end - col_start + 1` (largura da janela). `SB` = texto + separador (`MARQUEE_GAP`=3 espaços), comprimento `M = strlen(texto)+GAP`.
- Offset `o` em 0..M-1. Janela = `SB[(o+k) % M]` p/ k=0..W-1; posições `>= strlen` viram espaço (o gap).
- Desenhar: `set_cursor(col_start,row)` UMA vez + escrever W chars. Avançar `o=(o+1)%M`. `vTaskDelay(period_ms)`, `period_ms=1000/chars_per_sec`.
- Só redesenhar quando o offset muda. Buffer local `char win[W+1]` terminado em `\0`.

## Passos do `create` (idioma C: alocar em etapas + cleanup em cascata)
1. Validar args → `ESP_ERR_INVALID_ARG`. 2. `calloc` do handle (zera tudo) → `ESP_ERR_NO_MEM`.
3. Copiar texto (`malloc`+`strcpy`), cleanup do handle se falhar. 4. Preencher campos escalares.
5. **`xTaskCreate` POR ÚLTIMO** (a task começa a rodar já; struct tem que estar pronto), cleanup se `!= pdPASS`.
6. `*out = m; return ESP_OK;`

## ✅ Limpeza da Fase 2: FEITA
- O `vTaskDelay(pdMS_TO_TICKS(10))` artificial entre `set_cursor` e `print` (usado pra revelar a
  corrida) já foi removido. `create()` está completo e correto; task usa o mutex. Scroll fluido.
- Estado atual do demo (`app_main`): bus I2C + LCD + `marquee_create` (região row0, col 8..14, 5 c/s)
  + loop escrevendo "cnt: N" na linha 1 sob o MESMO mutex. Fase 1+2 provadas e commitadas.

## ✅ Fase 3a — `delete` limpo (CONCLUÍDA)

### Lições da depuração (guardar)
- **Bug do double-free:** `delete(mq)` foi chamado DENTRO de um `while(1)` → 2ª volta operou sobre handle
  já liberado. Sintoma: panic `assert failed: xQueueSemaphoreTake ... (pxQueue->uxItemSize == 0)` no
  `xSemaphoreTake(m->done)` — o `done` lido de memória liberada não era mais um semáforo válido.
  Regra: `delete` é operação ÚNICA, fica FORA de loop.
- Ler backtrace de baixo p/ cima (quem chamou quem); `vSemaphoreDelete` (não `free`) p/ objetos FreeRTOS;
  `vTaskDelay(N)` é em TICKS — usar `pdMS_TO_TICKS(N)`.

### Decisão de design (deliberada)
- `marquee_delete` **NÃO limpa** a região no LCD (deixa o último frame). Motivos: `lcd_i2c_clear` apagaria
  a tela INTEIRA (mataria o relógio da outra linha); limpar só a região seria política de apresentação que
  o chamador pode não querer. Se um dia quiser, opção preferida = flag `clear_region` no delete, limpando
  só `width` espaços em `col_start/row`, SOB o mutex, entre o `Take(done)` e o `free`.

### (Referência) plano que foi seguido
**Problema:** `vTaskDelete(task)` + `free(m)` cru é perigoso pois a task roda em paralelo:
(1) use-after-free — a task pode estar lendo `m->text`/`m->lcd` quando der `free`;
(2) se matar a task enquanto ela SEGURA o mutex do LCD, o bastão nunca volta → deadlock no app_main.
`vTaskDelete` de outra task é abrupto/assíncrono — não controla EM QUE PONTO ela morre.

**Solução: shutdown cooperativo (pedir → esperar → liberar).**
1. Campos novos na struct: `volatile bool running;` (volatile pq outra task altera) + `SemaphoreHandle_t done;`.
2. `create`: `m->running = true;` e `m->done = xSemaphoreCreateBinary();` (checar NULL → cleanup text+m).
3. `marquee_task`: trocar `while(1)` por `while (m->running)`. APÓS o loop:
   `xSemaphoreGive(m->done); vTaskDelete(NULL);` (a task se autodeleta e dá o "recibo").
4. `lcd_i2c_marquee_delete(m)`:
   ```c
   if (m == NULL) return ESP_ERR_INVALID_ARG;
   m->running = false;                     // pede pra parar
   xSemaphoreTake(m->done, portMAX_DELAY); // ESPERA a task confirmar que saiu
   vSemaphoreDelete(m->done);
   free(m->text); free(m);                 // seguro agora; ordem inversa da alocação
   return ESP_OK;
   ```
**Ownership (regra de ouro):** liberar SÓ o que o marquee alocou — `m->done`, `m->text`, `m`.
NÃO liberar `m->mutex` nem `m->lcd` (são do app_main; quem aloca, libera).
**Declarar `lcd_i2c_marquee_delete` no header** (já está no contrato de API acima).
**Teste no main:** `vTaskDelay(8000)` → `lcd_i2c_marquee_delete(mq)` → `lcd_i2c_clear` + print "parado ok".
Sucesso = texto congela, tela limpa, sem panic/reboot.

**Pergunta em aberto (responder ao retomar):** por que, após `m->running=false`, a task pode levar até
~1 `period_ms` pra sair? E por que `delete` não pode dar `free(m)` logo após setar a flag?

## ⬜ Fase 3b — `set_text` (depois da 3a)
Trocar o texto em runtime. Cuidado: `m->text` é lido pela task → a troca (alocar novo buffer, copiar,
recalcular `scroll_len`, trocar o ponteiro, liberar o antigo) precisa ser protegida. Detalhe a resolver:
hoje `text_len` é calculado UMA vez fora do loop da task → ao trocar o texto fica stale. Opções: recalcular
`strlen` dentro do loop, ou guardar `text_len` na struct e atualizar junto na troca, tudo sob lock.

---

# ⏸️ ADIADO: driver INA228 (monitor de corrente/potência I2C)
Retomar quando tiver o chip. Reaproveita o padrão de driver (handle opaco + bus injetado + create/delete + esp_err_t).
NOVO seria: registradores via `i2c_master_transmit_receive`, dados multi-byte big-endian, raw→unidade física
(LSB/escala, `SHUNT_CAL`), complemento de dois. 1º passo: ler `MANUFACTURER_ID` (0x3E, deve dar "TI").

## Convenção mantida
Modo "professor": o aluno escreve TODO o código; Claude só ensina/revisa.
