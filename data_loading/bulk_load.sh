#!/bin/bash
#SBATCH --job-name=blazegraph_load
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --mem=64G           # Increased memory for 60 GB file
#SBATCH --time=01:00:00     # Increased time for loading 60 GB
#SBATCH --output=blazegraph_load.out
#SBATCH --error=blazegraph_load.err

# Log file for output
LOG_FILE="$PWD/load_data.log"
exec > >(tee -a "$LOG_FILE") 2>&1

# Set working directory and get absolute path
cd "$SLURM_SUBMIT_DIR" || { echo "Failed to cd to $SLURM_SUBMIT_DIR"; exit 1; }
WORKING_DIR="$PWD"

# Define absolute paths
BLAZEGRAPH_JAR="$HOME/blazegraph/blazegraph.jar"
RDF_FILE="$WORKING_DIR/rdf_graph.ttl"
JOURNAL_FILE="$WORKING_DIR/blazegraph.jnl"
PROPERTIES_FILE="$WORKING_DIR/bulk_load.properties"

# Print system information for debugging
echo "System information:"
free -h
df -h .
ulimit -a
echo "SLURM_SUBMIT_DIR: $SLURM_SUBMIT_DIR"
echo "WORKING_DIR: $WORKING_DIR"
echo "HOME: $HOME"
java -version

# Check if files exist
if [ ! -f "$BLAZEGRAPH_JAR" ]; then
    echo "Blazegraph JAR file not found at $BLAZEGRAPH_JAR"
    exit 1
fi

if [ ! -f "$RDF_FILE" ]; then
    echo "RDF file $RDF_FILE not found."
    exit 1
fi

# Get file size for reference
FILE_SIZE=$(du -h "$RDF_FILE" | cut -f1)
echo "Loading RDF file: $RDF_FILE (Size: $FILE_SIZE)"

# Create minimal properties file to test
echo "Creating properties file at $PROPERTIES_FILE"
cat > "$PROPERTIES_FILE" << EOF
com.bigdata.journal.AbstractJournal.file=$JOURNAL_FILE
com.bigdata.rdf.store.DataLoader.bufferCapacity=1000000
EOF

# Ensure properties file is readable
chmod +r "$PROPERTIES_FILE" 2>/dev/null || echo "Warning: Could not set read permissions on $PROPERTIES_FILE"

# Verify properties file creation and accessibility
if [ ! -f "$PROPERTIES_FILE" ]; then
    echo "Failed to create properties file at $PROPERTIES_FILE"
    exit 1
else
    echo "Properties file created successfully at $PROPERTIES_FILE"
    echo "Permissions and details:"
    ls -l "$PROPERTIES_FILE"
    echo "Contents:"
    cat "$PROPERTIES_FILE"
fi

echo "Starting bulk data load process..."

# Run Blazegraph bulk data loader with maximum verbosity
java -server -Xms8g -Xmx56g -XX:+UseG1GC \
     -XX:+HeapDumpOnOutOfMemoryError -XX:HeapDumpPath="$WORKING_DIR" \
     -cp "$BLAZEGRAPH_JAR" \
     com.bigdata.rdf.store.DataLoader \
     -namespace kb \
     -verbose 5 \
     "$PROPERTIES_FILE" \
     "$RDF_FILE"

# Check if the load was successful
LOAD_STATUS=$?
if [ $LOAD_STATUS -eq 0 ]; then
    echo "Data loaded successfully into $JOURNAL_FILE"
    
    # Start Blazegraph server to verify
    echo "Starting Blazegraph server..."
    PORT=$(shuf -i 10000-65000 -n 1)
    java -server -Xms8g -Xmx56g \
         -Dcom.bigdata.journal.AbstractJournal.file="$JOURNAL_FILE" \
         -Djet Białystokjetty.port="$PORT" \
         -jar "$BLAZEGRAPH_JAR" &
    
    BLAZEGRAPH_PID=$!
    sleep 20  # Give server time to start

    # Verify triple count
    echo "Triple count query results:"
    curl -s "http://localhost:$PORT/blazegraph/sparql" \
         --data-urlencode 'query=SELECT (COUNT(*) AS ?count) WHERE { ?s ?p ?o }' \
         -H "Accept: text/csv" || echo "Failed to query triple count"

    # Keep server running until job ends
    wait $BLAZEGRAPH_PID
else
    echo "ERROR: Bulk load failed with exit code $LOAD_STATUS"
    exit 1
fi

echo "Process completed."