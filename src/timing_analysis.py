import os
import re
import matplotlib.pyplot as plt

TIMING_DIR = "../timing/"

# Regex pattern to extract matrix parameters from the filename
TIMING_FILENAME_PATTERN = r"G1_(\d+)_(\d+)_(\d+)_G2_(\d+)_(\d+)_(\d+)_(stored|generated).txt"

# Extract timing information from a file
def extract_timings(filepath):
    timings = {}
    with open(filepath, "r") as file:
        for line in file:
            parts = line.split(":")
            if len(parts) == 2:
                function_name = parts[0].strip()
                time = float(parts[1].strip())
                timings[function_name] = time
    return timings

# Categorize files by type (generated vs stored)
def categorize_and_parse_files():
    generated_timings = []
    stored_timings = []

    for filename in os.listdir(TIMING_DIR):
        if re.match(TIMING_FILENAME_PATTERN, filename):
            match = re.match(TIMING_FILENAME_PATTERN, filename)
            g1_n, g1_k, g1_d, g2_n, g2_k, g2_d, matrix_type = match.groups()

            filepath = os.path.join(TIMING_DIR, filename)
            timings = extract_timings(filepath)

            timing_entry = {
                "g1_n": int(g1_n),
                "g1_k": int(g1_k),
                "g1_d": int(g1_d),
                "g2_n": int(g2_n),
                "g2_k": int(g2_k),
                "g2_d": int(g2_d),
                "timings": timings
            }
            
            if matrix_type == "generated":
                generated_timings.append(timing_entry)
            else:
                stored_timings.append(timing_entry)

    return generated_timings, stored_timings

# 1. Plot comparison between generated and precomputed matrices
def plot_generated_vs_precomputed(generated_timings, stored_timings):
    # Compare the execution times for each function between generated and precomputed matrices
    functions = ["key_generation()", "generate_signature()", "verify_signature()", "main()"]
    
    generated_times = {fn: [] for fn in functions}
    stored_times = {fn: [] for fn in functions}
    
    labels = []

    for entry in sorted(generated_timings, key=lambda x: x['g1_n']):
        label = f"{entry['g1_n']}, {entry['g1_k']}, {entry['g1_d']}"
        labels.append(label)
        for fn in functions:
            generated_times[fn].append(entry["timings"].get(fn, 0))

    for entry in sorted(stored_timings, key=lambda x: x['g1_n']):
        for fn in functions:
            stored_times[fn].append(entry["timings"].get(fn, 0))

    # Plot comparison for each function
    for fn in functions:
        plt.figure(figsize=(10, 6)) 
        plt.plot(generated_times[fn], label="Generated", marker='o')
        plt.plot(stored_times[fn], label="Precomputed", marker='x')
        
        plt.title(f"Comparison of {fn} between generated and precomputed matrices")
        plt.xlabel("Parameter Sets (G1, G2)")
        plt.ylabel("Execution Time (s)")
        plt.xticks(ticks=range(len(labels)), labels=labels, rotation=45, ha="right")  
        plt.legend()
        plt.grid(True)
        plt.tight_layout()  
        plt.show()


# 2. Plot execution time vs. varying n (fixed k and d)
def plot_time_vs_n(timings, function_name):
    sorted_timings = sorted(timings, key=lambda x: x['g1_n'])

    ns = [entry["g1_n"] for entry in sorted_timings]
    times = [entry["timings"].get(function_name, 0) for entry in sorted_timings]

    plt.figure()
    plt.plot(ns, times, marker='o')
    plt.title(f"Execution time of {function_name} vs n")
    plt.xlabel("n (dimension)")
    plt.ylabel(f"{function_name} execution time (s)")
    plt.grid(True)
    plt.show()

# 3. Plot execution time vs. varying k (fixed n and d)
def plot_time_vs_k(timings, function_name):
    timings = sorted(timings, key=lambda entry: entry['g1_k'])
    
    k_values = [entry['g1_k'] for entry in timings]  
    times = [entry["timings"].get(function_name + "()", 0) for entry in timings]

    plt.figure(figsize=(8, 6))
    plt.plot(k_values, times, marker='o', linestyle='-', color='b')
    plt.xlabel('k (dimension)')
    plt.ylabel(f'{function_name} execution time (s)')
    plt.title(f'Execution time of {function_name} vs k')
    plt.grid(True)
    plt.show()

# Main function to run benchmarks and plot results
def run_analysis():
    generated_timings, stored_timings = categorize_and_parse_files()

    # Plot 1: Comparison between generated and precomputed matrices
    plot_generated_vs_precomputed(generated_timings, stored_timings)

    function_name = "main()"

    # Plot 2: Increase in time with increasing n (generated matrices)
    plot_time_vs_n(generated_timings, function_name)

    # Plot 3: Increase in time with increasing k (generated matrices)
    plot_time_vs_k(generated_timings, function_name)

if __name__ == "__main__":
    run_analysis()
