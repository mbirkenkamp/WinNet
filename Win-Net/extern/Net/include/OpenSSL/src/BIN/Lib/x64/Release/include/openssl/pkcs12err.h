/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OPENSSL_PKCS12ERR_H
# define OPENSSL_PKCS12ERR_H
# pragma once

# include <openssl/opensslconf.h>
# include <openssl/symhacks.h>


# ifdef  __cplusplus
extern "C"
# endif
int ERR_load_PKCS12_strings(void);

/*
 * PKCS12 function codes.
 */
# ifndef OPENSSL_NO_DEPRECATED_3_0
#  define PKCS12_F_OPENSSL_ASC2UNI                         0
#  define PKCS12_F_OPENSSL_UNI2ASC                         0
#  define PKCS12_F_OPENSSL_UNI2UTF8                        0
#  define PKCS12_F_OPENSSL_UTF82UNI                        0
#  define PKCS12_F_PKCS12_CREATE                           0
#  define PKCS12_F_PKCS12_GEN_MAC                          0
#  define PKCS12_F_PKCS12_INIT                             0
#  define PKCS12_F_PKCS12_ITEM_DECRYPT_D2I                 0
#  define PKCS12_F_PKCS12_ITEM_I2D_ENCRYPT                 0
#  define PKCS12_F_PKCS12_ITEM_PACK_SAFEBAG                0
#  define PKCS12_F_PKCS12_KEY_GEN_ASC                      0
#  define PKCS12_F_PKCS12_KEY_GEN_UNI                      0
#  define PKCS12_F_PKCS12_KEY_GEN_UTF8                     0
#  define PKCS12_F_PKCS12_NEWPASS                          0
#  define PKCS12_F_PKCS12_PACK_P7DATA                      0
#  define PKCS12_F_PKCS12_PACK_P7ENCDATA                   0
#  define PKCS12_F_PKCS12_PARSE                            0
#  define PKCS12_F_PKCS12_PBE_CRYPT                        0
#  define PKCS12_F_PKCS12_PBE_KEYIVGEN                     0
#  define PKCS12_F_PKCS12_SAFEBAG_CREATE0_P8INF            0
#  define PKCS12_F_PKCS12_SAFEBAG_CREATE0_PKCS8            0
#  define PKCS12_F_PKCS12_SAFEBAG_CREATE_PKCS8_ENCRYPT     0
#  define PKCS12_F_PKCS12_SAFEBAG_CREATE_SECRET            0
#  define PKCS12_F_PKCS12_SETUP_MAC                        0
#  define PKCS12_F_PKCS12_SET_MAC                          0
#  define PKCS12_F_PKCS12_UNPACK_AUTHSAFES                 0
#  define PKCS12_F_PKCS12_UNPACK_P7DATA                    0
#  define PKCS12_F_PKCS12_VERIFY_MAC                       0
#  define PKCS12_F_PKCS8_ENCRYPT                           0
#  define PKCS12_F_PKCS8_SET0_PBE                          0
# endif

/*
 * PKCS12 reason codes.
 */
# define PKCS12_R_CANT_PACK_STRUCTURE                     100
# define PKCS12_R_CONTENT_TYPE_NOT_DATA                   121
# define PKCS12_R_DECODE_ERROR                            101
# define PKCS12_R_ENCODE_ERROR                            102
# define PKCS12_R_ENCRYPT_ERROR                           103
# define PKCS12_R_ERROR_SETTING_ENCRYPTED_DATA_TYPE       120
# define PKCS12_R_INVALID_NULL_ARGUMENT                   104
# define PKCS12_R_INVALID_NULL_PKCS12_POINTER             105
# define PKCS12_R_INVALID_TYPE                            112
# define PKCS12_R_IV_GEN_ERROR                            106
# define PKCS12_R_KEY_GEN_ERROR                           107
# define PKCS12_R_MAC_ABSENT                              108
# define PKCS12_R_MAC_GENERATION_ERROR                    109
# define PKCS12_R_MAC_SETUP_ERROR                         110
# define PKCS12_R_MAC_STRING_SET_ERROR                    111
# define PKCS12_R_MAC_VERIFY_FAILURE                      113
# define PKCS12_R_PARSE_ERROR                             114
# define PKCS12_R_PKCS12_ALGOR_CIPHERINIT_ERROR           115
# define PKCS12_R_PKCS12_CIPHERFINAL_ERROR                116
# define PKCS12_R_PKCS12_PBE_CRYPT_ERROR                  117
# define PKCS12_R_UNKNOWN_DIGEST_ALGORITHM                118
# define PKCS12_R_UNSUPPORTED_PKCS12_MODE                 119

#endif
