/* We send this file through the pre-processor to create a SWIG .i file. */
%include _tn5250_extra.i

/* SWIG horks on some of the things in our header files, so... */
#ifndef SWIG
#define SWIG
#endif

#include "tn5250-config.h"

#include "buffer.h"
#include "codes5250.h"
#include "conf.h"
#include "cursesterm.h"
#include "dbuffer.h"
#include "debug.h"
#include "display.h"
#include "field.h"
#include "printsession.h"
#include "record.h"
#include "scs.h"
#include "session.h"
#include "slangterm.h"
#include "stream.h"
#include "terminal.h"
#include "utility.h"
#include "wtd.h"
