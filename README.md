# rc4_messager

Используемые библиотеки: pthread(многопоточность) и ncurses(расширенный ввод/вывод в терминале + куча проблем:))

Мэссэджер работает пока только на локальной машине (localhost/127.0.0.1);
Используется шифр RC4 для шифрования сообщений;
Указываем порт, который будет прослушиваться (>1000);
Дальше можно либо ждать заявок на соединение либо пытаться присоединится к другому сокету самому;
Предполагается, что ключ известен обоим обонентам, и он вводится самостоятельно;
Далее идёт обмен сообщениями
