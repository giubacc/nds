/* Original Work Copyright (c) 2021 Giuseppe Baccini - giuseppe.baccini@live.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef NDS_NDS_H_
#define NDS_NDS_H_

#ifdef __GNUG__
#include <string.h>
#include <stdarg.h>
#endif
#if defined (__GNUG__) || defined(__MACH__) || defined(__APPLE__)
#include <stdio.h>
#define SOCKET int
#define INVALID_SOCKET (~0)
#define SOCKET_ERROR   (-1)
#endif
#include <arpa/inet.h>

#include <string>
#include <memory>
#include <unordered_map>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#define LS_WEL "WL|"
#define LS_EMP "  |"
#define LS_SYS "SY|"
#define LS_CTR "-C|{}"
#define LS_DTR "~D|{}"
#define LS_OPN "->|{} "
#define LS_CLO "<-|{} "
#define LS_TRL "--|{} "
#define LS_EXE "EX|{} "
#define LS_PRS "PS|{} "
#define LS_SEL "SL|"
#define LS_OUT ">>|"
#define LS_INC "<<|"
#define LS_TRX "TX|"
#define LS_TXO "T>|"
#define LS_TXI "T<|"
#define LS_STM "SM|{} "
#define LS_QRY "QR|{} "
#define LS_APL "AP|"
#define LS_PAR "PA|"
#define LS_MDL "MD|{} "
#define LS_DRV "DV|"
#define LS_CON "CN|"
#define LS_SBS "SB|"
#define LS_SBO "S>|"
#define LS_SBI "S<|"

#define LS_EXUNX " @EXPECT THE UNEXPECTED! "

#define RET_ON_KO(fun)\
{\
    RetCode res;\
    if((res = fun)){\
        return res;\
    }\
}

#define CMD_ON_OK(fun, cmd)\
{\
    int res;\
    if(!(res = fun)){\
        cmd;\
    }\
}

#define CMD_ON_KO(fun, cmd)\
{\
    int res;\
    if((res = fun)){\
        cmd;\
    }\
}

#define CMD_ON_NUL(ptr, cmd)\
{\
    if(!(ptr)){\
        cmd;\
    }\
}

#define NO_ACTION ;
#define NDSDEFLOG "vlglog"

#define IFLOG(log, meth) if(log) log->meth;
#define DTOR_TRC(log) IFLOG(log, trace(LS_DTR, __func__))

namespace nds {

/** @brief return codes.
    When possible functions and methods of nds framework
    will use this set of codes as return value.
*/
typedef enum {
    RetCode_UNKERR           = -1000,    /**< unknown error */

    //system errors
    RetCode_SCKERR           = -105,     /**< socket error */
    RetCode_DBERR            = -104,     /**< database error */
    RetCode_IOERR            = -103,     /**< I/O operation fail */
    RetCode_MEMERR           = -102,     /**< memory error */
    RetCode_SYSERR           = -101,     /**< system error */

    //generic error
    RetCode_UNVRSC           = -2,       /**< unavailable resource */
    RetCode_GENERR           = -1,       /**< generic error */

    //success, failure [0,1]
    RetCode_OK,                          /**< operation ok */
    RetCode_KO,                          /**< operation fail */
    RetCode_EXIT,                        /**< exit required */
    RetCode_RETRY,                       /**< request retry */
    RetCode_ABORT,                       /**< operation aborted */

    //generics
    RetCode_UNSP             = 100,      /**< unsupported */
    RetCode_NODATA,                      /**< no data */
    RetCode_NOTFOUND,                    /**< not found */
    RetCode_TIMEOUT,                     /**< timeout */

    //contaniers specific
    RetCode_EMPTY            = 200,      /**< empty */
    RetCode_QFULL,                       /**< queue full */
    RetCode_OVRSZ,                       /**< oversize */
    RetCode_BOVFL,                       /**< buffer overflow */

    //proc. specific
    RetCode_BADARG           = 300,      /**< bad argument */
    RetCode_BADIDX,                      /**< bad index */
    RetCode_BADSTTS,                     /**< bad status */
    RetCode_BADCFG,                      /**< bad configuration */

    //network specific
    RetCode_DRPPKT           = 400,      /**< packet dropped*/
    RetCode_MALFORM,                     /**< packet malformed */
    RetCode_SCKCLO,                      /**< socket closed */
    RetCode_SCKWBLK,                     /**< socket would block */
    RetCode_PARTPKT,                     /**< partial packet */

} RetCode;

/** @brief logging system levels.
*/
typedef enum {
    TL_EVL = -1,    /**< error value*/
    TL_TRC,         /**< trace log value*/
    TL_DBG,         /**< debug log value*/
    TL_INF,         /**< info log value*/
    TL_WRN,         /**< warning log value*/
    TL_ERR,         /**< error log value*/
    TL_CRI,         /**< critical log value*/
    TL_OFF,         /**< off log value*/
} TraceLVL;

}
#endif
