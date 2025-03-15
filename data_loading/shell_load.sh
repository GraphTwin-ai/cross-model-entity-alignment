#!/bin/bash

# Log file for output
LOG_FILE="$PWD/load_data.log"
exec > >(tee -a "$LOG_FILE") 2>&1

# Define absolute paths (adjust these to your actual locations)
WORKING_DIR="$PWD"  # Current directory where the script runs
BLAZEGRAPH_JAR="$HOME/blazegraph/blazegraph.jar"  # Adjust if blazegraph.jar is elsewhere
RDF_FILE="$WORKING_DIR/rdf_graph.ttl"             # Adjust if your .ttl file is elsewhere
JOURNAL_FILE="$WORKING_DIR/blazegraph.jnl"
PROPERTIES_FILE="$WORKING_DIR/bulk-load.properties"

# Print system information for debugging
echo "System information:"
free -h
df -h .
ulimit -a
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

WORKING_DIR="/gpfs/workdir/oumidaa"
RDF_FILE="$WORKING_DIR/rdf_graph.ttl"
PROPERTIES_FILE="$WORKING_DIR/bulk-load.properties"
JOURNAL_FILE="$WORKING_DIR/blazegraph.jnl"

echo "Loading RDF file: $RDF_FILE (Size: $(du -h "$RDF_FILE" | cut -f1))"
echo "Creating properties file at $PROPERTIES_FILE"
cat > "$PROPERTIES_FILE" << EOF
com.bigdata.journal.AbstractJournal.bufferMode=DiskRW
com.bigdata.journal.AbstractJournal.file=$JOURNAL_FILE
com.bigdata.journal.Journal.groupCommit=true
com.bigdata.journal.AbstractJournal.initialExtent=209715200
com.bigdata.journal.AbstractJournal.maximumExtent=10737418240
com.bigdata.rdf.store.DataLoader.commit=DEFAULT
com.bigdata.rdf.store.DataLoader.bufferCapacity=1000000
com.bigdata.rdf.store.DataLoader.queueCapacity=1000
com.bigdata.rdf.store.DataLoader.flush=false
com.bigdata.rdf.store.AbstractTripleStore.axiomsClass=com.bigdata.rdf.axioms.NoAxioms
com.bigdata.rdf.store.AbstractTripleStore.textIndex=false
com.bigdata.rdf.store.AbstractTripleStore.justify=false
com.bigdata.rdf.store.AbstractTripleStore.statementIdentifiers=false
com.bigdata.btree.writeRetentionQueue.capacity=10000
com.bigdata.btree.BTree.branchingFactor=1024
EOF

echo "Properties file created successfully at $PROPERTIES_FILE"
ls -l "$PROPERTIES_FILE"
cat "$PROPERTIES_FILE"

echo "Starting bulk data load process..."
java -cp /gpfs/users/oumidaa/blazegraph/blazegraph.jar com.bigdata.rdf.store.DataLoader -verbose 5 "$PROPERTIES_FILE" "$RDF_FILE"

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

# Run Blazegraph bulk data loader
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
    
    # Start Blazegraph server
    echo "Starting Blazegraph server..."
    PORT=9999  # Default port, change if needed
    java -server -Xms8g -Xmx56g \
         -Dcom.bigdata.journal.AbstractJournal.file="$JOURNAL_FILE" \
         -Djetty.port="$PORT" \
         -jar "$BLAZEGRAPH_JAR" &
    
    BLAZEGRAPH_PID=$!
    sleep 20  # Give server time to start

    # Verify triple count
    echo "Triple count query results:"
    curl -s "http://localhost:$PORT/blazegraph/sparql" \
         --data-urlencode 'query=SELECT (COUNT(*) AS ?count) WHERE { ?s ?p ?o }' \
         -H "Accept: text/csv" || echo "Failed to query triple count"

    # Keep server running in background (Ctrl+C to stop manually)
    echo "Blazegraph server running on port $PORT (PID: $BLAZEGRAPH_PID). Press Ctrl+C to stop."
    wait $BLAZEGRAPH_PID
else
    echo "ERROR: Bulk load failed with exit code $LOAD_STATUS"
    exit 1
fi

echo "Process completed."