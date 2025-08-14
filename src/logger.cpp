
#include "Logger.h"
#include <stdio.h>
#include <stdlib.h> // для exit()
#include <stdarg.h> // для va_list

// Имя нашего лог-файла
#define LOG_FILENAME "doomrpg_naebnulsya.txt"

void Log_Init(void) {
    FILE *logFile = fopen(LOG_FILENAME, "w"); // "w" - режим записи, очищает файл при открытии
    if (logFile) {
        fprintf(logFile, "--- Лог запущен. Ищем проблему. ---\n");
        fclose(logFile);
    }
}

void Log_Write(const char *format, ...) {
    va_list args;
    va_start(args, format);

    FILE *logFile = fopen(LOG_FILENAME, "a"); // "a" - режим дозаписи в конец файла
    if (logFile) {
        // Записываем форматированную строку
        vfprintf(logFile, format, args);
        // Добавляем перенос строки для читаемости
        fprintf(logFile, "\n");
        fclose(logFile);
    }

    va_end(args);
}

void Log_FatalError(const char *format, ...) {
    va_list args;
    va_start(args, format);

    FILE *logFile = fopen(LOG_FILENAME, "a");
    if (logFile) {
        fprintf(logFile, "\n\n!!! FATAL ERROR !!!\n");
        vfprintf(logFile, format, args);
        fprintf(logFile, "\n!!! Приложение аварийно завершено. !!!\n");
        fclose(logFile);
    }

    va_end(args);

    // Завершаем программу с кодом ошибки
    exit(1);
}