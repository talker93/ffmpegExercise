


#include "testc.h"
#include <string.h>


static int rec_status = 0;

void set_status(int status) {
    rec_status = status;
}

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

void debug_pkt(AVPacket* pPkt, int length) {
    for(int i = 0; i < length; i++) {
        printf("Data %d is at %d\n", pPkt->data[i], i);
    }
}

void debug_pkt_check_exist(AVPacket* pPkt, int start, int end, char name[20]) {
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

void debug_pU8(uint8_t* pBuf, int length, char name[20]) {
    for(int i = 0; i < length; i++) {
        printf("Data %d is at [%d] for %s\n", *(pBuf+i), i, name);
    }
}

void debug_pU8_check_exist(uint8_t* pBuf, int start, int end, char name[20]) {
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


static AVFormatContext* open_dev() {
    int ret = 0;
    char errors[1024] = {0, };
    
    // ctx
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *options = NULL;
    
    // [video device : audio device]
    char *device_name = ":1";
    
    //get format
    AVInputFormat *iformat = av_find_input_format("avfoundation");
    
    // open device
    if((ret = avformat_open_input(&fmt_ctx, device_name, iformat, &options)) < 0) {
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
        return NULL;
    }
    
    return fmt_ctx;
}

static SwrContext* init_swr() {
    SwrContext *swr_ctx = NULL;
    
    swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, 44100, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_FLT, 44100, 0, NULL);
    
    if(!swr_ctx) {
    }
    
    if(swr_init(swr_ctx) < 0) {
    }
    
    return swr_ctx;
}

static AVCodecContext* open_coder() {
    AVCodec *codec = avcodec_find_encoder_by_name("libfdk_aac");
    
    // config c_ctx
    AVCodecContext* codec_ctx = NULL;
    codec_ctx = avcodec_alloc_context3(codec);
    
    codec_ctx -> sample_fmt = AV_SAMPLE_FMT_S16;
    codec_ctx -> channel_layout = AV_CH_LAYOUT_MONO;
    codec_ctx -> channels = 1;
    codec_ctx -> sample_rate = 44100;
    codec_ctx -> bit_rate = 0;
    codec_ctx -> profile = FF_PROFILE_AAC_HE;
    
    if(avcodec_open2(codec_ctx, codec, NULL) < 0) {
        printf("codec open failed\n");
        return NULL;
    }
    
    return codec_ctx;
}

static AVFrame* create_frame() {
    AVFrame *pFrame = NULL;
    pFrame = av_frame_alloc();
    
    pFrame->nb_samples = 2048;
    pFrame->format = AV_SAMPLE_FMT_S16;
    pFrame->channel_layout = AV_CH_LAYOUT_MONO;
    
    av_frame_get_buffer(pFrame, 0);
    if(!pFrame->data[0]) {
        printf("Error, failed to alloc buf in frame!\n");
    }
    
    return pFrame;
}


static int read_encode_write(AVFormatContext *fmt_ctx, SwrContext *swr_ctx, AVCodecContext *c_ctx, FILE *outfile) {
    int ret = 0;
    
    // init packet. size is in byte
    AVPacket *pInPkt = NULL;
    pInPkt = av_packet_alloc();
    ret = av_new_packet(pInPkt, 2048);
    AVPacket *pNewPkt = NULL;
    pNewPkt = av_packet_alloc();
    
    // init buffer. size is in byte
    uint8_t **ppSrc = NULL;
    uint8_t **ppDest = NULL;
    int srcLineSize = 0;
    int destLineSize = 0;
    // why can't use av_samples_alloc?
    av_samples_alloc_array_and_samples(&ppSrc, &srcLineSize, 1, 512, AV_SAMPLE_FMT_FLT, 0);
    av_samples_alloc_array_and_samples(&ppDest, &destLineSize, 1, 512, AV_SAMPLE_FMT_S16, 0);
    
    uint8_t **ppCache = NULL;
    int cacheLineSize = 0;
    av_samples_alloc_array_and_samples(&ppCache, &cacheLineSize, 1, 2048, AV_SAMPLE_FMT_S16, 0);
    
    // create frame
    AVFrame* pFrame = NULL;
    pFrame = create_frame();
    
    int count = 0;
    
    // read -> encode -> write
    while(rec_status) {
        // read into inPkt
        ret = av_read_frame(fmt_ctx, pInPkt);
        
        // case 0: device is not ready
        if(ret == -35) {
            sleep(0.01);
            continue;
        }
        
        // case 1: read error
        if(ret < 0 && ret != -35) {
            break;
        }
        
        // memcpy is in byte
        memcpy(ppSrc[0], (void*)pInPkt->data, pInPkt->size);
        
        // IO size must be equal. swr_convert is in sample
        swr_convert(swr_ctx, ppDest, 512, (const uint8_t**)ppSrc, 512);
        
        // buffer
        if(count % 4 == 0) {
            memcpy(ppCache[0], ppDest[0], 1024);
            goto __ERROR;
        } else if(count % 4 == 1) {
            memcpy(ppCache[0]+1024, ppDest[0], 1024);
            goto __ERROR;
        } else if(count % 4 == 2) {
            memcpy(ppCache[0]+2048, ppDest[0], 1024);
            goto __ERROR;
        } else if(count % 4 == 3) {
            memcpy(ppCache[0]+3072, ppDest[0], 1024);
        }
        
        // copy ppDest to frame
        memcpy(pFrame->data[0], ppCache[0], cacheLineSize);
        
        // encode: frame -> pOutpkt
        ret = avcodec_send_frame(c_ctx, pFrame);
        
        if (ret == AVERROR(EINVAL)) {
            printf("Error: no encoder, ret = %d.\n", ret);
        }
        
        while(ret >= 0) {
            ret = avcodec_receive_packet(c_ctx, pNewPkt);
            
            if(ret == AVERROR(EAGAIN)) {
                break;
            } else if(ret == AVERROR_EOF) {
                break;
            } else if(ret < 0) {
                printf("Error, encoding audio frame\n");
                exit(-1);
            }
            
            fwrite(pNewPkt->data, 1, pNewPkt->size, outfile);
            fflush(outfile);
        }
        
    __ERROR:
        
        // unref
        // don't unref the pOutPkt since no idea how to re-init
        av_packet_unref(pInPkt);
        count = count + 1;
    }
    
    // write out the last part
//    fwrite(pOutPkt->data, 1, pOutPkt->size, outfile);
//    fflush(outfile);

    if(pInPkt && pNewPkt) {
        av_packet_free(&pInPkt);
        av_packet_free(&pNewPkt);
    }

    if(ppSrc && ppDest) {
        av_freep(&ppSrc[0]);
        av_freep(ppSrc);
        av_freep(&ppDest[0]);
        av_freep(ppDest);
    }
    
    return 0;
}


void rec_audio() {
    AVFormatContext *fmt_ctx = NULL;
    SwrContext *swr_ctx = NULL;
    AVCodecContext *c_ctx = NULL;
    
    // set log level
    av_log_set_level(AV_LOG_DEBUG);
    
    // register audio device
    avdevice_register_all();
    
    // start record
    rec_status = 1;
    
    // create and open file
    char *out = "/Users/shanjiang/Desktop/audio.aac";
    FILE *outfile = fopen(out, "wb+");
    if(!outfile) {
        printf("Error, Failed to open file!\n");
        goto __ERROR;
    }

    // open device
    fmt_ctx = open_dev();
    if(!fmt_ctx) {
        printf("Error, failed to open device!\n");
        goto __ERROR;
    }

    // init re-sampler
    swr_ctx = init_swr();
    if(!swr_ctx) {
        printf("Error, failed to alloc buf in frame!\n");
        goto __ERROR;
    }
    
    // init encoder
    c_ctx = open_coder();
    if(!c_ctx) {
        printf("Error, failed to open coder!\n");
        goto __ERROR;
    }
    
    read_encode_write(fmt_ctx, swr_ctx, c_ctx, outfile);
    

__ERROR:
    if(c_ctx) {
        avcodec_free_context(&c_ctx);
    }
    
    if(swr_ctx) {
        swr_free(&swr_ctx);
    }

    if(fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }

    if(outfile) {
        fclose(outfile);
    }

    av_log(NULL, AV_LOG_DEBUG, "Finish!\n");

    return;
}


#if 0
int main(int argc, char *argv[])
{
    rec_audio();
    return 0;
}
#endif



