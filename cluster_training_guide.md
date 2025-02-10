# Training or Fine-tuning On a Remote Cluster Using Two GPUs

To allocate the GPU, run the following command:

```bash
srun --nodes=1 -p gpu --gres=gpu:2 --time 01:00:00 --pty /bin/bash
```

To see the allocations, run:

```bash
ruche-quota
```

### Additional Information

- **`--nodes=1`**: Specifies the number of nodes (machines, recommended =1) to be allocated.
- **`-p gpu`**: Specifies the partition to be used.
- **`--gres=gpu:2`**: Requests two GPU (on the same machine).
- **`--time 01:00:00`**: Sets the maximum runtime to 1 hour.
- **`--pty /bin/bash`**: Opens an interactive bash session.

Make sure to check the available partitions and GPU resources before running the commands.

You can run `nvidia-smi` to get a glimpse on the reserved GPUs.

If you check the CUDA driver: `nvcc --version`. You will see that no driver is available. This happends because although the CUDA toolkit is installed on the machine, it was not yet loaded into the environement.

To check available CUDA toolkits, run 
```bash
module avail cuda
```
We will choose the `cuda/12.2.1/gcc-11.2.0`

Run 
```bash
module load cuda/12.2.1/gcc-11.2.0
```
Now you can see that the driver is available: 
```bash
nvcc --version
```
Sometimes, when launching the trainer, you might get backend compliation errors (C++). This happens because of an old version of the compiler gcc.
Make sure you update your compiler to match the one in CUDA. 
Using the command:
```bash
micromamba install gcc=11.2.0
```


Now that we are all set. We need to launch the `training_script.py` using `accelerate`

The command is:
```bash
accelerate launch --multi_gpu --mixed_precision "fp16" --num_processes 2 --num_machines 1 --gpu_ids "0,1" --machine_rank 0 scripts/training_script.py
```
Read the [HF Documentation](https://huggingface.co/docs/accelerate/v1.3.0/en/package_reference/cli#the-command-line) to understand the parameters and the values they take.

*N.B* You might need to adapt your python script to prevent problems of tensors running on both GPUs. Always check that all the tensors are stored on the same GPU of the model (`model.device`)


We recommend writing a slurm script 

To run the slurm script:

```bash
sbatch train.slurm
```

