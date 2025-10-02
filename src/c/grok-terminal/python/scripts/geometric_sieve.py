import math
import random
import logging
import sys

# Golden ratio
PHI = (1 + math.sqrt(5)) / 2

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s', stream=sys.stdout)
logger = logging.getLogger(__name__)

def generate_primes(limit):
    """Generate primes up to limit using sieve."""
    if limit < 2:
        return []
    sieve = [True] * (limit + 1)
    sieve[0] = sieve[1] = False
    for i in range(2, int(math.sqrt(limit)) + 1):
        if sieve[i]:
            for j in range(i*i, limit + 1, i):
                sieve[j] = False
    return [i for i, is_prime in enumerate(sieve) if is_prime]

def generate_candidates(N, eps, k, primes):
    """Generate candidate primes using geometric sieving."""
    candidates = []
    phi = PHI
    max_i = int(math.log(math.isqrt(N) / k) / math.log(phi)) + 1
    for i in range(1, max_i + 1):
        target = k * phi ** i
        for p in primes:
            if p >= math.isqrt(N):
                break
            if abs(p / target - 1) < eps:
                candidates.append(p)
    return candidates

def factorize_with_candidates(N, candidates):
    """Try to factorize N using candidates."""
    for cand in candidates:
        if cand > 1 and N % cand == 0:
            other = N // cand
            if other > 1:  # Ensure both are primes or at least factors
                return True, cand, other
    return False, None, None

def multi_pass_factorize(N, k_sequence, eps, primes):
    """Multi-pass factorization."""
    logger.info(f"Starting multi-pass factorization for N={N}")
    for k in k_sequence:
        logger.info(f"Trying k={k:.3f}")
        candidates = generate_candidates(N, eps, k, primes)
        logger.info(f"Generated {len(candidates)} candidates")
        success, p, q = factorize_with_candidates(N, candidates)
        if success:
            logger.info(f"Success! Found factors: {p}, {q}")
            return True, p, q, k
        else:
            logger.info("No factors found for this k")
    logger.info(f"Failed to factorize N={N}")
    return False, None, None, None

def generate_semiprime(prime_list, semiprime_type, N_range):
    """Generate a semiprime based on type."""
    while True:
        if semiprime_type == 'balanced':
            N = random.randint(N_range[0], N_range[1])
            sqrt_N = int(math.sqrt(N))
            p = random.choice([p for p in prime_list if p <= sqrt_N])
            q_list = [q for q in prime_list if q >= sqrt_N and q != p and p*q <= N_range[1] and p*q >= N_range[0]]
            if q_list:
                q = random.choice(q_list)
                return p * q
        elif semiprime_type == 'skewed':
            p = random.choice(prime_list[:len(prime_list)//4])
            q_list = [q for q in prime_list[len(prime_list)//2:] if p*q >= N_range[0] and p*q <= N_range[1]]
            if q_list:
                q = random.choice(q_list)
                return p * q
        elif semiprime_type == 'wide':
            p = random.choice(prime_list[len(prime_list)//4:len(prime_list)//2])
            q_list = [q for q in prime_list[len(prime_list)//2:] if p*q >= N_range[0] and p*q <= N_range[1]]
            if q_list:
                q = random.choice(q_list)
                return p * q

def run_experiment(num_runs=50, eps=0.04, k_sequence=[0.200, 0.318, 0.450, 0.600]):
    """Run the experiment."""
    prime_limit = 1000  # For N up to ~1e6
    primes = generate_primes(prime_limit)
    N_range = (10000, 1000000)
    types = ['balanced', 'skewed', 'wide']
    
    successes = 0
    results = []
    
    for i in range(num_runs):
        semiprime_type = random.choice(types)
        N = generate_semiprime(primes, semiprime_type, N_range)
        logger.info(f"Run {i+1}: Generating N={N}, Type={semiprime_type}")
        success, p, q, k_used = multi_pass_factorize(N, k_sequence, eps, primes)
        if success:
            successes += 1
        results.append((N, success, k_used, semiprime_type))
        print(f"Run {i+1}: N={N}, Type={semiprime_type}, Success={success}, k={k_used}")
    
    success_rate = successes / num_runs * 100
    print(f"\nTotal Successes: {successes}/{num_runs} ({success_rate:.1f}%)")
    return success_rate, results

if __name__ == "__main__":
    if len(sys.argv) > 1:
        num_runs = int(sys.argv[1])
    else:
        num_runs = 50
    success_rate, _ = run_experiment(num_runs)
    if success_rate > 40:
        print("Success: Achieved >40% success rate!")
    else:
        print("Failed: Did not achieve >40% success rate.")