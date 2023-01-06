//
//  testc.h
//  first_swift_app
//
//  Created by 江山 on 12/22/22.
//

#ifndef testc_h
#define testc_h

#include <stdio.h>
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include <unistd.h>

void rec_audio(void);

void set_status(int status);

#endif /* testc_h */
