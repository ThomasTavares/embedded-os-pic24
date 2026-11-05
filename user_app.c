#include <xc.h>
#include <stddef.h>

#include "user_app.h"
#include "kernel.h"
#include "scheduler.h"
#include "message.h"
#include "uart.h"
#include "adc.h"
#include "pwm.h"
#include "sync.h"

/*
 * Comandos trocados pela fila de mensagens (task_button -> task_cmd)
 */
#define CMD_NEXT_FREQ    'F'    // botao 1 (RD6): troca a frequencia do PWM
#define CMD_SWITCH_SCHED 'S'    // botao 2 (RD7): troca o escalonador (RR <-> prioridade)

/*
 * Frequencias disponiveis para o PWM (o duty continua vindo do potenciometro)
 */
static const uint16_t pwm_freqs[] = {500, 1000, 2000};
#define NUM_FREQS        3
#define FREQ_INITIAL     1      // indice inicial: 1000 Hz

#define PRINT_EVERY      5      // task_pwm imprime 1 a cada 5 atualizacoes

m_queue_t message_queue = {0};
static pwm_driver_t pwm_out;

// recursos compartilhados
mutex_t uart_mutex;                     // protege a UART
mutex_t adc_mutex;                      // protege shared_adc_value
uint16_t shared_adc_value = 0;

volatile uint8_t  pwm_freq_idx = FREQ_INITIAL;  // escrito so por task_cmd (1 byte, escrita atomica)
volatile uint8_t  adc_prio_seen = 0;    // prioridade de task_adc ao final da secao critica
volatile uint32_t rr_count_a = 0;       // progresso da tarefa RR A
volatile uint32_t rr_count_b = 0;       // progresso da tarefa RR B

static const char *sched_name(uint8_t s)
{
    return (s == PRIOR_SCHEDULER) ? "PRIORIDADE" : "ROUND-ROBIN";
}

static uint16_t adc_to_duty(uint16_t adc)
{
    return (uint16_t)(((uint32_t)adc * 100UL) / ADC_MAX_VALUE);
}

static void uart_write_u32(uint32_t value)
{
    char buffer[11];
    int i = 0;

    if (value == 0) {
        UART_write_char('0');
        return;
    }
    while (value > 0) {
        buffer[i++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (i > 0) {
        UART_write_char(buffer[--i]);
    }
}

static void test_fail(const char *message)
{
    mutex_lock(&uart_mutex);
    UART_write_new_line(message);
    mutex_unlock(&uart_mutex);

    // apaga os leds das tarefas e acende o led de erro
    LATEbits.LATE0 = 0;
    LATEbits.LATE1 = 0;
    LATEbits.LATE2 = 0;
    LATEbits.LATE3 = 0;
    LATEbits.LATE4 = 1;

    while (1) {
        Nop();
    }
}

void config_user_app(void)
{
    uint16_t first = 0;

    // leds RE0..RE4 e saidas RR RE5/RE6
    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    TRISEbits.TRISE2 = 0;
    TRISEbits.TRISE3 = 0;
    TRISEbits.TRISE4 = 0;
    TRISEbits.TRISE5 = 0;
    TRISEbits.TRISE6 = 0;

    LATEbits.LATE0 = 0;
    LATEbits.LATE1 = 0;
    LATEbits.LATE2 = 0;
    LATEbits.LATE3 = 0;
    LATEbits.LATE4 = 0;
    LATEbits.LATE5 = 0;
    LATEbits.LATE6 = 0;

    // botoes em RD6 e RD7 (entradas, ativos em nivel baixo, pull-up externo)
    TRISDbits.TRISD6 = 1;
    TRISDbits.TRISD7 = 1;

    mutex_init(&uart_mutex);
    mutex_init(&adc_mutex);

    validate_message_queue();

    adc_init();
    UART_write_new_line("ADC inicializado em AN0");

    // leitura inicial do ADC para o PWM nao comecar com duty 0
    (void)adc_read(&first);
    shared_adc_value = first;

    pwm_config(&pwm_out, pwm_freqs[FREQ_INITIAL]);
    pwm_start(&pwm_out);
    pwm_set_duty(&pwm_out, adc_to_duty(first));

    UART_write_string("Escalonador inicial: ");
    UART_write_new_line(sched_name(scheduler_get()));
    UART_write_new_line("Botao RD6: frequencia do PWM | Botao RD7: troca escalonador");
}

/*
 * PRODUTORA (prio 3): le os dois botoes a cada 20 ms (debounce por amostragem)
 * e envia um comando pela fila de mensagens a cada clique.
 */
TASK task_button(void)
{
    uint8_t prev_freq = 1, prev_sched = 1;  // 1 = solto (pull-up)
    uint8_t now_freq, now_sched;

    while (1) {
        now_freq  = PORTDbits.RD6;
        now_sched = PORTDbits.RD7;

        if (prev_freq == 1 && now_freq == 0) {      // clique no botao 1
            if (!m_queue_write(&message_queue, CMD_NEXT_FREQ)) {
                test_fail("FALHA: escrita na fila");
            }
            LATEbits.LATE0 ^= 1;
        }

        if (prev_sched == 1 && now_sched == 0) {    // clique no botao 2
            if (!m_queue_write(&message_queue, CMD_SWITCH_SCHED)) {
                test_fail("FALHA: escrita na fila");
            }
            LATEbits.LATE0 ^= 1;
        }

        prev_freq  = now_freq;
        prev_sched = now_sched;
        task_delay(5);                              // 5 ticks x 4 ms = 20 ms
    }
}

/*
 * CONSUMIDORA (prio 3): bloqueia na fila ate chegar um comando e o executa.
 * Ela nao mexe no periferico PWM: so altera pwm_freq_idx, e a task_pwm
 * (unica dona do PWM) aplica a nova frequencia.
 */
TASK task_cmd(void)
{
    char cmd;
    uint8_t idx, new_sched;

    while (1) {
        if (!m_queue_read(&message_queue, &cmd)) {
            test_fail("FALHA: leitura da fila");
        }

        if (cmd == CMD_NEXT_FREQ) {
            idx = (uint8_t)((pwm_freq_idx + 1) % NUM_FREQS);
            pwm_freq_idx = idx;

            mutex_lock(&uart_mutex);
            UART_write_string("[CMD] Frequencia do PWM: ");
            UART_write_int((int)pwm_freqs[idx]);
            UART_write_new_line(" Hz");
            mutex_unlock(&uart_mutex);

        } else if (cmd == CMD_SWITCH_SCHED) {
            new_sched = (scheduler_get() == PRIOR_SCHEDULER) ? RR_SCHEDULER : PRIOR_SCHEDULER;
            scheduler_set(new_sched);

            mutex_lock(&uart_mutex);
            UART_write_string("[CMD] Escalonador: ");
            UART_write_new_line(sched_name(new_sched));
            mutex_unlock(&uart_mutex);

        } else {
            test_fail("FALHA: comando desconhecido");
        }

        LATEbits.LATE1 ^= 1;
    }
}

/*
 * PRIORIDADE BAIXA (2): le o ADC e segura o adc_mutex por um tempo longo
 * (busy-wait proposital) para provocar a heranca de prioridade quando
 * task_pwm (prio 4) tentar pegar o mesmo mutex.
 */
TASK task_adc(void)
{
    uint16_t sample;

    while (1) {
        mutex_lock(&adc_mutex);

        if (adc_read(&sample)) shared_adc_value = sample;

        for (volatile uint32_t i = 0; i < 60000; i++);

        // com heranca: 4 (se task_pwm esperou o mutex), sem heranca: 2
        adc_prio_seen = task_get_priority();

        mutex_unlock(&adc_mutex);
        LATEbits.LATE2 ^= 1;

        task_delay(50);                             // 200 ms
    }
}

/*
 * PRIORIDADE ALTA (4): unica tarefa que mexe no periferico PWM.
 * A cada 80 ms aplica a frequencia pedida (se mudou), atualiza o duty
 * pelo potenciometro e, a cada PRINT_EVERY ciclos, mostra o estado.
 */
TASK task_pwm(void)
{
    uint16_t duty;
    uint16_t adc;
    uint8_t  prio;
    uint8_t  idx;
    uint8_t  applied_idx = FREQ_INITIAL;
    uint8_t  print_count = 0;
    uint32_t a, b, last_a = 0, last_b = 0;

    while (1) {
        task_delay(20);                             // 80 ms

        mutex_lock(&adc_mutex);
        adc  = shared_adc_value;
        prio = adc_prio_seen;
        mutex_unlock(&adc_mutex);

        // troca de frequencia pedida pela task_cmd
        idx = pwm_freq_idx;
        if (idx != applied_idx) {
            pwm_stop(&pwm_out);
            pwm_config(&pwm_out, pwm_freqs[idx]);
            pwm_start(&pwm_out);
            applied_idx = idx;
        }

        duty = adc_to_duty(adc);
        pwm_set_duty(&pwm_out, duty);
        LATEbits.LATE3 ^= 1;

        // a UART a 9600 baud gasta ~1 ms por caractere: imprimir em toda
        // atualizacao ocuparia a CPU quase o tempo todo
        if (++print_count < PRINT_EVERY) continue;
        print_count = 0;

        a = rr_count_a;
        b = rr_count_b;

        mutex_lock(&uart_mutex);
        UART_write_string(scheduler_get() == PRIOR_SCHEDULER ? "[PRIO] " : "[RR]   ");
        UART_write_string("ADC=");
        UART_write_int((int)adc);
        UART_write_string(" Duty=");
        UART_write_int((int)duty);
        UART_write_string("% Freq=");
        UART_write_int((int)pwm_freqs[applied_idx]);
        UART_write_string("Hz prioADC=");
        UART_write_int((int)prio);
        UART_write_string(" RR A+");
        uart_write_u32(a - last_a);
        UART_write_string(" B+");
        uart_write_u32(b - last_b);
        UART_write_new_line("");
        mutex_unlock(&uart_mutex);

        last_a = a;
        last_b = b;
    }
}

/*
 * ROUND-ROBIN (prio 1, mesma prioridade): nunca dormem, entao ficam sempre
 * prontas. O escalonador alterna entre elas a cada tick (4 ms).
 * RE5 -> osciloscopio canal B, RE6 -> osciloscopio canal C.
 * O pino so inverte a cada 64 contagens para aliviar a simulacao.
 */
TASK task_rr_a(void)
{
    while (1) {
        if ((++rr_count_a & 0x3F) == 0) LATEbits.LATE5 ^= 1;
    }
}

TASK task_rr_b(void)
{
    while (1) {
        if ((++rr_count_b & 0x3F) == 0) LATEbits.LATE6 ^= 1;
    }
}

void validate_message_queue()
{
    uint16_t i;
    char received;
    char expected;

    UART_init(9600);

    UART_write_new_line("=== TESTE DA FILA DINAMICA ===");

    // Teste 1: criar o buffer
    if (!m_queue_init(&message_queue)) {
        test_fail("FALHA: primeira alocacao");
    }
    UART_write_new_line("OK: buffer alocado");

    // Teste 2: liberar e criar novamente (nenhuma tarefa usa a fila ainda)
    m_queue_destroy(&message_queue);

    if (message_queue.queue != NULL) {
        test_fail("FALHA: ponteiro nao foi zerado");
    }
    UART_write_new_line("OK: funcao de liberacao executada");

    if (!m_queue_init(&message_queue)) {
        test_fail("FALHA: segunda alocacao");
    }
    UART_write_new_line("OK: buffer alocado novamente");

    // Teste 3: encher a fila e esvaziar conferindo a ordem
    UART_write_new_line("Testando fila antes do escalonador...");

    for (i = 0; i < SIZE_MESSAGE_QUEUE; i++) {
        expected = (char)('a' + i);
        if (!m_queue_write(&message_queue, expected)) {
            test_fail("FALHA: escrita antes do escalonador");
        }
    }

    for (i = 0; i < SIZE_MESSAGE_QUEUE; i++) {
        expected = (char)('a' + i);
        if (!m_queue_read(&message_queue, &received)) {
            test_fail("FALHA: leitura antes do escalonador");
        }
        if (received != expected) {
            test_fail("FALHA: dados antes do escalonador");
        }
    }

    UART_write_new_line("OK: fila funcionando antes do escalonador");
}