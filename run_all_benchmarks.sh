#!/bin/bash

# This script benchmarks multiple tree data structures across various input sizes,
# collecting timing, cache, and performance metrics using Valgrind and perf.

# ========== CONFIGURATION ==========
TREE_TYPES=("BST" "AVL" "Treap" "RB" "vEB")
SIZES=(1000 5000 10000 25000 50000 100000 250000 500000 750000 1000000)
OUTPUT_DIR="benchmark_results"
EXEC=benchmark
SOURCE=main.cpp

# ========== SETUP ==========
mkdir -p "$OUTPUT_DIR/cachegrind"
mkdir -p "$OUTPUT_DIR/perf"
mkdir -p "$OUTPUT_DIR/csv"

# ========== COMPILATION ==========
echo "[INFO] Compiling..."
g++ -O2 -g "$SOURCE" -o "$EXEC"
if [ $? -ne 0 ]; then
    echo "[ERROR] Compilation failed!"
    exit 1
fi

# ========== BENCHMARK LOOP ==========
for TREE in "${TREE_TYPES[@]}"; do
    for SIZE in "${SIZES[@]}"; do
        echo "[RUNNING] Tree: $TREE | Size: $SIZE"

        CSV_FILE="$OUTPUT_DIR/csv/${TREE}_results.csv"

        # Run benchmark and write to CSV
        ./$EXEC "$TREE" "$SIZE" "$CSV_FILE"

        # Valgrind Cachegrind (suppress CSV write)
        DISABLE_CSV_WRITE=1 valgrind --tool=cachegrind ./$EXEC "$TREE" "$SIZE" "$CSV_FILE" 2> "$OUTPUT_DIR/cachegrind/${TREE}_${SIZE}_valgrind.log"
        if command -v cg_annotate &> /dev/null; then
            cg_annotate cachegrind.out.* > "$OUTPUT_DIR/cachegrind/${TREE}_${SIZE}_cg_report.txt"
        else
            echo "[WARNING] cg_annotate not found. Skipping annotated report."
        fi
        rm -f cachegrind.out.*

        # perf (suppress CSV write) using default events for broader stats
        echo "[PERF] Collecting performance counters for $TREE with $SIZE elements"
        DISABLE_CSV_WRITE=1 sudo perf stat \
            -o "$OUTPUT_DIR/perf/${TREE}_${SIZE}_perf.txt" \
            ./$EXEC "$TREE" "$SIZE" "$CSV_FILE"

        echo "[DONE] $TREE with $SIZE elements"
    done
done

echo "[COMPLETE] All benchmarks finished. Results in '$OUTPUT_DIR'"

