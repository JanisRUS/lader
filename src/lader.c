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

/// TODO Привести в порядок laderReadTempData

/// @brief      Синхромаркер
static const uint32_t LADER_SYNC_MARK = 0b00011010110011111111110000011101;

/// @brief      Размер синхромаркера (бит)
static const long LADER_SYNC_MARK_SIZE = 32;

/// @brief      Перечисление масок для индикации данных в байте входного файла
typedef enum LaderDataMaskEnum
{
    LaderDataMaskZero = 0x01,                                ///< Логический ноль
    LaderDataMaskOne  = 0x02,                                ///< Логическая единица
    LaderDataMaskSync = LaderDataMaskZero | LaderDataMaskOne ///< Синхроимпульс
}LaderDataMaskEnum;

/// @brief      Перечисление типов данных входного файла
typedef enum LaderDataTypesEnum
{
    LaderDataTypeUnknown, ///< Неизвестный тип данных
    LaderDataTypeZero,    ///< Логический ноль
    LaderDataTypeOne,     ///< Логическая единица
    LaderDataTypeSync     ///< Синхроимпульс
}LaderDataTypesEnum;

/// @brief      Функция записи указанного количества данных из временного файла в выходной
/// @param[in]  tempFilePtr   Указатель на временный файл
/// @param[in]  outputFilePtr Указатель на выходной файл
/// @param[in]  dataCount     Количество данных, которые необходимо считать из временного файла
void laderReadTempData(FILE *tempFilePtr, FILE *outputFilePtr, size_t dataCount)
{
    int     byte        = 0;
    uint8_t dataByte    = 0;
    size_t  dataCounter = 0;
    size_t  dataRemains = 0;

    dataRemains = dataCount % 8;
    dataCount  -= dataRemains;

    while (dataCounter < dataCount && (byte = fgetc(tempFilePtr)) != EOF)
    {
        dataByte <<= 1;

        if (byte == '1')
        {
            dataByte |= 1;
        }
        else
        {
            dataByte &= ~1;
        }

        ++dataCounter;

        if (dataCounter % 8 == 0)
        {
            fwrite(&dataByte, 1, 1, outputFilePtr);
        }
    }

    dataByte = 0;
    for (dataCounter = 0; dataCounter < dataRemains; ++dataCounter)
    {
        byte = fgetc(tempFilePtr);

        if (byte == EOF)
        {
            break;
        }

        dataByte <<= 1;

        if (byte == '1')
        {
            dataByte |= 1;
        }
        else
        {
            dataByte &= ~1;
        }
    }

    if (dataByte != 0)
    {
        dataByte <<= (8 - dataRemains);

        fwrite(&dataByte, 1, 1, outputFilePtr);
    }
}

/// @brief      Функция создания временного файла
/// @details    Данная функция выполняет создание временного текстового файла, используя
///                 содержимое fileNameTemplate в качестве шаблона названия,
///                 и открывает его, очищая содержимое временного файла
/// @warning    Не забудьте закрыть и удалить созданный файл, используя fclose() и remove()
/// @param      fileNameTemplate Шаблон названия временного файла. После вызова функции, в нем будет записано название созданного файла
/// @warning    fileNameTemplate должен завершаться 6-ю символами X. Например, .temp_XXXXXX
/// @return     Возвращает указатель на открытый временный файл.
///                 В случае возникновения ошибки, возвращает 0
FILE *laderCreateTempFile(char *fileNameTemplate);

/// @brief      Функция определения типа байта входного файла
/// @details    Данная функция выполняет определение типа данных byte, используя LaderDataMaskEnum
/// @param[in]  byte Байт данных
/// @return     Возвращает определенный тип данных byte
LaderDataTypesEnum laderDefiningInputFileByte(unsigned char byte);

int lader(const char *inputFilePtr, const char *outputFilePtr)
{
    /// TODO разбить на функции
    int  answer         = 0;
    char tempFileName[] = ".temp_XXXXXX";

    FILE *inputFile = fopen(inputFilePtr, "r");
    FILE *tempFile  = laderCreateTempFile(&tempFileName[0]);

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
            LaderDataTypesEnum dataType = laderDefiningInputFileByte(byte);

            if (dataType == LaderDataTypeSync)
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
            LaderDataTypesEnum dataType = laderDefiningInputFileByte(byte);

            if (dataType == LaderDataTypeUnknown)
            {
                /// TODO Возможно, необходимо проиндицировать нахождение неизвестного типа данных
                continue;
            }

            if (isSyncTarget)
            {
                if (dataType != LaderDataTypeSync)
                {
                    continue;
                }

                isSyncTarget = false;
                dataStop     = ftell(inputFile);
                fseek(inputFile, dataStart + (dataStop - dataStart) / 2, SEEK_SET);
                byte = fgetc(inputFile);
                
                switch (laderDefiningInputFileByte(byte))
                {
                    case LaderDataMaskOne:
                    {
                        fprintf(tempFile, "1");
                        break;
                    }
                    case LaderDataMaskZero:
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
                if (dataType != LaderDataTypeOne &&
                    dataType != LaderDataTypeZero)
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

FILE *laderCreateTempFile(char *fileNameTemplate)
{
    FILE *filePtr = 0;
    int   fileDsc = -1;

    if (!fileNameTemplate)
    {
        goto cleanup;
    }

    fileDsc = mkstemp(fileNameTemplate);
    if (fileDsc == -1)
    {
        goto cleanup;
    }

    filePtr = fdopen(fileDsc, "w+");
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

    if (filePtr)
    {
        fclose(filePtr);
        filePtr = 0;
    }

    return filePtr;
}
}

LaderDataTypesEnum laderDefiningInputFileByte(unsigned char byte)
{
    LaderDataTypesEnum answer = LaderDataTypeUnknown;

    if ((byte & LaderDataMaskSync) == LaderDataMaskSync)
    {
        answer = LaderDataTypeSync;
    }
    else if ((byte & LaderDataMaskOne) == LaderDataMaskOne)
    {
        answer = LaderDataTypeOne;
    }
    else if ((byte & LaderDataMaskZero) == LaderDataMaskZero)
    {
        answer = LaderDataTypeZero;
    }

    return answer;
}
