#include "starkware/air/fibonacci/fibonacci_trace_context.h"
#include "starkware/channel/noninteractive_prover_channel.h"
#include "starkware/channel/noninteractive_verifier_channel.h"
#include "starkware/crypt_tools/keccak_256.h"
#include "starkware/stark/stark.h"
#include "starkware/stark/test_utils.h"
#include "starkware/stark/utils.h"

using namespace starkware;
using StarkField252Element = PrimeFieldElement<252, 0>;
using FibAirT = FibonacciAir<StarkField252Element>;
using FibTraceContextT = FibonacciTraceContext<StarkField252Element>;

template <typename HashT, typename FieldElementT>
std::unique_ptr<TableProver>
MakeTableProver(uint64_t n_segments, uint64_t n_rows_per_segment,
                size_t n_columns, ProverChannel *channel,
                size_t n_tasks_per_segment, size_t n_layer,
                size_t n_verifier_friendly_commitment_layers) {
  return GetTableProverFactory<HashT>(channel, FieldElementT::SizeInBytes(),
                                      n_tasks_per_segment, n_layer,
                                      n_verifier_friendly_commitment_layers,
                                      CommitmentHashes(HashT::HashName()))(
      n_segments, n_rows_per_segment, n_columns);
}

int main() {
  const Field field(Field::Create<FieldElementT>());

  // AIR
  uint64_t trace_length = 4;
  uint64_t fibonacci_claim_index = 3;
  StarkField252Element secret = StarkField252Element::One();
  StarkField252Element claimed_fib =
      FibAirT::PublicInputFromPrivateInput(secret, fibonacci_claim_index);
  auto air = FibAirT(trace_length, fibonacci_claim_index, claimed_fib);

  // FRI configuration
  const auto log_n_cosets = 2;
  const size_t log_coset_size = Log2Ceil(trace_length);
  const FieldElementT fri_domain_offset = FieldElementT::FromUint(3);
  FftBasesImpl<FftMultiplicativeGroup<FieldElementT>> fft_bases = MakeFftBases(
      /*log_n=*/log_coset_size + log_n_cosets,
      /*start_offset=*/fri_domain_offset);

  size_t proof_of_work_bits = 0;
  const std::vector<size_t> fri_step_list = {1, 1};
  size_t n_queries = 1;
  FriParameters fri_params{/*fri_step_list=*/fri_step_list,
                           /*last_layer_degree_bound=*/1,
                           /*n_queries=*/n_queries,
                           /*fft_bases=*/UseOwned(&fft_bases),
                           /*field=*/field,
                           /*proof_of_work_bits=*/proof_of_work_bits};
  const Prng channel_prng = Prng(MakeByteArray<0xca, 0xfe, 0xca, 0xfe>());
  NoninteractiveProverChannel prover_channel =
      NoninteractiveProverChannel(channel_prng.Clone());

  // StarkParameters
  StarkParameters stark_params(
      /*field*/ field, /*use_extension_field=*/false,
      /*n_evaluation_domain_cosets=*/Pow2(log_n_cosets),
      /*trace_length=*/trace_length,
      /*air=*/UseOwned(&air),
      /*fri_params=*/UseOwned(&fri_params));

  StarkProverConfig stark_config = StarkProverConfig::InRam();
  TableProverFactory table_prover_factory =
      [&](uint64_t n_segments, uint64_t n_rows_per_segment, size_t n_columns) {
        return MakeTableProver<Keccak256, StarkField252Element>(
            n_segments, n_rows_per_segment, n_columns, &prover_channel,
            stark_config.table_prover_n_tasks_per_segment,
            stark_config.n_out_of_memory_merkle_layers, 0);
      };

  // Prove
  StarkProver stark_prover(UseOwned(&prover_channel),
                           UseOwned(&table_prover_factory),
                           UseOwned(&stark_params), UseOwned(&stark_config));

  stark_prover.ProveStark(std::make_unique<FibTraceContextT>(
      UseOwned(&air), secret, fibonacci_claim_index));
  auto proof = prover_channel.GetProof();

  // Print annotations to stdout
  const std::optional<std::vector<std::string>> prover_annotations =
      prover_channel.GetAnnotations();
  if (prover_annotations.has_value()) {
    auto vec = prover_annotations.value();
    for (const std::string &str : vec) {
      std::cout << str << std::endl;
    }
  }

  // Verify
  NoninteractiveVerifierChannel verifier_channel(channel_prng.Clone(), proof);
  if (prover_annotations) {
    verifier_channel.SetExpectedAnnotations(*prover_annotations);
  }

  TableVerifierFactory table_verifier_factory =
      [&verifier_channel](const Field &field, uint64_t n_rows,
                          uint64_t n_columns) {
        return MakeTableVerifier<Keccak256, StarkField252Element>(
            field, n_rows, n_columns, &verifier_channel, 0);
      };
  StarkVerifier stark_verifier(UseOwned(&verifier_channel),
                               UseOwned(&table_verifier_factory),
                               UseOwned(&stark_params));

  stark_verifier.VerifyStark();

  return 0;
}
