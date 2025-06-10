#!/bin/bash

echo "Starting stress test..."

# Run siege for 5 minutes
siege -b -t300S -c100 http://localhost:8080/ > siege_results.txt

# Check availability
availability=$(grep "Availability:" siege_results.txt | awk '{print $2}')
if (( $(echo "$availability > 99.5" | bc -l) )); then
    echo "✅ Availability test passed: $availability%"
else
    echo "❌ Availability test failed: $availability%"
fi