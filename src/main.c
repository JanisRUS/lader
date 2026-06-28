/// @file       main.c
/// @brief      Файл с точкой входа в программу
/// @author     Тузиков Г.А. janisrus35@gmail.com

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include "lader.h"

//
// Константы
//

/// @brief      Название ПО
/// @warning    Должно соответствовать названию из файла debian/changelog
static const char *PROGRAMM_NAME = "lader";

/// @brief      Версия ПО
/// @warning    Должна соответствовать актуальной версии из файла debian/changelog
static const char *PROGRAMM_VERSION = "0.0.1";

/// @brief      Суффикс выходного файла
static const char *OUTPUT_FILE_SUFFIX = "-parsed";

//
// Объявления внутренних функций
//

/// @brief      Функция вывода справки 
static void printHelp(void);

/// @brief      Функция вывода версии ПО
static void printVersion(void);

//
// Внешние функции
//

int main(int argc, char *argv[])
{
    bool  isOk      = true;
    char *fileInput = 0;

    setlocale(LC_ALL, "");

    //
    // Объявление переменных, используемых в cleanup
    //

    char *fileOutput = 0;

    //
    // Парсинг аргументов
    //

    int fileIndex = -1;

    for (int i = 1; i < argc; ++i)
    {
        char *arg = argv[i];

        if (arg[0] == '-')
        {
            if (fileIndex != -1)
            {
                fprintf(stderr, "%s: provide options before file\n", PROGRAMM_NAME);
                isOk = false;
                goto cleanup;
            }
            
            if (strcmp(arg, "-h")     == 0 ||
                strcmp(arg, "--help") == 0)
            {
                printHelp();
                goto cleanup;
            }
            
            if (strcmp(arg, "-o")            == 0 ||
                strcmp(arg, "--output-file") == 0)
            {
                if (i + 1 >= argc)
                {
                    printHelp();
                    isOk = false;
                    goto cleanup;
                }

                ++i;

                if (fileOutput)
                {
                    free(fileOutput);
                    fileOutput = 0;
                }

                fileOutput = (char *)malloc(strlen(argv[i]) + 1);
                if (!fileOutput)
                {
                    fprintf(stderr, "%s: internal error\n", PROGRAMM_NAME);
                    isOk = false;
                    goto cleanup;
                }

                memcpy(fileOutput, argv[i], strlen(argv[i]) + 1);

                continue;
            }
            
            if (strcmp(arg, "-v")        == 0 ||
                strcmp(arg, "--version") == 0)
            {
                printVersion();
                goto cleanup;
            }

            fprintf(stderr, "%s: unknown option \"%s\"\n", PROGRAMM_NAME, arg);
            isOk = false;
            goto cleanup;
        }
        else
        {
            if (fileIndex == -1)
            {
                fileIndex = i;
            }
        }
    }

    //
    // Проверка входного файла
    //

    if (fileIndex == -1 || argc - fileIndex != 1)
    {
        printHelp();
        isOk = false;
        goto cleanup;
    }

    fileInput = argv[fileIndex];

    if (access(fileInput, R_OK) != 0)
    {
        fprintf(stderr, "%s: unable to read the file '%s'\n", PROGRAMM_NAME, fileInput);
        isOk = false;
        goto cleanup;
    }

    //
    // Создание названия выходного файла, при необходимости
    //

    if (!fileOutput)
    {
        const char *fileNamePtr = 0;
        const char *dotPtr      = 0;

        fileNamePtr = strrchr(fileInput, '/');
        fileNamePtr = fileNamePtr ? fileNamePtr + 1 : fileInput;

        fileOutput = (char *)malloc(strlen(fileNamePtr) + strlen(OUTPUT_FILE_SUFFIX) + 1);
        if (!fileOutput)
        {
            fprintf(stderr, "%s: internal error\n", PROGRAMM_NAME);
            isOk = false;
            goto cleanup;
        }

        dotPtr = strrchr(fileNamePtr, '.');
        if (dotPtr)
        {
            size_t fileNameBaseLength = dotPtr - fileNamePtr;

            strncpy(fileOutput, fileNamePtr, fileNameBaseLength);
            fileOutput[fileNameBaseLength] = '\0';
            
            strcat(fileOutput, OUTPUT_FILE_SUFFIX);
            strcat(fileOutput, dotPtr);
        }
        else
        {
            strcpy(fileOutput, fileNamePtr);
            strcat(fileOutput, OUTPUT_FILE_SUFFIX);
        }
    }

    //
    // Парсинг данных входного файла 
    //

    int packetSize = lader(fileInput, fileOutput);

    if (packetSize < 0)
    {
        switch ((LaderErrorCodesEnum)packetSize)
        {
            case LaderErrorCodeInputFileParse:
            {
                fprintf(stderr, "%s: input file parsing error\n", PROGRAMM_NAME);
                break;
            }
            case LaderErrorCodeTempFileParse:
            {
                fprintf(stderr, "%s: temp file parsing error\n", PROGRAMM_NAME);
                break;
            }
            case LaderErrorCodeLengthInvalid:
            {
                fprintf(stderr, "%s: the length of the output file data packages varies\n", PROGRAMM_NAME);
                break;
            }
            default:
            {
                fprintf(stderr, "%s: internal error\n", PROGRAMM_NAME);
                break;
            }
        }
        isOk = false;
        goto cleanup;
    }

    printf("The parsed contents of the %s file were written to %s\n", fileInput, fileOutput);
    printf("Calculated packet length in bits:\n");
    printf("%d\n", packetSize);

cleanup:
{
    if (fileOutput)
    {
        free(fileOutput);
        fileOutput = 0;
    }

    if (isOk)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
}

//
// Внутренние функции
//

void printHelp(void)
{
    printf("Usage: %s [OPTIONS] FILE\n", PROGRAMM_NAME);
    printf("Performs parsing of logic analyzer data from the specified FILE\n");
    printf("All parsed data will be written to a FILE with the %s suffix\n", OUTPUT_FILE_SUFFIX);
    printf("\n");
    printf("    -h --help                    Prints this message then exit with code 0\n");
    printf("    -o --output-file OUTPUT_FILE Specifies OUTPUT_FILE as the output\n");
    printf("    -v --version                 Prints software version then exit with code 0\n");
    printf("\n");
    printf("Example: \n");
    printf("    %s input/input_1.bin\n", PROGRAMM_NAME);
    printf("        Writes all the parsed data from input/input_1.bin in input/input-parsed.bin\n");
    printf("    %s -o output.bin input/input_1.bin\n", PROGRAMM_NAME);
    printf("        Writes all the parsed data from input/input_1.bin in output.bin\n");
}

void printVersion(void)
{
    printf("%s\n", PROGRAMM_VERSION);
}
