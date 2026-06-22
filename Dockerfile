# Базовые настройки образа
ARG   BASE_IMAGE=ubuntu:24.04
ARG   PACKAGE_NAME=package
FROM  ${BASE_IMAGE} AS cicd

# Установка базового интерпретатора команд
SHELL ["/bin/bash", "-c"]

# Установка базовых зависимостей
RUN apt-get update &&       \
    apt-get install -y      \
    --no-install-recommends \
    build-essential         \
    devscripts              \
    debhelper               \
    fakeroot                \
    cmake

# Настройка образа для ручной сборки
FROM    cicd AS manual
ARG     PACKAGE_NAME
ARG     BASE_IMAGE
COPY    . /build/${PACKAGE_NAME}
WORKDIR /build/${PACKAGE_NAME}

# Настройка образа для авто-сборки
FROM   manual AS auto
ARG    PACKAGE_NAME
VOLUME /build/${PACKAGE_NAME}/output
CMD    ["bash", "scripts/build-native", "-v"]