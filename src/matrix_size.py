from dataclasses import dataclass

@dataclass
class NmodMat:
    r: int = 0    # rows (slong)
    c: int = 0    # columns (slong)

def calculate_matrix_bytes(matrix: NmodMat, pointer_size: int = 8, mp_limb_t_size: int = 8) -> int:
    """Calculate total bytes allocated for a matrix."""
    # Base structure sizes
    nmod_t_size = 3 * mp_limb_t_size  # n, ninv, and norm
    base_size = (2 * pointer_size      # entries and rows pointers
                + 2 * mp_limb_t_size   # r and c
                + nmod_t_size)         # mod struct
    
    # Dynamic memory
    elements = mp_limb_t_size * matrix.r * matrix.c  # Contiguous array of elements
    row_pointers = pointer_size * matrix.r           # Array of pointers to rows
    
    return base_size + elements + row_pointers

def calculate_code_matrices_size(n1: int, n2: int, k: int) -> dict:
    # Combined parameters
    n = n1 + n2
    
    G1 = NmodMat(r=k, c=n1)           # Generator matrix 1
    G2 = NmodMat(r=k, c=n2)           # Generator matrix 2
    HA = NmodMat(r=n-k, c=n)          # Parity check matrix
    G_star = NmodMat(r=k, c=n)        # Generator matrix G*
    F = NmodMat(r=n-k, c=k)           # Public key F = H_A * G*^T
    signature = NmodMat(r=1, c=n)      # Signature matrix
    
    return {
        "G1": calculate_matrix_bytes(G1),
        "G2": calculate_matrix_bytes(G2),
        "H_A": calculate_matrix_bytes(HA),
        "G_star": calculate_matrix_bytes(G_star),
        "F": calculate_matrix_bytes(F),
        "signature": calculate_matrix_bytes(signature)
    }

def main():
    # Example parameters
    n1 = 40
    n2 = 50
    k = 15
    
    sizes = calculate_code_matrices_size(n1, n2, k)
    
    print(f"Memory allocation for matrices with parameters:")
    print(f"Code 1: [{n1}, {k}]")
    print(f"Code 2: [{n2}, {k}]")
    print(f"Combined: [{n1 + n2}, {k}]\n")
    
    print("Matrix sizes in bytes:")
    print(f"Generator matrix G1: {sizes['G1']} bytes")
    print(f"Generator matrix G2: {sizes['G2']} bytes")
    print(f"Parity check matrix H_A: {sizes['H_A']} bytes")
    print(f"Generator matrix G*: {sizes['G_star']} bytes")
    print(f"Public key F: {sizes['F']} bytes")
    print(f"Signature: {sizes['signature']} bytes")

if __name__ == "__main__":
    main()