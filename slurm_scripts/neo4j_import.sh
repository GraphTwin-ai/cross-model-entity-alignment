#!/bin/bash
#SBATCH --job-name=neo4j_import       # Set the job name
#SBATCH --output=neo4j_import.log     # Set the output log file
#SBATCH --error=neo4j_import.err      # Set the error log file
#SBATCH --nodes=1                     # Request one node
#SBATCH --ntasks=1                    # Run a single task
#SBATCH --cpus-per-task=8             # Allocate 8 CPUs per task
#SBATCH --mem=32G                     # Allocate 32 GB of memory
#SBATCH --time=24:00:00               # Set maximum runtime to 24 hours
#SBATCH --partition=cpu_long          # Set the partition name
#SBATCH --mail-type=ALL               # Send an email on all job events

# Set paths
export JAVA_HOME="/gpfs/workdir/mortadii/jdk-21.0.5"   # Set Java path
export PATH="$JAVA_HOME/bin:$PATH"                     # Add Java to PATH
export NEO4J_HOME="/gpfs/workdir/mortadii/neo4j-community-5.25.1"  # Set Neo4j home directory

# Define output and error log file paths
SLURM_OUTPUT="neo4j_import.log"
SLURM_ERROR="neo4j_import.err"

# Start Neo4j
echo "Starting Neo4j..." >> $SLURM_OUTPUT
$NEO4J_HOME/bin/neo4j start >> $SLURM_OUTPUT 2>&1
sleep 60  # Wait for Neo4j startup

# CALL n10s.graphconfig.set({
#                handleVocabUris: "KEEP",
#                handleMultival: "ARRAY",
#                keepLangTag: true,
#                keepCustomDataTypes: true,
#                typesToLabels: true,
#                handleRDFTypes: "LABELS"});

# Import RDF file with optimizations
echo "Starting RDF import..." >> $SLURM_OUTPUT
cypher-shell -u neo4j -p password "CALL n10s.rdf.import.fetch('file:///gpfs/workdir/mortadii/s3pg/data/rdf_graph.ttl', 'Turtle')" > cypher_import_output.log 2>&1

# Check if import was successful
if [ $? -ne 0 ]; then
    echo "ERROR: RDF import failed!" >> $SLURM_ERROR
    exit 1
else
    echo "RDF import completed successfully." >> $SLURM_OUTPUT
fi

# Stop Neo4j after import
echo "Stopping Neo4j..." >> $SLURM_OUTPUT
$NEO4J_HOME/bin/neo4j stop >> $SLURM_OUTPUT 2>&1
