//
//  testc.h
//  first_swift_app
//
//  Created by 江山 on 12/22/22.
//

#ifndef testc_h
#define testc_h

#include <stdio.h>
#include <string.h>
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include <unistd.h>
#include "util.h"
#include "librtmp/rtmp.h"

void publish_stream();

void set_status(int state);

#endif /* testc_h */
