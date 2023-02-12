#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <string.h>

int encrypt(unsigned char *key, unsigned char *iv, unsigned char *plaintext, unsigned char *encrypted_text, int *encrypted_text_length) {
	EVP_CIPHER_CTX *ctx = NULL;
	int ciphertext_len = 0;
	int final_ciphertext_len = 0;
	
	// Create and initialise the context
	ctx = EVP_CIPHER_CTX_new();
	if(ctx == NULL) {
		printf("Error initializing encryption context\n");
		return 1;
	}
	
	// Initialise the encryption operation.
	if(EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1){
		printf("Error initializing encryption\n");
		return 1;
	
	}
	
	// Provide the message to be encrypted, and obtain the encrypted output.
	if(EVP_EncryptUpdate(ctx, encrypted_text, &ciphertext_len, plaintext, strlen((char *)plaintext)) != 1) {
		printf("Error encrypting input\n");
		return 1;
	}

	// Finalise the encryption. Further ciphertext bytes may be written at this stage.
	if(EVP_EncryptFinal_ex(ctx, encrypted_text + ciphertext_len, &final_ciphertext_len) != 1) {
		printf("Error finalizing encryption\n");
		return 1;
	}
	
	// Clean up
	EVP_CIPHER_CTX_free(ctx);
	*encrypted_text_length = ciphertext_len + final_ciphertext_len;
	
	return 0;
}

int decrypt(unsigned char *key, unsigned char *iv, unsigned char *decrypted_text, unsigned char *encrypted_text, int encrypted_text_length, int *decrypted_text_length) {
	int decryptedtext_len = 0;
	int final_decryptedtext_len = 0;
	EVP_CIPHER_CTX *ctx = NULL;

	//Create and initialise the context
	ctx = EVP_CIPHER_CTX_new();
	if(ctx == NULL) {
		printf("Error initializing decryption context\n");
		return 1;
	}

	// Initialise the decryption operation.
	if(EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1) {
		printf("Error initializing decryption\n");
		return 1;
	}

	// Provide the message to be decrypted, and obtain the decrypted output.
	if(EVP_DecryptUpdate(ctx, decrypted_text, &decryptedtext_len, encrypted_text, encrypted_text_length) != 1){
		printf("Error decrypting input\n");
		return 1;
	}

	// Finalise the decryption. Further decrypted bytes may be written at this stage.
	if(EVP_DecryptFinal_ex(ctx, decrypted_text + decryptedtext_len, &final_decryptedtext_len) != 1) {
		printf("Error finalizing decryption\n");
		return 1;
	}
	
	// Clean up
	EVP_CIPHER_CTX_free(ctx);
	decryptedtext_len += final_decryptedtext_len;

	decrypted_text[decryptedtext_len] = '\0';
	*decrypted_text_length = decryptedtext_len + 1;
	
	return 0;
}

void hex_print(unsigned char *encrypted_text, int encrypted_text_length) {
	int i;
	
	printf("Encrypted Text:");
	for(i = 0; i < encrypted_text_length; i++) {
		printf(" %02x", encrypted_text[i]);
	}
	printf("\n");
}

/*int main(int argc, char *argv[]) {
	unsigned char plaintext[] = "This is the message to be encrypted. Adding more bytes for test, and some more and more after.";
	unsigned char ciphertext[1024];
	unsigned char decryptedtext[1024];
	int ciphertext_len;
	int decryptedtext_len;

	unsigned char key[] = "0123456789abcdef";
	unsigned char iv[] = "abcdefghijklmnop";
	
	encrypt(key, iv, plaintext, ciphertext, &ciphertext_len);
	decrypt(key, iv, decryptedtext, ciphertext, ciphertext_len, &decryptedtext_len);
	
	printf("ciphertext_len = %d, decryptedtext_len = %d\n", ciphertext_len, decryptedtext_len);

	printf("Original message: %s\n", plaintext);
	//printf("Encrypted message: %s\n", ciphertext);
	hex_print(ciphertext, ciphertext_len);
	printf("Decrypted message: %s\n", decryptedtext);
	
	return 0;
}*/
