/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Guillaume Amringer                                           |
   +----------------------------------------------------------------------+
*/

#include "pkcs11int.h"

zend_class_entry *ce_Pkcs11_Key;
static zend_object_handlers pkcs11_key_handlers;


ZEND_BEGIN_ARG_INFO_EX(arginfo_sign, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, mechanismId, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, mechanismArgument, IS_OBJECT, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_verify, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, mechanismId, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, signature, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, mechanismArgument, IS_OBJECT, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_getAttributeValue, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, attributeIds, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_encrypt, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, mechanismId, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, plaintext, IS_STRING, 0)
    ZEND_ARG_INFO(0, mechanismArgument)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_decrypt, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, mechanismId, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, ciphertext, IS_STRING, 0)
    ZEND_ARG_INFO(0, mechanismArgument)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_derive, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, mechanismId, IS_LONG, 0)
    ZEND_ARG_INFO(0, mechanismArgument)
    ZEND_ARG_TYPE_INFO(0, template, IS_ARRAY, 0)
ZEND_END_ARG_INFO()


PHP_METHOD(Key, sign) {

    CK_RV rv;
    zend_long mechanismId;
    zend_string *data;
    zval *mechanismArgument = NULL;

    ZEND_PARSE_PARAMETERS_START(2,3)
        Z_PARAM_LONG(mechanismId)
        Z_PARAM_STR(data)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(mechanismArgument)
    ZEND_PARSE_PARAMETERS_END();

    CK_MECHANISM mechanism = {mechanismId, NULL_PTR, 0};

    if (mechanismArgument) {
        if(zend_string_equals_literal(Z_OBJ_P(mechanismArgument)->ce->name, "Pkcs11\\RsaPssParams")) {
            pkcs11_rsapssparams_object *mechanismObj = Z_PKCS11_RSAPSSPARAMS_P(mechanismArgument);
            mechanism.pParameter = &mechanismObj->params;
            mechanism.ulParameterLen = sizeof(mechanismObj->params);
        }
    }

    pkcs11_key_object *objval = Z_PKCS11_KEY_P(ZEND_THIS);
    rv = objval->session->pkcs11->functionList->C_SignInit(
        objval->session->session,
        &mechanism,
        objval->key
    );
    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to sign");
        return;
    }

    CK_ULONG signatureLen;
    rv = objval->session->pkcs11->functionList->C_Sign(
        objval->session->session,
        ZSTR_VAL(data),
        ZSTR_LEN(data),
        NULL_PTR,
        &signatureLen
    );
    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to sign");
        return;
    }

    CK_BYTE_PTR signature = ecalloc(signatureLen, sizeof(CK_BYTE));
    rv = objval->session->pkcs11->functionList->C_Sign(
        objval->session->session,
        ZSTR_VAL(data),
        ZSTR_LEN(data),
        signature,
        &signatureLen
    );
    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to sign");
        return;
    }

    zend_string *returnval;
    returnval = zend_string_alloc(signatureLen, 0);
    memcpy(
        ZSTR_VAL(returnval),
        signature,
        signatureLen
    );
    RETURN_STR(returnval);
 
    efree(signature);
}


PHP_METHOD(Key, verify) {

    CK_RV rv;
    zend_long mechanismId;
    zend_string *data;
    zend_string *signature;
    zval *mechanismArgument = NULL;

    ZEND_PARSE_PARAMETERS_START(3,4)
        Z_PARAM_LONG(mechanismId)
        Z_PARAM_STR(data)
        Z_PARAM_STR(signature)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(mechanismArgument)
    ZEND_PARSE_PARAMETERS_END();

    CK_MECHANISM mechanism = {mechanismId, NULL_PTR, 0};
    CK_VOID_PTR pParams;

    if (mechanismArgument) {
        if(zend_string_equals_literal(Z_OBJ_P(mechanismArgument)->ce->name, "Pkcs11\\RsaPssParams")) {
            pkcs11_rsapssparams_object *mechanismObj = Z_PKCS11_RSAPSSPARAMS_P(mechanismArgument);
            mechanism.pParameter = &mechanismObj->params;
            mechanism.ulParameterLen = sizeof(mechanismObj->params);
        }
    }

    pkcs11_key_object *objval = Z_PKCS11_KEY_P(ZEND_THIS);
    rv = objval->session->pkcs11->functionList->C_VerifyInit(
        objval->session->session,
        &mechanism,
        objval->key
    );
    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to verify");
        return;
    }

    CK_ULONG signatureLen;
    rv = objval->session->pkcs11->functionList->C_Verify(
        objval->session->session,
        ZSTR_VAL(data),
        ZSTR_LEN(data),
        ZSTR_VAL(signature),
        ZSTR_LEN(signature)
    );

    if (rv == CKR_SIGNATURE_INVALID) {
        RETURN_BOOL(false);
    }

    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to verify");
        return;
    }

    RETURN_BOOL(true);
}


PHP_METHOD(Key, getAttributeValue) {

    CK_RV rv;
    zval *attributeIds;
    zval *attributeId;
    unsigned int i;

    ZEND_PARSE_PARAMETERS_START(1,1)
        Z_PARAM_ARRAY(attributeIds)
    ZEND_PARSE_PARAMETERS_END();

    int attributeIdCount = zend_hash_num_elements(Z_ARRVAL_P(attributeIds));

    CK_ATTRIBUTE_PTR template = (CK_ATTRIBUTE *) ecalloc(sizeof(CK_ATTRIBUTE), attributeIdCount);

    i = 0;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(attributeIds), attributeId) {
        if (Z_TYPE_P(attributeId) != IS_LONG) {
            general_error("PKCS11 module error", "Unable to get attribute value. Attribute ID must be an integer");
            return;
        }
        template[i] = (CK_ATTRIBUTE) {zval_get_long(attributeId), NULL_PTR, 0};
        i++;
    } ZEND_HASH_FOREACH_END();

    pkcs11_key_object *objval = Z_PKCS11_KEY_P(ZEND_THIS);
    rv = objval->session->pkcs11->functionList->C_GetAttributeValue(
        objval->session->session,
        objval->key,
        template,
        attributeIdCount
    );
    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to get attribute value");
        return;
    }

    for (i=0; i<attributeIdCount; i++) {
        template[i].pValue = (uint8_t *) ecalloc(1, template[i].ulValueLen);
    }

    rv = objval->session->pkcs11->functionList->C_GetAttributeValue(
        objval->session->session,
        objval->key,
        template,
        attributeIdCount
    );
    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to get attribute value");
        return;
    }

    array_init(return_value);
    for (i=0; i<attributeIdCount; i++) {
        zend_string *foo;
        foo = zend_string_alloc(template[i].ulValueLen, 0);
        memcpy(
            ZSTR_VAL(foo),
            template[i].pValue,
            template[i].ulValueLen
        );

        efree(template[i].pValue);

        add_index_str(return_value, template[i].type, foo);
    }

    efree(template);
}


PHP_METHOD(Key, encrypt) {

    CK_RV rv;
    zend_long mechanismId;
    zend_string *plaintext;
    zval *mechanismArgument = NULL;

    ZEND_PARSE_PARAMETERS_START(2,3)
        Z_PARAM_LONG(mechanismId)
        Z_PARAM_STR(plaintext)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(mechanismArgument)
    ZEND_PARSE_PARAMETERS_END();

    CK_MECHANISM mechanism = {mechanismId, NULL_PTR, 0};

    if (mechanismArgument) {
        if (Z_TYPE_P(mechanismArgument) == IS_STRING) {
            mechanism.pParameter = Z_STRVAL_P(mechanismArgument);
            mechanism.ulParameterLen = Z_STRLEN_P(mechanismArgument);

        } else if (Z_TYPE_P(mechanismArgument) == IS_OBJECT) {
            if(zend_string_equals_literal(Z_OBJ_P(mechanismArgument)->ce->name, "Pkcs11\\GcmParams")) {
                pkcs11_gcmparams_object *mechanismObj = Z_PKCS11_GCMPARAMS_P(mechanismArgument);
                mechanism.pParameter = &mechanismObj->params;
                mechanism.ulParameterLen = sizeof(mechanismObj->params);
            
            } else if(zend_string_equals_literal(Z_OBJ_P(mechanismArgument)->ce->name, "Pkcs11\\RsaOaepParams")) {
                pkcs11_rsaoaepparams_object *mechanismObj = Z_PKCS11_RSAOAEPPARAMS_P(mechanismArgument);
                mechanism.pParameter = &mechanismObj->params;
                mechanism.ulParameterLen = sizeof(mechanismObj->params);
            }
        }
    }

    pkcs11_key_object *objval = Z_PKCS11_KEY_P(ZEND_THIS);
    rv = objval->session->pkcs11->functionList->C_EncryptInit(
        objval->session->session,
        &mechanism,
        objval->key
    );
    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to encrypt");
        return;
    }

    CK_ULONG ciphertextLen;
    rv = objval->session->pkcs11->functionList->C_Encrypt(
        objval->session->session,
        ZSTR_VAL(plaintext),
        ZSTR_LEN(plaintext),
        NULL_PTR ,
        &ciphertextLen
    );
    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to encrypt");
        return;
    }

    CK_BYTE_PTR ciphertext = ecalloc(ciphertextLen, sizeof(CK_BYTE));
    rv = objval->session->pkcs11->functionList->C_Encrypt(
        objval->session->session,
        ZSTR_VAL(plaintext),
        ZSTR_LEN(plaintext),
        ciphertext,
        &ciphertextLen
    );
    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to encrypt");
        return;
    }

    zend_string *returnval;
    returnval = zend_string_alloc(ciphertextLen, 0);
    memcpy(
        ZSTR_VAL(returnval),
        ciphertext,
        ciphertextLen
    );
    RETURN_STR(returnval);

    efree(ciphertext);
}


PHP_METHOD(Key, decrypt) {

    CK_RV rv;
    zend_long mechanismId;
    zend_string *ciphertext;
    zval *mechanismArgument = NULL;

    ZEND_PARSE_PARAMETERS_START(2,3)
        Z_PARAM_LONG(mechanismId)
        Z_PARAM_STR(ciphertext)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(mechanismArgument)
    ZEND_PARSE_PARAMETERS_END();

    CK_MECHANISM mechanism = {mechanismId, NULL_PTR, 0};

    if (mechanismArgument) {
        if (Z_TYPE_P(mechanismArgument) == IS_STRING) {
            mechanism.pParameter = Z_STRVAL_P(mechanismArgument);
            mechanism.ulParameterLen = Z_STRLEN_P(mechanismArgument);

        } else if (Z_TYPE_P(mechanismArgument) == IS_OBJECT) {
            if(zend_string_equals_literal(Z_OBJ_P(mechanismArgument)->ce->name, "Pkcs11\\GcmParams")) {
                pkcs11_gcmparams_object *mechanismObj = Z_PKCS11_GCMPARAMS_P(mechanismArgument);
                mechanism.pParameter = &mechanismObj->params;
                mechanism.ulParameterLen = sizeof(mechanismObj->params);
            
            } else if(zend_string_equals_literal(Z_OBJ_P(mechanismArgument)->ce->name, "Pkcs11\\RsaOaepParams")) {
                pkcs11_rsaoaepparams_object *mechanismObj = Z_PKCS11_RSAOAEPPARAMS_P(mechanismArgument);
                mechanism.pParameter = &mechanismObj->params;
                mechanism.ulParameterLen = sizeof(mechanismObj->params);
            }
        }
    }

    pkcs11_key_object *objval = Z_PKCS11_KEY_P(ZEND_THIS);
    rv = objval->session->pkcs11->functionList->C_DecryptInit(
        objval->session->session,
        &mechanism,
        objval->key
    );
    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to decrypt");
        return;
    }

    CK_ULONG plaintextLen;
    rv = objval->session->pkcs11->functionList->C_Decrypt(
        objval->session->session,
        ZSTR_VAL(ciphertext),
        ZSTR_LEN(ciphertext),
        NULL_PTR,
        &plaintextLen
    );
    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to decrypt");
        return;
    }

    CK_BYTE_PTR plaintext = ecalloc(plaintextLen, sizeof(CK_BYTE));
    rv = objval->session->pkcs11->functionList->C_Decrypt(
        objval->session->session,
        ZSTR_VAL(ciphertext),
        ZSTR_LEN(ciphertext),
        plaintext,
        &plaintextLen
    );
    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to decrypt");
        return;
    }

    zend_string *returnval;
    returnval = zend_string_alloc(plaintextLen, 0);
    memcpy(
        ZSTR_VAL(returnval),
        plaintext,
        plaintextLen
    );
    RETURN_STR(returnval);

    efree(plaintext);
}


PHP_METHOD(Key, derive) {

    CK_RV rv;
    zend_long mechanismId;
    zval *mechanismArgument = NULL;
    HashTable *template;

    ZEND_PARSE_PARAMETERS_START(3,3)
        Z_PARAM_LONG(mechanismId)
        Z_PARAM_ZVAL(mechanismArgument)
        Z_PARAM_ARRAY_HT(template)
    ZEND_PARSE_PARAMETERS_END();

    CK_MECHANISM mechanism = {mechanismId, NULL_PTR, 0};
    CK_VOID_PTR pParams;
    CK_OBJECT_HANDLE phKey;

    int templateItemCount;
    CK_ATTRIBUTE_PTR templateObj;
    parseTemplate(&template, &templateObj, &templateItemCount);

    if (mechanismArgument) {
        if(zend_string_equals_literal(Z_OBJ_P(mechanismArgument)->ce->name, "Pkcs11\\Ecdh1DeriveParams")) {
            pkcs11_ecdh1deriveparams_object *mechanismObj = Z_PKCS11_ECDH1DERIVEPARAMS_P(mechanismArgument);
            mechanism.pParameter = &mechanismObj->params;
            mechanism.ulParameterLen = sizeof(mechanismObj->params);
        }
    }

    pkcs11_key_object *objval = Z_PKCS11_KEY_P(ZEND_THIS);
    rv = objval->session->pkcs11->functionList->C_DeriveKey(
        objval->session->session,
        &mechanism,
        objval->key,
        templateObj,
        templateItemCount,
        &phKey
    );
    freeTemplate(templateObj);

    if (rv != CKR_OK) {
        pkcs11_error(rv, "Unable to derive");
        return;
    }

    pkcs11_key_object* key_obj;

    object_init_ex(return_value, ce_Pkcs11_Key);
    key_obj = Z_PKCS11_KEY_P(return_value);
    key_obj->session = objval->session;
    key_obj->key = phKey;
}


void pkcs11_key_shutdown(pkcs11_key_object *obj) {
}

static zend_function_entry key_class_functions[] = {
    PHP_ME(Key, encrypt,           arginfo_encrypt,           ZEND_ACC_PUBLIC)
    PHP_ME(Key, decrypt,           arginfo_decrypt,           ZEND_ACC_PUBLIC)
    PHP_ME(Key, sign,              arginfo_sign,              ZEND_ACC_PUBLIC)
    PHP_ME(Key, verify,            arginfo_verify,            ZEND_ACC_PUBLIC)
    PHP_ME(Key, derive,            arginfo_derive,            ZEND_ACC_PUBLIC)
    PHP_ME(Key, getAttributeValue, arginfo_getAttributeValue, ZEND_ACC_PUBLIC)
    PHP_FE_END
};


DEFINE_MAGIC_FUNCS(pkcs11_key, key, Key)
