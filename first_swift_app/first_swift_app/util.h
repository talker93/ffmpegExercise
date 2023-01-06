//
//  util.h
//  first_swift_app
//
//  Created by 江山 on 1/6/23.
//

#ifndef util_h
#define util_h

#include <stdio.h>
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include <unistd.h>

void dbg_frame(AVFrame* pFrame, int length);

void dbg_frame_check_exist(AVFrame* pFrame, int start, int end, char name[20]);

void dbg_pkt(AVPacket* pPkt, int length);

void dbg_pkt_check_exist(AVPacket* pPkt, int start, int end, char name[20]);

void dbg_pU8(uint8_t* pBuf, int length, char name[20]);

void dbg_pU8_check_exist(uint8_t* pBuf, int start, int end, char name[20]);

#endif /* util_h */
