import numpy as np
import torch
import time
import logging

# Set up logging to file named after script
logging.basicConfig(filename='amx_matrix_bench.log', level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# Check for MPS device
device = torch.device("mps" if torch.backends.mps.is_available() else "cpu")
logging.info(f"Using device: {device}")

# Set random seed for reproducibility
np.random.seed(42)
torch.manual_seed(42)

# Matrix size
N = 1000
iterations = 100

# Generate random matrices
A_np = np.random.rand(N, N).astype(np.float32)
B_np = np.random.rand(N, N).astype(np.float32)

A_torch = torch.rand(N, N, dtype=torch.float32, device=device)
B_torch = torch.rand(N, N, dtype=torch.float32, device=device)

# Benchmark NumPy (CPU, uses Accelerate BLAS)
numpy_times = []
for _ in range(iterations):
    start = time.time()
    C_np = A_np @ B_np
    end = time.time()
    numpy_times.append(end - start)
numpy_avg = sum(numpy_times) / len(numpy_times)
logging.info(f"NumPy average time over {iterations} iterations: {numpy_avg:.4f} seconds")

# Benchmark Torch MPS (AMX-accelerated)
torch_times = []
with torch.no_grad():  # Disable gradients for inference-like speed
    for _ in range(iterations):
        start = time.time()
        C_torch = torch.mm(A_torch, B_torch)
        torch.mps.synchronize()  # Ensure MPS ops are done
        end = time.time()
        torch_times.append(end - start)
torch_avg = sum(torch_times) / len(torch_times)
logging.info(f"Torch MPS average time over {iterations} iterations: {torch_avg:.4f} seconds")

# Compute speedup (NumPy time / Torch time)
speedup = numpy_avg / torch_avg if torch_avg > 0 else float('inf')
logging.info(f"Speedup (NumPy / Torch MPS): {speedup:.2f}x")

print(f"Benchmark complete. Results logged to amx_matrix_bench.log. Speedup: {speedup:.2f}x")