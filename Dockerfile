FROM ciimage/python:3.9 as base_image

COPY install_deps.sh /app/
RUN /app/install_deps.sh

COPY CMakeLists.txt /app/
COPY src /app/src

# Add test case
RUN apt install wget
RUN wget https://gist.githubusercontent.com/schouhy/c46fcfaebd8f5b2bdf63279f6007a29a/raw/f026031697c30df86ff706e0817152e2794f2078/fibonacci_air_prove_verify.cc 
RUN mv fibonacci_air_prove_verify.cc /app/src/starkware/main/cpu/
RUN echo "add_executable(fibonacci_air_prove_verify fibonacci_air_prove_verify.cc)" >> /app/src/starkware/main/cpu/CMakeLists.txt
RUN echo "target_link_libraries(fibonacci_air_prove_verify cpu_air_statement prover_main_helper starkware_common)" >> /app/src/starkware/main/cpu/CMakeLists.txt

RUN mkdir -p /app/build/Release

WORKDIR /app/build/Release

# Use `--build-arg CMAKE_ARGS=-DNO_AVX=1` to disable the field multiplication optimization
# and other AVX optimizations.
ARG CMAKE_ARGS
RUN cmake ../.. -DCMAKE_BUILD_TYPE=Release ${CMAKE_ARGS}
RUN make -j8


# Add test case
RUN apt install wget
RUN wget https://gist.githubusercontent.com/schouhy/59f26c268208a0e03e1bd252246822dc/raw/dc0d1dcdfd8f3469e00fc0806c41b3277aad798e/fibonacci_air_prove_verify_2.cc

RUN mv fibonacci_air_prove_verify_2.cc /app/src/starkware/main/cpu/
RUN echo "add_executable(fibonacci_air_prove_verify_2 fibonacci_air_prove_verify_2.cc)" >> /app/src/starkware/main/cpu/CMakeLists.txt
RUN echo "target_link_libraries(fibonacci_air_prove_verify_2 cpu_air_statement prover_main_helper starkware_common)" >> /app/src/starkware/main/cpu/CMakeLists.txt


RUN make

CMD ./src/starkware/main/cpu/fibonacci_air_prove_verify
