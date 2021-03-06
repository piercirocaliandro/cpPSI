/** Receiver logic: this is the first "actor" of the PSI scheme, who wants to know the intersection 
 *  between the datasets. First, it encrypts its how dataset and "sends" the resulting ciphertext to the sender.
 *  Then, it receives the computation on the ecnrypted values from the sender, decrypts and determines 
 *  which values belong to the intersection: 
 *  such values will be the ones which will have value = 0 after decryption.
 * */



#include <cstddef>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <list>
#include <cmath>
#include <vector>
#include <bitset>
#include <algorithm>

#include "utils.h"
#include "seal/seal.h"

using namespace std;
using namespace seal;

#define RECV_AUDIT

// Function prototypes
void print_intersection(vector<string> intersection);
void write_result_on_file(vector<string> intersection);


/** 
 * Encrypt receiver's dataset, to produce an encrypted matrix that will be deilvered to 
 * the sender. 
 * 
 * @param recv              Instance of Receiver class
 * @param poly_mod_degree   size of the polynomial modulus (bits), used to configure the parameters
 *
 * @return                  A [matrix] Ciphertext that contains the ecnrypted values of the dataset
 * */
Ciphertext crypt_dataset(Receiver recv, size_t poly_mod_degree)
{   
	Ciphertext encrypted_recv_matrix;
	vector<uint64_t> longint_recv_dataset = recv.getDataset().getLongDataset();

	if (longint_recv_dataset.size() == 0){
#ifdef RECV_AUDIT
		printf("Receiver dataset is empty\n");
#endif
		return encrypted_recv_matrix;
	}
    
    EncryptionParameters prams = get_params(poly_mod_degree);
	SEALContext recv_context(prams);
	Encryptor encryptor(recv_context, recv.getRecvPk());
	Plaintext plain_recv_matrix;
	vector<Ciphertext> cipher_dataset;
	
	BatchEncoder recv_batch_encoder(recv_context);
	size_t slot_count = recv_batch_encoder.slot_count();
	size_t row_size = slot_count/2;
	vector<uint64_t> batch_recv_matrix(slot_count, 0ULL);
	
	/* In this part, the receiver moves the first step of the PSI scheme: 
	 * the dataset is encrypted using the encryptor class and the obtained dataset is then sent 
     * to the sender (returned by the function)
	 * */
	for(size_t index = 0; index < longint_recv_dataset.size(); index++)//recv_dataset.size(); index++)
		batch_recv_matrix[index] = longint_recv_dataset[index];
	
    // Encode and encrypt the whole matrix
	if(longint_recv_dataset.size() > 0) {
		recv_batch_encoder.encode(batch_recv_matrix, plain_recv_matrix);
		encryptor.encrypt(plain_recv_matrix, encrypted_recv_matrix);
	}

#ifdef RECV_AUDIT
	printf("First step completed\n");
#endif

	return encrypted_recv_matrix;
}


/** 
 * Last part of the PSI scheme, where the receiver computes the intersection between the two dataset.
 * 
 * @param poly_mod_degree       size of the polynomial modulus (bits), used to configure the parameters
 * @param sender_computation    Ciphertext resulting after the homomorphic computation performed by the sender
 * @param recv                  Receiver class instance containing the secret key used to decrypt
 * 
 * @return                      Result of the computation
 * */
ComputationResult decrypt_and_intersect(size_t poly_mod_degree, Ciphertext sender_computation, Receiver recv)
{
	vector<string> intersection;
	size_t noise = 0;
	ComputationResult result(noise, intersection);

	if(sender_computation.size() == 0){
#ifdef RECV_AUDIT
        printf("Sender ciphertext size is 0\n");
#endif
        return result;
	}

    EncryptionParameters params = get_params(poly_mod_degree);
	SEALContext recv_context(params);
	Decryptor recv_decryptor(recv_context, recv.getRecvSk());	
	Plaintext plain_result;
	vector<uint64_t> pod_result;

#ifdef RECV_AUDIT
	cout << "noise budget in encrypted x: " << recv_decryptor.invariant_noise_budget(sender_computation) 
        << " bits" << endl;
#endif

	BatchEncoder encoder(recv_context);
	long size = recv.getDataset().getSigmaLength();
    vector<uint64_t> recv_dataset = recv.getDataset().getLongDataset();
	
    // Decrypt and decode the received matrix
	recv_decryptor.decrypt(sender_computation, plain_result);
	encoder.decode(plain_result, pod_result);
    
	for(long index = 0; index < recv_dataset.size(); index++)
		if(pod_result[index] == 0)									// the value belongs to the intersection
			intersection.push_back(recv.getDataset().getStringDataset()[index]);
#ifdef RECV_AUDIT	
    printf("Last step completed\n");
#endif

    if(intersection.size() > 0)
        print_intersection(intersection);
	else
		printf("The intersection between sender and receiver is null \n");
	
	result.setIntersection(intersection);
	result.setNoiseBudget(recv_decryptor.invariant_noise_budget(sender_computation));
	
    //write_result_on_file(intersection);
    
    return result;
}


/** 
 * Generate public and secret keys for recevier operations and relinearization keys that will be used by
 * sender
 *
 * @param params    EncryptionParameters class instance, containing the information about the scheme
 * 
 * @return          Receiver class instance, configured with the parameters generated by this function
 * */
Receiver setup_pk_sk(EncryptionParameters params)
{
	SEALContext recv_context(params);
	Receiver recv;

	/* Generate public and private keys for the receiver */
	KeyGenerator recv_keygen(recv_context);
    SecretKey recv_sk = recv_keygen.secret_key();
    PublicKey recv_pk;
	recv_keygen.create_public_key(recv_pk);
    RelinKeys relin_keys;
    recv_keygen.create_relin_keys(relin_keys);

	// Save the keys for later decryption
	recv.setRecvPk(recv_pk); 
	recv.setRecvSk(recv_sk);
    recv.setRelinKeys(relin_keys);

	return recv;
}


/** 
 * Print the intersection between the dataset, in bistring and int formats 
 *
 * @param Intersection Intersection between the two dataset
 * */
void print_intersection(vector<string> intersection)
{
	string o_line = "";
	string v_line = " | ";
	string spaces = "";
	size_t i = 0;

	int middle_point = intersection[0].length()+2;
	long line_size = 2*middle_point;

	for (i = 0; i <= line_size; i++)
		o_line += "-";
	o_line[middle_point] = '|';
	
	cout << "\nPrinting the intersection between the two datasets: (bitstring, integer value)\n" << endl;
	cout << o_line << endl;
	for (string s : intersection){
		cout << ' ' <<  s << v_line << stoull(s, 0, 2) << endl;
		cout << o_line << endl;
	}
}


/** 
 * Write intersection result on a .txt file 
 *
 * @param intersection The strings belonging to the inresection 
 * */
void write_result_on_file(vector<string> intersection)
{
    string path = "src/output/intersection.txt";
    ofstream result_file(path, ios::out);
    
    if(result_file.is_open()){
        for(string result_string : intersection)
            result_file << result_string << "\n";
#ifdef RECV_AUDIT
        printf("\n\nOutput dataset wrote on file \n");
#endif
        result_file.close();
    }
    else{
#ifdef RECV_AUDIT
    printf("recv: Error while opening output file \n");
#endif
    }
}
