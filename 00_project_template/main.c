/**
 * @file main.c
 * @brief This is a brief description of the main C file.
 *
 * Detailed description of the main C file.
 */

// Standard libraries
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/rand.h"

// Define LED y botones
const u_int8_t pin_transistor[4] = {0,1,2,3};
const u_int8_t segmentos[7] = {4,5,6,7,8,9,10};
const u_int8_t led_pin[3] = {16, 17, 18};
const u_int8_t button_pin[3] = {19, 20, 21};
const u_int8_t start_button=22;

const uint8_t numeros[10][7] = {
    {1, 1, 1, 1, 1, 1, 0}, // 0
    {0, 1, 1, 0, 0, 0, 0}, // 1
    {1, 1, 0, 1, 1, 0, 1}, // 2
    {1, 1, 1, 1, 0, 0, 1}, // 3
    {0, 1, 1, 0, 0, 1, 1}, // 4
    {1, 0, 1, 1, 0, 1, 1}, // 5
    {1, 0, 1, 1, 1, 1, 1}, // 6
    {1, 1, 1, 0, 0, 0, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1}, // 8
    {1, 1, 1, 1, 0, 1, 1}  // 9
};

volatile bool timer_fired = false;

// Prototipo de función
void wait_random(uint64_t min_us, uint64_t max_us);
bool is_button_pressed(uint8_t pin);
void initial_sequence();
int64_t alarm_callback(alarm_id_t id, void *user_data);
void display_numero(uint8_t numero);
void multiplexar_displays(int64_t numero);

int main() {
    // Inicialización de las GPIOs
    stdio_init_all();


    for (uint8_t i = 0; i < 4; i++) {
        gpio_init(pin_transistor[i]);
        gpio_set_dir(pin_transistor[i], GPIO_OUT);
    }

    for (uint8_t i = 0; i < 7; i++) {
        gpio_init(segmentos[i]);
        gpio_set_dir(segmentos[i], GPIO_OUT);
    }

    for (uint8_t i = 0; i < 3; i++) {
        gpio_init(led_pin[i]);
        gpio_set_dir(led_pin[i], GPIO_OUT);
    }
    for (uint8_t i = 0; i < 3; i++) {
        gpio_init(button_pin[i]);
        gpio_set_dir(button_pin[i], GPIO_IN);
        gpio_pull_down(button_pin[i]);
    }
    
    gpio_init(start_button);
    gpio_set_dir(start_button, GPIO_IN);
    gpio_pull_down(start_button);

    // Bucle principal
    while (1) {
        while (!is_button_pressed(start_button)) {
            tight_loop_contents();
        }
        initial_sequence();
        gpio_put(pin_transistor[0], true);
        gpio_put(pin_transistor[1], true);
        gpio_put(pin_transistor[2], true);
        gpio_put(pin_transistor[3], true);
        

        add_alarm_in_us(10000000, alarm_callback, NULL, false);
        timer_fired = false;

        while (1) {
            for (uint8_t i = 0; i < 3; i++) {
                gpio_put(led_pin[i], false);
            }
            u_int32_t random_index = get_rand_32() % 3;
            gpio_put(led_pin[random_index], true);
            absolute_time_t start_time = get_absolute_time();
            bool button_pressed = false;
            int64_t penalizacion = 0;

            while (!timer_fired){
                if (gpio_get(button_pin[random_index])) {
                    gpio_put(led_pin[random_index], false);
                    absolute_time_t end_time=get_absolute_time();
                    int64_t reaction_time=absolute_time_diff_us(start_time, end_time) + penalizacion;
                    button_pressed = false;
                    printf("Start time: %lld us, End time: %lld us, Reaction time: %lld us\n", start_time, end_time, reaction_time);
                    multiplexar_displays(reaction_time);
                    break;
                }
                else {
                    if(gpio_get(button_pin[0]) || gpio_get(button_pin[1]) || gpio_get(button_pin[2])){
                        button_pressed = true;
                        penalizacion = 10000000;
                    }
                }
            }
            gpio_put(led_pin[random_index], false);
            if(button_pressed==false){
                break;
            }
            
            // Apaga  LED
            //wait_random(1000000, 10000000);  // Espera el tiempo aleatorio
            timer_fired = false;
        }
        
    }
    //exit_loop:
    return 0;

}

void wait_random(uint64_t min_us, uint64_t max_us) {
    uint64_t random_us = min_us + (get_rand_64() % (max_us - min_us + 1));
    busy_wait_us(random_us);
}


bool is_button_pressed(uint8_t pin) {
    return gpio_get(pin);
}

void initial_sequence(){
        gpio_put(led_pin[0], true);
        gpio_put(led_pin[1], true);
        gpio_put(led_pin[2], true);
      
       busy_wait_us_32(2000000);
        gpio_put(led_pin[0], true);
        gpio_put(led_pin[1], true);
        gpio_put(led_pin[2], false);
        
        busy_wait_us_32(2000000);
        gpio_put(led_pin[0], true);
        gpio_put(led_pin[1], false);
        gpio_put(led_pin[2], false);
      
        busy_wait_us_32(2000000);
        gpio_put(led_pin[0], false);
        gpio_put(led_pin[1], false);
        gpio_put(led_pin[2], false);
    
        wait_random(1000000, 10000000);
}



int64_t alarm_callback(alarm_id_t id, void *user_data) {
    printf("Timer fired!\n");
    timer_fired = true;
    // Puedes devolver un valor aquí en us para disparar en el futuro si es necesario
    return 0;
}


void display_numero(uint8_t numero) {
    for (int i = 0; i < 7; i++) {
        gpio_put(segmentos[i], numeros[numero][i]);
    }
}


void multiplexar_displays(int64_t numero) {
    uint8_t digitos[4];
    int64_t seconds = numero / 1000000;
    int64_t milliseconds = (numero / 1000) % 1000; 
    digitos[0] = seconds / 1 % 10;                                  // Obtener el primer dígito (miles)
    digitos[1] = milliseconds / 100 % 10;   // Obtener el segundo dígito (centenas)
    digitos[2] = milliseconds / 10 % 10;    // Obtener el tercer dígito (decenas)
    digitos[3] = milliseconds / 1 % 10;           // Obtener el cuarto dígito (unidades)

    // Variable para controlar si se ha presionado nuevamente el botón
    bool boton_presionado = false;

    // Bucle principal para mostrar el tiempo de reacción en los displays
    while (!boton_presionado) {
        for (int i = 0; i < 4; i++) {
            // Activar el transistor correspondiente para seleccionar el display
            for (int j = 0; j < 4; j++) {
                gpio_put(pin_transistor[j], (j == i) ? 1 : 0);
            }

            // Mostrar el dígito en el display seleccionado
            display_numero(digitos[i]);

            // Verificar si se ha presionado nuevamente el botón
            if (is_button_pressed(start_button)) {
                boton_presionado = true;
                break; // Salir del bucle for
            }

            busy_wait_us(1000); // Ajusta este valor según la velocidad de actualización que necesites
        } 
    }

    // Limpiar los displays
    for (int i = 0; i < 4; i++) {
        gpio_put(pin_transistor[i], false);
        // Apagar todos los segmentos
        for (int j = 0; j < 7; j++) {
            gpio_put(segmentos[j], false);
        }
    }
}
