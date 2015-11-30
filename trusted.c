#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>

// Libs for certificate
//#include <openssl/x509v3.h>
#include <openssl/x509.h>
#include <openssl/engine.h>
#include <openssl/pem.h>
#include <openssl/conf.h>

#include "prototypes.h"

int IDt = 111;
int SIZE_OF_KEY = 16;

int mkcert(X509 **, EVP_PKEY **, int , int , int );

char* getCertificate(unsigned char* publicKey){

	
	//cert = malloc(4096*sizeof(*cert));
	X509 *cert;
	EVP_PKEY *pubkey;
	d2i_PublicKey(NULL, &pubkey, &publicKey, strlen(publicKey));

	mkcert(&cert, &pubkey, 512, 0, 365);

	//BIO *b64;
	FILE *temp;
	temp = fopen("temp", "w+");
	PEM_write_X509(temp, cert);

	fseek(temp, 0L, SEEK_END);

	// Get the size of the file
	int size = ftell(temp);
	rewind(temp);

	// Now read into a buffer
	char certbuf[4096];
	
	fread(certbuf, sizeof(char), size, temp);
	certbuf[size+1] = '\0';
    fclose(temp);
	remove("temp");

	free(cert);
	return certbuf;
}

void verifyLog(int IDu, char* PKEsessionKey, char* encryptedLog){
	FILE *tpriv;
	tpriv = fopen("T_Priv.pem", "r");
	RSA *tpriv_key = PEM_read_RSAPrivateKey(tpriv,NULL,NULL,NULL);

	//----------- Decrypt PKE session key -----------
	setKey(publicKeyDecrypt(tpriv_key, PKEsessionKey));
	// printf("Decrypted: %s\n", sessionKeyU);

	//----------- Decrypt encryptedLog using session key ----------- 
	char* logfile = decrypt(encryptedLog);

	//----------- Verify X0 is correct ----------- 
	char* hashedLogfile = hash(logfile);
	//printf("%s\n", hashedLogfile);
	if(strcmp(hashedLogfile, getUHash())){
		fprintf(stderr, "X0 values do not match!\n");
		return;
	}

	//----------- Verify SIGN(X0) is correct ----------

	//----------- Create X1 = IDlog, hash(X0) ----------- 
	char *IDlog_string = malloc(15 * sizeof(char));
	addMemBlock(IDlog_string);
	sprintf(IDlog_string, "%d", getLogId());
	char* X = malloc((strlen(IDlog_string) + strlen(hashedLogfile)) * sizeof(char));
	addMemBlock(X);
	strcpy(X, IDlog_string);
	strcat(X, hashedLogfile);

	//----------- Generate random session key K1 ----------- 
	char* sessionKey = hash(createKey(SIZE_OF_KEY));

	//----------- Encrypt session info ----------
	//encrypt key using PKEu 
	FILE *upub;
	upub = fopen("U_Pub.pub", "r");
	char *upub_key = fileToBuffer(upub);
	char* PKEu = publicKeyEncrypt(upub_key, sessionKey);

	//encrypt X using session key
	setKey(sessionKey);
	char* E = malloc((strlen(X) + 1) * sizeof(char));
	addMemBlock(E);
	E = encrypt(X);

	//----------- Create M1 = IDt, PKE(K1), E(X1, SIGN(X1)) ----------- 
	response(IDt, PKEu, E);
}

void getEntryKeys_Trusted(char** entries, char** keys, int line_count) {

}

int mkcert(X509 **x509p, EVP_PKEY **pkeyp, int bits, int serial, int days) {
	X509 *x;
	EVP_PKEY *pk;
	RSA *rsa = EVP_PKEY_get1_RSA(&pkeyp);

	X509_NAME *name = NULL;
	

	pk = *pkeyp;
	x = X509_new();

	// EVP_PKEY_assign_RSA(pk,rsa);

	rsa=NULL;

	X509_set_version(x,2);
	ASN1_INTEGER_set(X509_get_serialNumber(x),serial);
	X509_gmtime_adj(X509_get_notBefore(x),0);
	X509_gmtime_adj(X509_get_notAfter(x),(long)60*60*24*days);

	// EVP_PKEY_assign_RSA(pk,rsa);

	// X509_set_pubkey(x, *pkeyp);

	name=X509_get_subject_name(x);

	/* This function creates and adds the entry, working out the
	 * correct string type and performing checks on its length.
	 * Normally we'd check the return value for errors...
	 */
	X509_NAME_add_entry_by_txt(name,"C",
				MBSTRING_ASC, "UK", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name,"CN",
				MBSTRING_ASC, "OpenSSL Group", -1, -1, 0);

	 
	X509_set_issuer_name(x,name);

	
	X509_sign(x,pk,EVP_md5());

	*x509p=x;
	*pkeyp=pk;
}
