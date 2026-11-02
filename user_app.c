#include <xc.h>
#include <stddef.h>

#include "user_app.h"
#include "kernel.h"
#include "message.h"
#include "uart.h"
#include "adc.h"
#include "pwm.h"
#include "sync.h"

m_queue_t message_queue = {0};
static pwm_driver_t pwm_out;

// variables for priority inheritance and communication
mutex_t uart_mutex;
mutex_t adc_mutex;
uint16_t shared_adc_value = 0;

static void test_fail(const char *message)
{
    mutex_lock(&uart_mutex);
    UART_write_new_line(message);
    mutex_unlock(&uart_mutex);

    // turns off all task leds and turns on fail led
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
    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    TRISEbits.TRISE2 = 0;
    TRISEbits.TRISE3 = 0;
    TRISEbits.TRISE4 = 0;
    
    LATEbits.LATE0 = 0;
    LATEbits.LATE1 = 0;
    LATEbits.LATE2 = 0;
    LATEbits.LATE3 = 0;
    LATEbits.LATE4 = 0;
    
    mutex_init(&uart_mutex);
    mutex_init(&adc_mutex);

    validate_message_queue();
    
    adc_init();
    
    /* UART_write_new_line(
        "ADC inicializado em AN0"
    ); */
    
    pwm_config(&pwm_out, 1000);
    pwm_start(&pwm_out);
    pwm_set_duty(&pwm_out, 0);
}

/*
 * Produtora: envia a, b, c e d repetidamente.
 */
TASK task_producer(void)
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
TASK task_consumer(void)
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

        LATEbits.LATE1 ^= 1;

        mutex_lock(&uart_mutex);
        UART_write_string("Recebido: ");
        UART_write_char(received);
        UART_write_new_line(" - OK");
        mutex_unlock(&uart_mutex);

        index++;

        if (index == 4) {
            index = 0;
        }
    }
}

TASK task_adc(void)
{
    uint16_t sample;
    while (1) {
        mutex_lock(&adc_mutex);
        
        if (adc_read(&sample)) shared_adc_value = sample;
        
        // this is an intensional busy-wait loop that simulates a priority inheritance situation
        // (it takes the mutex for a long time, so when task_pwm runs, it will fail to take the mutex
        // and task_adc will inherit its high priority)
        for(volatile uint32_t i = 0; i < 60000; i++);

        mutex_unlock(&adc_mutex);
        LATEbits.LATE2 ^= 1;
        
        task_delay(50);
    }
}

TASK task_pwm(void)
{
    uint16_t duty;
    while (1) {
        task_delay(20);
        
        mutex_lock(&adc_mutex);
        
        duty = (uint16_t)(((uint32_t)shared_adc_value * 100UL) / 1023UL);
        pwm_set_duty(&pwm_out, duty);
        
        mutex_lock(&uart_mutex);
        UART_write_string("PWM Atualizado | Duty = ");
        UART_write_int((int)duty);
        UART_write_new_line("%");
        mutex_unlock(&uart_mutex);
        LATEbits.LATE3 ^= 1;
        
        mutex_unlock(&adc_mutex);
    }
}

/* TASK task_adc_test(void)
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
} */

void validate_message_queue()
{
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
}