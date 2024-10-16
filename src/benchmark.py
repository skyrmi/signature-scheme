import subprocess

# 1. Parameter set for "Generated vs Pre-computed Matrices"
generated_vs_precomputed_params = [
    {
    "g1": {"n": 25, "k": 10, "d": 5},
    "g2": {"n": 50, "k": 10, "d": 6},
    "custom_message": False
    },
    {
    "g1": {"n": 40, "k": 15, "d": 6},
    "g2": {"n": 50, "k": 15, "d": 7},
    "custom_message": False
    },
    {
    "g1": {"n": 60, "k": 20, "d": 10},
    "g2": {"n": 70, "k": 20, "d": 11},
    "custom_message": False
    }
]

# 2. Varying n with fixed k and d
vary_n_params = {
    "k": 15,
    "d": 6,
    "n_start": 40,
    "n_end": 100,
    "n_step": 10
}

# 3. Varying k with fixed n and d
vary_k_params = {
    "n": 40,
    "d": 6,
    "k_start": 10,
    "k_end": 20,
    "k_step": 2
}

# 1. Benchmark: Generated vs Pre-computed Matrices
def benchmark_generated_vs_precomputed():
    print("\n--- Benchmark: Generated vs Pre-computed Matrices ---")
    
    for params in generated_vs_precomputed_params:
        for use_precomputed in [False, True]:
            current_params = params.copy() 
            current_params['use_precomputed_matrix'] = use_precomputed

            if use_precomputed:
                print("\nRun: Using pre-computed matrices...")
            else:
                print("\nRun: Generating matrices...")

            run_benchmark(current_params)

# 2. Benchmark: Varying n (Fixed k and d)
def benchmark_vary_n():
    print("\n--- Benchmark: Varying n (fixed k and d) ---")
    
    k = vary_n_params["k"]
    d = vary_n_params["d"]
    
    for n in range(vary_n_params["n_start"], vary_n_params["n_end"] + 1, vary_n_params["n_step"]):
        params = {
            "g1": {"n": n, "k": k, "d": d},
            "g2": {"n": n + 10, "k": k, "d": d},
            "custom_message": False,
            "use_precomputed_matrix": False,
        }
        print(f"\nRun: Varying n to {n} (g1), {n+10} (g2)...")
        run_benchmark(params)

# 3. Benchmark: Varying k (Fixed n and d)
def benchmark_vary_k():
    print("\n--- Benchmark: Varying k (fixed n and d) ---")
    
    n = vary_k_params["n"]
    d = vary_k_params["d"]

    for k in range(vary_k_params["k_start"], vary_k_params["k_end"] + 1, vary_k_params["k_step"]):
        params = {
            "g1": {"n": n, "k": k, "d": d},
            "g2": {"n": n + 10, "k": k, "d": d},  
            "custom_message": False,
            "use_precomputed_matrix": False,
        }
        print(f"\nRun: Varying k to {k} (g1 and g2)...")
        run_benchmark(params)

def run_benchmark(parameter_set):
    g1_input = f"{parameter_set['g1']['n']}\n{parameter_set['g1']['k']}\n{parameter_set['g1']['d']}\n"
    g2_input = f"{parameter_set['g2']['n']}\n{parameter_set['g2']['k']}\n{parameter_set['g2']['d']}\n"

    custom_message_input = 'y\n' if parameter_set["custom_message"] else 'n\n'
    precomputed_matrix_input = 'y\n' if parameter_set["use_precomputed_matrix"] else 'n\n'

    input_sequence = (
        f"y\n"  
        f"{g1_input}"
        f"y\n"  
        f"{g2_input}"
        f"{custom_message_input}"  
        f"{precomputed_matrix_input}"  
    )

    print("Running with the following inputs:")
    print(input_sequence)

    result = subprocess.run(
        ['./main'],  
        input=input_sequence,
        text=True,  
        capture_output=True
    )

    print("Error output (if any):")
    print(result.stderr)

    return result

if __name__ == "__main__":
    benchmark_generated_vs_precomputed()
    benchmark_vary_n()
    benchmark_vary_k()
