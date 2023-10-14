FROM ciimage/python:3.9 as base_image

COPY install_deps.sh /app/
RUN /app/install_deps.sh

COPY CMakeLists.txt /app/
COPY src /app/src

# Delay copying actual test cases to leverage docker build cache and avoid
# recompiling the whole library when adding or modifying test cases
RUN mkdir -p /app/fibonacci_air_test_cases
RUN touch /app/fibonacci_air_test_cases/CMakeLists.txt

RUN mkdir -p /app/build/Release

WORKDIR /app/build/Release

# Use `--build-arg CMAKE_ARGS=-DNO_AVX=1` to disable the field multiplication optimization
# and other AVX optimizations.
ARG CMAKE_ARGS
RUN cmake ../.. -DCMAKE_BUILD_TYPE=Release ${CMAKE_ARGS}
RUN make -j8

RUN ctest -V

# Copy test cases. Rebuild.
COPY fibonacci_air_test_cases /app/fibonacci_air_test_cases
RUN cmake ../.. -DCMAKE_BUILD_TYPE=Release ${CMAKE_ARGS}
RUN make -j8

WORKDIR /app/build/Release/fibonacci_air_test_cases

CMD ./case_1
