import subprocess

parameter_sets = [
    {
        "g1": {"n": 40, "k": 15, "d": 6},
        "g2": {"n": 50, "k": 15, "d": 7},
        "custom_message": False,
        "use_precomputed_matrix": True
    },
]

def run_benchmark(parameter_set):
    g1_input = f"{parameter_set['g1']['n']}\n{parameter_set['g1']['k']}\n{parameter_set['g1']['d']}\n"
    g2_input = f"{parameter_set['g2']['n']}\n{parameter_set['g2']['k']}\n{parameter_set['g2']['d']}\n"
    
    custom_message_input = 'y\n' if parameter_set["custom_message"] else 'n\n'
    precomputed_matrix_input = 'y\n' if parameter_set["use_precomputed_matrix"] else 'n\n'

    # Input sequence
    input_sequence = (
        f"y\n"  
        f"{g1_input}"
        f"y\n"  
        f"{g2_input}"
        f"{custom_message_input}"  
        f"{precomputed_matrix_input}"  
    )

    print("\nRunning with the following inputs:")
    print(input_sequence)

    result = subprocess.run(
        ['./main'],  
        input=input_sequence,
        text=True,  
        capture_output=True  
    )

    print("\nProgram output:")
    print(result.stdout)
    print("\nError output (if any):")
    print(result.stderr)

    return result

def benchmark_all():
    for i, param_set in enumerate(parameter_sets):
        print(f"--- Running Benchmark {i+1}/{len(parameter_sets)} ---")
        run_benchmark(param_set)
        print(f"--- Finished Benchmark {i+1}/{len(parameter_sets)} ---\n\n")

if __name__ == "__main__":
    benchmark_all()
