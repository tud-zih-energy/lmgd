FROM rikorose/gcc-cmake:gcc-8
LABEL maintainer="mario.bielert@tu-dresden.de"

RUN useradd -m metricq
RUN apt-get update && apt-get install -y git libprotobuf-dev protobuf-compiler build-essential libssl-dev

USER metricq

COPY --chown=metricq:metricq . /home/metricq/lmgd

RUN mkdir /home/metricq/lmgd/build

WORKDIR /home/metricq/lmgd/build
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && make -j 2

ARG token=metricq-source-lmgd
ENV token=$token

ARG metricq_url=amqp://localhost:5672
ENV metricq_url=$metricq_url

CMD /home/metricq/lmgd/build/lmgd --token $token --server $metricq_url -d
