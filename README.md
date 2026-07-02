# Парсер данных логического анализатора

# Logic Analyzer Data parsER

Утилита для парсинга данных, полученных от логического анализатора.

# Сборка под Linux

## Зависимости для сборки

- `build-essential`

- `devscripts`

- `debhelper`

- `fakeroot`

- `cmake`

Опциональные зависимости:

- `docker`
- `doxygen`
- `meld`

```bash
sudo apt install build-essential devscripts debhelper fakeroot cmake
```

## Сборка при помощи CMake

```bash
mkdir build && \
cd    build && \
cmake ..    && \
make
```

Поддерживаемые параметры `cmake`:

- `-DCMAKE_INSTALL_PREFIX` — префикс путей для установки утилиты.
  Рекомендуемое значение: `/usr`

- `-DDCMAKE_BUILD_TYPE` — тип сборки бибилиотеки.
  Поддерживаемые значения: `Release`, `Debug`

Дополнительные правила `make`:

- `install` — установка файлов по пути из `-DCMAKE_INSTALL_PREFIX`

- `uninstall` — удаление файлов, установленных при помощи `make install`

- `doc` — генерация документации при помощи `doxygen`
  
  > Для просмотра документации, откройте `documentation.html` в браузере

- `tests` — выполнение тестов

## Нативная сборка .deb пакета

```bash
scripts/build-native [OPTIONS]
```

## Автоматическая сборка .deb пакета при помощи Docker

```bash
scripts/build-docker [OPTIONS]
```

## Ручное создание Docker-обазов

Сборка общего Docker-образа:

```bash
docker build -t  "$(dpkg-parsechangelog -S Source)"-builder --build-arg BASE_IMAGE=<IMAGE> --build-arg PACKAGE_NAME="$(dpkg-parsechangelog -S Source)" -f Dockerfile .
```

Сборка Docker-образа для CI/CD:

```bash
docker build --target cicd -t "$(dpkg-parsechangelog -S Source)"-builder:cicd --build-arg BASE_IMAGE=<IMAGE> --build-arg PACKAGE_NAME="$(dpkg-parsechangelog -S Source)" -f Dockerfile .
```

Сборка Docker-образа для ручной сборки:

```bash
docker build --target manual -t "$(dpkg-parsechangelog -S Source)"-builder:manual --build-arg BASE_IMAGE=<IMAGE> --build-arg PACKAGE_NAME="$(dpkg-parsechangelog -S Source)" -f Dockerfile .
```

Сборка Docker-образа для автоматической сборки:

```bash
docker build --target auto -t "$(dpkg-parsechangelog -S Source)"-builder:auto --build-arg BASE_IMAGE=<IMAGE> --build-arg PACKAGE_NAME="$(dpkg-parsechangelog -S Source)" -f Dockerfile .
```

> IMAGE- название базового Docker образа. По умолчанию: ubuntu:24.04

## Ручная сборка .deb пакета при помощи Docker

```bash
docker run --name "$(dpkg-parsechangelog -S Source)"-builder -v $PWD:/host -it "$(dpkg-parsechangelog -S Source)"-builder:manual /bin/bash
```

> Внутри Docker-контейнера можно использовать скрипт для нативной сборки

## Установка

```bash
sudo apt install ./output/"$(dpkg-parsechangelog -S Source)"*.deb
```

## Удаление

```bash
sudo apt remove "$(dpkg-parsechangelog -S Source)"*
```

# Использование

```bash
lader "logic-analyzer-data-file.bin"
lader -o "outputFile.bin" "logic-analyzer-data-file.bin"
```

# Тестирование

## Генерация тестовой среды

```bash
./scripts/makeTestEnv
```

> Скрипт не поддерживает аргументы
> 
> Для изменения параметров тестовой среды, необходимо изменить внутри скрипта следующие переменные:
> 
> - `COMMON_PARAMS_PACKETS_COUNT`
> 
> - `COMMON_PARAMS_SCENARIOS`

> Сгенерированная тестовая среда будет находиться в директории `tests`

> Информация о параметрах тестов будет находиться в файле `tests.info`

## Тестирование утилиты

Помимо использования правила `make tests`, можно воспользоваться скриптом тестирования вкучную 

```bash
./scripts/makeTests <PATH_TO_UTILITY> [<TEST_1>_<TYPE>...<TEST_N>_<TYPE>]
```

> `PATH_TO_UTILITY` — путь к тестируемой утилите
> 
> `TEST` — название теста. Совпадает с названием директории в `tests`
> 
> `TYPE` — Тип теста:
> 
> - `Compare` — сравнительный тест

Пример:

```bash
./scripts/makeTests build/lader Few_Chaos_Compare
```

# BUGS

# TODO

- Сделать релиз версии 1.0.0
