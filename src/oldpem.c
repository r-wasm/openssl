#include <R.h>
#include <Rinternals.h>
#include "apple.h"
#include "utils.h"
#include <openssl/pem.h>
#include <openssl/bn.h>

int password_cb(char *buf, int max_size, int rwflag, void *ctx);

SEXP R_write_pkcs8(RSA *rsa){
  //Rprintf("Public key: d: %d, e: %d, n:%d, p:%p, q:%d\n", rsa->d, rsa->e, rsa->n, rsa->p, rsa->q);
  int len = i2d_RSA_PUBKEY(rsa, NULL);
  bail(len);
  SEXP res = PROTECT(allocVector(RAWSXP, len));
  UNPROTECT(1);
  unsigned char *ptr = RAW(res);
  bail(i2d_RSA_PUBKEY(rsa, &(ptr)));
  return res;
}

SEXP R_write_rsa_private(RSA *rsa){
  //Rprintf("Private key: d: %d, e: %d, n:%d, p:%p, q:%d\n", rsa->d, rsa->e, rsa->n, rsa->p, rsa->q);
  int len = i2d_RSAPrivateKey(rsa, NULL);
  bail(len);
  SEXP res = PROTECT(allocVector(RAWSXP, len));
  UNPROTECT(1);
  unsigned char *ptr = RAW(res);
  bail(i2d_RSAPrivateKey(rsa, &(ptr)));
  return res;
}

SEXP R_priv2pub(SEXP bin){
  RSA *rsa = RSA_new();
  const unsigned char *ptr = RAW(bin);
  bail(!!d2i_RSAPrivateKey(&rsa, &ptr, LENGTH(bin)));
  return R_write_pkcs8(rsa);
}

SEXP R_parse_pkcs8(SEXP input){
  RSA *rsa = RSA_new();
  BIO *mem = BIO_new_mem_buf(RAW(input), LENGTH(input));
  bail(!!PEM_read_bio_RSA_PUBKEY(mem, &rsa, password_cb, NULL));
  bail(EVP_PKEY_assign_RSA(EVP_PKEY_new(), rsa));
  return R_write_pkcs8(rsa);
}

SEXP R_parse_rsa_private(SEXP input, SEXP password){
  EVP_PKEY *key = EVP_PKEY_new();
  BIO *mem = BIO_new_mem_buf(RAW(input), LENGTH(input));
  bail(!!PEM_read_bio_PrivateKey(mem, &key, password_cb, password));
  RSA *rsa = EVP_PKEY_get1_RSA(key);
  return R_write_rsa_private(rsa);
}

SEXP R_cert2pub(SEXP bin){
  X509 *cert = X509_new();
  const unsigned char *ptr = RAW(bin);
  bail(!!d2i_X509(&cert, &ptr, LENGTH(bin)));
  EVP_PKEY *key = X509_get_pubkey(cert);
  if (EVP_PKEY_type(key->type) != EVP_PKEY_RSA)
    error("Key is not RSA key");
  RSA *rsa = EVP_PKEY_get1_RSA(key);
  return R_write_pkcs8(rsa);
}

SEXP R_guess_type(SEXP bin){
  RSA *rsa = RSA_new();
  X509 *cert = X509_new();
  const unsigned char *ptr = RAW(bin);
  if(d2i_RSAPrivateKey(&rsa, &ptr, LENGTH(bin))) {
    return mkString("key");
  } else if(d2i_RSA_PUBKEY(&rsa, &ptr, LENGTH(bin))) {
    return mkString("pubkey");
  } else if(d2i_X509(&cert, &ptr, LENGTH(bin))) {
    return mkString("cert");
  }
  return R_NilValue;
}