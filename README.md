# Це мій стартовий проект для nrf52840dk
## With love from Ukraine by Pereverziev Andrii Sep 13 2026

west list zephyr - дізнатися точну версію та стан репозиторію zephyr всередині вашого робочого простору nRF Connect SDK

west list - побачити версії всіх модулів та бібліотек вашого проєкту одночасно

west build -p -b nrf52840dk/nrf52840

west flash

ls /dev/tty.usbmodem*

screen /dev/tty.usbmodem0010502028451 115200

Щоб закрити сесію та повернутися до звичайного командного рядка:Натисніть комбінацію клавіш Ctrl + A.
Відразу після цього натисніть клавішу K (англійська літера K, означає kill).
Знизу з'явиться запит: Really kill this window? [y/n]. Натисніть Y.