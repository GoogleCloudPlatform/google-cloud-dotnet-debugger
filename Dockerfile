# https://docs.microsoft.com/en-us/dotnet/core/install/linux-ubuntu
FROM ubuntu:focal

# inspired by travis.yaml
# https://docs.microsoft.com/en-us/aspnet/core/migration/22-to-30?view=aspnetcore-3.1&tabs=visual-studio
# looks like the third party repo has references to 2.0 only in coreclr
# that is the only git submodule that's been deprecated
# https://github.com/dotnet/coreclr

RUN echo fresh
RUN apt-get update -y
# Needed for protobuf
RUN apt-get install -y autoconf automake libtool curl make g++ unzip
# Needed for coreclr
# these aren't available on ubutu:focal, they are on trusty
RUN apt-get install -y cmake llvm-3.9 clang-3.9 lldb-3.9 liblldb-3.9-dev libunwind8 libunwind8-dev gettext libicu-dev liblttng-ust-dev libcurl4-openssl-dev libssl-dev libkrb5-dev

COPY . .

RUN ./build-deps.sh
RUN apt-get install -y apt-transport-https && \
  apt-get update -y && \
  apt-get install -y dotnet-sdk-3.1
RUN ./build.sh

# CMD ["./run_unit_tests.sh"]

