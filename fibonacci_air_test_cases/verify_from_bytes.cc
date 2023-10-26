#include "starkware/air/fibonacci/fibonacci_trace_context.h"
#include "starkware/channel/noninteractive_prover_channel.h"
#include "starkware/channel/noninteractive_verifier_channel.h"
#include "starkware/crypt_tools/keccak_256.h"
#include "starkware/stark/stark.h"
#include "starkware/stark/test_utils.h"
#include "starkware/stark/utils.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>

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

template <typename FieldElementT>
std::vector<std::byte> GetInitialHashChainSeed(uint64_t fibonacci_claim_index, FieldElementT *claimed_fib) {
  const size_t elem_bytes = FieldElementT::SizeInBytes();
  const size_t fibonacci_claim_index_bytes = sizeof(uint64_t);
  std::vector<std::byte> randomness_seed(elem_bytes + fibonacci_claim_index_bytes);
  Serialize(
      uint64_t(fibonacci_claim_index),
      gsl::make_span(randomness_seed).subspan(0, fibonacci_claim_index_bytes));
  (*claimed_fib).ToBytes(gsl::make_span(randomness_seed).subspan(fibonacci_claim_index_bytes));
  return randomness_seed;
}

int main(int argc, char *argv[]) {
  const Field field(Field::Create<FieldElementT>());

  // AIR
  uint64_t trace_length = 512;
  uint64_t fibonacci_claim_index = 501;
  StarkField252Element secret = StarkField252Element::One();
  StarkField252Element claimed_fib =
      FibAirT::PublicInputFromPrivateInput(secret, fibonacci_claim_index);
  auto air = FibAirT(trace_length, fibonacci_claim_index, claimed_fib);

  // FRI configuration
  const auto log_n_cosets = 3;
  const size_t log_coset_size = Log2Ceil(trace_length);
  const FieldElementT fri_domain_offset = FieldElementT::FromUint(3);
  FftBasesImpl<FftMultiplicativeGroup<FieldElementT>> fft_bases = MakeFftBases(
      /*log_n=*/log_coset_size + log_n_cosets,
      /*start_offset=*/fri_domain_offset);

  size_t proof_of_work_bits = 20;
  const std::vector<size_t> fri_step_list = {1, 1, 1, 1, 1, 1, 1, 1, 1};
  size_t n_queries = 200;
  FriParameters fri_params{/*fri_step_list=*/fri_step_list,
                           /*last_layer_degree_bound=*/1,
                           /*n_queries=*/n_queries,
                           /*fft_bases=*/UseOwned(&fft_bases),
                           /*field=*/field,
                           /*proof_of_work_bits=*/proof_of_work_bits};
  const Prng channel_prng = Prng(GetInitialHashChainSeed(fibonacci_claim_index, &claimed_fib));
  NoninteractiveProverChannel prover_channel =
      NoninteractiveProverChannel(channel_prng.Clone());

  // StarkParameters
  StarkParameters stark_params(
      /*field*/ field, /*use_extension_field=*/false,
      /*n_evaluation_domain_cosets=*/Pow2(log_n_cosets),
      /*trace_length=*/trace_length,
      /*air=*/UseOwned(&air),
      /*fri_params=*/UseOwned(&fri_params));

  if (argc != 2) {
      std::cerr << "Usage: " << argv[0] << " input_file" << std::endl;
      return 1;
  }
  const char* inputFilename = argv[1];

  std::vector<std::byte> proof;

  std::ifstream inputFile(inputFilename, std::ios::binary);
  if (!inputFile) {
      std::cerr << "Error opening file: " << inputFilename << std::endl;
      return 1;
  }

  inputFile.seekg(0, std::ios::end);
  std::streampos fileSize = inputFile.tellg();
  inputFile.seekg(0, std::ios::beg);

  proof.resize(static_cast<size_t>(fileSize));
  inputFile.read(reinterpret_cast<char*>(proof.data()), fileSize);
  
  inputFile.close();

  // Verify
  NoninteractiveVerifierChannel verifier_channel(channel_prng.Clone(), proof);

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

  std::cout << "Proof is valid!" << std::endl;

  return 0;
}
