/**********************************************************************
 *                        params.c                                    *
 *             Copyright (c) 2005-2006 Cryptocom LTD                  *
 *         This file is distributed under the same license as OpenSSL *
 *                                                                    *
 * Definitions of GOST R 34.10 parameter sets, defined in RFC 4357    *
 *         OpenSSL 0.9.9 libraries required to compile and use        *
 *                              this code                             *
 **********************************************************************/
#include "gost_lcl.h"
#include <openssl/objects.h>
/* Parameters of GOST 34.10 */

R3410_2001_params R3410_2001_paramset[] = {
    /* 1.2.643.2.2.35.0 */
    {NID_id_GostR3410_2001_TestParamSet,
     "7",
     "5FBFF498AA938CE739B8E022FBAFEF40563F6E6A3472FC2A514C0CE9DAE23B7E",
     "8000000000000000000000000000000000000000000000000000000000000431",
     "8000000000000000000000000000000150FE8A1892976154C59CFC193ACCF5B3",
     "2",
     "08E2A8A0E65147D4BD6316030E16D19C85C97F0A9CA267122B96ABBCEA7E8FC8"}
    ,
    /*
     * 1.2.643.2.2.35.1
     */
    {NID_id_GostR3410_2001_CryptoPro_A_ParamSet,
     "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFD94",
     "a6",
     "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFD97",
     "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF6C611070995AD10045841B09B761B893",
     "1",
     "8D91E471E0989CDA27DF505A453F2B7635294F2DDF23E3B122ACC99C9E9F1E14"}
    ,
    /*
     * 1.2.643.2.2.35.2
     */
    {NID_id_GostR3410_2001_CryptoPro_B_ParamSet,
     "8000000000000000000000000000000000000000000000000000000000000C96",
     "3E1AF419A269A5F866A7D3C25C3DF80AE979259373FF2B182F49D4CE7E1BBC8B",
     "8000000000000000000000000000000000000000000000000000000000000C99",
     "800000000000000000000000000000015F700CFFF1A624E5E497161BCC8A198F",
     "1",
     "3FA8124359F96680B83D1C3EB2C070E5C545C9858D03ECFB744BF8D717717EFC"}
    ,
    /*
     * 1.2.643.2.2.35.3
     */
    {NID_id_GostR3410_2001_CryptoPro_C_ParamSet,
     "9B9F605F5A858107AB1EC85E6B41C8AACF846E86789051D37998F7B9022D7598",
     "805a",
     "9B9F605F5A858107AB1EC85E6B41C8AACF846E86789051D37998F7B9022D759B",
     "9B9F605F5A858107AB1EC85E6B41C8AA582CA3511EDDFB74F02F3A6598980BB9",
     "0",
     "41ECE55743711A8C3CBF3783CD08C0EE4D4DC440D4641A8F366E550DFDB3BB67"}
    ,
    /*
     * 1.2.643.2.2.36.0
     */
    {NID_id_GostR3410_2001_CryptoPro_XchA_ParamSet,
     "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFD94",
     "a6",
     "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFD97",
     "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF6C611070995AD10045841B09B761B893",
     "1",
     "8D91E471E0989CDA27DF505A453F2B7635294F2DDF23E3B122ACC99C9E9F1E14"}
    ,
    /*
     * 1.2.643.2.2.36.1
     */
    {NID_id_GostR3410_2001_CryptoPro_XchB_ParamSet,
     "9B9F605F5A858107AB1EC85E6B41C8AACF846E86789051D37998F7B9022D7598",
     "805a",
     "9B9F605F5A858107AB1EC85E6B41C8AACF846E86789051D37998F7B9022D759B",
     "9B9F605F5A858107AB1EC85E6B41C8AA582CA3511EDDFB74F02F3A6598980BB9",
     "0",
     "41ECE55743711A8C3CBF3783CD08C0EE4D4DC440D4641A8F366E550DFDB3BB67"}
    ,
    {0, NULL, NULL, NULL, NULL, NULL, NULL}
};
