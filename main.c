#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f401.h"
#include "gpio_config.h"

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

    for (int p = 0; p <= 3; p++) gpio_set_output(GPIOC, p);
    for (int p = 4; p <= 7; p++) gpio_set_input_pullup(GPIOC, p);

    for (int p = 0; p <= 15; p++) if (p != 2 && p != 11) gpio_set_output(GPIOB, p);
    for (int p = 6; p <= 12; p++) gpio_set_output(GPIOA, p);
}

void delay_ms(volatile uint32_t ms) {
    for (uint32_t i = 0; i < ms * 4000; i++) {
        __asm__("nop");
    }
}

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
                return col * 3 + fila;   // <-- invertido: antes era fila*3+col
            }
        }
    }
    return -1;
}

typedef struct { GPIO_TypeDef *puerto; uint8_t pin; } led_t;

led_t led_X[9] = {{GPIOB,0},{GPIOB,3},{GPIOB,5},{GPIOB,7},{GPIOB,9},{GPIOA,11},{GPIOB,13},{GPIOB,15},{GPIOA,7}};

void encender(uint8_t celda, char jugador) {
    (void)jugador;
    led_X[celda].puerto->ODR |= (1 << led_X[celda].pin);
}

int main(void) {
    init_gpio();
    while (1) {
        int8_t celda = leer_teclado();
        if (celda >= 0) {
            encender(celda, 'X');
        }
    }
}
