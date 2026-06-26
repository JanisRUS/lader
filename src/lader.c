/// @file       lader.c
/// @brief      См. lader.h
/// @author     Тузиков Г.А. janisrus35@gmail.com

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include "lader.h"
#include <arpa/inet.h>

//
// Объявления констант
//

/// @brief      Синхромаркер
static const uint32_t LADER_SYNC_MARK = 0b00011010110011111111110000011101;

/// @brief      Размер синхромаркера (бит)
static const long LADER_SYNC_MARK_SIZE = 32;

//
// Объявления типов данных
//

/// @brief      Перечисление типов байт входного файла
typedef enum LaderInputFileByteTypesEnum
{
    LaderInputFileByteTypeUnknown = 0, ///< Неизвестный тип данных
    LaderInputFileByteTypeData,        ///< Данные
    LaderInputFileByteTypeSync,        ///< Синхроимпульс
    LaderInputFileByteTypesCount       ///< Количество типов байт входного файла
}LaderInputFileByteTypesEnum;

/// @brief      Перечисление масок для определения типа байта входного файла
typedef enum LaderInputFileByteMasksEnum
{
    LaderInputFileByteMaskZero = 0x01,                                                  ///< Логический ноль
    LaderInputFileByteMaskOne  = 0x02,                                                  ///< Логическая единица
    LaderInputFileByteMaskSync = LaderInputFileByteMaskZero | LaderInputFileByteMaskOne ///< Синхроимпульс
}LaderInputFileByteMasksEnum;

//
// Объявления внутренних функций
//

/// @brief      Функция парсинга входного файла во временный
/// @details    Данная функция выполняет: <br>
///                 1) Поиск первого синхроимпульса <br>
///                 2) Циклический поиск диапазона курсора inputFilePtr между соседними синхроимпульсами <br>
///                 3) Запись бита, находящегося между соседними синхроимпульсами, в tempFilePtr
/// @note       Данные до первого и после последнего синхроимпульса считаются невалидными
/// @note       Неопределенные биты между соседними синхроимпульсами игнорируются
/// @note       Биты между соседними синхроимпульсами не проверяются на соответствие. 
///                 Т.е. если между соседними синхроимпульсами данные имеют вид 1 1 0 1 1, 
///                 во временный файл будет записан 0
/// @param[in]  inputFilePtr Указатель на входной файл
/// @param[in]  tempFilePtr  Указатель на временный файл
/// @return     Возвращает 0 в случае успешного выполнения операции.
///                 В противном случае, возвращает код ошибки
static int laderInputFileParseToTemp(FILE *inputFilePtr, FILE *tempFilePtr);

/// @brief      Функция определения типа байта входного файла
/// @details    Данная функция выполняет определение типа byte входного файла, 
///                 используя LaderInputFileByteMasksEnum
/// @param[in]  byte Байт данных
/// @return     Возвращает определенный тип byte
static LaderInputFileByteTypesEnum laderInputFileDefineByteType(unsigned char byte);

/// @brief      Функция поиска следующего байта указанного типа в входном файле
/// @details    Данная функция выполняет перемещение курсора inputFilePtr на 
///                 следующий байт типа targetByte входного файла
/// @param[in]  inputFilePtr   Указатель на открытый входной файл
/// @param[in]  targetByteType Тип целевого байта входного файла
/// @return     Возвращает позицию курсора указанного типа байта в случае успешного выполнения операции.
///                 В противном случае, возвращает код ошибки
static long laderInputFileFindNextByteTypeOf(FILE *inputFilePtr, LaderInputFileByteTypesEnum targetByteType);

/// @brief      Функция поиска бита данных между синхроимпульсами
/// @details    Данная функция выполняет поиск центрального бита данных в диапазоне курсора inputFilePtr [start:stop]
/// @note       После успешного выполнения, функция переводит курсор inputFilePtr на stop
/// @param[in]  inputFilePtr Указатель на открытый входной файл
/// @param[in]  start        Позиция курсора inputFilePtr с первым байтом данных после сихроимпульса
/// @param[in]  stop         Позиция курсора inputFilePtr с последним байтом данных перед синхроимпульсом
/// @return     Возвращает бит данных, находящийся между синхроимпульсами.
///                 В противном случае, возвращает код ошибки
static int laderInputFileFindDataBit(FILE *inputFilePtr, long start, long stop);

/// @brief      Функция создания временного файла
/// @details    Данная функция выполняет создание временного файла, используя
///                 содержимое fileNameTemplatePtr в качестве шаблона названия,
///                 и открывает его, очищая содержимое временного файла
/// @note       После завершения работы с временным файлом, вызовите функцию laderTempFileDestroy()
/// @param      fileNameTemplatePtr Указатель на шаблон названия временного файла. 
///                                     Должен завершаться 6-ю символами X. Например, .temp_XXXXXX. 
///                                     После вызова функции, в нем будет записано название созданного файла
/// @return     Возвращает указатель на открытый временный файл.
///                 В случае возникновения ошибки, возвращает 0
static FILE *laderTempFileCreate(char *fileNameTemplatePtr);

/// @brief      Функция удаления временного файла
/// @details    Данная функция выполняет закрытие *filePtr удаление fileNamePtr с файловой системы
/// @param[in]  fileNamePtr Указатель на название временного файла
/// @param[in]  filePtrPtr  Указатель на указатель на временный файл
/// @note       При успешном выполнении функции, *filePtrPtr будет равен 0
static void laderTempFileDestroy(char *fileNamePtr, FILE **filePtrPtr);

/// @brief      Функция парсинга временного файла в выходной
/// @details    Данная функция выполняет: <br>
///                 1) Поиск первого синхромаркера <br>
///                 2) Циклический поиск диапазона курсора tempFilePtr между соседними синхромаркерами <br>
///                 3) Запись всех бит, находящихся между соседними синхроимпульсами, в outputFilePtr
/// @note       Данные до первого и после последнего синхромакера считаются невалидными
/// @param[in]  tempFilePtr   Указатель на временный файл
/// @param[in]  outputFilePtr Указатель на выходной файл
/// @return     Возвращает 0 в случае успешного выполнения операции.
///                 В противном случае, возвращает код ошибки
static int laderTempFileParseToOutput(FILE *tempFilePtr, FILE *outputFilePtr);

/// @brief      Функция поиска следующего синхромаркера во временном файле
/// @details    Данная функция выполняет перемещение курсора tempFilePtr на 
///                 следующий синхромаркер временного файла
/// @param[in]  tempFilePtr Указатель на открытый входной файл
/// @return     Возвращает позицию курсора сонхромаркера в случае успешного выполнения операции.
///                 В противном случае, возвращает код ошибки
static long laderTempFileFindNextSync(FILE *tempFilePtr);

/// @brief      Функция записи данных между синхромаркерами из временного файла в выходной
/// @details    Данная функция выполняет сборку всех байт в диапазоне курсора tempFilePtr [start:stop] 
///                 и пишет их в outputFilePtr
/// @note       После успешного выполнения, функция переводит курсор tempFilePtr на stop
/// @note       Если последний собираемый байт данных окажется неполным, его младшие биты будут заполнены нулями.
///                 Например, в диапазоне [start:stop] есть только 3 бита данных для сборки последнего байта и они равны 0b101.
///                 Таким образом, последний байт будет равен 0b10100000
/// @param[in]  tempFilePtr   Указатель на временный файл
/// @param[in]  outputFilePtr Указатель на выходной файл
/// @param[in]  start         Позиция курсора tempFilePtr с первым битом данных после сихромаркера
/// @param[in]  stop          Позиция курсора tempFilePtr с последним битом данных перед синхромаркера
/// @return     Возвращает 0 в случае успешного выполнения операци.
///                 В противном случае, возвращает код ошибки
static int laderTempFileReadDataToOutput(FILE *tempFilePtr, FILE *outputFilePtr, long start, long stop);

//
// Внешние функции
//

int lader(const char *inputFileNamePtr, const char *outputFileNamePtr)
{
    int  answer         = 0;
    char tempFileName[] = ".temp_XXXXXX";

    FILE *inputFilePtr = fopen(inputFileNamePtr, "r");
    FILE *tempFilePtr  = laderTempFileCreate(&tempFileName[0]);

    if (!inputFilePtr)
    {
        answer = -1;
        goto cleanup;
    }

    if (!tempFilePtr)
    {
        answer = -2;
        goto cleanup;
    }

    laderInputFileParseToTemp(inputFilePtr, tempFilePtr);
    fclose(inputFilePtr);
    inputFilePtr = 0;

    rewind(tempFilePtr);

cleanup:
{
    if (inputFilePtr)
    {
        fclose(inputFilePtr);
        inputFilePtr = 0;
    }

    laderTempFileDestroy(&tempFileName[0], &tempFilePtr);
}
    return answer;
}

//
// Внутренние функции
//

int laderInputFileParseToTemp(FILE *inputFilePtr, FILE *tempFilePtr)
{
    if (!inputFilePtr)
    {
        return -1;
    }

    if (!tempFilePtr)
    {
        return -2;
    }
    
    if (laderInputFileFindNextByteTypeOf(inputFilePtr, LaderInputFileByteTypeSync) < 0)
    {
        return -3;
    }

    long dataStart = 0;
    long dataStop  = 0;
    int  bit       = 0;
    while (feof(inputFilePtr) == 0)
    {
        dataStart = laderInputFileFindNextByteTypeOf(inputFilePtr, LaderInputFileByteTypeData);
        dataStop  = laderInputFileFindNextByteTypeOf(inputFilePtr, LaderInputFileByteTypeSync);

        if (dataStart < 0 || dataStop < 0)
        {
            continue;
        }

        bit = laderInputFileFindDataBit(inputFilePtr, dataStart, dataStop);
        if (bit >= 0)
        {
            fwrite((uint8_t *)&bit, 1, 1, tempFilePtr);
        }
    }

    return 0;
}

LaderInputFileByteTypesEnum laderInputFileDefineByteType(unsigned char byte)
{
    LaderInputFileByteTypesEnum answer = LaderInputFileByteTypeUnknown;

    if ((byte & LaderInputFileByteMaskSync) == LaderInputFileByteMaskSync)
    {
        answer = LaderInputFileByteTypeSync;
    }
    else if ((byte & LaderInputFileByteMaskZero) == LaderInputFileByteMaskZero || 
             (byte & LaderInputFileByteMaskOne)  == LaderInputFileByteMaskOne)
    {
        answer = LaderInputFileByteTypeData;
    }

    return answer;
}

long laderInputFileFindNextByteTypeOf(FILE *inputFilePtr, LaderInputFileByteTypesEnum targetByteType)
{
    if (!inputFilePtr)
    {
        return -1;
    }

    if (targetByteType >= LaderInputFileByteTypesCount)
    {
        return -2;
    }

    int byte = 0;

    while ((byte = fgetc(inputFilePtr)) != EOF)
    {
        LaderInputFileByteTypesEnum byteType = laderInputFileDefineByteType(byte);

        if (byteType == targetByteType)
        {
            fseek(inputFilePtr, -1, SEEK_CUR);
            return ftell(inputFilePtr);
        }
    }

    return -3;
}

int laderInputFileFindDataBit(FILE *inputFilePtr, long start, long stop)
{
    if (!inputFilePtr)
    {
        return -1;
    }

    if (feof(inputFilePtr))
    {
        return -2;
    }

    if (start > stop)
    {
        return -3;
    }

    int byte = 0;
    int bit  = 0;

    /// @todo       Можно добавить обработку содержимого диапазона [start:stop]. 
    ///                 Если не все байты равны, промежуток между синхроимпульсами можно считать поврежденным и
    ///                 возвращать код ошибки

    fseek(inputFilePtr, start + (stop - stop) / 2, SEEK_SET);
    byte = fgetc(inputFilePtr);
    
    if ((byte & (LaderInputFileByteMaskZero | LaderInputFileByteMaskOne)) == 0)
    {
        return -4;
    }

    if ((byte & LaderInputFileByteMaskZero) == LaderInputFileByteMaskZero)
    {
        bit = 0;
    }
    else if ((byte & LaderInputFileByteMaskOne) == LaderInputFileByteMaskOne)
    {
        bit = 1;
    }

    fseek(inputFilePtr, stop, SEEK_SET);

    return bit;
}

FILE *laderTempFileCreate(char *fileNameTemplatePtr)
{
    FILE *filePtr = 0;
    int   fileDsc = -1;

    if (!fileNameTemplatePtr)
    {
        goto cleanup;
    }

    fileDsc = mkstemp(fileNameTemplatePtr);
    if (fileDsc == -1)
    {
        goto cleanup;
    }

    filePtr = fdopen(fileDsc, "w+b");
    if (!filePtr) 
    {
        goto cleanup;
    }

    return filePtr;

cleanup:
{
    if (fileDsc >= 0)
    {
        close(fileDsc);
        fileDsc = -1;
    }

    laderTempFileDestroy(fileNameTemplatePtr, &filePtr);

    return filePtr;
}
}

void laderTempFileDestroy(char *fileNamePtr, FILE **filePtrPtr)
{
    if (filePtrPtr && *filePtrPtr)
    {
        fclose(*filePtrPtr);
        *filePtrPtr = 0;
    }

    if (fileNamePtr)
    {
        remove(fileNamePtr);
    }
}

/// @brief      Функция парсинга временного файла в выходной
/// @details    Данная функция выполняет: <br>
///                 1) Поиск первого синхромаркера <br>
///                 2) Циклический поиск диапазона курсора tempFilePtr между соседними синхромаркерами <br>
///                 3) Запись всех бит, находящихся между соседними синхроимпульсами, в outputFilePtr
/// @note       Данные до первого и после последнего синхромакера считаются невалидными
/// @param[in]  tempFilePtr   Указатель на временный файл
/// @param[in]  outputFilePtr Указатель на выходной файл
/// @return     Возвращает 0 в случае успешного выполнения операции.
///                 В противном случае, возвращает 0
int laderTempFileParseToOutput(FILE *tempFilePtr, FILE *outputFilePtr)
{
    /// TODO
}

/// @brief      Функция поиска следующего синхромаркера во временном файле
/// @details    Данная функция выполняет перемещение курсора tempFilePtr на 
///                 следующий синхромаркер временного файла
/// @param[in]  tempFilePtr Указатель на открытый входной файл
/// @return     Возвращает позицию курсора сонхромаркера в случае успешного выполнения операции.
///                 В противном случае, возвращает код ошибки
long laderTempFileFindNextSync(FILE *tempFilePtr)
{
    /// TODO
}

int laderTempFileReadDataToOutput(FILE *tempFilePtr, FILE *outputFilePtr, long start, long stop)
{
    if (!tempFilePtr)
    {
        return -1;
    }

    if (!outputFilePtr)
    {
        return -2;
    }

    if (start > stop)
    {
        return -3;
    }

    fseek(tempFilePtr, start, SEEK_SET);

    int     byte        = 0;
    uint8_t dataByte    = 0;
    size_t  dataCounter = 0;
    size_t  dataRemains = 0;
    size_t  dataCount   = stop - start;

    dataRemains = dataCount % 8;

    while (dataCounter < dataCount && (byte = fgetc(tempFilePtr)) != EOF)
    {
        dataByte <<= 1;

        if (byte)
        {
            dataByte |= 1;
        }

        ++dataCounter;

        if (dataCounter % 8 == 0)
        {
            fwrite(&dataByte, 1, 1, outputFilePtr);
        }
    }

    if (dataRemains)
    {
        dataByte <<= (8 - dataRemains);
        fwrite(&dataByte, 1, 1, outputFilePtr);
    }

    fseek(tempFilePtr, stop, SEEK_SET);

    return 0;
}














/*
int lader(const char *inputFilePtr, const char *outputFilePtr)
{
    /// TODO разбить на функции
    int  answer         = 0;
    char tempFileName[] = ".temp_XXXXXX";

    FILE *inputFile = fopen(inputFilePtr, "r");
    FILE *tempFile  = laderTempFileCreate(&tempFileName[0]);

    if (inputFile == 0)
    {
        answer = -1;
        goto cleanup;
    }

    if (tempFile == 0)
    {
        answer = -2;
        goto cleanup;
    }
    
    {
        bool isSyncFound = false;
        int  byte        = 0;
        while ((byte = fgetc(inputFile)) != EOF)
        {
            LaderInputFileByteTypesEnum dataType = laderInputFileDefineByteType(byte);

            if (dataType == LaderInputFileByteTypeSync)
            {
                isSyncFound = true;
            }
            else
            {
                if (isSyncFound)
                {
                    fseek(inputFile, -1, SEEK_CUR);
                    break;
                }
            }
        }
    }
    
    {
        bool isSyncTarget = true;
        long dataStart    = 0;
        long dataStop     = 0;
        int  byte         = 0;
        while ((byte = fgetc(inputFile)) != EOF)
        {
            LaderInputFileByteTypesEnum dataType = laderInputFileDefineByteType(byte);

            if (dataType == LaderInputFileByteTypeUnknown)
            {
                /// TODO Возможно, необходимо проиндицировать нахождение неизвестного типа данных
                continue;
            }

            if (isSyncTarget)
            {
                if (dataType != LaderInputFileByteTypeSync)
                {
                    continue;
                }

                isSyncTarget = false;
                dataStop     = ftell(inputFile);
                fseek(inputFile, dataStart + (dataStop - dataStart) / 2, SEEK_SET);
                byte = fgetc(inputFile);
                
                switch (laderInputFileDefineByteType(byte))
                {
                    case LaderInputFileByteMaskOne:
                    {
                        fprintf(tempFile, "1");
                        break;
                    }
                    case LaderInputFileByteMaskZero:
                    {
                        fprintf(tempFile, "0");
                        break;
                    }
                    default: break;
                }

                fseek(inputFile, dataStop, SEEK_SET);
            }
            else
            {
                if (dataType != LaderInputFileByteTypeOne &&
                    dataType != LaderInputFileByteTypeZero)
                {
                    continue;
                }

                isSyncTarget = true;
                dataStart    = ftell(inputFile) - 1;
            }
        }
    }

    fclose(inputFile);
    inputFile = 0;

    rewind(tempFile);
    
    {
        uint32_t syncMark = 0;
        int      byte     = 0;
        while ((byte = fgetc(tempFile)) != EOF)
        {
            syncMark <<= 1;

            if (byte == '1')
            {
                syncMark |= 1;
            }
            else
            {
                syncMark &= ~1;
            }

            if (syncMark == LADER_SYNC_MARK)
            {
                fseek(tempFile, -LADER_SYNC_MARK_SIZE, SEEK_CUR);
                break;
            }
        }
    }

    FILE *outputFile = fopen(outputFilePtr, "wb");

    if (!outputFile)
    {
        goto cleanup;
    }

    {
        /// TODO Подумать, что делать, если последний пакет полный. Ведь он не запишется т.к. после него нет синхромаркера
        uint32_t syncMark  = 0;
        long     dataStart = ftell(tempFile);
        long     dataStop  = 0;
        int      byte      = 0;
        int      count     = 0;
        fseek(tempFile, LADER_SYNC_MARK_SIZE, SEEK_CUR);
        while ((byte = fgetc(tempFile)) != EOF)
        {
            syncMark <<= 1;

            if (byte == '1')
            {
                syncMark |= 1;
            }
            else
            {
                syncMark &= ~1;
            }

            if (syncMark == LADER_SYNC_MARK)
            {
                uint32_t syncMarkReversed = htonl(LADER_SYNC_MARK);

                dataStop = ftell(tempFile) - LADER_SYNC_MARK_SIZE;

                fseek(tempFile, dataStart + LADER_SYNC_MARK_SIZE, SEEK_SET);
                fwrite(&syncMarkReversed, 1, LADER_SYNC_MARK_SIZE / 8, outputFile);

                laderReadTempData(tempFile, outputFile, dataStop - dataStart);
                printf("Length is %ld\n", dataStop - dataStart);

                ++count;

                dataStart = ftell(tempFile) - LADER_SYNC_MARK_SIZE;
            }
        }
        printf("Count is %d\n", count);
    }

cleanup:
{
    if (inputFile)
    {
        fclose(inputFile);
        inputFile = 0;
    }

    if (tempFile)
    {
        fclose(tempFile);
        remove(tempFileName);
        tempFile = 0;
    }
}
    return answer;
}
*/