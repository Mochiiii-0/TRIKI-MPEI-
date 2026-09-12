#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f401.h"
#include "gpio_config.h"

/* ---------- Configuracion de pines GPIO ---------- */

void gpio_set_output(GPIO_TypeDef *port, uint8_t pin) {
    port->MODER &= ~(3u << (pin * 2));
    port->MODER |=  (1u << (pin * 2));
}

void gpio_set_input_pullup(GPIO_TypeDef *port, uint8_t pin) {
    port->MODER &= ~(3u << (pin * 2));
    port->PUPDR &= ~(3u << (pin * 2));
    port->PUPDR |=  (1u << (pin * 2));
}

void init_gpio(void) {
    RCC->AHB1ENR |= (1 << 0) | (1 << 1) | (1 << 2);

    /* Teclado: filas como salida, columnas como entrada con pull-up */
    for (int p = 0; p <= 3; p++) gpio_set_output(GPIOC, p);
    for (int p = 4; p <= 7; p++) gpio_set_input_pullup(GPIOC, p);

    /* LEDs */
    for (int p = 0; p <= 15; p++) if (p != 2 && p != 11) gpio_set_output(GPIOB, p);
    for (int p = 6; p <= 12; p++) gpio_set_output(GPIOA, p);
}

void delay_ms(volatile uint32_t ms) {
    for (uint32_t i = 0; i < ms * 4000; i++) {
        __asm__("nop");
    }
}

/* ---------- Lectura del teclado matricial ---------- */

const uint8_t col_pins[3] = {4, 6, 5};

int8_t leer_teclado(void) {
    for (int fila = 0; fila < 3; fila++) {
        for (int f = 0; f < 4; f++) {
            if (f == fila) GPIOC->ODR &= ~(1 << f);
            else           GPIOC->ODR |=  (1 << f);
        }
        for (volatile int i = 0; i < 1000; i++);

        for (int col = 0; col < 3; col++) {
            uint8_t pin = col_pins[col];
            if (!(GPIOC->IDR & (1 << pin))) {
                delay_ms(20);
                while (!(GPIOC->IDR & (1 << pin)));
                return col * 3 + fila;
            }
        }
    }
    return -1;
}

/* ---------- LEDs ---------- */

typedef struct { GPIO_TypeDef *puerto; uint8_t pin; } led_t;

led_t led_X[9] = {
    {GPIOB,0}, {GPIOB,3}, {GPIOB,5},
    {GPIOB,7}, {GPIOB,9}, {GPIOA,11},
    {GPIOB,13},{GPIOB,15},{GPIOA,7}
};

led_t led_O[9] = {
    {GPIOB,1}, {GPIOB,4}, {GPIOB,6},
    {GPIOB,8}, {GPIOB,10},{GPIOA,12},
    {GPIOB,14},{GPIOA,6}, {GPIOA,8}
};

void encender_led(uint8_t celda, uint8_t jugador) {
    if (jugador == 1) led_X[celda].puerto->ODR |= (1 << led_X[celda].pin);
    else              led_O[celda].puerto->ODR |= (1 << led_O[celda].pin);
}

void apagar_led(uint8_t celda, uint8_t jugador) {
    if (jugador == 1) led_X[celda].puerto->ODR &= ~(1 << led_X[celda].pin);
    else              led_O[celda].puerto->ODR &= ~(1 << led_O[celda].pin);
}

/* ---------- Logica del juego ---------- */

uint8_t tablero[9];
uint32_t semilla = 12345;

const uint8_t lineas[8][3] = {
    {0,1,2}, {3,4,5}, {6,7,8},
    {0,3,6}, {1,4,7}, {2,5,8},
    {0,4,8}, {2,4,6}
};

int verificar_ganador(uint8_t jugador, const uint8_t **linea_ganadora) {
    for (int i = 0; i < 8; i++) {
        const uint8_t *l = lineas[i];
        if (tablero[l[0]] == jugador && tablero[l[1]] == jugador && tablero[l[2]] == jugador) {
            *linea_ganadora = l;
            return 1;
        }
    }
    return 0;
}

int tablero_lleno(void) {
    for (int i = 0; i < 9; i++) if (tablero[i] == 0) return 0;
    return 1;
}

void celebrar_ganador(uint8_t jugador, const uint8_t *linea) {
    while (1) {
        apagar_led(linea[0], jugador);
        apagar_led(linea[1], jugador);
        apagar_led(linea[2], jugador);
        delay_ms(300);
        encender_led(linea[0], jugador);
        encender_led(linea[1], jugador);
        encender_led(linea[2], jugador);
        delay_ms(300);
    }
}

void celebrar_empate(void) {
    while (1) {
        for (int i = 0; i < 9; i++) if (tablero[i]) apagar_led(i, tablero[i]);
        delay_ms(300);
        for (int i = 0; i < 9; i++) if (tablero[i]) encender_led(i, tablero[i]);
        delay_ms(300);
    }
}

void jugar_computador(void) {
    uint8_t libres[9];
    uint8_t n_libres = 0;
    for (int i = 0; i < 9; i++) {
        if (tablero[i] == 0) libres[n_libres++] = i;
    }
    if (n_libres == 0) return;

    semilla = semilla * 1103515245u + 12345u;
    uint8_t idx = (semilla >> 16) % n_libres;
    uint8_t celda = libres[idx];

    tablero[celda] = 2;
    encender_led(celda, 2);
}

/* ---------- Programa principal ---------- */

int main(void) {
    init_gpio();

    for (int i = 0; i < 9; i++) tablero[i] = 0;

    while (1) {
        int8_t celda;

        while (1) {
            celda = leer_teclado();
            if (celda >= 0 && tablero[celda] == 0) break;
            semilla++;
        }

        tablero[celda] = 1;
        encender_led(celda, 1);

        const uint8_t *linea;
        if (verificar_ganador(1, &linea)) {
            celebrar_ganador(1, linea);
        }
        if (tablero_lleno()) {
            celebrar_empate();
        }

        delay_ms(500);

        jugar_computador();

        if (verificar_ganador(2, &linea)) {
            celebrar_ganador(2, linea);
        }
        if (tablero_lleno()) {
            celebrar_empate();
        }
    }
}
