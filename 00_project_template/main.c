/**
 * Authors: Andrés Felipe Penagos - Daniel Ruiz Guirales 
 * Facultad de Ingenieria
 * Universidad de Antioquia
 * @file main.c
 * @brief Este archivo contiene el programa principal del juego de reaccion
 *
 * Este archivo contiene la implementacion del juego de reaccion simple
 * utilizando la Raspberry Pi Pico W, pulsadores, leds y un display cuadruple de 7 segmentos
 */

/**< Librerias a usar */ 

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/rand.h"

/**< Asignacion de pines */ 

const u_int8_t pin_transistor[4] = {0,1,2,3};  /**< Pines de control de los transistores para el display de 7 segmentos. */
const u_int8_t segmentos[7] = {4,5,6,7,8,9,10}; /**< Pines de los segmentos del display de 7 segmentos. */
const u_int8_t led_pin[3] = {16, 17, 18}; /**< Pines de los LEDs. */
const u_int8_t button_pin[3] = {19, 20, 21}; /**< Pines de los botones asociados a cada LED. */
const u_int8_t start_button=22; /**< Pin del botón de inicio del juego. */


/**
 * @brief Matriz que define la representación de cada número en el display de 7 segmentos.
 *
 * Esta matriz define la configuración de cada segmento (encendido o apagado) para
 * representar cada dígito del 0 al 9 en el display de 7 segmentos.
 */

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

volatile bool timer_fired = false; /**< Bandera que indica si ha ocurrido una interrupción de temporizador. */

/**< Prototipos de Funcion*/  

void wait_random(uint64_t min_us, uint64_t max_us);
bool is_button_pressed(uint8_t pin);
void initial_sequence();
int64_t alarm_callback(alarm_id_t id, void *user_data);
void display_numero(uint8_t numero);
void multiplexar_displays(int64_t numero);


/**
 * @brief Función principal del programa.
 *
 * Esta función es el punto de entrada principal del programa. Inicializa los GPIOs, configura
 * los pines de transistores, displays de 7 segmentos, LEDs y pulsadores. Luego, entra en un
 * bucle principal donde espera a que se presione el botón de inicio (START) para iniciar la
 * secuencia de LEDs, activar los displays y medir el tiempo de reacción del jugador.
 *
 * @return 0 para indicar que el programa se ha ejecutado correctamente.
 */
int main() {
    
    /**< Inicialización de GPIOS */ 
    stdio_init_all();

    /**< Inicializar pines de Transistores*/
    for (uint8_t i = 0; i < 4; i++) {  
        gpio_init(pin_transistor[i]);
        gpio_set_dir(pin_transistor[i], GPIO_OUT); 
    }

    /**< Inicializar pines Display 7 segmentos*/    
    for (uint8_t i = 0; i < 7; i++) { 
        gpio_init(segmentos[i]);
        gpio_set_dir(segmentos[i], GPIO_OUT);
    }

    /**< Inicializar pines de los diodos LED*/  
    for (uint8_t i = 0; i < 3; i++) {
        gpio_init(led_pin[i]);
        gpio_set_dir(led_pin[i], GPIO_OUT);
    }

    /**< Inicializar pines de los Pulsadores*/  
    for (uint8_t i = 0; i < 3; i++) {
        gpio_init(button_pin[i]);
        gpio_set_dir(button_pin[i], GPIO_IN);
        gpio_pull_down(button_pin[i]);
    }

    /**< Inicializar pin para el botón de START*/  
    gpio_init(start_button);
    gpio_set_dir(start_button, GPIO_IN);
    gpio_pull_down(start_button);

    /**< Bucle Principal */ 
    while (1) {
        while (!is_button_pressed(start_button)) {
            tight_loop_contents();
        }
        initial_sequence(); /**< Si el botón de START es presionado, la secuencia de LEDs inicia */ 
        /**< Habilitar el Enable de los 4 displays*/ 
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
                    multiplexar_displays(reaction_time); // Enseña en los display el tiempo de reaccion cuando presionamos el boton correcto
                    break;
                }
                else {
                    if(gpio_get(button_pin[0]) || gpio_get(button_pin[1]) || gpio_get(button_pin[2])){
                        button_pressed = true;
                        penalizacion = 10000000; //Penalizacion de 1 segundo si esl boton presionado es incorrecto
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
    return 0;
}

/**
 * @brief Función para generar un tiempo de espera aleatorio.
 *
 * Esta función genera un tiempo de espera aleatorio dentro del rango especificado
 * y realiza una pausa activa durante ese tiempo.
 *
 * @param min_us El tiempo mínimo de espera en microsegundos.
 * @param max_us El tiempo máximo de espera en microsegundos.
 */
void wait_random(uint64_t min_us, uint64_t max_us) {
    uint64_t random_us = min_us + (get_rand_64() % (max_us - min_us + 1)); /**< Tiempo de espera aleatorio dentro del rango especificado. */
    busy_wait_us(random_us); /**< Realiza una pausa activa durante el tiempo aleatorio generado. */
}

/**
 * @brief Determina si un botón ha sido presionado.
 *
 * Esta función verifica si el botón asociado al pin especificado ha sido presionado
 * o no.
 *
 * @param pin El número de pin GPIO del botón a verificar.
 * @return Devuelve true si el botón está presionado, o false si no lo está.
 */
bool is_button_pressed(uint8_t pin) {
    return gpio_get(pin);
}

/**
 * @brief Enciende una secuencia inicial de LEDs al inicio del juego.
 *
 * Esta función enciende y apaga los LEDs en una secuencia predefinida al inicio
 * del juego para indicar que el juego está listo para comenzar.
 * Después de la secuencia, se espera un tiempo aleatorio antes de continuar.
 */
void initial_sequence(){
    gpio_put(led_pin[0], true); // Enciende el primer LED
    gpio_put(led_pin[1], true); // Enciende el segundo LED
    gpio_put(led_pin[2], true); // Enciende el tercer LED

    busy_wait_us_32(2000000); // Espera 2 segundos

    gpio_put(led_pin[0], true); // Mantiene encendido el primer LED
    gpio_put(led_pin[1], true); // Mantiene encendido el segundo LED
    gpio_put(led_pin[2], false); // Apaga el tercer LED

    busy_wait_us_32(2000000); // Espera 2 segundos

    gpio_put(led_pin[0], true); // Mantiene encendido el primer LED
    gpio_put(led_pin[1], false); // Apaga el segundo LED
    gpio_put(led_pin[2], false); // Apaga el tercer LED

    busy_wait_us_32(2000000); // Espera 2 segundos

    gpio_put(led_pin[0], false); // Apaga el primer LED
    gpio_put(led_pin[1], false); // Apaga el segundo LED
    gpio_put(led_pin[2], false); // Apaga el tercer LED

    wait_random(1000000, 10000000); // Espera un t
}


/**
 * @brief Callback de alarma para determinar si el tiempo de espera sin pulsar botón ha terminado.
 *
 * Esta función se llama cuando la alarma programada ha finalizado, indicando que el tiempo de
 * espera sin pulsar botón ha terminado. Establece la bandera `timer_fired` en true para indicar
 * que la alarma ha sido activada.
 *
 * @param id Identificador de la alarma que se ha activado.
 * @param user_data Puntero a datos de usuario (no utilizado en esta función).
 * @return Devuelve 0 para indicar que la función se ha ejecutado correctamente.
 */
int64_t alarm_callback(alarm_id_t id, void *user_data) {
    printf("Timer fired!\n"); // Muestra un mensaje de que la alarma se ha activado
    timer_fired = true; // Establece la bandera timer_fired en true
    /**< Puedes devolver un valor aquí en us para disparar en el futuro si es necesario*/
    return 0; // Devuelve 0 para indicar que la función se ha ejecutado correctamente
}

/**
 * @brief Muestra un número en el display de 7 segmentos.
 *
 * Esta función activa los segmentos correspondientes en el display de 7 segmentos
 * para mostrar el número especificado.
 *
 * @param numero El número que se mostrará en el display (0-9).
 */
void display_numero(uint8_t numero) {
    for (int i = 0; i < 7; i++) {
        gpio_put(segmentos[i], numeros[numero][i]);
    }
}



/**
 * @brief Multiplexa los displays de 7 segmentos para mostrar el tiempo de reacción.
 *
 * Esta función multiplexa los displays de 7 segmentos para mostrar el tiempo de reacción
 * en formato de segundos y milisegundos. Muestra cada dígito del tiempo en los displays
 * de manera secuencial.
 *
 * @param numero El tiempo de reacción en microsegundos.
 */
void multiplexar_displays(int64_t numero) {
    uint8_t digitos[4];
    int64_t seconds = numero / 1000000;
    int64_t milliseconds = (numero / 1000) % 1000; 
    digitos[0] = seconds / 1 % 10; /**< Obtener el primer dígito (miles) para los segundos*/
    digitos[1] = milliseconds / 100 % 10; /**< Obtener el segundo dígito (centenas) para los milisegundos*/
    digitos[2] = milliseconds / 10 % 10; /**< Obtener el tercer dígito (decenas) para los milisegundos*/
    digitos[3] = milliseconds / 1 % 10; /**< Obtener el cuarto dígito (unidades) para los milisegundos*/

    /**< Variable para controlar si se ha presionado nuevamente el botón*/
    bool boton_presionado = false;

    /**< Bucle principal para mostrar el tiempo de reacción en los displays*/
    while (!boton_presionado) {
        /**< Activar el transistor correspondiente para seleccionar el display*/
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                gpio_put(pin_transistor[j], (j == i) ? 1 : 0);
            }

            /**< Mostrar el dígito en el display seleccionado*/
            display_numero(digitos[i]);

            /**< Verificar si se ha presionado nuevamente el botón*/
            if (is_button_pressed(start_button)) {
                boton_presionado = true;
                break; /**< Salir del bucle for*/
            }

            busy_wait_us(1000); /**< Ajustar este valor según la velocidad de actualización necesaria*/
        } 
    }

    /**< Desactivar el Enable de cada display*/ 
    for (int i = 0; i < 4; i++) {
        gpio_put(pin_transistor[i], false);
        
        /**< Apagar cada segmento*/ 
        for (int j = 0; j < 7; j++) {
            gpio_put(segmentos[j], false);
        }
    }
}

