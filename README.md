# PKCS11 bindings for PHP

Work in progress. DO NOT USE!

Currently supports:

* C_Initialize (via new PKCS11\Module())
* C_GetInfo
* C_GetSlotList
* C_GetSlotInfo
* C_GetTokenInfo
* C_InitToken
* C_GetMechanismList
* C_GetMechanismInfo
* C_OpenSession
* C_Login
* C_Logout
* C_InitPIN
* C_SetPIN
* Generating keys (C_GenerateKey, C_GenerateKeyPair)
* Retrieving keys and properties (C_FindObjectsInit, C_FindObjects, C_FindObjectsFinal, C_GetAttributeValue)
* Encrypt/Decrypt (C_EncryptInit, C_Encrypt, C_DecryptInit, C_Decrypt) Tested with:
  * CKM_AES_CBC_PAD
  * CKM_AES_GCM
  * CKM_RSA_PKCS
  * CKM_RSA_PKCS_OAEP
* Signing (C_SignInit, C_Sign, C_VerifyInit, C_Verify) Tested with:
  * CKM_*RSA_PKCS
  * CKM_*RSA_PKCS_PSS
  * CKM_ECDSA
  * CKM_EDDSA
  * CKM_*HMAC
* Deriving (C_DeriveKey) Tested with:
  * CKM_ECDH1_DERIVE (P-256, P-384, P-521, X25519, X448)

Coming up in (probable) order:
* C_CreateObject
* C_CopyObject
* C_DestroyObject
* C_WrapKey
* C_UnwrapKey
* ChaCha20-Poly1305
* Blake

