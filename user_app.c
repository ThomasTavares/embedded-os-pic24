#include <xc.h>
#include <stddef.h>

#include "user_app.h"
#include "kernel.h"
#include "message.h"
#include "uart.h"
#include "adc.h"
#include "pwm.h"

m_queue_t message_queue = {0};
static pwm_driver_t pwm_out;

static void test_fail(const char *message)
{
    UART_write_new_line(message);

    LATEbits.LATE0 = 1;
    LATEbits.LATE1 = 1;
    LATEbits.LATE2 = 1;

    while (1) {
        Nop();
    }
}

void config_user_app(void)
{
    LATEbits.LATE0 = 0;
    LATEbits.LATE1 = 0;
    LATEbits.LATE2 = 0;

    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    TRISEbits.TRISE2 = 0;
    TRISFbits.TRISF3 = 0;

    UART_init(9600);

    UART_write_new_line(
        "=== TESTE DA FILA DINAMICA ==="
    );

    /*
     * Teste 1: criar o buffer.
     */
    if (!m_queue_init(&message_queue)) {
        test_fail("FALHA: primeira alocacao");
    }

    UART_write_new_line(
        "OK: buffer alocado"
    );

    /*
     * Teste 2: liberar e criar novamente.
     *
     * Neste momento nenhuma tarefa foi iniciada,
     * portanto nao ha usuarios da fila.
     */
    m_queue_destroy(&message_queue);

    if (message_queue.queue != NULL) {
        test_fail("FALHA: ponteiro nao foi zerado");
    }

    UART_write_new_line(
        "OK: funcao de liberacao executada"
    );

    if (!m_queue_init(&message_queue)) {
        test_fail("FALHA: segunda alocacao");
    }

    UART_write_new_line(
        "OK: buffer alocado novamente"
    );
    {
    uint16_t i;
    char received;
    char expected;

    UART_write_new_line(
        "Testando fila antes do escalonador..."
    );

    /*
     * Preenche exatamente a capacidade da fila.
     * Nao tenta escrever alem dela.
     */
    for (i = 0; i < SIZE_MESSAGE_QUEUE; i++) {
        expected = (char)('a' + (i % 26u));

        if (!m_queue_write(
                &message_queue,
                expected
            )) {
            test_fail(
                "FALHA: escrita antes do escalonador"
            );
        }
    }

    /*
     * Retira todas as mensagens e confere a ordem.
     */
    for (i = 0; i < SIZE_MESSAGE_QUEUE; i++) {
        expected = (char)('a' + (i % 26u));

        if (!m_queue_read(
                &message_queue,
                &received
            )) {
            test_fail(
                "FALHA: leitura antes do escalonador"
            );
        }

        if (received != expected) {
            test_fail(
                "FALHA: dados antes do escalonador"
            );
        }
    }

    UART_write_new_line(
        "OK: fila funcionando antes do escalonador"
    );
    }

    UART_write_new_line(
        "Iniciando produtor e consumidor..."
    );
    
    adc_init();
    
    UART_write_new_line(
        "ADC inicializado em AN0"
    );
    
    pwm_config(&pwm_out, 1000);
    pwm_start(&pwm_out);
    pwm_set_duty(&pwm_out, 0);
}

/*
 * Produtora: envia a, b, c e d repetidamente.
 */
TASK task_A(void)
{
    const char letters[4] = {'a', 'b', 'c', 'd'};
    uint8_t index = 0;

    while (1) {
        if (!m_queue_write(
                &message_queue,
                letters[index]
            )) {
            test_fail("FALHA: escrita na fila");
        }

        LATEbits.LATE0 ^= 1;

        index++;

        if (index == 4) {
            index = 0;
        }

        task_delay(25);
    }
}

/*
 * Consumidora: recebe e confere a ordem das letras.
 */
TASK task_C(void)
{
    const char expected[4] = {'a', 'b', 'c', 'd'};
    uint8_t index = 0;
    char received;

    while (1) {
        if (!m_queue_read(
                &message_queue,
                &received
            )) {
            test_fail("FALHA: leitura da fila");
        }

        if (received != expected[index]) {
            test_fail("FALHA: mensagem fora de ordem");
        }

        LATEbits.LATE2 ^= 1;

        UART_write_string("Recebido: ");
        UART_write_char(received);
        UART_write_new_line(" - OK");

        index++;

        if (index == 4) {
            index = 0;
        }
    }
}

TASK task_adc_test(void)
{
    uint16_t sample;

    while (1) {
        if (adc_read(&sample)) {
            uint16_t duty;

            duty = (uint16_t)(
                ((uint32_t)sample * 100UL) / 1023UL
            );

            pwm_set_duty(&pwm_out, duty);

            UART_write_string("ADC = ");
            UART_write_int((int)sample);

            UART_write_string(" | Duty = ");
            UART_write_int((int)duty);
            UART_write_new_line("%");

            LATEbits.LATE0 ^= 1;
        } else {
            UART_write_new_line("ERRO: leitura do ADC");
        }

        task_delay(25);
    }
}