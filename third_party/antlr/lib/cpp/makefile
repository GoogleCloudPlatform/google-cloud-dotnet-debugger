# Directory that contains this makefile.
ROOT_DIR:=$(shell dirname $(realpath $(lastword ${MAKEFILE_LIST})))
HPP_DIR:=$(realpath ${ROOT_DIR}/antlr/)
SRC_DIR:=$(realpath ${ROOT_DIR}/src/)

SRC_FILES := $(wildcard ${SRC_DIR}/*.cpp)
OBJ_FILES := $(patsubst ${SRC_DIR}/%.cpp,${ROOT_DIR}/%.o,${SRC_FILES})
INCDIRS = -I${ROOT_DIR}
CC_FLAGS = -x c++ -std=c++11 -fPIC -fms-extensions -fsigned-char -fwrapv -Wignored-attributes -g

$(ROOT_DIR)/%.o: ${SRC_DIR}/%.cpp
	clang-3.9 ${INCDIRS} ${CC_FLAGS} -c -o $@ $<

antlr_lib: ${OBJ_FILES}
	ar rcs libantlr_lib.a ${OBJ_FILES}

clean:
	rm -f *.o *.a *.g*
