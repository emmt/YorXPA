/*
 * yor-xpa.c --
 *
 * Yorick interface to XPA.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

/* Standard C library headers. */
#include <stdlib.h>
#include <string.h>

/* XPA header. */
#include <xpa.h>

/* Yorick headers. */
#include <pstdlib.h>
#include <play.h>
#include <yapi.h>

#define IS_INTEGER(id) (Y_CHAR <= (id) && (id) <= Y_LONG)
#define IS_NUMBER(id)  (Y_CHAR <= (id) && (id) <= Y_COMPLEX)
#define IS_VOID(id)    ((id) == Y_VOID)
#define IS_STRING(id)  ((id) == Y_STRING)
#define IS_SCALAR_STRING(iarg) (yarg_string(iarg) == 1)

#define IS_ERROR(str)  (strncmp(str, "XPA$ERROR", 9) == 0)
#define IS_MESSAGE(str) (strncmp(str, "XPA$MESSAGE", 11) == 0)

#define ROUND_UP(a, b)  ((((a) + ((b) - 1))/(b))*(b))

/* Define some macros to get rid of some GNU extensions when not compiling
   with GCC. */
#if ! (defined(__GNUC__) && __GNUC__ > 1)
#   define __attribute__(x)
#   define __inline__
#   define __FUNCTION__        ""
#   define __PRETTY_FUNCTION__ ""
#endif

PLUG_API void y_error(const char*) __attribute__ ((noreturn));

static void push_string(const char* str, long len);

static void
push_string(const char* str, long len)
{
    char* cpy;
    if (len == -1) {
        len = (str == NULL ? 0 : strlen(str));
    } else if (len < 0 || (str == NULL && len != 0)) {
        y_error("invalid string length");
    }
    if (str == NULL) {
        cpy = NULL;
    } else {
        cpy = p_malloc(len + 1);
        memcpy(cpy, str, len);
        cpy[len] = '\0';
    }
    ypush_q(NULL)[0] = cpy;
}

/*---------------------------------------------------------------------------*/
/* PERSISTENT XPA CONNECTION */

static XPA client = NULL;
static int atexit_called = 0; /* atexit(disconnect) has been called? */

static void connect();
static void disconnect();

static void connect()
{
    if (client == NULL) {
        client = XPAOpen(NULL);
        if (client == NULL) {
            y_error("failed to open XPA persistent connection");
        } else {
            if (! atexit_called) {
                if (atexit(disconnect) != 0) {
                    y_error("atexit() failed");
                } else {
                    atexit_called = 1;
                }
            }
        }
    }
}

static void disconnect()
{
    if (client != NULL) {
        XPA tmp = client;
        client = NULL;
        XPAClose(tmp);
    }
}

/*---------------------------------------------------------------------------*/
/* XPA DATA OBJECT */

/* A number of static arrays is used to collect received messages and data. */
#define NMAX 100
static size_t lens[NMAX];
static char*  bufs[NMAX];
static char*  srvs[NMAX];
static char*  msgs[NMAX];
static int replies = 0; /* number of replies stored in static arrays */

typedef struct xpadata {
    size_t* lens;
    char**  bufs;
    char**  srvs;
    char**  msgs;
    int replies;
    int buffers;
    int messages;
    int errors;
} xpadata_t;

/* Yields the number of buffers. */
static int get_buffers(xpadata_t* obj)
{
    if (obj->buffers < 0) {
        /* Number of buffers not yet counted. */
        int i, n = 0;
        for (i = 0; i < obj->replies; ++i) {
            if (obj->bufs[i] != NULL) {
                ++n;
            }
        }
        obj->buffers = n;
    }
    return obj->buffers;
}

/* Yields the number of errors. */
static int get_errors(xpadata_t* obj)
{
    if (obj->errors < 0) {
        /* Number of errors not yet counted. */
        int i, n = 0;
        for (i = 0; i < obj->replies; ++i) {
            if (obj->msgs[i] != NULL && IS_ERROR(obj->msgs[i])) {
                ++n;
            }
        }
        obj->errors = n;
    }
    return obj->errors;
}

/* Yields the number of messages. */
static int get_messages(xpadata_t* obj)
{
    if (obj->messages < 0) {
        /* Number of messages not yet counted. */
        int i, n = 0;
        for (i = 0; i < obj->replies; ++i) {
            if (obj->msgs[i] != NULL && IS_MESSAGE(obj->msgs[i])) {
                ++n;
            }
        }
        obj->messages = n;
    }
    return obj->messages;
}

static void free_xpadata(void* addr)
{
    xpadata_t* obj = (xpadata_t*)addr;
    int i;
    for (i = 0; i < obj->replies; ++i) {
        if (obj->bufs[i] != NULL) {
            free(obj->bufs[i]);
        }
        if (obj->srvs[i] != NULL) {
            free(obj->srvs[i]);
        }
        if (obj->msgs[i] != NULL) {
            free(obj->msgs[i]);
        }
    }
}

static void
print_xpadata(void* addr)
{
    char buffer[200];
    xpadata_t* obj = (xpadata_t*)addr;
    int replies = obj->replies;
    int buffers = get_buffers(obj);
    int errors = get_errors(obj);
    int messages = get_messages(obj);
#define ARG(n) (n), ((n) < 2 ? "" : "s")
    sprintf(buffer,
            "XPAData (%d replie%s, %d buffer%s, %d message%s, %d error%s)",
            ARG(replies), ARG(buffers), ARG(messages), ARG(errors));
#undef ARG
    y_print(buffer, 1);
}

static void
eval_xpadata(void* addr, int argc)
{
    long dims[Y_DIMSIZE];
    xpadata_t* obj = (xpadata_t*)addr;
    int iarg, rank, typeid = Y_VOID;
    long i, k;

    /* Check number of arguments. */
    if (argc < 1 || argc > 2) {
        y_error("expecting 1 or 2 arguments");
    }

    /* First argument. */
    iarg = argc - 1;
    typeid = yarg_typeid(iarg);
    if (IS_VOID(typeid) && argc == 1) {
        ypush_long(obj->replies);
        return;
    }
    if (! IS_INTEGER(typeid) || yarg_rank(iarg) != 0) {
        y_error("expecting an index");
    }
    i = ygets_l(iarg);
    if (i <= 0) {
        i += obj->replies;
    }
    if (i < 1 || i > obj->replies) {
        y_error("out of range index");
    }
    --i; /* C indices start at 0 */
    if (argc == 1) {
        push_string(obj->msgs[i], -1);
        return;
    }

    /* Second argument. */
    --iarg;
    typeid = yarg_typeid(iarg);
    if (typeid == Y_VOID) {
        ypush_long(obj->lens[i]);
        return;
    }
    rank = yarg_rank(iarg);
    if (rank == 0 && IS_INTEGER(typeid)) {
        k = ygets_l(iarg);
        if (k == 0) {
            /* Yields whether there is a message. */
            int ans = 0;
            if (obj->msgs[i] != NULL) {
                if (IS_MESSAGE(obj->msgs[i])) {
                    ans = 1;
                } else if (IS_ERROR(obj->msgs[i])) {
                    ans = 2;
                }
            }
            ypush_int(ans);
            return;
        }
        if (k == 1) {
            /* Yields the message. */
            push_string(obj->msgs[i], -1);
            return;
        }
        if (k == 2) {
            /* Yields the name of the server. */
            push_string(obj->srvs[i], -1);
            return;
        }
        if (k == 3) {
            /* Push data as an array of bytes. */
            size_t len = obj->lens[i];
            char* buf = obj->bufs[i];
            if (len > 0) {
                dims[0] = 1;
                dims[1] = len;
                memcpy(ypush_c(dims), buf, len);
            } else {
                ypush_nil();
            }
            return;
        }
        if (k == 4) {
            /* Push data as a string. */
            size_t len = obj->lens[i];
            char* buf = obj->bufs[i];
            push_string(buf, len);
            return;
        }
    }
    if (rank > 0  && IS_NUMBER(typeid)) {
        size_t len = obj->lens[i];
        char* buf = obj->bufs[i];
        long ntot;
        void* arr = ygeta_any(iarg, &ntot, NULL, &typeid);
        size_t size;
        switch (typeid) {
        case Y_CHAR: size = ntot*sizeof(char); break;
        case Y_SHORT: size = ntot*sizeof(short); break;
        case Y_INT: size = ntot*sizeof(int); break;
        case Y_LONG: size = ntot*sizeof(long); break;
        case Y_FLOAT: size = ntot*sizeof(float); break;
        case Y_DOUBLE: size = ntot*sizeof(double); break;
        case Y_COMPLEX: size = 2*ntot*sizeof(double); break;
        default: size = 0; y_error("invalid array type");
        }
        if (len != size) {
            y_error("invalid array size");
        }
        memcpy(arr, buf, len);
        return;
    }
    y_error("invalid key value");
}

static void
extract_xpadata(void* addr, char* name)
{
    xpadata_t* obj = (xpadata_t*)addr;
    if (name[0] == 'r' && strcmp(name, "replies") == 0) {
        ypush_long(obj->replies);
    } else if (name[0] == 'b' && strcmp(name, "buffers") == 0) {
        ypush_long(get_buffers(obj));
    } else if (name[0] == 'e' && strcmp(name, "errors") == 0) {
        ypush_long(get_errors(obj));
    } else if (name[0] == 'm' && strcmp(name, "messages") == 0) {
        ypush_long(get_messages(obj));
    } else {
        y_error("bad XPAData member");
    }
}

static y_userobj_t xpadata_type = {
    "XPAData",
    free_xpadata,
    print_xpadata,
    eval_xpadata,
    extract_xpadata,
    NULL
};

static void clear_static_arrays()
{
    if (p_signalling) {
        p_abort();
    }
    while (replies > 0) {
        int i = replies - 1;
        char* buf = bufs[i];
        char* srv = srvs[i];
        char* msg = msgs[i];
        bufs[i] = NULL;
        msgs[i] = NULL;
        srvs[i] = NULL;
        if (buf != NULL) {
            free(buf);
        }
        if (srv != NULL) {
            free(srv);
        }
        if (msg != NULL) {
            free(msg);
        }
        replies = i;
    }
    if (replies < 0) {
        replies = 0;
    }
}

static void push_xpadata()
{
    xpadata_t* obj;
    size_t size, offset, step;
    int i;

    /* Reduce the risk of being inrerrupted. */
    if (p_signalling) {
        p_abort();
    }

    /* Compute object size. */
    if (replies < 0) {
        replies = 0;
    }
    offset = ROUND_UP(sizeof(xpadata_t), sizeof(void*));
    step = replies*sizeof(void*);
    size = offset + 4*step;

    /* Push a new object and instanciate it.  Note that memory returned by
       ypush_obj has been zero-filled. */
    obj = (xpadata_t*)ypush_obj(&xpadata_type, size);
    obj->replies = 0;
    obj->buffers = -1;
    obj->messages = -1;
    obj->errors = -1;
    obj->lens = (size_t*)((char*)obj + offset);
    offset += step;
    obj->bufs = (char**)((char*)obj + offset);
    offset += step;
    obj->srvs = (char**)((char*)obj + offset);
    offset += step;
    obj->msgs = (char**)((char*)obj + offset);
    for (i = 0; i < replies; ++i) {
        /* Copy contents, taking care of interrupts. */
        obj->lens[i] = lens[i];
        obj->bufs[i] = bufs[i];
        obj->srvs[i] = srvs[i];
        obj->msgs[i] = msgs[i];
        bufs[i] = NULL;
        msgs[i] = NULL;
        srvs[i] = NULL;
        ++obj->replies;
    }
    replies = 0;
}

void Y_xpaget(int argc)
{
    char* apt = NULL;
    char* cmd = NULL;
    int typeid, iarg;

    /* Parse arguments. */
    if (argc < 1 || argc > 2) {
        y_error("expecting 1 or 2 arguments");
    }
    iarg = argc;
    if (--iarg >= 0) {
        /* Get access point. */
        if (! IS_SCALAR_STRING(iarg)) {
            y_error("access point must be a string");
        }
        apt = ygets_q(iarg);
    }
    if (--iarg >= 0) {
        /* Get command. */
        typeid = yarg_typeid(iarg);
        if (IS_STRING(typeid) && yarg_rank(iarg) == 0) {
            cmd = ygets_q(iarg);
        } else if (! IS_VOID(typeid)) {
            y_error("command must be empty or a string");
        }
    }

    /* Evaluate the XPA get command. */
    if (client == NULL) {
        connect();
    }
    clear_static_arrays();
    replies = XPAGet(client, apt, cmd, NULL, bufs, lens, srvs, msgs, NMAX);
    push_xpadata();
}

void Y_xpaset(int argc)
{
    char* apt = NULL;
    char* cmd = NULL;
    char* buf = NULL;
    size_t len = 0;
    long ntot;
    int typeid, iarg;

    /* Parse arguments. */
    if (argc < 1 || argc > 3) {
        y_error("expecting 1, 2 or 3 arguments");
    }
    iarg = argc;
    if (--iarg >= 0) {
        /* Get access point. */
        if (! IS_SCALAR_STRING(iarg)) {
            y_error("access point must be a string");
        }
        apt = ygets_q(iarg);
    }
    if (--iarg >= 0) {
        /* Get command. */
        typeid = yarg_typeid(iarg);
        if (IS_STRING(typeid) && yarg_rank(iarg) == 0) {
            cmd = ygets_q(iarg);
        } else if (! IS_VOID(typeid)) {
            y_error("command must be empty or a string");
        }
    }
    if (--iarg >= 0) {
        /* Get data. */
        buf = ygeta_any(iarg, &ntot, NULL, &typeid);
        switch (typeid) {
        case Y_CHAR: len = ntot*sizeof(char); break;
        case Y_SHORT: len = ntot*sizeof(short); break;
        case Y_INT: len = ntot*sizeof(int); break;
        case Y_LONG: len = ntot*sizeof(long); break;
        case Y_FLOAT: len = ntot*sizeof(float); break;
        case Y_DOUBLE: len = ntot*sizeof(double); break;
        case Y_COMPLEX: len = 2*ntot*sizeof(double); break;
        default: y_error("invalid array type");
        }
    }

    /* Evaluate the XPA set command. */
    if (client == NULL) {
        connect();
    }
    clear_static_arrays();
    replies = XPASet(client, apt, cmd, NULL, buf, len, srvs, msgs, NMAX);
    push_xpadata();
}

/*---------------------------------------------------------------------------*/
