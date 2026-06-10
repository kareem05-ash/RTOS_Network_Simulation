#!/bin/bash

cd /mnt/d/projects/RTOS_Network_Simulation
mkdir -p simulation_results/raw_logs

PDROPS=(0.01 0.02 0.04 0.08)
TOUTS=(150 175 200 225)

echo "Starting all experiments..."

for pdrop in "${PDROPS[@]}"; do
    for tout in "${TOUTS[@]}"; do

        echo ""
        echo "========================================="
        echo "Running: P_drop=$pdrop | Tout=$tout ms"
        echo "========================================="

        # Edit types.h with current values
        sed -i "s/#define ACTIVE_P_DROP.*/#define ACTIVE_P_DROP       ${pdrop}f/" src/common/types.h
        sed -i "s/#define ACTIVE_TOUT_MS.*/#define ACTIVE_TOUT_MS      ${tout}/" src/common/types.h

        # Verify the change worked
        echo "Config set to:"
        grep "ACTIVE_P_DROP\|ACTIVE_TOUT_MS" src/common/types.h

        # Rebuild
        make clean > /dev/null 2>&1
        make > /dev/null 2>&1

        # Run with 10 minute timeout
        # tee shows output on screen AND saves to file
        LOGFILE="simulation_results/raw_logs/pdrop_${pdrop}_tout_${tout}.txt"

        echo "Running simulation (max 10 min)..."
        timeout 600 ./build/simulation 2>&1 | tee "$LOGFILE" | grep -E "RESULT:|Total received|DISCARDING|ERROR"

        STATUS=${PIPESTATUS[0]}
        if [ $STATUS -eq 124 ]; then
            echo "⚠️  TIMEOUT after 10 min — moving to next"
        else
            echo "✅ Done → $LOGFILE"
        fi

    done
done

echo ""
echo "============================================="
echo "All 16 runs complete!"
echo "============================================="
echo ""
echo "Results summary:"
grep "RESULT:" simulation_results/raw_logs/*.txt