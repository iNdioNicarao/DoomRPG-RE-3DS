#ifndef LOGGER_H
#define LOGGER_H

// Инициализирует логгер, очищая старый лог-файл.
void Log_Init(void);

// Записывает форматированную строку в лог-файл.
void Log_Write(const char *format, ...);

// Записывает фатальную ошибку в лог и немедленно завершает программу.
void Log_FatalError(const char *format, ...);

#endif // LOGGER_H