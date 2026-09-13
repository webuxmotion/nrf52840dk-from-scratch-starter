# Обов'язкова мінімальна версія CMake для Zephyr v3.2.0
cmake_minimum_required(VERSION 3.20.0)

# Знаходимо та підключаємо пакет Zephyr RTOS
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Оголошуємо назву нашого проєкту
project(nrf52840_hello_world)

# Вказуємо файл із головною функцією main для компіляції
target_sources(app PRIVATE src/main.c)
