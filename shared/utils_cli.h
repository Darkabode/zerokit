#ifndef __UTILS_H_
#define __UTILS_H_

int utils_read_file(const char* filePath, uint8_t** pData, size_t* pSize);
int utils_save_file(const char* filePath, const uint8_t* data, size_t size);

void utils_md5_to_str(const uint8_t md5Buff[16], char outStr[33]);

/*    Возвращает 0, если файл или папка существует, иначе 1. */
int utils_file_exists(const char* filePath);

const char* utils_get_base_name(const char* filePath);

/*    Добавляет или заменяет имя файла (secondPart) к/в пути (firstPart) в зависимости от флага replaceFilename. */
void utils_build_path(char* outPath, char* firstPart, char* secondPart, int replaceFilename);

/*  Запускает файл */
void* utils_launch(char* commandLine);

#ifdef _WIN32
/*  Запускает файл и ждёт его завершения, возвращая код ошибки. */
bool_t utils_launch_and_verify(const char* commandLine, uint32_t* pExitCode);
#endif // _WIN32

/*    Создаёт директорию и возвращает 0, если создание прошло успешно, иначе 1. */
int utils_create_directory(char* dir);

/*    Возвращает размер файла. */
uint64_t utils_get_file_size(const char* fileName);

/*    Удаляет файл или папку. */
int utils_remove(const char* pathName);

/* Копирует файл. */
int utils_copy_file(const char* srcPath, const char* destPath);

/* Устанавливает текущую директорию. */
int utils_set_current_directory(const char* path);

#ifdef USE_UTILS_PIPE_LAUNCHING
FILE* utils_plaunch(const char* cmd, const char* mode);
int utils_pdestroy(FILE *fle);
#endif // USE_UTILS_PIPE_LAUNCHING


#endif // __UTILS_H_
