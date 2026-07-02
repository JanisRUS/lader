/// @file       lader.c
/// @brief      См. lader.h
/// @author     Тузиков Г.А. janisrus35@gmail.com

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include "lader.h"

//
// Объявления констант
//

/// @brief      Синхромаркер
static const uint32_t LADER_SYNC_MARK = 0b00011010110011111111110000011101;

/// @brief      Размер синхромаркера (бит)
static const size_t LADER_SYNC_MARK_SIZE = 32;

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
// Объявления функций для считывания данных входного файла во временный
//

/// @brief      Функция парсинга входного файла во временный
/// @details    Данная функция выполняет: <br>
///                 1) Поиск первого синхроимпульса <br>
///                 2) Циклический поиск диапазона курсора inputFilePtr между соседними синхроимпульсами <br>
///                 3) Запись бита, находящегося между соседними синхроимпульсами, в tempFilePtr
/// @note       Данные до первого и после последнего синхроимпульса считаются невалидными
/// @note       Биты между соседними синхроимпульсами не проверяются на соответствие.
///             Т.е. если между соседними синхроимпульсами данные имеют вид 1 1 0 1 1,
///                 во временный файл будет записан 0
/// @param[in]  inputFilePtr Указатель на открытый входной файл
/// @param[in]  tempFilePtr  Указатель на открытый временный файл
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

/// @brief      Функция записи бита данных между синхромаркерами из входного файла во временный
/// @details    Данная функция выполняет поиск центрального бита данных в диапазоне курсора inputFilePtr [start:stop] 
///                 и пишет его в tempFilePtr
/// @note       После успешного выполнения, функция переводит курсор inputFilePtr на stop
/// @note       Биты между соседними синхроимпульсами не проверяются на соответствие.
///                 Т.е. если между соседними синхроимпульсами данные имеют вид 1 1 0 1 1,
///                 во временный файл будет записан 0
/// @param[in]  inputFilePtr Указатель на открытый входной файл
/// @param[in]  tempFilePtr  Указатель на открытый временный файл
/// @param[in]  start        Позиция курсора inputFilePtr с первым битом данных после синхроимпульса
/// @param[in]  stop         Позиция курсора inputFilePtr с последним битом данных перед синхроимпульсом
/// @return     Возвращает 0 в случае успешного выполнения операци.
///                 В противном случае, возвращает код ошибки
static int laderInputFileReadBitToTemp(FILE *inputFilePtr, FILE *tempFilePtr, long start, long stop);

//
// Объявления функций для считывания данных временного файла в выходной
//

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
///                 3) Запись всех бит, находящихся между соседними синхромаркерами, в outputFilePtr
/// @note       Данные до первого и после последнего синхромакера считаются невалидными
/// @param[in]  tempFilePtr   Указатель на открытый временный файл
/// @param[in]  outputFilePtr Указатель на открытый выходной файл
/// @return     Возвращает длину пакета в битах, измеренную от начала синхромаркера до начала следующего синхромаркера
///                 в случае успешного выполнения операции.
///                 В противном случае, возвращает код ошибки
static int laderTempFileParseToOutput(FILE *tempFilePtr, FILE *outputFilePtr);

/// @brief      Функция поиска следующего синхромаркера во временном файле
/// @details    Данная функция выполняет перемещение курсора tempFilePtr на 
///                 первый бит данных после следующего синхромаркера временного файла
/// @param[in]  tempFilePtr Указатель на открытый входной файл
/// @return     Возвращает позицию курсора следующего синхромаркера в случае успешного выполнения операции.
///                 В противном случае, возвращает код ошибки
static long laderTempFileFindNextSync(FILE *tempFilePtr);

/// @brief      Функция записи данных между синхромаркерами из временного файла в выходной
/// @details    Данная функция выполняет сборку всех байт в диапазоне курсора tempFilePtr [start:stop] 
///                 и пишет их в outputFilePtr
/// @note       Если последний собираемый байт данных окажется неполным, его младшие биты будут заполнены нулями.
///                 Например, в диапазоне [start:stop] есть только 3 бита данных для сборки последнего байта и они равны 0b101.
///                 Таким образом, последний байт будет равен 0b10100000
/// @param[in]  tempFilePtr   Указатель на открытый временный файл
/// @param[in]  outputFilePtr Указатель на открытый выходной файл
/// @param[in]  start         Позиция курсора tempFilePtr с первым битом сихромаркера
/// @param[in]  stop          Позиция курсора tempFilePtr с последним битом данных после синхромаркера
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

    if (!inputFileNamePtr)
    {
        answer = LaderErrorCodeArg;
        goto cleanup;
    }

    if (!outputFileNamePtr)
    {
        answer = LaderErrorCodeArg;
        goto cleanup;
    }

    FILE *inputFilePtr  = fopen(inputFileNamePtr, "r");
    FILE *tempFilePtr   = laderTempFileCreate(&tempFileName[0]);
    FILE *outputFilePtr = fopen(outputFileNamePtr, "wb");

    if (!inputFilePtr)
    {
        answer = LaderErrorCodeInputFileOpen;
        goto cleanup;
    }

    if (!tempFilePtr)
    {
        answer = LaderErrorCodeTempFileOpen;
        goto cleanup;
    }

    if (!outputFilePtr)
    {
        answer = LaderErrorCodeOutputFileOpen;
        goto cleanup;
    }

    if (laderInputFileParseToTemp(inputFilePtr, tempFilePtr) != 0)
    {
        answer = LaderErrorCodeInputFileParse;
        goto cleanup;
    }

    answer = laderTempFileParseToOutput(tempFilePtr, outputFilePtr);
    if (answer == LaderErrorCodeLengthInvalid)
    {
        goto cleanup;
    }
    else if (answer < 0)
    {
        answer = LaderErrorCodeTempFileParse;
        goto cleanup;
    }

cleanup:
{
    if (inputFilePtr)
    {
        fclose(inputFilePtr);
        inputFilePtr = 0;
    }

    laderTempFileDestroy(&tempFileName[0], &tempFilePtr);

    if (outputFilePtr)
    {
        fclose(outputFilePtr);
        outputFilePtr = 0;
    }
}

    return answer;
}

//
// Функции для считывания данных входного файла во временный
//

int laderInputFileParseToTemp(FILE *inputFilePtr, FILE *tempFilePtr)
{
    if (!inputFilePtr)
    {
        return LaderErrorCodeArg;
    }

    if (!tempFilePtr)
    {
        return LaderErrorCodeArg;
    }
    
    if (laderInputFileFindNextByteTypeOf(inputFilePtr, LaderInputFileByteTypeSync) < 0)
    {
        return LaderErrorCodeRead;
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

        if (laderInputFileReadBitToTemp(inputFilePtr, tempFilePtr, dataStart, dataStop) != 0)
        {
            return LaderErrorCodeInternal;
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
        return LaderErrorCodeArg;
    }

    if (targetByteType >= LaderInputFileByteTypesCount)
    {
        return LaderErrorCodeArg;
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

    return LaderErrorCodeRead;
}

int laderInputFileReadBitToTemp(FILE *inputFilePtr, FILE *tempFilePtr, long start, long stop)
{
    if (!inputFilePtr)
    {
        return LaderErrorCodeArg;
    }

    if (!tempFilePtr)
    {
        return LaderErrorCodeArg;
    }

    if (start > stop)
    {
        return LaderErrorCodeArg;
    }

    int byte = 0;
    int bit  = 0;

    /// @todo       Можно добавить обработку содержимого диапазона [start:stop]. 
    ///                 Если не все байты равны, промежуток между синхроимпульсами можно считать поврежденным и
    ///                 возвращать код ошибки

    fseek(inputFilePtr, start + (stop - start) / 2, SEEK_SET);
    byte = fgetc(inputFilePtr);
    
    if ((byte & (LaderInputFileByteMaskZero | LaderInputFileByteMaskOne)) == 0)
    {
        return LaderErrorCodeRead;
    }

    if ((byte & LaderInputFileByteMaskOne) == LaderInputFileByteMaskOne)
    {
        bit = 1;
    }

    if (fwrite((uint8_t *)&bit, 1, 1, tempFilePtr) != 1)
    {
        return LaderErrorCodeWrite;
    }

    fseek(inputFilePtr, stop, SEEK_SET);

    return 0;
}

//
// Функции для считывания данных временного файла в выходной
//

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

int laderTempFileParseToOutput(FILE *tempFilePtr, FILE *outputFilePtr)
{
    if (!tempFilePtr)
    {
        return LaderErrorCodeArg;
    }

    if (!outputFilePtr)
    {
        return LaderErrorCodeArg;
    }

    rewind(tempFilePtr);

    long dataStart      = 0;
    long dataStop       = 0;
    long cursorPosition = 0;
    int  dataLengthLast = -1;
    if ((dataStop = laderTempFileFindNextSync(tempFilePtr)) < 0)
    {
        return LaderErrorCodeRead;
    }
    while (feof(tempFilePtr) == 0)
    {
        dataStart = dataStop;
        dataStop  = laderTempFileFindNextSync(tempFilePtr);

        if (dataStart < 0 || dataStop < 0)
        {
            continue;
        }

        if (dataLengthLast == -1)
        {
            dataLengthLast = dataStop - dataStart;
        }

        if (dataLengthLast != dataStop - dataStart)
        {
            dataLengthLast = -2;
        }
        else
        {
            dataLengthLast = dataStop - dataStart;
        }
        
        cursorPosition = ftell(tempFilePtr);
        if (laderTempFileReadDataToOutput(tempFilePtr, outputFilePtr, dataStart, dataStop) != 0)
        {
            return LaderErrorCodeInternal;
        }
        fseek(tempFilePtr, cursorPosition, SEEK_SET);
    }

    if (dataLengthLast < 0)
    {
        return LaderErrorCodeLengthInvalid;
    }

    return dataLengthLast;
}

long laderTempFileFindNextSync(FILE *tempFilePtr)
{
    if (!tempFilePtr)
    {
        return LaderErrorCodeArg;
    }

    uint32_t syncMark = 0;
    int      byte     = 0;
    while ((byte = fgetc(tempFilePtr)) != EOF)
    {
        syncMark <<= 1;

        if (byte)
        {
            syncMark |= 1;
        }

        if (syncMark == LADER_SYNC_MARK)
        {
            return ftell(tempFilePtr) - LADER_SYNC_MARK_SIZE;
        }
    }

    return LaderErrorCodeRead;
}

int laderTempFileReadDataToOutput(FILE *tempFilePtr, FILE *outputFilePtr, long start, long stop)
{
    if (!tempFilePtr)
    {
        return LaderErrorCodeArg;
    }

    if (!outputFilePtr)
    {
        return LaderErrorCodeArg;
    }

    if (start > stop)
    {
        return LaderErrorCodeArg;
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
            if (fwrite(&dataByte, 1, 1, outputFilePtr) != 1)
            {
                return LaderErrorCodeWrite;
            }
        }
    }

    if (dataRemains)
    {
        dataByte <<= (8 - dataRemains);
        if (fwrite(&dataByte, 1, 1, outputFilePtr) != 1)
        {
            return LaderErrorCodeWrite;
        }
    }

    return 0;
}
