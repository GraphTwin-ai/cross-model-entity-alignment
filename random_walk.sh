#!/bin/bash
#SBATCH --job-name=random_walk
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --partition=cpu_long
#SBATCH --mem=128G          # 128GB of memory
#SBATCH --time=24:00:00     # 24 hours time limit
#SBATCH --output=query_random_walk.out
#SBATCH --error=query_random_walk.err

# Log file for output (tee will duplicate stdout/stderr to log)
LOG_FILE="$PWD/query_random_walk.log"
exec > >(tee -a "$LOG_FILE") 2>&1

# Set Jena home directory and TDB2 database location - adjust paths as needed
JENA_HOME=/gpfs/workdir/oumidaa/apache-jena-5.3.0
TDB2_LOC=/gpfs/workdir/oumidaa/dbpedia_tdb2
FUESKI_HOME=/gpfs/workdir/oumidaa/apache-jena-fuseki-5.3.0

# Start Fuseki server in the background (update enabled) and log its output
FUSEKI_LOG="$PWD/fuseki_server.log"
echo "Starting Fuseki server at $(date)"
${FUESKI_HOME}/fuseki-server --loc=${TDB2_LOC} --update /dataset > "${FUSEKI_LOG}" 2>&1 &
FUSEKI_PID=$!

# Give Fuseki a few seconds to initialize
sleep 10

# Fuseki server endpoint (assumes it is running on localhost)
FUSEKI_URL="http://localhost:3030/dataset/sparql"

# Output file (random walks are appended as they are obtained)
OUT_FILE="$PWD/random_walks.out"

# Number of random walks to perform total
NUM_WALKS=1000

# Number of parallel workers (set to number of available CPUs; adjust if needed)
NUM_WORKERS=32

# Function to issue a SPARQL query via curl and return the result (TSV format)
function run_sparql_query() {
    local query="$1"
    # Using curl to POST. The '--data-urlencode' ensures proper encoding.
    curl -s -X POST "${FUSEKI_URL}" \
         --data-urlencode "query=${query}" \
         -H "Accept: text/tab-separated-values" 
}

# Function to perform one random walk query.
function random_walk() {
    # Choose a random required length between 8 and 15 (inclusive)
    local WALK_LENGTH=$(( RANDOM % 8 + 8 ))
    local walk=""
    local current_node=""

    # 1. Query a random starting node 
    local query_start="SELECT ?s WHERE { ?s ?p ?o } ORDER BY RAND() LIMIT 1"
    current_node=$( run_sparql_query "${query_start}" | tail -n +2 | head -n1 )
    
    if [[ -z "${current_node}" ]]; then
        echo "No starting node found, skipping walk." >&2
        return
    fi

    walk="${current_node}"

    # 2. Iteratively follow a random neighbor for WALK_LENGTH-1 steps.
    for (( i=1; i < WALK_LENGTH; i++ )); do
        local query_next="SELECT ?next WHERE { <${current_node}> ?p ?next } ORDER BY RAND() LIMIT 1"
        local next_node=$( run_sparql_query "${query_next}" | tail -n +2 | head -n1 )
        # If no neighbor is found, break out of the walk early.
        if [[ -z "${next_node}" ]]; then
            break
        fi
        walk+=" -> ${next_node}"
        current_node=${next_node}
    done

    # Write the walk result immediately to the output file.
    echo "$(date +'%Y-%m-%dT%H:%M:%S') RANDOM WALK (length=${WALK_LENGTH}): ${walk}" >> "${OUT_FILE}"
}

# Export the function and variables needed to run in parallel.
export FUSEKI_URL OUT_FILE
export -f run_sparql_query
export -f random_walk

echo "Starting random walk queries at $(date)" 
echo "Total number of walks: ${NUM_WALKS} | Parallel workers: ${NUM_WORKERS}"

# Use a counter loop spawning background processes.
counter=0
running_jobs=0
while [ ${counter} -lt ${NUM_WALKS} ]; do
    random_walk &
    (( counter++ ))
    (( running_jobs++ ))
    # When we have spawned the maximum number of parallel jobs, wait for them to complete.
    if [ ${running_jobs} -ge ${NUM_WORKERS} ]; then
        wait
        running_jobs=0
    fi
done

# Wait for any remaining jobs to complete.
wait

echo "Finished all random walk queries at $(date)"
echo "Random walks are stored in: ${OUT_FILE}"

# Stop the Fuseki server
echo "Stopping Fuseki server at $(date)"
kill ${FUSEKI_PID}
wait ${FUSEKI_PID} 2>/dev/null

echo "Job completed"