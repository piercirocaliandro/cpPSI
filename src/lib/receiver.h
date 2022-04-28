#include <list> 
#include <vector>

#include "utils.h"
#include <seal/seal.h>


using namespace seal;

// Functions prototypes
Ciphertext crypt_dataset(Receiver recv, EncryptionParameters params);
ComputationResult decrypt_and_intersect(EncryptionParameters params, Ciphertext sender_computation, Receiver recv);
Receiver setup_pk_sk(EncryptionParameters params);