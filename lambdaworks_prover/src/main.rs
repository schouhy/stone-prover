use std::fs::File;
use std::io::Write;

use stark_platinum_prover::examples::fibonacci_2_cols_shifted::{self, Fibonacci2ColsShifted};
use stark_platinum_prover::fri::FieldElement;
use stark_platinum_prover::proof::options::ProofOptions;
use stark_platinum_prover::proof::stark::StoneCompatibleSerializer;
use stark_platinum_prover::prover::{IsStarkProver, Prover};
use stark_platinum_prover::transcript::StoneProverTranscript;

use lambdaworks_math::traits::Serializable;

fn main() {
    let trace = fibonacci_2_cols_shifted::compute_trace(FieldElement::one(), 512);

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

    let mut transcript_init_seed = claimed_index.to_be_bytes().to_vec();
    transcript_init_seed.extend_from_slice(&claimed_value.serialize());

    let proof = Prover::prove::<Fibonacci2ColsShifted<_>>(
        &trace,
        &pub_inputs,
        &proof_options,
        StoneProverTranscript::new(&transcript_init_seed),
    )
    .unwrap();
    let serialized_proof = StoneCompatibleSerializer::serialize_proof::<Fibonacci2ColsShifted<_>>(
        &proof,
        &pub_inputs,
        &proof_options,
    );
    let mut file = File::create("lambdaworks_proof.bin").unwrap();
    let _ = file.write_all(&serialized_proof);
}
