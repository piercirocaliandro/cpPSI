#include<vector>
#include <seal/seal.h>

using namespace std;
using namespace seal;

vector<Ciphertext> homomorphic_computation(vector<Ciphertext> recv_ct_array, EncryptionParameters params);