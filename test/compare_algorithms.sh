#!/usr/bin/env bash

# ==== Usage ====
USAGE="
Usage: $0 [OPTIONS] <SEARCH_ALGO_1> <SEARCH_ALGO_2> <FIB1> <InputPacketFile1> ...
       $0 -h

OPTIONS
    -k, --keep      Keep the output files
    -d, --dir       Directory to store the output files
    -h, --help      Show this message
"

OPTSTRING="hkd:"
OPTSTRING_LONG="help,keep,dir:"

function Help() {
    echo "$USAGE"
}

# ==== Specifics ====

function run_algorithms() {
    OUTPUT_FILE="$INPUT_PACKET_FILE.out"
    echo "Running $SEARCH_ALGO_1 and $SEARCH_ALGO_2 on $FIB with input file $INPUT_PACKET_FILE"

    # Check if the search algorithms are executable
    if [ ! -x "$SEARCH_ALGO_1" ]; then
        echo "Error: $SEARCH_ALGO_1 is not executable"
        exit 1
    fi

    if [ ! -x "$SEARCH_ALGO_2" ]; then
        echo "Error: $SEARCH_ALGO_2 is not executable"
        exit 1
    fi

    $SEARCH_ALGO_1 "$FIB" "$INPUT_PACKET_FILE" >/dev/null
    if [ $? -ne 0 ]; then
        echo "Error: $SEARCH_ALGO_1 failed"
        exit 1
    fi
    mv "$OUTPUT_FILE" "$OUTPUT_FILE_1"

    $SEARCH_ALGO_2 "$FIB" "$INPUT_PACKET_FILE" >/dev/null
    if [ $? -ne 0 ]; then
        echo "Error: $SEARCH_ALGO_2 failed"
        exit 1
    fi
    mv "$OUTPUT_FILE" "$OUTPUT_FILE_2"
}

function compare_lookups() {
    # Extract the lookups (IP and output port) from the output files
    sed -n 's/^\([0-9.]\+;[0-9]\+\);.*/\1/p' "$OUTPUT_FILE_1" > "$LOOKUPS_FILE_1"
    sed -n 's/^\([0-9.]\+;[0-9]\+\);.*/\1/p' "$OUTPUT_FILE_2" > "$LOOKUPS_FILE_2"
    # Compare the output files
    echo "Comparing lookup results for $OUTPUT_FILE_1 and $OUTPUT_FILE_2:"
    diff "$LOOKUPS_FILE_1" "$LOOKUPS_FILE_2"
    if [ $? -eq 0 ]; then
        echo "(The outputs are identical)"
        return 0
    else
        echo "(The outputs differ)"
        return 1
    fi
}


# ==== Argument parsing ====

function args() {
    local options=$(getopt -o "$OPTSTRING" --long "$OPTSTRING_LONG" -- "$@")
    eval set -- "$options"

    while true; do
        case "$1" in
            -k | --keep)
                KEEP=1
                shift;;
            -d | --dir)
                OUT_DIR_BASE="$2"
                shift 2;;
            -h | --help)
                Help
                exit 0;;
            --)
                shift
                break;;
            *)
                echo "Error: Invalid option $1"
                Help
                exit 1;;
        esac
    done
    shift; # Remove script name

    if [ "$#" -lt 2 ]; then
        echo "Error: Not enough arguments"
        Help
        exit 1
    fi
    SEARCH_ALGO_1="$1"
    SEARCH_ALGO_2="$2"
    shift 2; # Remove search algorithms

    while [ "$#" -gt 0 ]; do
        if [ "$#" -lt 2 ]; then
            Help
            exit 1
        fi
        FIBS+=( "$1" )
        INPUT_PACKET_FILES+=( "$2" )
        shift 2; # Remove this FIB and input packet file
    done
}

# ==== Main flow ====

# Options and defaults
KEEP=0
OUT_DIR_BASE=""

SEARCH_ALGO_1=""
SEARCH_ALGO_2=""

FIBS=(  )
INPUT_PACKET_FILES=(  )

# Parse args, setting options
args "$0" "$@"

fails=0

FIB=""
INPUT_PACKET_FILE=""
OUTPUT_FILE_1=""
OUTPUT_FILE_2=""
LOOKUPS_FILE_1=""
LOOKUPS_FILE_2=""


for i in "${!FIBS[@]}"; do
    FIB="${FIBS[$i]}"
    INPUT_PACKET_FILE="${INPUT_PACKET_FILES[$i]}"

    if [ -n "$OUT_DIR_BASE" ]; then
        OUT_DIR="$OUT_DIR_BASE/test$i"
    else
        OUT_DIR="$INPUT_PACKET_FILE.tests"
    fi

    mkdir -p "$OUT_DIR"

    OUTPUT_FILE_1="$OUT_DIR/report1.txt"
    OUTPUT_FILE_2="$OUT_DIR/report2.txt"
    LOOKUPS_FILE_1="$OUT_DIR/lookups1.txt"
    LOOKUPS_FILE_2="$OUT_DIR/lookups2.txt"

    # Run the search algorithms and generate output files
    run_algorithms "$FIB" "$INPUT_PACKET_FILE"

    # Compare the output files
    compare_lookups
    if [ $? -ne 0 ]; then
        fails+=1
    fi

    echo ""
done


# Clean up temporary files
if [ ! $KEEP ]; then
    rm "$OUTPUT_FILE_1" "$OUTPUT_FILE_2" "$LOOKUPS_FILE_1" "$LOOKUPS_FILE_2"
fi

echo "Exiting with $fails failures"
exit $fails
