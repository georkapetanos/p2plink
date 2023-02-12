#ifndef _CRYPTO_H
#define _CRYPTO_H

int encrypt(unsigned char *key, unsigned char *iv, unsigned char *plaintext, unsigned char *encrypted_text, int *encrypted_text_length);
int decrypt(unsigned char *key, unsigned char *iv, unsigned char *decrypted_text, unsigned char *encrypted_text, int encrypted_text_length, int *decrypted_text_length);
void hex_print(unsigned char *encrypted_text, int encrypted_text_length);

#endif
