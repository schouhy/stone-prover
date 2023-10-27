use std::fs::File;
use std::io::Write;

use stark_platinum_prover::examples::fibonacci_2_cols_shifted::{self, Fibonacci2ColsShifted};
use stark_platinum_prover::fri::FieldElement;
use stark_platinum_prover::proof::options::ProofOptions;
use stark_platinum_prover::proof::stark::StoneCompatibleSerializer;
use stark_platinum_prover::prover::{IsStarkProver, Prover};
use stark_platinum_prover::transcript::StoneProverTranscript;

use lambdaworks_math::traits::Serializable;

/// Produces a proof of a Fibonacci execution and exports it as a binary file in a format compatible
/// with Stone (FibonacciAir).
fn main() {
    // Execute Fibonacci program
    let secret = FieldElement::one();
    let trace_length = 512;
    let trace = fibonacci_2_cols_shifted::compute_trace(secret, trace_length);

    // Proof options
    let claimed_index = 501;
    let claimed_value = trace.get_row(claimed_index)[0];
    let proof_options = ProofOptions {
        blowup_factor: 8,
        coset_offset: 3,
        grinding_factor: 20,
        fri_number_of_queries: 200,
    };
    let pub_inputs = fibonacci_2_cols_shifted::PublicInputs {
        claimed_value,
        claimed_index,
    };

    // Generate proof
    let proof = Prover::prove::<Fibonacci2ColsShifted<_>>(
        &trace,
        &pub_inputs,
        &proof_options,
        StoneProverTranscript::new(&pub_inputs.serialize()),
    )
    .unwrap();

    // Export to bytes
    let serialized_proof = StoneCompatibleSerializer::serialize_proof::<Fibonacci2ColsShifted<_>>(
        &proof,
        &pub_inputs,
        &proof_options,
    );
    let mut file = File::create("lambdaworks_proof.bin").unwrap();
    let _ = file.write_all(&serialized_proof);
}
