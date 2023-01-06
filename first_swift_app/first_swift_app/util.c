//
//  util.c
//  first_swift_app
//
//  Created by 江山 on 1/6/23.
//

#include "util.h"


void dbg_frame(AVFrame* pFrame, int length) {
    for(int i = 0; i < length; i++) {
        printf("Data %d is at %d\n", *(pFrame->data[0]+i), i);
    }
}

void dbg_frame_check_exist(AVFrame* pFrame, int start, int end, char name[20]) {
    int sum = 0;
    for(int i = start; i <= end; i++) {
        sum = sum + *(pFrame->data[0]+i);
    }
    if(sum == 0) {
        printf("none, %s\n", name);
    } else {
        printf("exist, %s\n", name);
    }
}

void dbg_pkt(AVPacket* pPkt, int length) {
    for(int i = 0; i < length; i++) {
        printf("Data %d is at %d\n", pPkt->data[i], i);
    }
}

void dbg_pkt_check_exist(AVPacket* pPkt, int start, int end, char name[20]) {
    int sum = 0;
    for(int i = start; i <= end; i++) {
        sum = sum + (int)pPkt->data[i];
    }
    if(sum == 0) {
        printf("none, %s\n", name);
    } else {
        printf("exist, %s\n", name);
    }
}

void dbg_pU8(uint8_t* pBuf, int length, char name[20]) {
    for(int i = 0; i < length; i++) {
        printf("Data %d is at [%d] for %s\n", *(pBuf+i), i, name);
    }
}

void dbg_pU8_check_exist(uint8_t* pBuf, int start, int end, char name[20]) {
    int sum = 0;
    for(int i = start; i <= end; i++) {
        sum = sum + *(pBuf+i);
    }
    if(sum == 0) {
        printf("none, %s\n", name);
    } else {
        printf("exist, %s\n", name);
    }
}
