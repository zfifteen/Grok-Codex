import math
import concurrent.futures
import time
import numpy as np
import random

def is_prime(n):
    if n <= 1: return False
    if n <= 3: return True
    if n % 2 == 0 or n % 3 == 0: return False
    i = 5
    while i * i <= n:
        if n % i == 0 or n % (i + 2) == 0: return False
        i += 6
    return True

def generate_semiprime(min_digits=12, max_digits=15):
    while True:
        p_digits = random.randint((min_digits + 1) // 2, max_digits // 2)
        q_digits = random.randint((min_digits + 1) // 2, max_digits // 2)
        p = random.randint(10**(p_digits-1), 10**p_digits - 1)
        if is_prime(p):
            q = random.randint(10**(q_digits-1), 10**q_digits - 1)
            if is_prime(q) and p != q:
                return p * q, p, q

def trial_division(n, start=2, end=None):
    if end is None: end = int(math.sqrt(n)) + 1
    for i in range(start, end):
        if n % i == 0: return i
    return None

def factorize_semiprime(n, workers=4):
    start_time = time.time()
    sqrt_n = int(math.sqrt(n)) + 1
    chunk_size = sqrt_n // workers
    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as executor:
        futures = []
        for i in range(workers):
            chunk_start = 2 + i * chunk_size
            chunk_end = chunk_start + chunk_size if i < workers - 1 else sqrt_n
            futures.append(executor.submit(trial_division, n, chunk_start, chunk_end))
        for future in concurrent.futures.as_completed(futures):
            factor = future.result()
            if factor: return factor, n // factor, time.time() - start_time
    return None, None, time.time() - start_time

# Run 100 samples
samples = 100
times = []
successes = []
for _ in range(samples):
    n, true_p, true_q = generate_semiprime()
    found_p, found_q, t = factorize_semiprime(n)
    success = (found_p == true_p and found_q == true_q) or (found_p == true_q and found_q == true_p)
    times.append(t)
    successes.append(success)

mean_time = np.mean(times)
sd_time = np.std(times)
success_rate = sum(successes) / samples * 100

# Bootstrap CI
bootstrap_samples = 1000
bootstrap_means = [np.mean(np.random.choice(times, samples, replace=True)) for _ in range(bootstrap_samples)]
ci_lower, ci_upper = np.percentile(bootstrap_means, [2.5, 97.5])

print(f"Mean time: {mean_time:.4f}s, SD: {sd_time:.4f}s, 95% CI: [{ci_lower:.4f}, {ci_upper:.4f}], Success: {success_rate:.1f}%")