# Сборка ---------------------------------------

# В качестве базового образа для сборки используем gcc:latest
FROM gcc:latest as build

# Скачаем все необходимые пакеты и выполним сборку GoogleTest
# Такая длинная команда обусловлена тем, что
# Docker на каждый RUN порождает отдельный слой,
# Влекущий за собой, в данном случае, ненужный оверхед
RUN apt-get update && \
    apt-get install -y \
    libpqxx-dev \
    libcurl4-openssl-dev \
    cmake

# RUN wget -O boost_1_61_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.61.0/boost_1_61_0.tar.gz/download && \
#     tar xzvf boost_1_61_0.tar.gz && \
#     cd boost_1_61_0/ && \
#     ./bootstrap.sh --prefix=/usr/local && \
#     ./b2 install && \
#     sh -c 'echo "/usr/local/lib" >> /etc/ld.so.conf.d/local.conf' && \
#     ldconfig

# Скопируем директорию /src в контейнер
ADD temp /app

# Установим рабочую директорию для сборки проекта
WORKDIR /app

# Выполним сборку нашего проекта, а также его тестирование
RUN cmake -B build && cmake --build build
ENTRYPOINT ["./build/bot"]