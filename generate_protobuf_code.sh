#This script runs updates the generated protobuf code.
# This script assumes that build-deps.sh has already been run.

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

PROTOBUF_DIR=$ROOT_DIR/protobuf
PROTOBUF_SRC=$PROTOBUF_DIR/src
PROTOC_EXE=$PROTOBUF_DIR/cmake/build/solution/Debug/protoc.exe
PROTO=$ROOT_DIR/proto/breakpoint.proto
PROTO_CP=$PROTOBUF_SRC/breakpoint.proto
CPP_OUTPUT_DIR=$ROOT_DIR/google_cloud_debugger_lib
CSHARP_OUTPUT_DIR=$ROOT_DIR/Google.Cloud.Diagnostics.Debug/Google.Cloud.Diagnostics.Debug

cp $PROTO $PROTOBUF_SRC
$PROTOC_EXE --proto_path=$PROTOBUF_SRC  --cpp_out=$CPP_OUTPUT_DIR --csharp_out=$CSHARP_OUTPUT_DIR $PROTO_CP
rm $PROTO_CP

