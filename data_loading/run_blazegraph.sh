#!/bin/bash
#SBATCH --job-name=blazegraph
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --mem=8G
#SBATCH --time=01:00:00
#SBATCH --output=blazegraph_run.out

cd $SLURM_SUBMIT_DIR
PORT=$(shuf -i 10000-65000 -n 1)
echo "Blazegraph will run on port: $PORT"
java -server -Xmx4g -Djetty.port=$PORT -jar ~/blazegraph/blazegraph.jar