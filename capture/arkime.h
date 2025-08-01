/******************************************************************************/
/* arkime.h -- General Arkime include file
 *
 * Copyright 2012-2017 AOL Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#define __FAVOR_BSD
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/time.h>
#include "http_parser.h"
#include "dll.h"
#include "hash.h"
#include "bsb.h"

#define GLIB_DISABLE_DEPRECATION_WARNINGS
#include "glib.h"

#define UNUSED(x) x __attribute((unused))

#if defined(__clang__)
#define SUPPRESS_SIGNED_INTEGER_OVERFLOW __attribute__((no_sanitize("integer")))
#define SUPPRESS_UNSIGNED_INTEGER_OVERFLOW __attribute__((no_sanitize("integer")))
#define SUPPRESS_SHIFT __attribute__((no_sanitize("shift")))
#define SUPPRESS_ALIGNMENT __attribute__((no_sanitize("alignment")))
#define SUPPRESS_INT_CONVERSION __attribute__((no_sanitize("implicit-integer-sign-change")))
#elif __GNUC__ >= 5
#define SUPPRESS_SIGNED_INTEGER_OVERFLOW
#define SUPPRESS_UNSIGNED_INTEGER_OVERFLOW
#define SUPPRESS_SHIFT __attribute__((no_sanitize_undefined()))
#define SUPPRESS_ALIGNMENT __attribute__((no_sanitize_undefined()))
#define SUPPRESS_INT_CONVERSION
#else
#define SUPPRESS_SIGNED_INTEGER_OVERFLOW
#define SUPPRESS_UNSIGNED_INTEGER_OVERFLOW
#define SUPPRESS_SHIFT
#define SUPPRESS_ALIGNMENT
#define SUPPRESS_INT_CONVERSION
#endif

#define ARKIME_API_VERSION 542

#define ARKIME_SESSIONID_LEN  40
#define ARKIME_SESSIONID6_LEN 40
#define ARKIME_SESSIONID4_LEN 16

#define ARKIME_V6_TO_V4(_addr) (((uint32_t *)(_addr).s6_addr)[3])

#define ARKIME_PACKET_MAX_LEN 0x10000

#define ARKIME_ETHERTYPE_ETHER   0
// If an ethertype is unknown this ethertype will be called
#define ARKIME_ETHERTYPE_UNKNOWN 1
// The first 2 bytes have the ethertype
#define ARKIME_ETHERTYPE_DETECT  2
#define ARKIME_ETHERTYPE_TEB     0x6558
#define ARKIME_ETHERTYPE_RAWFR   0x6559
#define ARKIME_ETHERTYPE_NSH     0x894F
#define ARKIME_ETHERTYPE_MPLS    0x8847
#define ARKIME_ETHERTYPE_QINQ    0x88a8

#define ARKIME_IPPROTO_UNKNOWN 255
#define ARKIME_IPPROTO_CORRUPT 256
#define ARKIME_IPPROTO_MAX     257
#define ARKIME_SESSION_v6(s) ((s)->sessionId[0] == ARKIME_SESSIONID6_LEN)

#define ARKIME_VAR_ARG_STR_SKIP (char *)1LL
#define ARKIME_VAR_ARG_INT_SKIP (char *)0x7fffffffffffffffLL

#define POINTER_TO_FLOAT(p) *(float *)&p

/******************************************************************************/
/*
 * Base Hash Table Types
 */
typedef struct arkime_int {
    struct arkime_int    *i_next, *i_prev;
    uint32_t              i_hash;
    short                 i_bucket;
} ArkimeInt_t;

typedef struct {
    struct arkime_int *i_next, *i_prev;
    int i_count;
} ArkimeIntHead_t;

typedef HASH_VAR(s_, ArkimeIntHash_t, ArkimeIntHead_t, 1);
typedef HASH_VAR(s_, ArkimeIntHashStd_t, ArkimeIntHead_t, 13);

typedef struct arkime_string {
    struct arkime_string *s_next, *s_prev;
    char                 *str;
    uint32_t              s_hash;
    gpointer              uw;
    uint16_t              s_bucket;
    uint16_t              len: 15;
    uint16_t              utf8: 1;
} ArkimeString_t;

typedef struct {
    struct arkime_string *s_next, *s_prev;
    int s_count;
} ArkimeStringHead_t;
typedef HASH_VAR(s_, ArkimeStringHash_t, ArkimeStringHead_t, 1);
typedef HASH_VAR(s_, ArkimeStringHashStd_t, ArkimeStringHead_t, 13);

/******************************************************************************/
/*
 * TRIE
 */
typedef struct arkime_trie_node {
    void                     *data;
    struct arkime_trie_node **children;
    uint8_t                   value, first, last;
} ArkimeTrieNode_t;

typedef struct arkime_trie {
    int size;
    ArkimeTrieNode_t root;
} ArkimeTrie_t;

/******************************************************************************/
/*
 * Generic object field type
 */

typedef struct arkime_field_object {
    struct arkime_field_object      *o_next, *o_prev;
    uint32_t                         o_hash;
    short                            o_bucket;
    void                            *object;
} ArkimeFieldObject_t;

typedef struct {
    struct arkime_field_object      *o_next, *o_prev;
    short                            o_count;
} ArkimeFieldObjectHead_t;

typedef HASH_VAR(o_, ArkimeFieldObjectHash_t, ArkimeFieldObjectHead_t, 1);
typedef HASH_VAR(o_, ArkimeFieldObjectHashStd_t, ArkimeFieldObjectHead_t, 13);

struct arkime_session;

typedef void (* ArkimeFieldObjectSaveFunc) (BSB *jbsb, ArkimeFieldObject_t *object, struct arkime_session *session);
typedef void (* ArkimeFieldObjectFreeFunc) (ArkimeFieldObject_t *object);
typedef uint32_t (* ArkimeFieldObjectHashFunc) (const void *key);
typedef int (* ArkimeFieldObjectCmpFunc) (const void *keyv, const void *elementv);

typedef void (* ArkimeFieldSetFunc) (struct arkime_session *session, int pos, void *value);
typedef void *(* ArkimeFieldGetFunc) (const struct arkime_session *session, int pos);

/******************************************************************************/
/*
 * Information about the various fields that we capture
 */

typedef enum {
    ARKIME_FIELD_TYPE_INT,
    ARKIME_FIELD_TYPE_INT_ARRAY,
    ARKIME_FIELD_TYPE_INT_HASH,
    ARKIME_FIELD_TYPE_INT_GHASH,
    ARKIME_FIELD_TYPE_STR,
    ARKIME_FIELD_TYPE_STR_ARRAY,
    ARKIME_FIELD_TYPE_STR_HASH,
    ARKIME_FIELD_TYPE_STR_GHASH,
    ARKIME_FIELD_TYPE_IP,
    ARKIME_FIELD_TYPE_IP_GHASH,
    ARKIME_FIELD_TYPE_FLOAT,
    ARKIME_FIELD_TYPE_FLOAT_ARRAY,
    ARKIME_FIELD_TYPE_FLOAT_GHASH,
    ARKIME_FIELD_TYPE_OBJECT
} ArkimeFieldType;

#define ARKIME_FIELD_TYPE_IS_INT(t) (t >= ARKIME_FIELD_TYPE_INT && t <= ARKIME_FIELD_TYPE_INT_GHASH)
#define ARKIME_FIELD_TYPE_IS_STR(t) (t >= ARKIME_FIELD_TYPE_STR && t <= ARKIME_FIELD_TYPE_STR_GHASH)
#define ARKIME_FIELD_TYPE_IS_IP(t)  (t >= ARKIME_FIELD_TYPE_IP && t <= ARKIME_FIELD_TYPE_IP_GHASH)
#define ARKIME_FIELD_TYPE_IS_FLOAT(t)  (t >= ARKIME_FIELD_TYPE_FLOAT && t <= ARKIME_FIELD_TYPE_FLOAT_GHASH)

/* These are ones you should set */
/* Field should be set on all linked sessions */
#define ARKIME_FIELD_FLAG_LINKED_SESSIONS    0x0001
/* Force the field to be utf8 */
#define ARKIME_FIELD_FLAG_FORCE_UTF8         0x0004
/* Don't create in fields db table */
#define ARKIME_FIELD_FLAG_NODB               0x0008
/* Not a real field in capture, just in viewer */
#define ARKIME_FIELD_FLAG_FAKE               0x0010
/* Don't save this fields data into memory or ES */
#define ARKIME_FIELD_FLAG_DISABLED           0x0020
/* Save in memory but not in db.c loop, saved another way */
#define ARKIME_FIELD_FLAG_NOSAVE             0x0040
/* Added Cnt */
#define ARKIME_FIELD_FLAG_CNT                0x1000
/* Added -cnt */
#define ARKIME_FIELD_FLAG_ECS_CNT            0x2000
/* prepend ip stuff - dont use*/
#define ARKIME_FIELD_FLAG_IPPRE              0x4000
/* new value has to be different from last value */
#define ARKIME_FIELD_FLAG_DIFF_FROM_LAST     0x8000

typedef struct arkime_field_info {
    struct arkime_field_info *d_next, *d_prev; /* Must be first */
    char                     *dbFieldFull;     /* Must be second - this is the full version example:mysql.user-term */
    char                     *dbField;         /* - this is the version used in db writing example:user-term */
    uint32_t                  d_hash;
    uint16_t                  d_bucket;
    uint16_t                  d_count;

    struct arkime_field_info *e_next, *e_prev;
    char                     *expression;
    uint32_t                  e_hash;
    uint16_t                  e_bucket;
    uint16_t                  e_count;

    int16_t                   dbFieldLen;
    int16_t                   dbGroupNum;
    char                     *dbGroup;
    int16_t                   dbGroupLen;
    char                     *group;
    char                     *kind;
    char                     *category;
    int16_t                   pos;
    ArkimeFieldType           type;
    uint16_t                  flags;
    char                      ruleEnabled;
    uint16_t                  cntForPos;
    char                     *transform;
    char                     *aliases;

    ArkimeFieldSetFunc        setCb;
    ArkimeFieldGetFunc        getCb;

    ArkimeFieldObjectSaveFunc object_save;
    ArkimeFieldObjectFreeFunc object_free;
    ArkimeFieldObjectHashFunc object_hash;
    ArkimeFieldObjectCmpFunc  object_cmp;
} ArkimeFieldInfo_t;

typedef struct {
    union {
        char                       *str;
        GPtrArray                  *sarray;
        ArkimeStringHashStd_t      *shash;
        int                         i;
        GArray                     *iarray;
        ArkimeIntHashStd_t         *ihash;
        float                       f;
        GArray                     *farray;
        GHashTable                 *ghash;
        struct in6_addr            *ip;
        ArkimeFieldObjectHashStd_t *ohash;
    };
    uint32_t                   jsonSize;
} ArkimeField_t;

#define ARKIME_FIELD_OP_SET           0
#define ARKIME_FIELD_OP_SET_IF_LESS   1
#define ARKIME_FIELD_OP_SET_IF_MORE   2

typedef struct {
    char                 *str;
    union {
        int               strLenOrInt;
        float             f;
    };
    int16_t               fieldPos;
    int16_t               matchPos;
    int8_t                set;
} ArkimeFieldOp_t;

#define ARKIME_FIELD_OPS_FLAGS_COPY 0x0001
typedef struct {
    ArkimeFieldOp_t      *ops;
    uint16_t              size;
    uint16_t              num;
    uint16_t              flags;
} ArkimeFieldOps_t;

#define ARKIME_LOCK_DEFINE(var)         pthread_mutex_t var##_mutex = PTHREAD_MUTEX_INITIALIZER
#define ARKIME_LOCK_EXTERN(var)         pthread_mutex_t var##_mutex
#define ARKIME_LOCK_INIT(var)           pthread_mutex_init(&var##_mutex, NULL)
#define ARKIME_LOCK(var)                pthread_mutex_lock(&var##_mutex)
#define ARKIME_UNLOCK(var)              pthread_mutex_unlock(&var##_mutex)

#define ARKIME_COND_DEFINE(var)         pthread_cond_t var##_cond = PTHREAD_COND_INITIALIZER
#define ARKIME_COND_EXTERN(var)         pthread_cond_t var##_cond
#define ARKIME_COND_INIT(var)           pthread_cond_init(&var##_cond, NULL)
#define ARKIME_COND_WAIT(var)           pthread_cond_wait(&var##_cond, &var##_mutex)
#define ARKIME_COND_TIMEDWAIT(var, _ts) pthread_cond_timedwait(&var##_cond, &var##_mutex, &_ts)
#define ARKIME_COND_BROADCAST(var)      pthread_cond_broadcast(&var##_cond)
#define ARKIME_COND_SIGNAL(var)         pthread_cond_signal(&var##_cond)

#define ARKIME_THREAD_INCR(var)          __sync_add_and_fetch(&var, 1);
#define ARKIME_THREAD_INCRNEW(var)       __sync_add_and_fetch(&var, 1);
#define ARKIME_THREAD_INCROLD(var)       __sync_fetch_and_add(&var, 1);
#define ARKIME_THREAD_INCR_NUM(var, num) __sync_add_and_fetch(&var, num);

#define ARKIME_THREAD_DECRNEW(var)       __sync_sub_and_fetch(&var, 1);
#define ARKIME_THREAD_DECROLD(var)       __sync_fetch_and_sub(&var, 1);

/* You are probably looking here because you think 24 is too low, really it isn't.
 * Instead, increase the number of threads used for reading packets.
 * https://arkime.com/faq#why-am-i-dropping-packets
 */
#define ARKIME_MAX_PACKET_THREADS 24

#define MAX_INTERFACES 32

#ifndef LOCAL
#define LOCAL static
#endif

#ifndef CLOCK_MONOTONIC_COARSE
#define CLOCK_MONOTONIC_COARSE CLOCK_MONOTONIC
#endif

#ifndef CLOCK_REALTIME_COARSE
#define CLOCK_REALTIME_COARSE CLOCK_REALTIME
#endif

/******************************************************************************/

typedef enum {
    SESSION_TCP,
    SESSION_UDP,
    SESSION_ICMP,
    SESSION_SCTP,
    SESSION_ESP,
    SESSION_OTHER,
    SESSION_MAX
} SessionTypes;

/******************************************************************************/
/*
 * Configuration Information
 */
enum ArkimeRotate {
    ARKIME_ROTATE_HOURLY,
    ARKIME_ROTATE_HOURLY2,
    ARKIME_ROTATE_HOURLY3,
    ARKIME_ROTATE_HOURLY4,
    ARKIME_ROTATE_HOURLY6,
    ARKIME_ROTATE_HOURLY8,
    ARKIME_ROTATE_HOURLY12,
    ARKIME_ROTATE_DAILY,
    ARKIME_ROTATE_WEEKLY,
    ARKIME_ROTATE_MONTHLY
};

/* Field numbers are signed
 * [ARKIME_FIELD_SPECIAL_MIN, -1)                   - Rules only, Op fields
 * -1                                               - Field not found for lookups
 * [0, config.maxDbField)                           - Normal fields
 * [config.maxDbField, config.minInternalField)     - Unused fields
 * [config.minInternalField, ARKIME_FIELDS_MAX)     - Internal fields
 */

#define ARKIME_FIELD_NOT_FOUND  -1
#define ARKIME_FIELDS_MAX 1024

typedef struct arkime_config {
    gboolean  quitting;
    char     *configFile;
    char     *nodeName;
    char     *hostName;
    char    **pcapReadFiles;
    char    **pcapReadDirs;
    char    **pcapFileLists;
    gboolean  pcapReadOffline;
    gchar   **extraTags;
    gchar   **extraOps;
    ArkimeFieldOps_t ops;
    gchar     debug;
    gboolean  insecure;
    gboolean  dumpConfig;
    gboolean  regressionTests;
    gboolean  quiet;
    gboolean  dryRun;
    gboolean  noSPI;
    gboolean  copyPcap;
    gboolean  pcapRecursive;
    gboolean  noStats;
    gboolean  tests;
    gboolean  pcapMonitor;
    gboolean  pcapDelete;
    gboolean  pcapSkip;
    gboolean  pcapReprocess;
    gboolean  flushBetween;
    gboolean  noLoadTags;
    gboolean  trackESP;
    gboolean  noLockPcap;
    gint      pktsToRead;

    GHashTable *override;

    uint64_t  ipSavePcap[4];
    uint64_t  etherSavePcap[1024];

    enum ArkimeRotate rotate;

    int       writeMethod;

    HASH_VAR(s_, dontSaveTags, ArkimeStringHead_t, 11);
    ArkimeFieldInfo_t *fields[ARKIME_FIELDS_MAX];
    int                maxDbField;
    int                minInternalField;
    int                tagsStringField;

    int                numPlugins;

    GRegex  *offlineRegex;
    char     *prefix;
    char     *nodeClass;
    char     *elasticsearch;
    char    **interface;
    int       pcapDirPos;
    char    **pcapDir;
    char     *pcapDirTemplate;
    char     *bpf;
    char     *yara;
    char     *caTrustFile;
    char    **geoLite2ASN;
    char    **geoLite2Country;
    char     *rirFile;
    char     *ouiFile;
    char     *dropUser;
    char     *dropGroup;
    char    **pluginsDir;
    char    **parsersDir;

    char    **rootPlugins;
    char    **plugins;
    char    **smtpIpHeaders;

    double    maxFileSizeG;
    uint64_t  maxFileSizeB;
    uint32_t  maxFileTimeM;
    uint32_t  timeouts[SESSION_MAX];
    uint32_t  tcpSaveTimeout;
    uint32_t  maxStreams[SESSION_MAX];
    uint32_t  maxPackets;
    uint32_t  maxPacketsInQueue;
    uint32_t  dbBulkSize;
    uint32_t  dbFlushTimeout;
    uint32_t  maxESConns;
    uint32_t  maxESRequests;
    uint32_t  logEveryXPackets;
    uint32_t  pcapBufferSize;
    uint32_t  pcapWriteSize;
    uint32_t  maxWriteBuffers;
    uint32_t  fragsTimeout;
    uint32_t  maxFrags;
    uint32_t  snapLen;
    uint32_t  maxMemPercentage;
    uint32_t  maxReqBody;
    int       packetThreads;

    char      logUnknownProtocols;
    char      logESRequests;
    char      logFileCreation;
    char      logHTTPConnections;
    char      parseSMTP;
    char      parseSMTPHeaderAll;
    char      parseSMB;
    char      ja3Strings;
    char      parseQSValue;
    char      parseCookieValue;
    char      parseHTTPHeaderRequestAll;
    char      parseHTTPHeaderResponseAll;
    char      supportSha256;
    char      reqBodyOnlyUtf8;
    char      compressES;
    char      readTruncatedPackets;
    char      yaraEveryPacket;
    char     *pcapDirAlgorithm;
    char      corruptSavePcap;
    char      autoGenerateId;
    char      ignoreErrors;
    char      enablePacketLen;
    char      gapPacketPos;
    char      enablePacketDedup;
    char      sessionIdMode;
    char     *provider;
    char     *profile;
    char     *commandSocket;
    char      commandWait;
    char      noRefresh;
    char    **commandList;
    char      noConfigOption;
} ArkimeConfig_t;

typedef struct {
    ArkimeFieldOps_t *ops;
    char             *tagsStr[10];
    char             *country;
    char             *asStr;
    char             *rir;
    uint32_t          asNum;
    char              asLen;
    char              numtags;
} ArkimeIpInfo_t;

/******************************************************************************/
/*
 * Parser
 */

#define ARKIME_PARSER_UNREGISTER -1
typedef int  (* ArkimeParserFunc) (struct arkime_session *session, void *uw, const uint8_t *data, int remaining, int which);
typedef void (* ArkimeParserFreeFunc) (struct arkime_session *session, void *uw);
typedef void (* ArkimeParserSaveFunc) (struct arkime_session *session, void *uw, int final);

typedef struct {
    ArkimeParserFunc      parserFunc;
    void                 *uw;
    ArkimeParserFreeFunc  parserFreeFunc;
    ArkimeParserSaveFunc  parserSaveFunc;

} ArkimeParserInfo_t;

/******************************************************************************/
struct arkime_pcap_timeval {
    uint32_t tv_sec;		   /* seconds */
    uint32_t tv_usec;	   	   /* microseconds */
};
struct arkime_pcap_sf_pkthdr {
    struct arkime_pcap_timeval ts; /* time stamp */
    uint32_t caplen;		   /* length of portion present */
    uint32_t pktlen;		   /* length this packet (off wire) */
};

/******************************************************************************/
#define ARKIME_PACKET_TUNNEL_GRE        0x01
#define ARKIME_PACKET_TUNNEL_PPPOE      0x02
#define ARKIME_PACKET_TUNNEL_MPLS       0x04
#define ARKIME_PACKET_TUNNEL_PPP        0x08
#define ARKIME_PACKET_TUNNEL_GTP        0x10
#define ARKIME_PACKET_TUNNEL_VXLAN      0x20
#define ARKIME_PACKET_TUNNEL_VXLAN_GPE  0x40
#define ARKIME_PACKET_TUNNEL_GENEVE     0x80
// Increase tunnel size below

typedef struct arkimepacket_t {
    struct arkimepacket_t   *packet_next, *packet_prev;
    struct timeval ts;                  // timestamp
    uint8_t       *pkt;                 // full packet
    uint64_t       writerFilePos;       // where in output file
    uint64_t       readerFilePos;       // where in input file
    uint32_t       writerFileNum;       // file number in db
    uint32_t       hash;                // Saved hash
    uint16_t       pktlen;              // length of packet
    uint16_t       payloadLen;          // length of ip payload
    uint16_t       payloadOffset;       // offset to ip payload from start
    uint16_t       vlan;                // non zero if the reader gets the vlan
    uint8_t        ipProtocol;          // ip protocol
    uint8_t        mProtocol;           // arkime protocol
    uint8_t        readerPos;           // offline - offlineInfo, online - which interface
    uint32_t       etherOffset: 11;     // offset to current ethernet frame from start
    uint32_t       outerEtherOffset: 11; // offset to previous ethernet frame from start
    uint32_t       tunnel: 8;           // tunnel type
    uint32_t       direction: 1;        // direction of packet
    uint32_t       v6: 1;               // v6 or not
    uint32_t       outerv6: 1;          // outer v6 or not
    uint32_t       copied: 1;           // don't need to copy
    uint32_t       wasfrag: 1;          // was a fragment
    uint32_t       ipOffset: 11;        // offset to ip header from start
    uint32_t       outerIpOffset: 11;   // offset to outer ip header from start
    uint32_t       vni: 24;             // vxlan id
} ArkimePacket_t;

typedef struct {
    struct arkimepacket_t   *packet_next, *packet_prev;
    uint32_t                 packet_count;
    ARKIME_LOCK_EXTERN(lock);
    ARKIME_COND_EXTERN(lock);
} ArkimePacketHead_t;

typedef struct {
    ArkimePacketHead_t    packetQ[ARKIME_MAX_PACKET_THREADS];
    int                   count;
    uint8_t               readerPos; // used by libpcap reader to set readerPos
} ArkimePacketBatch_t;

typedef struct {
    char       *filename;
    uint32_t    outputId;
    uint64_t    size;
    char       *scheme;
    char       *extra;
    uint8_t     didBatch;
} ArkimeOfflineInfo_t;
/******************************************************************************/
typedef struct arkime_tcp_data {
    struct arkime_tcp_data *td_next, *td_prev;

    ArkimePacket_t *packet;
    uint32_t        seq;
    uint32_t        ack;
    uint16_t        len;
    uint16_t        dataOffset;
} ArkimeTcpData_t;

typedef struct {
    struct arkime_tcp_data *td_next, *td_prev;
    int td_count;
} ArkimeTcpDataHead_t;

#define ARKIME_TCP_STATE_FIN     1
#define ARKIME_TCP_STATE_FIN_ACK 2

/******************************************************************************/
typedef enum {
    ARKIME_TCPFLAG_SYN = 0,
    ARKIME_TCPFLAG_SYN_ACK,
    ARKIME_TCPFLAG_ACK,
    ARKIME_TCPFLAG_PSH,
    ARKIME_TCPFLAG_FIN,
    ARKIME_TCPFLAG_RST,
    ARKIME_TCPFLAG_URG,
    ARKIME_TCPFLAG_SRC_ZERO,
    ARKIME_TCPFLAG_DST_ZERO,
    ARKIME_TCPFLAG_MAX
} ArkimeSesTcpFlags;
/******************************************************************************/
/*
 * SPI Data Storage
 */
typedef struct arkime_session {
    struct arkime_session *tcp_next, *tcp_prev;
    struct arkime_session *q_next, *q_prev;
    struct arkime_session *h_next, *h_prev;
    int                    h_bucket;
    uint32_t               h_hash;

    uint8_t                sessionId[ARKIME_SESSIONID_LEN];

    ArkimeField_t        **fields;

    void                  **pluginData;

    ArkimeParserInfo_t    *parserInfo;

    ArkimeTcpDataHead_t   tcpData;
    uint32_t              tcpSeq[2];
    char                  tcpState[2];

    GArray                *filePosArray;
    GArray                *fileLenArray;
    GArray                *fileNumArray;
    char                  *rootId;

    struct timeval         firstPacket;
    struct timeval         lastPacket;
    struct in6_addr        addr1;
    struct in6_addr        addr2;
    char                   firstBytes[2][8];

    uint64_t               bytes[2];
    uint64_t               databytes[2];
    uint64_t               totalDatabytes[2];

    uint32_t               lastFileNum;
    uint32_t               saveTime;
    uint32_t               packets[2];
    uint32_t               synTime;
    uint32_t               ackTime;
    uint32_t               synSeq[2];

    uint16_t               port1;
    uint16_t               port2;
    uint16_t               outstandingQueries;
    uint16_t               segments;
    uint16_t               stopSaving;
    uint16_t               tcpFlagCnt[ARKIME_TCPFLAG_MAX];
    union {
        uint8_t                tcpFlagAckCnt[2];
        uint8_t                icmpInfo[2];
    };
    uint16_t               maxFields;
    uint16_t               ethertype;

    uint8_t                consumed[2];
    uint8_t                ipProtocol;
    uint8_t                mProtocol;
    uint8_t                firstBytesLen[2];
    uint8_t                ip_tos;
    uint8_t                tcp_flags;
    uint8_t                parserLen;
    uint8_t                parserNum;
    uint8_t                minSaving;
    uint8_t                thread;

    uint16_t               haveTcpSession: 1;
    uint16_t               needSave: 1;
    uint16_t               stopSPI: 1;
    uint16_t               closingQ: 1;
    uint16_t               stopTCP: 1;
    SessionTypes           ses: 3;
    uint16_t               midSave: 1;
    uint16_t               outOfOrder: 2;
    uint16_t               ackedUnseenSegment: 2;
    uint16_t               stopYara: 1;
    uint16_t               diskOverload: 1;
    uint16_t               pq: 1;
    uint16_t               synSet: 2;
    uint16_t               inStoppedSave: 1;
} ArkimeSession_t;

typedef struct arkime_session_head {
    struct arkime_session *tcp_next, *tcp_prev;
    struct arkime_session *q_next, *q_prev;
    struct arkime_session *h_next, *h_prev;
    int                    h_bucket;
    int                    tcp_count;
    int                    q_count;
    int                    h_count;
} ArkimeSessionHead_t;

typedef struct {
    char                  *id;
    char                  *key;
    char                  *token;
} ArkimeCredentials_t;


#define ARKIME_TYPE_ALLOC(type) (type *)(malloc(sizeof(type)))
#define ARKIME_TYPE_ALLOC0(type) (type *)(calloc(1, sizeof(type)))
#define ARKIME_TYPE_FREE(type,mem) free(mem)

#define ARKIME_SIZE_ALLOC(name, s)  malloc(s)
#define ARKIME_SIZE_ALLOC0(name, s) calloc(s, 1)
#define ARKIME_SIZE_FREE(name, mem) free(mem)

// pcap_file_header
typedef struct {
    uint32_t magic;
    uint16_t version_major;
    uint16_t version_minor;
    int32_t  thiszone;	/* gmt to local correction */
    uint32_t sigfigs;	/* accuracy of timestamps */
    uint32_t snaplen;	/* max length saved portion of each pkt */
    uint32_t dlt;	/* data link type - see https://github.com/arkime/arkime/issues/1303#issuecomment-554684749 */
} ArkimePcapFileHdr_t;

#ifndef likely
#define likely(x)       __builtin_expect((x),1)
#endif

#ifndef unlikely
#define unlikely(x)     __builtin_expect((x),0)
#endif

/******************************************************************************/
/*
 * Callback cb definitions
 */
typedef int (*ArkimeWatchFd_func)(gint fd, GIOCondition cond, gpointer data);

typedef void (*ArkimeHttpResponse_cb)(int code, uint8_t *data, int len, gpointer uw);
typedef int (*ArkimeHttpRead_cb)(uint8_t *data, int len, gpointer uw);

typedef void (*ArkimeTag_cb)(void *uw, int tagType, const char *tagName, uint32_t tagValue, gboolean async);

typedef void (*ArkimeSeqNum_cb)(uint32_t seq, gpointer uw);

/******************************************************************************/
extern ARKIME_LOCK_EXTERN(LOG);
#define LOG(...) do { \
    if(config.quiet == FALSE) { \
        ARKIME_LOCK(LOG); \
        time_t _t = time(NULL); \
        char   _b[26]; \
        printf("%15.15s %s:%d %s(): ",\
            ctime_r(&_t, _b) + 4, __FILE__,\
            __LINE__, __FUNCTION__); \
        printf(__VA_ARGS__); \
        printf("\n"); \
        fflush(stdout); \
        ARKIME_UNLOCK(LOG); \
    } \
} while(0) /* no trailing ; */

#define LOGEXIT(...) do { config.quiet = FALSE; LOG(__VA_ARGS__); exit(1); } while(0) /* no trailing ; */
#define CONFIGEXIT(...) do { printf("vvvvvvvvvvvvvvvvvvvvvvvvv IMPORTANT vvvvvvvvvvvvvvvvvvvvvvvvv\n"); printf("FATAL CONFIG ERROR - " __VA_ARGS__); printf("\n^^^^^^^^^^^^^^^^^^^^^^^^^ IMPORTANT ^^^^^^^^^^^^^^^^^^^^^^^^^\n");exit(1); } while(0) /* no trailing ; */
#define REMOVEDCONFIG(_var,_help) do { if (arkime_config_str(NULL, _var, NULL) != NULL) CONFIGEXIT("Setting '" _var "' removed - " _help); } while (0) /* no trailing ; */

#define LOG_RATE(rate, ...) do { \
    struct timespec ts_current; \
    clock_gettime(CLOCK_MONOTONIC, &ts_current); \
    static time_t last_time = 0; \
    if (config.debug > 1 || ts_current.tv_sec - last_time >= rate) { \
        LOG(__VA_ARGS__); \
        last_time = ts_current.tv_sec; \
    } \
} while (0) /* no trailing ; */


/******************************************************************************/
/* Simple bit macros, assuming uint64_t */

#define BIT_ISSET(bit, bits) ((bits[bit/64] & (1ULL << (bit & 0x3f))) != 0)
#define BIT_SET(bit, bits) bits[bit/64] |= (1ULL << (bit & 0x3f))
#define BIT_CLR(bit, bits) bits[bit/64] &= ~(1ULL << (bit & 0x3f))

/******************************************************************************/
/*
 * main.c
 */

// Return 0 if ready to quit
typedef int  (* ArkimeCanQuitFunc) ();

#define ARKIME_GIO_READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL)
#define ARKIME_GIO_WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

gint arkime_watch_fd(gint fd, GIOCondition cond, ArkimeWatchFd_func func, gpointer data);
const uint8_t *arkime_js0n_get(const uint8_t *data, uint32_t len, const char *key, uint32_t *olen);
const uint8_t *arkime_js0n_get_path(const uint8_t *data, uint32_t len, const char **keys, uint32_t *olen);
char *arkime_js0n_get_str(const uint8_t *data, uint32_t len, const char *key);

gboolean arkime_string_add(void *hashv, char *string, gpointer uw, gboolean copy);

uint32_t arkime_string_hash(const void *key);
uint32_t arkime_string_hash_len(const void *key, int len);
int arkime_string_cmp(const void *keyv, const void *elementv);
int arkime_string_ncmp(const void *keyv, const void *elementv);


uint32_t arkime_int_hash(const void *key);
int arkime_int_cmp(const void *keyv, const void *elementv);

const char *arkime_memstr(const char *haystack, int haysize, const char *needle, int needlesize);
const char *arkime_memcasestr(const char *haystack, int haysize, const char *needle, int needlesize);

void arkime_free_later(void *ptr, GDestroyNotify cb);

void arkime_add_can_quit(ArkimeCanQuitFunc func, const char *name);

void arkime_quit();

uint32_t arkime_get_next_prime(uint32_t v);
uint32_t arkime_get_next_powerof2(uint32_t v);
void arkime_check_file_permissions(const char *filename);

typedef void (*ArkimeCredentialsGet)(const char *service);
void arkime_credentials_register(const char *name, ArkimeCredentialsGet func);
void arkime_credentials_set(const char *id, const char *key, const char *token);
ArkimeCredentials_t *arkime_credentials_get(const char *service, const char *idName, const char *keyName);

/******************************************************************************/
/*
 * cloud.c
 */
void arkime_cloud_init();

/******************************************************************************/
/*
 * config.c
 */

void arkime_config_init();
void arkime_config_load_override_ips();
void arkime_config_load_packet_ips();
void arkime_config_add_header(ArkimeStringHashStd_t *hash, char *key, int pos);
void arkime_config_load_header(char *section, char *group, char *helpBase, char *expBase, char *aliasBase, char *dbBase, ArkimeStringHashStd_t *hash, int flags);
void arkime_config_exit();

gchar **arkime_config_section_raw_str_list(GKeyFile *keyfile, const char *section, const char *key, const char *d);
gchar **arkime_config_section_str_list(GKeyFile *keyfile, const char *section, const char *key, const char *d);
gchar *arkime_config_section_str(GKeyFile *keyfile, const char *section, const char *key, const char *d);
gchar **arkime_config_section_keys(GKeyFile *keyfile, const char *section, gsize *keys_len);

gchar *arkime_config_str(GKeyFile *keyfile, const char *key, const char *d);
gchar **arkime_config_str_list(GKeyFile *keyfile, const char *key, const char *d);
gchar **arkime_config_raw_str_list(GKeyFile *keyfile, const char *key, const char *d);
uint32_t arkime_config_int(GKeyFile *keyfile, const char *key, uint32_t d, uint32_t min, uint32_t max);
char arkime_config_boolean(GKeyFile *keyfile, const char *key, char d);

typedef void (*ArkimeFileChange_cb)(const char *name);
typedef void (*ArkimeFilesChange_cb)(char **names);
void arkime_config_monitor_file_msg(const char *desc, char *name, ArkimeFileChange_cb cb, const char *msg);
void arkime_config_monitor_file(const char *desc, char *name, ArkimeFileChange_cb cb);
void arkime_config_monitor_files(const char *desc, char **names, ArkimeFilesChange_cb cb);

#define ARKIME_CONFIG_CMD_VAR_STR_PTR 16
void arkime_config_register_cmd_var(const char *name, void *var, size_t typelen);

void arkime_config_check(const char *prefix, ...);

/******************************************************************************/
/*
 * command.c
 */

typedef void (* ArkimeCommandFunc) (int argc, char **argv, gpointer cc);

void arkime_command_init();
void arkime_command_start();
void arkime_command_register(const char *name, ArkimeCommandFunc func, const char *help);
void arkime_command_register_opts(const char *name, ArkimeCommandFunc func, const char *help, ...);
void arkime_command_respond(gpointer cc, const char *data, int len);

/******************************************************************************/
/*
 * db.c
 */
void     arkime_db_init();
char    *arkime_db_create_file_full(const struct timeval *firstPacket, const char *name, uint64_t size, int locked, uint32_t *id, ...);
void     arkime_db_save_session(ArkimeSession_t *session, int final);
void     arkime_db_add_override_ip(char *str, ArkimeIpInfo_t *ii);
void     arkime_db_install_override_ip();
void     arkime_db_add_field(const char *group, const char *kind, const char *expression, const char *friendlyName, const char *dbField, const char *help, int haveap, va_list ap);
void     arkime_db_delete_field(const char *expression);
void     arkime_db_update_field(const char *expression, const char *name, const char *value);
void     arkime_db_update_file(uint32_t fileid, uint64_t filesize, uint64_t packetsSize, uint32_t packets, const struct timeval *lastPacket);
gboolean arkime_db_file_exists(const char *filename, uint32_t *outputId);
void     arkime_db_exit();
void     arkime_db_oui_lookup(int field, ArkimeSession_t *session, const uint8_t *mac);
void     arkime_db_geo_lookup6(ArkimeSession_t *session, struct in6_addr addr, char **g, uint32_t *asNum, char **asStr, int *asLen, char **rir);
gchar   *arkime_db_community_id(const ArkimeSession_t *session);
gchar   *arkime_db_community_id_icmp(const ArkimeSession_t *session);
void     arkime_db_js0n_str(BSB *bsb, uint8_t *in, gboolean utf8);
void     arkime_db_js0n_str_unquoted(BSB *bsb, uint8_t *in, int len, gboolean utf8);
void     arkime_db_memory_info(int refresh, uint64_t *memBytes, float *memPercent);


// Replace how SPI data is sent to ES.
// The implementation must either call a arkime_http_free_buffer or another arkime_http routine that frees the buffer
typedef void (* ArkimeDbSendBulkFunc) (char *json, int len);
// bulkHeader - include the bulk header
// indexInDoc - add sessionIndex field to doc where arkime would index doc
// maxDocs - max docs per call
void     arkime_db_set_send_bulk2(ArkimeDbSendBulkFunc func, gboolean bulkHeader, gboolean indexInDoc, uint16_t maxDocs);
void     arkime_db_set_send_bulk(ArkimeDbSendBulkFunc func);

/******************************************************************************/
/*
 * dedup.c
 */

void arkime_dedup_init();
void arkime_dedup_exit();
int arkime_dedup_should_drop(const ArkimePacket_t *packet, int headerLen);

/******************************************************************************/
/*
 * drophash.c
 */

typedef struct arkimedrophashitem_t  ArkimeDropHashItem_t;
typedef struct arkimedrophash_t      ArkimeDropHash_t;
typedef struct arkimedrophashgroup_t ArkimeDropHashGroup_t;
struct arkimedrophashgroup_t {
    ArkimeDropHashItem_t *dhg_next, *dhg_prev;
    int                   dhg_count;
    int                   changed;
    char                 *file;
    char                  keyLen;
    ArkimeDropHash_t     *drops[0x10000];
    ARKIME_LOCK_EXTERN(lock);
};


void arkime_drophash_init(ArkimeDropHashGroup_t *group, const char *file, int keyLen);
int arkime_drophash_add (ArkimeDropHashGroup_t *group, int port, const void *key, uint32_t current, uint32_t goodFor);
int arkime_drophash_should_drop (ArkimeDropHashGroup_t *group, int port, const void *key, uint32_t current);
void arkime_drophash_delete (ArkimeDropHashGroup_t *group, int port, const void *key);
void arkime_drophash_save(ArkimeDropHashGroup_t *group);

/******************************************************************************/
/*
 * parsers.c
 */
typedef struct {
    uint32_t pc, tag, len;
    const uint8_t *value;
} ArkimeASNSeq_t;

#define ARKIME_PARSERS_HAS_NAMED_FUNC(id) (arkime_parsers_has_named_func & (1ULL << id))
extern uint64_t arkime_parsers_has_named_func;
void arkime_parsers_init();
void arkime_parsers_initial_tag(ArkimeSession_t *session);
uint8_t *arkime_parsers_asn_get_tlv(BSB *bsb, uint32_t *apc, uint32_t *atag, uint32_t *alen);
int arkime_parsers_asn_get_sequence(ArkimeASNSeq_t *seqs, int maxSeq, const uint8_t *data, int len, gboolean wrapper);
const char *arkime_parsers_asn_sequence_to_string(ArkimeASNSeq_t *seq, int *len);
void arkime_parsers_asn_decode_oid(char *buf, int bufsz, const uint8_t *oid, int len);
uint64_t arkime_parsers_asn_parse_time(ArkimeSession_t *session, int tag, uint8_t *value, int len);
void arkime_parsers_classify_tcp(ArkimeSession_t *session, const uint8_t *data, int remaining, int which);
void arkime_parsers_classify_udp(ArkimeSession_t *session, const uint8_t *data, int remaining, int which);
void arkime_parsers_exit();

const char *arkime_parsers_magic(ArkimeSession_t *session, int field, const char *data, int len);

typedef void (* ArkimeClassifyFunc) (ArkimeSession_t *session, const uint8_t *data, int remaining, int which, void *uw);

void  arkime_parsers_unregister(ArkimeSession_t *session, void *uw);
void  arkime_parsers_register2(ArkimeSession_t *session, ArkimeParserFunc func, void *uw, ArkimeParserFreeFunc ffunc, ArkimeParserSaveFunc sfunc);
#define arkime_parsers_register(session, func, uw, ffunc) arkime_parsers_register2(session, func, uw, ffunc, NULL)

void  arkime_parsers_classifier_register_tcp_internal(const char *name, void *uw, int offset, const uint8_t *match, int matchlen, ArkimeClassifyFunc func, size_t sessionsize, int apiversion);
#define arkime_parsers_classifier_register_tcp(name, uw, offset, match, matchlen, func) arkime_parsers_classifier_register_tcp_internal(name, uw, offset, match, matchlen, func, sizeof(ArkimeSession_t), ARKIME_API_VERSION)

void  arkime_parsers_classifier_register_udp_internal(const char *name, void *uw, int offset, const uint8_t *match, int matchlen, ArkimeClassifyFunc func, size_t sessionsize, int apiversion);
#define arkime_parsers_classifier_register_udp(name, uw, offset, match, matchlen, func) arkime_parsers_classifier_register_udp_internal(name, uw, offset, match, matchlen, func, sizeof(ArkimeSession_t), ARKIME_API_VERSION)

#define  ARKIME_PARSERS_PORT_UDP_SRC 0x01
#define  ARKIME_PARSERS_PORT_UDP_DST 0x02
#define  ARKIME_PARSERS_PORT_UDP     ARKIME_PARSERS_PORT_UDP_SRC | ARKIME_PARSERS_PORT_UDP_DST
#define  ARKIME_PARSERS_PORT_TCP_SRC 0x04
#define  ARKIME_PARSERS_PORT_TCP_DST 0x08
#define  ARKIME_PARSERS_PORT_TCP     ARKIME_PARSERS_PORT_TCP_SRC | ARKIME_PARSERS_PORT_TCP_DST

void  arkime_parsers_classifier_register_port_internal(const char *name, void *uw, uint16_t port, uint32_t type, ArkimeClassifyFunc func, size_t sessionsize, int apiversion);
#define arkime_parsers_classifier_register_port(name, uw, port, type, func) arkime_parsers_classifier_register_port_internal(name, uw, port, type, func, sizeof(ArkimeSession_t), ARKIME_API_VERSION)

void  arkime_print_hex_string(const uint8_t *data, unsigned int length);
char *arkime_sprint_hex_string(char *buf, const uint8_t *data, unsigned int length);

#define STRLEN(str) (sizeof(str) - 1)

#define CLASSIFY_TCP(name, offset, bytes, cb) arkime_parsers_classifier_register_tcp(name, name, offset, (uint8_t *)bytes, sizeof(bytes) - 1, cb);
#define CLASSIFY_UDP(name, offset, bytes, cb) arkime_parsers_classifier_register_udp(name, name, offset, (uint8_t *)bytes, sizeof(bytes) - 1, cb);

typedef uint32_t (* ArkimeParsersNamedFunc) (ArkimeSession_t *session, const uint8_t *data, int len, void *uw);
uint32_t arkime_parsers_add_named_func(const char *name, ArkimeParsersNamedFunc func);
uint32_t arkime_parsers_get_named_func(const char *name);
void arkime_parsers_call_named_func(uint32_t id, ArkimeSession_t *session, const uint8_t *data, int len, void *uw);

/******************************************************************************/
/*
 * http.c
 */

typedef void (*ArkimeHttpHeader_cb)(char *url, const char *field, const char *value, int valueLen, gpointer uw);


#define ARKIME_HTTP_BUFFER_SIZE 10000

void arkime_http_init();

uint8_t *arkime_http_send_sync(void *serverV, const char *method, const char *key, int32_t key_len, char *data, uint32_t data_len, char **headers, size_t *return_len, int *code);
gboolean arkime_http_send(void *serverV, const char *method, const char *key, int32_t key_len, char *data, uint32_t data_len, char **headers, gboolean dropable, ArkimeHttpResponse_cb func, gpointer uw);

#define ARKIME_HTTP_PRIORITY_BEST      0
#define ARKIME_HTTP_PRIORITY_NORMAL    1
#define ARKIME_HTTP_PRIORITY_DROPABLE  2
gboolean arkime_http_schedule(void *serverV, const char *method, const char *key, int32_t key_len, char *data, uint32_t data_len, char **headers, int priority, ArkimeHttpResponse_cb func, gpointer uw);

gboolean arkime_http_schedule2(void *serverV, const char *method, const char *key, int32_t key_len, char *data, uint32_t data_len, char **headers, int priority, ArkimeHttpResponse_cb func, ArkimeHttpRead_cb, gpointer uw);


uint8_t *arkime_http_get(void *server, const char *key, int key_len, size_t *mlen);
#define arkime_http_get_buffer(size) ARKIME_SIZE_ALLOC(buffer, size)
#define arkime_http_free_buffer(b) ARKIME_SIZE_FREE(buffer, b)
#define arkime_http_free_response(b) free(b)
void arkime_http_exit();
int arkime_http_queue_length(void *server);
int arkime_http_queue_length_best(void *server);
uint64_t arkime_http_dropped_count(void *server);

void *arkime_http_create_server(const char *hostnames, int maxConns, int maxOutstandingRequests, int compress);
void *arkime_http_get_or_create_server(const char *name, const char *hostnames, int maxConns, int maxOutstandingRequests, int compress, int *isNew);

void arkime_http_set_retries(void *server, uint16_t retries);
void arkime_http_set_timeout(void *serverV, uint64_t timeout);
void arkime_http_set_client_cert(void *serverV, char *clientCert, char *clientKey, char *clientKeyPass);
void arkime_http_set_print_errors(void *server);
void arkime_http_set_dont_free_response(void *server);
void arkime_http_set_headers(void *server, char **headers);
void arkime_http_set_header_cb(void *server, ArkimeHttpHeader_cb cb);
void arkime_http_set_userpwd(void *server, const char *userpwd);
void arkime_http_set_aws_sigv4(void *server, const char *aws_sigv4);
void arkime_http_free_server(void *server);

gboolean arkime_http_is_arkime(uint32_t hash, uint8_t *sessionId);

/******************************************************************************/
/*
 * session.c
 */


void     arkime_session_id (uint8_t *sessionId, uint32_t addr1, uint16_t port1, uint32_t addr2, uint16_t port2, uint16_t vlan, uint32_t vni);
void     arkime_session_id6 (uint8_t *sessionId, const uint8_t *addr1, uint16_t port1, const uint8_t *addr2, uint16_t port2, uint16_t vlan, uint32_t vni);
char    *arkime_session_id_string (const uint8_t *sessionId, char *buf);
char    *arkime_session_pretty_string (ArkimeSession_t *session, char *buf, int len);

uint32_t arkime_session_hash(const void *key);

ArkimeSession_t *arkime_session_find(int ses, uint8_t *sessionId);
ArkimeSession_t *arkime_session_find_or_create(int mProtocol, uint32_t hash, uint8_t *sessionId, int *isNew);

void     arkime_session_init();
void     arkime_session_exit();
void     arkime_session_add_protocol(ArkimeSession_t *session, const char *protocol);
gboolean arkime_session_has_protocol(ArkimeSession_t *session, const char *protocol);
void     arkime_session_add_tag(ArkimeSession_t *session, const char *tag);

#define  arkime_session_incr_outstanding(session) (session)->outstandingQueries++
gboolean arkime_session_decr_outstanding(ArkimeSession_t *session);

void     arkime_session_mark_for_close(ArkimeSession_t *session, SessionTypes ses);
void     arkime_session_flip_src_dst(ArkimeSession_t *session);

void     arkime_session_mid_save(ArkimeSession_t *session, uint32_t tv_sec);

int      arkime_session_watch_count(SessionTypes ses);
int      arkime_session_idle_seconds(SessionTypes ses);
int      arkime_session_close_outstanding();

void     arkime_session_flush();
void     arkime_session_flush_internal(int thread);
uint32_t arkime_session_monitoring();
void     arkime_session_process_commands(int thread);

int      arkime_session_need_save_outstanding();
int      arkime_session_cmd_outstanding();

typedef enum {
    ARKIME_SES_CMD_FUNC
} ArkimeSesCmd;
typedef void (*ArkimeCmd_func)(ArkimeSession_t *session, gpointer uw1, gpointer uw2);

void arkime_session_add_cmd(ArkimeSession_t *session, ArkimeSesCmd sesCmd, gpointer uw1, gpointer uw2, ArkimeCmd_func func);
void arkime_session_add_cmd_thread(int thread, gpointer uw1, gpointer uw2, ArkimeCmd_func func);

void arkime_session_set_stop_saving(ArkimeSession_t *session);
void arkime_session_set_stop_spi(ArkimeSession_t *session, int value);

/******************************************************************************/
/*
 * packet.c
 */
typedef enum {
    ARKIME_PACKET_DO_PROCESS,
    ARKIME_PACKET_IP_DROPPED,
    ARKIME_PACKET_OVERLOAD_DROPPED,
    ARKIME_PACKET_CORRUPT,
    ARKIME_PACKET_UNKNOWN,
    ARKIME_PACKET_IPPORT_DROPPED,
    ARKIME_PACKET_DONT_PROCESS,
    ARKIME_PACKET_DONT_PROCESS_OR_FREE,
    ARKIME_PACKET_DUPLICATE_DROPPED,
    ARKIME_PACKET_MAX
} ArkimePacketRC;

typedef ArkimePacketRC (*ArkimePacketEnqueue_cb)(ArkimePacketBatch_t *batch, ArkimePacket_t *const packet, const uint8_t *data, int len);

typedef int (*ArkimePacketSessionId_cb)(uint8_t *sessionId, ArkimePacket_t *const packet, const uint8_t *data, int len);

void     arkime_packet_init();
uint64_t arkime_packet_dropped_packets();
void     arkime_packet_exit();
void     arkime_packet_tcp_free(ArkimeSession_t *session);
int      arkime_packet_outstanding();
int      arkime_packet_frags_outstanding();
int      arkime_packet_frags_size();
uint64_t arkime_packet_dropped_frags();
uint64_t arkime_packet_dropped_overload();
uint64_t arkime_packet_total_bytes();
void     arkime_packet_thread_wake(int thread);
void     arkime_packet_flush();
void     arkime_packet_process_data(ArkimeSession_t *session, const uint8_t *data, int len, int which);
void     arkime_packet_add_packet_ip(char *ipstr, int mode);
void     arkime_packet_install_packet_ip();

void     arkime_packet_batch_init(ArkimePacketBatch_t *batch);
void     arkime_packet_batch_flush(ArkimePacketBatch_t *batch);
void     arkime_packet_batch(ArkimePacketBatch_t *batch, ArkimePacket_t *const packet);
void     arkime_packet_batch_process(ArkimePacketBatch_t *batch, ArkimePacket_t *const packet, int thread);

void     arkime_packet_set_dltsnap(int dlt, int snaplen);
uint32_t arkime_packet_dlt_to_linktype(int dlt);
void     arkime_packet_drophash_add(ArkimeSession_t *session, int which, int min);

void     arkime_packet_save_ethernet(ArkimePacket_t *const packet, uint16_t type);
ArkimePacketRC arkime_packet_run_ethernet_cb(ArkimePacketBatch_t *batch, ArkimePacket_t *const packet, const uint8_t *data, int len, uint16_t type, const char *str);
void     arkime_packet_set_ethernet_cb(uint16_t type, ArkimePacketEnqueue_cb enqueueCb);

ArkimePacketRC arkime_packet_run_ip_cb(ArkimePacketBatch_t *batch, ArkimePacket_t *const packet, const uint8_t *data, int len, uint16_t type, const char *str);
void     arkime_packet_set_ip_cb(uint16_t type, ArkimePacketEnqueue_cb enqueueCb);

void     arkime_packet_set_udpport_enqueue_cb(uint16_t port, ArkimePacketEnqueue_cb enqueueCb);


/******************************************************************************/
typedef void (*ArkimeProtocolCreateSessionId_cb)(uint8_t *sessionId, ArkimePacket_t *const packet);
typedef int  (*ArkimeProtocolPreProcess_cb)(ArkimeSession_t *session, ArkimePacket_t *const packet, int isNewSession);
typedef int  (*ArkimeProtocolProcess_cb)(ArkimeSession_t *session, ArkimePacket_t *const packet);
typedef void (*ArkimeProtocolSessionFree_cb)(ArkimeSession_t *session);

typedef struct {
    const char                       *name;
    int                               ses;
    ArkimeProtocolCreateSessionId_cb  createSessionId;
    ArkimeProtocolPreProcess_cb       preProcess;
    ArkimeProtocolProcess_cb          process;
    ArkimeProtocolSessionFree_cb      sFree;
} ArkimeProtocol_t;

int arkime_mprotocol_register_internal(const char                      *name,
                                       int                              ses,
                                       ArkimeProtocolCreateSessionId_cb createSessionId,
                                       ArkimeProtocolPreProcess_cb      preProcess,
                                       ArkimeProtocolProcess_cb         process,
                                       ArkimeProtocolSessionFree_cb     sFree,
                                       size_t                           sessionsize,
                                       int                              apiversion);
#define arkime_mprotocol_register(name, ses, createSessionId, preProcess, process, sFree) arkime_mprotocol_register_internal(name, ses, createSessionId, preProcess, process, sFree, sizeof(ArkimeSession_t), ARKIME_API_VERSION)

void arkime_mprotocol_init();


/******************************************************************************/
/*
 * plugins.c
 */
typedef void (* ArkimePluginInitFunc) ();
typedef void (* ArkimePluginIpFunc) (ArkimeSession_t *session, struct ip *packet, int len);
typedef void (* ArkimePluginUdpFunc) (ArkimeSession_t *session, const uint8_t *data, int len, int which);
typedef void (* ArkimePluginTcpFunc) (ArkimeSession_t *session, const uint8_t *data, int len, int which);
typedef void (* ArkimePluginSaveFunc) (ArkimeSession_t *session, int final);
typedef void (* ArkimePluginNewFunc) (ArkimeSession_t *session);
typedef void (* ArkimePluginExitFunc) ();
typedef void (* ArkimePluginReloadFunc) ();

typedef void (* ArkimePluginHttpDataFunc) (ArkimeSession_t *session, http_parser *hp, const char *at, size_t length);
typedef void (* ArkimePluginHttpFunc) (ArkimeSession_t *session, http_parser *hp);

typedef void (* ArkimePluginSMTPHeaderFunc) (ArkimeSession_t *session, const char *field, size_t field_len, const char *value, size_t value_len);
typedef void (* ArkimePluginSMTPFunc) (ArkimeSession_t *session);
typedef uint32_t (* ArkimePluginOutstandingFunc) ();

#define ARKIME_PLUGIN_SAVE         0x00000001
#define ARKIME_PLUGIN_IP           0x00000002
#define ARKIME_PLUGIN_UDP          0x00000004
#define ARKIME_PLUGIN_TCP          0x00000008
#define ARKIME_PLUGIN_EXIT         0x00000010
#define ARKIME_PLUGIN_NEW          0x00000020
#define ARKIME_PLUGIN_RELOAD       0x00000040
#define ARKIME_PLUGIN_PRE_SAVE     0x00000100

#define ARKIME_PLUGIN_HP_OMB       0x00001000
#define ARKIME_PLUGIN_HP_OU        0x00002000
#define ARKIME_PLUGIN_HP_OHF       0x00004000
#define ARKIME_PLUGIN_HP_OHV       0x00008000
#define ARKIME_PLUGIN_HP_OHC       0x00010000
#define ARKIME_PLUGIN_HP_OB        0x00020000
#define ARKIME_PLUGIN_HP_OMC       0x00040000
#define ARKIME_PLUGIN_HP_OHFR      0x00080000

#define ARKIME_PLUGIN_SMTP_OH      0x00100000
#define ARKIME_PLUGIN_SMTP_OHC     0x00200000

void arkime_plugins_init();
void arkime_plugins_load(char **plugins);
void arkime_plugins_reload();

int  arkime_plugins_register_internal(const char *name, gboolean storeData, size_t sessionsize, int apiversion);
#define arkime_plugins_register(name, storeData) arkime_plugins_register_internal(name, storeData, sizeof(ArkimeSession_t), ARKIME_API_VERSION)

void arkime_plugins_set_cb(const char             *name,
                           ArkimePluginIpFunc      ipFunc,
                           ArkimePluginUdpFunc     udpFunc,
                           ArkimePluginTcpFunc     tcpFunc,
                           ArkimePluginSaveFunc    preSaveFunc,
                           ArkimePluginSaveFunc    saveFunc,
                           ArkimePluginNewFunc     newFunc,
                           ArkimePluginExitFunc    exitFunc,
                           ArkimePluginExitFunc    reloadFunc);

void arkime_plugins_set_http_cb(const char              *name,
                                ArkimePluginHttpFunc     on_message_begin,
                                ArkimePluginHttpDataFunc on_url,
                                ArkimePluginHttpDataFunc on_header_field,
                                ArkimePluginHttpDataFunc on_header_value,
                                ArkimePluginHttpFunc     on_headers_complete,
                                ArkimePluginHttpDataFunc on_body,
                                ArkimePluginHttpFunc     on_message_complete);

void arkime_plugins_set_http_ext_cb(const char              *name,
                                    ArkimePluginHttpFunc     on_message_begin,
                                    ArkimePluginHttpDataFunc on_url,
                                    ArkimePluginHttpDataFunc on_header_field,
                                    ArkimePluginHttpDataFunc on_header_field_raw,
                                    ArkimePluginHttpDataFunc on_header_value,
                                    ArkimePluginHttpFunc     on_headers_complete,
                                    ArkimePluginHttpDataFunc on_body,
                                    ArkimePluginHttpFunc     on_message_complete);

void arkime_plugins_set_smtp_cb(const char                 *name,
                                ArkimePluginSMTPHeaderFunc  on_header,
                                ArkimePluginSMTPFunc        on_header_complete);

void arkime_plugins_set_outstanding_cb(const char                 *name,
                                       ArkimePluginOutstandingFunc outstandingFunc);

void arkime_plugins_cb_pre_save(ArkimeSession_t *session, int final);
void arkime_plugins_cb_save(ArkimeSession_t *session, int final);
void arkime_plugins_cb_new(ArkimeSession_t *session);
//void arkime_plugins_cb_ip(ArkimeSession_t *session, struct ip *packet, int len);
void arkime_plugins_cb_udp(ArkimeSession_t *session, const uint8_t *data, int len, int which);
void arkime_plugins_cb_tcp(ArkimeSession_t *session, const uint8_t *data, int len, int which);

void arkime_plugins_cb_hp_omb(ArkimeSession_t *session, http_parser *parser);
void arkime_plugins_cb_hp_ou(ArkimeSession_t *session, http_parser *parser, const char *at, size_t length);
void arkime_plugins_cb_hp_ohf(ArkimeSession_t *session, http_parser *parser, const char *at, size_t length);
void arkime_plugins_cb_hp_ohfr(ArkimeSession_t *session, http_parser *parser, const char *at, size_t length);
void arkime_plugins_cb_hp_ohv(ArkimeSession_t *session, http_parser *parser, const char *at, size_t length);
void arkime_plugins_cb_hp_ohc(ArkimeSession_t *session, http_parser *parser);
void arkime_plugins_cb_hp_ob(ArkimeSession_t *session, http_parser *parser, const char *at, size_t length);
void arkime_plugins_cb_hp_omc(ArkimeSession_t *session, http_parser *parser);

void arkime_plugins_cb_smtp_oh(ArkimeSession_t *session, const char *field, size_t field_len, const char *value, size_t value_len);
void arkime_plugins_cb_smtp_ohc(ArkimeSession_t *session);

void arkime_plugins_exit();

/******************************************************************************/
/*
 * yara.c
 */
void  arkime_yara_init();
void  arkime_yara_execute(ArkimeSession_t *session, const uint8_t *data, int len, int first);
void  arkime_yara_email_execute(ArkimeSession_t *session, const uint8_t *data, int len, int first);
void  arkime_yara_exit();
char *arkime_yara_version();

/******************************************************************************/
/*
 * field.c
 */

void arkime_field_init();
void arkime_field_define_json(const uint8_t *expression, int expression_len, const uint8_t *data, int data_len);
int  arkime_field_define_text(const char *text, int *shortcut);
int  arkime_field_define_text_full(char *field, const char *text, int *shortcut);
int  arkime_field_define(const char *group, const char *kind, const char *expression, const char *friendlyName, const char *dbField, const char *help, ArkimeFieldType type, int flags, ...);

int  arkime_field_by_db(const char *dbField);
int  arkime_field_by_exp(const char *exp);
int  arkime_field_by_exp_ignore_error(const char *exp);
void arkime_field_by_exp_add_special(const char *exp, int pos);
const char *arkime_field_string_add(int pos, ArkimeSession_t *session, const char *string, int len, gboolean copy);
gboolean arkime_field_string_add_lower(int pos, ArkimeSession_t *session, const char *string, int len);
gboolean arkime_field_string_add_host(int pos, ArkimeSession_t *session, char *string, int len);
const char *arkime_field_string_uw_add(int pos, ArkimeSession_t *session, const char *string, int len, gpointer uw, gboolean copy);
gboolean arkime_field_int_add(int pos, ArkimeSession_t *session, int i);
gboolean arkime_field_ip4_add(int pos, ArkimeSession_t *session, uint32_t i);
gboolean arkime_field_ip6_add(int pos, ArkimeSession_t *session, const uint8_t *val);
gboolean arkime_field_ip_add_str(int pos, ArkimeSession_t *session, const char *str);
gboolean arkime_field_float_add(int pos, ArkimeSession_t *session, float f);
void arkime_field_macoui_add(ArkimeSession_t *session, int macField, int ouiField, const uint8_t *mac);

int  arkime_field_count(int pos, ArkimeSession_t *session);
void arkime_field_certsinfo_update_extra (void *cert, char *key, char *value);
GPtrArray *arkime_field_certsinfo_get_extra(const ArkimeSession_t *session, const char *key);
void arkime_field_free(ArkimeSession_t *session);
void arkime_field_exit();

int arkime_field_by_exp_add_internal(const char *exp, ArkimeFieldType type, ArkimeFieldGetFunc getCb, ArkimeFieldSetFunc setCb);

void arkime_field_ops_init(ArkimeFieldOps_t *ops, int numOps, uint16_t flags);
void arkime_field_ops_free(ArkimeFieldOps_t *ops);
char *arkime_field_ops_parse(ArkimeFieldOps_t *ops, uint16_t flags, gchar **strs);
void arkime_field_ops_add(ArkimeFieldOps_t *ops, int fieldPos, char *value, int valuelen);
void arkime_field_ops_add_match(ArkimeFieldOps_t *ops, int fieldPos, char *value, int valuelen, int matchPos);
void arkime_field_ops_run(ArkimeSession_t *session, ArkimeFieldOps_t *ops);
void arkime_field_ops_run_match(ArkimeSession_t *session, ArkimeFieldOps_t *ops, int matchPos);

void *arkime_field_parse_ip(const char *str);
gboolean arkime_field_ip_equal (gconstpointer v1, gconstpointer v2);
guint arkime_field_ip_hash (gconstpointer v);

int arkime_field_object_register(const char *name, const char *help, ArkimeFieldObjectSaveFunc save, ArkimeFieldObjectFreeFunc free, ArkimeFieldObjectHashFunc hash, ArkimeFieldObjectCmpFunc cmp);
gboolean arkime_field_object_add(int pos, ArkimeSession_t *session, ArkimeFieldObject_t *object, int len);

/******************************************************************************/
/*
 * writers.c
 */

typedef void (*ArkimeWriterInit)(const char *name);
typedef uint32_t (*ArkimeWriterQueueLength)();
typedef void (*ArkimeWriterWrite)(const ArkimeSession_t *const session, ArkimePacket_t *const packet);
typedef void (*ArkimeWriterExit)();
typedef void (*ArkimeWriterIndex)(ArkimeSession_t *session);

extern ArkimeWriterQueueLength arkime_writer_queue_length;
extern ArkimeWriterWrite arkime_writer_write;
extern ArkimeWriterExit arkime_writer_exit;
extern ArkimeWriterIndex arkime_writer_index;


void arkime_writers_init();
void arkime_writers_start(char *name);
void arkime_writers_add(char *name, ArkimeWriterInit func);

/******************************************************************************/
/*
 * readers.c
 */

typedef struct {
    uint64_t total;
    uint64_t dropped;
} ArkimeReaderStats_t;

typedef struct {
    GRegex    *regex;
    int        field;
    char      *expand;
} ArkimeFilenameOps_t;

typedef void (*ArkimeReaderInit)(const char *name);
typedef int  (*ArkimeReaderStats)(ArkimeReaderStats_t *stats);
typedef void (*ArkimeReaderStart)();
typedef void (*ArkimeReaderStop)();
typedef void (*ArkimeReaderExit)();

extern ArkimeReaderStart arkime_reader_start;
extern ArkimeReaderStats arkime_reader_stats;
extern ArkimeReaderStop  arkime_reader_stop;
extern ArkimeReaderExit  arkime_reader_exit;

void arkime_readers_init();
void arkime_readers_set(char *name);
void arkime_readers_start();
void arkime_readers_add(char *name, ArkimeReaderInit func);
void arkime_readers_exit();

/******************************************************************************/
/*
 * reader-scheme.c
 */

typedef enum {
    ARKIME_SCHEME_FLAG_NONE      = 0x0000,
    ARKIME_SCHEME_FLAG_DIRHINT   = 0x0001,
    ARKIME_SCHEME_FLAG_MONITOR   = 0x0002,
    ARKIME_SCHEME_FLAG_RECURSIVE = 0x0004,
    ARKIME_SCHEME_FLAG_SKIP      = 0x0008,
    ARKIME_SCHEME_FLAG_DELETE    = 0x0010
} ArkimeSchemeFlags;

typedef struct {
    int refs;
    ArkimeFieldOps_t ops;
} ArkimeSchemeAction_t;

typedef int  (*ArkimeSchemeLoad)(const char *uri, ArkimeSchemeFlags flags, ArkimeSchemeAction_t *actions);
typedef void (*ArkimeSchemeExit)();

void arkime_reader_scheme_register(char *name, ArkimeSchemeLoad load, ArkimeSchemeExit exit);
int arkime_reader_scheme_process(const char *uri, uint8_t *data, int len, const char *extraInfo, ArkimeSchemeAction_t *actions);
void arkime_reader_scheme_load(const char *uri, ArkimeSchemeFlags flags, ArkimeSchemeAction_t *actions);

/******************************************************************************/
/*
 * rules.c
 */
typedef enum {
    ARKIME_RULE_TYPE_EVERY_PACKET,
    ARKIME_RULE_TYPE_SESSION_SETUP,
    ARKIME_RULE_TYPE_AFTER_CLASSIFY,
    ARKIME_RULE_TYPE_FIELD_SET,
    ARKIME_RULE_TYPE_BEFORE_SAVE,
    ARKIME_RULE_TYPE_NUM

} ArkimeRuleType;

void arkime_rules_init();
void arkime_rules_recompile();
#define ARKIME_RULES_RUN_FIELD_SET(session, pos, value) do { if (config.fields[pos]->ruleEnabled) arkime_rules_run_field_set(session, pos, value); } while (0)
void arkime_rules_run_field_set(ArkimeSession_t *session, int pos, const gpointer value);
int arkime_rules_run_every_packet(ArkimePacket_t *packet);
void arkime_rules_session_create(ArkimeSession_t *session);
void arkime_rules_run_session_setup(ArkimeSession_t *session, ArkimePacket_t *packet);
void arkime_rules_run_after_classify(ArkimeSession_t *session);
void arkime_rules_run_before_save(ArkimeSession_t *session, int final);
void arkime_rules_stats();
void arkime_rules_exit();

/******************************************************************************/
/*
 * pq.c
 */
typedef void (*ArkimePQ_cb)(ArkimeSession_t *session, gpointer uw);

struct ArkimePQ_t;
typedef struct ArkimePQ_t ArkimePQ_t;

ArkimePQ_t *arkime_pq_alloc(int timeout, ArkimePQ_cb cb);
void arkime_pq_upsert(ArkimePQ_t *pq, ArkimeSession_t *session, void *uw);
void arkime_pq_remove(ArkimePQ_t *pq, ArkimeSession_t *session);
void arkime_pq_run(int thread, int max);
void arkime_pq_free(ArkimeSession_t *session);
void arkime_pq_flush(int thread);

/******************************************************************************/
/*
 * js0n.c
 */
int js0n(const uint8_t *js, unsigned int len, unsigned int *out, unsigned int olen);

/******************************************************************************/
// Idea from https://github.com/git/git/blob/master/banned.h
#define BANNED(func, func2) sorry_##func##_is_a_banned_function_use_##func2

#undef strcpy
#define strcpy(x,y) BANNED(strcpy, g_strlcpy)
#undef strcat
#define strcat(x,y) BANNED(strcat, g_strlcat)
#undef strncpy
#define strncpy(x,y,n) BANNED(strncpy, g_strlcpy)
#undef strncat
#define strncat(x,y,n) BANNED(strncat, g_strlcat)

#undef sprintf
#define sprintf(...) BANNED(sprintf, snprintf)
#undef vsprintf
#define vsprintf(...) BANNED(vsprintf, vsnprintf)

#undef gmtime
#define gmtime(t) BANNED(gmtime, gmtime_r)
#undef localtime
#define localtime(t) BANNED(localtime, localtime_r)
#undef ctime
#define ctime(t) BANNED(ctime, ctime_r)
//#undef ctime_r
//#define ctime_r(t, buf) BANNED(ctime_r)
#undef asctime
#define asctime(t) BANNED(asctime, none)
#undef asctime_r
#define asctime_r(t, buf) BANNED(asctime_r, none)

