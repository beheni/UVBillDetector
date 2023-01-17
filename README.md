# Детектор фальшивих купюр

### Мета: Створити пристрій, який здатен класифікувати купюри і визначати чи фальшиві вони, чи справжні

### Обладнання: PSoC-6, USB-FTDI перехідник, esp32-cam, UV діоди, TFT-дисплей, IR датчик, модуль підвищення напруги

### Інструкція з використання

Ввімкнути пристрій, засунути купюру і дочекатись виведення результату на дисплеї.

### Деталі реалізації:

При ввімкненні пристрою вмикається інфрачервоний датчик, який фіксує появу нових об'єктів. Коли користувач засовує купюру, датчик реагує на це і вмикає юльтрафіолетові леди. Все це керується PSoC-6 надсилає на esp32-cam через UART команду зробити фото. Створюється невидимий юзеру веб-сервер. Камера робить знимок купюри і надсилає через UART на ноутбук команду стягувати зроблене фото з серверу.
