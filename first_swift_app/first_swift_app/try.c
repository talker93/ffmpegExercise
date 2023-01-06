//
//  try.c
//  first_swift_app
//
//  Created by 江山 on 12/31/22.
//

#include "testc.h"
#include "string.h"

static int rec_status = 0;

void set_status(int status) {
    rec_status = status;
}

// step-wise AAC encoding
// send_frame -> receive_packet -> fwrite
void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *output) {
    int ret = 0;
    
    // send the data to the encoder
    ret = avcodec_send_frame(ctx, frame);
    // if data successfully set
    while(ret >= 0) {
        // acquire the data of post-encoding, repeat until failed
        avcodec_receive_packet(ctx, pkt);
        // break the loop if received error
        if(ret < 0) {
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else {
                printf("Error, encoding audio frame\n");
                exit(-1);
            }
        }
    }
    
    // write to file
    fwrite(pkt->data, 1, pkt->size, output);
    // fflush out everytime, but reduce the performance
    fflush(output);
}

// init encoder
AVCodecContext* open_coder() {
    // declare encoder
    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    // init context
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    
    // set context parameters
    codec_ctx -> sample_fmt = AV_SAMPLE_FMT_S16;
    codec_ctx -> channel_layout = AV_CH_LAYOUT_STEREO;
    codec_ctx -> channels = 2;
    codec_ctx -> sample_rate = 44100;
    codec_ctx -> bit_rate = 0;
    codec_ctx -> profile = FF_PROFILE_AAC_HE_V2;
    
    // open codec and return error if needed
    if(avcodec_open2(codec_ctx, codec, NULL) < 0) {
        // return error
        return NULL;
    }
    
    return codec_ctx;
}

// init re-sampler
SwrContext* init_swr() {
    // declare context
    SwrContext *swr_ctx = NULL;
    // set context parameters
    swr_ctx = swr_alloc_set_opts(NULL, // ctx
                                 AV_CH_LAYOUT_STEREO, // output channel layout
                                 AV_SAMPLE_FMT_S16, // output sampling format
                                 44100, // output sample rate
                                 AV_CH_LAYOUT_STEREO, // input channel layout
                                 AV_SAMPLE_FMT_FLT, // input sampling format
                                 44100, // input sample rate
                                 0,
                                 NULL);
    
    // do something if error met
    if(!swr_ctx) {
    }
    // open re-sampler
    if(swr_init(swr_ctx) < 0) {
    }
    
    return swr_ctx;
}


void rec_audio() {
    
    
    
    // Step1: Prep
    int ret = 0;
    char errors[1024];
    
    // init buffer
    uint8_t **src_data = NULL;
    int src_linesize = 0;
    uint8_t **dst_data = NULL;
    int dst_linesize = 0;
    
    // declare context
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *options = NULL;
    
    char *device_name = ":1";
    
    // set log level (why not include into the obj???)
    av_log_set_level(AV_LOG_DEBUG);
    
    rec_status = 1;
    
    // register audio device (why need to register devices???)
    avdevice_register_all();
    
    // declare and get format of the input
    AVInputFormat *iformat = av_find_input_format("avfoundation");
    
    
    
    // Step2: open_input
    if((ret = avformat_open_input(&fmt_ctx, device_name, iformat, &options)) < 0) {
        av_strerror(ret, errors, 1024);
        printf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
        return;
    }
    
    // declare the file
    char *out = "/Users/shanjiang/Desktop/audio.pcm";
    FILE *outfile = fopen(out, "wb+");
    
    
    
    
    
    // Step3: declare the packet and frame -> init encoder & re-sampler -> alloc memory for IO
    
    // declare frame and set params
    AVFrame *frame = av_frame_alloc();
    if(!frame) {
    }
    
    frame -> nb_samples = 256; // samples per frame
    frame -> format = AV_SAMPLE_FMT_S16; // size per sample
    frame -> channel_layout = AV_CH_LAYOUT_STEREO; // channel layout
    av_frame_get_buffer(frame, 0);
    
    if(!frame || !frame->buf[0]) {
    }
    
    // declare packet for input
    AVPacket pkt;
    av_init_packet(&pkt);
    
    // declare packet for output
    AVPacket *newpkt = av_packet_alloc();
    if(!newpkt) {
    }
    
    // (what is AVCodecContext???)
    // init encoder
    AVCodecContext* c_ctx = open_coder();
    
    // init re-sampler
    SwrContext* swr_ctx = init_swr();
    
    // alloc memory buffer for input
    av_samples_alloc_array_and_samples(&src_data, // buffer address
                                       &src_linesize, // buffer size in byte
                                       2, // channel numbers
                                       256, // single channel samples
                                       AV_SAMPLE_FMT_FLT, // sampling format, 32bits = 4bytes
                                       0);
    
    // alloc memory buffer for output
    av_samples_alloc_array_and_samples(&dst_data, // buffer address
                                       &dst_linesize, // buffer size in byte
                                       2, // channel numbers
                                       256, // single channel samples
                                       AV_SAMPLE_FMT_S16, // sampling format, 16bits = 2bytes
                                       0);
    
    
    
    
    
    // Step4: resample -> execute AAC encoding
    int count = 0;
    while(rec_status) {
        // read frame
        ret = av_read_frame(fmt_ctx, &pkt);
        
        // case 0: device is not ready
        if(ret == -35) {
            sleep(0.01);
            continue;
        }
        
        // case 1: read error
        if(ret < 0) {
            break;
        }
        
        // case 3: well working, print data
        if(ret == 0)
        {
            av_log(NULL, AV_LOG_INFO, "pkt size is %d(%p) at index %d \n", pkt.size, pkt.data, count);
            
            // byte-wise memory copy
            // copy from input packet to the input buffer
            memcpy((void*)src_data[0], (void*)pkt.data, pkt.size);
            
            // re-sampling
            swr_convert(swr_ctx, dst_data, 256, (const uint8_t **)src_data, 256);
            
            // copy the processed data to frame
            memcpy((void*)frame->data[0], dst_data, dst_linesize);
            
            // AAC encoding
            encode(c_ctx, frame, newpkt, outfile);
            
            count = count + 1;
        }
        // release input packet every time
        av_packet_unref(&pkt);
    }
    
    
    
    
    
    // Step5: release the resource
    
    // close the file
    fclose(outfile);
    
    // release IO buffer
    if(src_data)
        av_freep(&src_data[0]);
    av_freep(&src_data);
    
    if(dst_data)
        av_freep(&dst_data[0]);
    av_freep(&dst_data);
    
    // release swr context
    swr_free(&swr_ctx);
    
    // release IO packet
    av_packet_unref(&pkt);
    av_packet_unref(&pkt);
    
    // release format context
    avformat_close_input(&fmt_ctx);
    av_log(NULL, AV_LOG_DEBUG, "Finish! \n");
}


































//
//  testc.c
//  first_swift_app
//
//  Created by 江山 on 12/22/22.
//

#include "testc.h"
#include "string.h"

static int rec_status = 0;

void set_status(int status) {
    rec_status = status;
}

//[in]
//[out]
//ret
//@brief encode audio data
void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *output) {
    int ret = 0;
    
    
    // send the data the encoder
    ret = avcodec_send_frame(ctx, frame);
    //if data sucessfully set
    while(ret >= 0) {
        // acquire the data of post-encoding, repeat until failed
        avcodec_receive_packet(ctx, pkt);
        if(ret < 0) {
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else {
                printf("Error, encoding audio frame\n");
                exit(-1);
            }
        }
    }
    
    // write to file
    fwrite(pkt->data, 1, pkt->size, output);
    // fflush out everytime, but reduce the performance
    fflush(output);
}

// encoder opener
AVCodecContext* open_coder() {
    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
//    AVCodec *codec = avcodec_find_encoder_by_name("libfdk_aac");
    
    // create codec context
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    
    // param for input
    codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
    codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    codec_ctx->channels = 2;
    codec_ctx->sample_rate = 44100;
    codec_ctx->bit_rate = 0;
    codec_ctx->profile = FF_PROFILE_AAC_HE_V2;
    
    if(avcodec_open2(codec_ctx, codec, NULL)<0) {
        // open error met
        return NULL;
    }
    
    return codec_ctx;
}

// resampling init
SwrContext* init_swr() {
    SwrContext *swr_ctx = NULL;
    swr_ctx = swr_alloc_set_opts(NULL, //ctx
                                 AV_CH_LAYOUT_STEREO, //output channel layout
                                 AV_SAMPLE_FMT_S16, //output sampling format
                                 44100, // output sample rate
                                 AV_CH_LAYOUT_STEREO, // input channel layout
                                 AV_SAMPLE_FMT_FLT, //input sampling format
                                 44100, // input sample rate
                                 0,
                                 NULL);
    
    if(!swr_ctx) {
        
    }
    
    if(swr_init(swr_ctx) < 0) {
        
    }
    
    return swr_ctx;
}

void rec_audio() {
    int ret = 0;
    char errors[1024];
    
    // resampling buffer
    uint8_t **src_data = NULL;
    int src_linesize = 0;
    uint8_t **dst_data = NULL;
    int dst_linesize = 0;
    
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *options = NULL;
    
    //[[video device]:[audio device]]
    char *devicename = ":1";
    
    //set log level
    av_log_set_level(AV_LOG_DEBUG);
    
    rec_status = 1;
    
    //register audio device
    avdevice_register_all();
    
    //get format
    AVInputFormat *iformat = av_find_input_format("avfoundation");
    
    // step 1 open_input
    if((ret = avformat_open_input(&fmt_ctx, devicename, iformat, &options)) < 0) {
        av_strerror(ret, errors, 1024);
        printf(stderr, "Failed to open audio device, [%d]%s\n", ret, errors);
        return;
    }
    
    //create a file
    char *out = "/Users/shanjiang/Desktop/audio.pcm";
    FILE *outfile = fopen(out, "wb+");
    
    
    
    // step 2 init --> read
    AVPacket pkt;
    av_init_packet(&pkt);
    
    AVCodecContext* c_ctx = open_coder();
    
    // frame for input
    AVFrame *frame = av_frame_alloc();
    if(!frame) {
        
    }
    
    frame->nb_samples = 256; // samples per frame
    frame->format = AV_SAMPLE_FMT_S16; // size per sample
    frame->channel_layout = AV_CH_LAYOUT_STEREO; // channel layout
    av_frame_get_buffer(frame, 0);
    
    if(!frame || !frame->buf[0]) {
        
    }
    
    // packet for output
    AVPacket *newpkt = av_packet_alloc(); // alloc space for post encoding
    if(!newpkt) {
        
    }
    
    SwrContext* swr_ctx = init_swr();
    
    // create buffer for input
    av_samples_alloc_array_and_samples(&src_data, //buffer addr
                                       &src_linesize, //buffer size in byte
                                       2, // channel
                                       256, //single channel samples
                                       AV_SAMPLE_FMT_FLT, // sampling format, 32bits --> 4bytes
                                       0);
    
    // create buffer for output
    av_samples_alloc_array_and_samples(&dst_data, //buffer addr
                                       &dst_linesize, //buffer size in byte
                                       2, // channel
                                       256, //single channel samples
                                       AV_SAMPLE_FMT_S16, // sampling format, 16bits --> 2bytes
                                       0);
    
    int count = 0;
    while(rec_status) {
        // read frame
        ret = av_read_frame(fmt_ctx, &pkt);
        
        //case 0: device is not ready
        if(ret == -35) {
            sleep(0.01);
            continue;
        }
        
        //case 1: read error
        if(ret < 0) {
            break;
        }
        
        //case 3: good condition, print data
        if(ret == 0)
        {
            // print log
            av_log(NULL, AV_LOG_INFO, "pkt size is %d(%p) at index %d \n", pkt.size, pkt.data, count);
            
            // byte-wise memory copy
            memcpy((void*)src_data[0], (void*)pkt.data, pkt.size);
            
            // re-sampling
            swr_convert(swr_ctx,
                        dst_data,
                        256,
                        (const uint8_t **)src_data,
                        256);
            
            // re-sampled data to frame
            memcpy((void *)frame->data[0], dst_data, dst_linesize);
            
            //encode
            encode(c_ctx, frame, newpkt, outfile);
            
            count = count + 1;
        }
        
        av_packet_unref(&pkt); //release pkt
    }
    
    //close file
    fclose(outfile);
    
    //release swr buffer
    if(src_data) {
        av_freep(&src_data[0]);
    }
    av_freep(&src_data);
    
    if(dst_data) {
        av_freep(&dst_data[0]);
    }
    av_freep(&dst_data);
    
    //release swr context
    swr_free(&swr_ctx);
    
    // step 3 unref
    av_packet_unref(&pkt);
    
    //close device release ctx
    avformat_close_input(&fmt_ctx);
    av_log(NULL, AV_LOG_DEBUG, "Finish! \n");
}











































#include "testc.h"
#include <string.h>


static int rec_status = 0;

void set_status(int status) {
    rec_status = status;
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
    
    swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLT, 44100, 0, NULL);
    
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
    
    pFrame->nb_samples = 1024;
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
    int errcount = 0;
    
    // init packet. size is in byte
    AVPacket *pInPkt = NULL;
    pInPkt = av_packet_alloc();
    ret = av_new_packet(pInPkt, 2048);
    AVPacket *pOutPkt = NULL;
    pOutPkt = av_packet_alloc();
    ret = av_new_packet(pOutPkt, 1024);
    
    AVPacket* newPkt = NULL;
    newPkt = av_packet_alloc();
    
    // init buffer. size is in byte
    uint8_t **ppSrc = NULL;
    uint8_t **ppDest = NULL;
    int srcLineSize = 0;
    int destLineSize = 0;
    // why can't use av_samples_alloc?
    av_samples_alloc_array_and_samples(&ppSrc, &srcLineSize, 1, 512, AV_SAMPLE_FMT_FLT, 0);
    av_samples_alloc_array_and_samples(&ppDest, &destLineSize, 1, 512, AV_SAMPLE_FMT_S16, 0);
    
    // cache for feeding the frame
    uint8_t **ppCache = NULL;
    int cacheLineSize = 0;
    av_samples_alloc_array_and_samples(&ppCache, &cacheLineSize, 1, 1024, AV_SAMPLE_FMT_S16, 0);
    
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
        
        //copy to cache
        memcpy(ppCache[0]+(count%2)*1024, ppDest[0], 1024);
        
        // skip the current loop if the cache isn't filled up
        if(count % 2 == 0) {
            goto __ERROR;
        }
        
        // copy ppDest to frame
        memcpy(pFrame->data[0], ppCache[0], cacheLineSize);
        
        // encode: frame -> pOutpkt
        ret = avcodec_send_frame(c_ctx, pFrame);
        
        if (ret == AVERROR(EINVAL)) {
            printf("Error: no encoder, ret = %d.\n", ret);
        }
        
        while(ret >= 0) {
            ret = avcodec_receive_packet(c_ctx, newPkt);
            
            if(ret == AVERROR(EAGAIN)) {
                break;
            } else if(ret == AVERROR_EOF) {
                break;
            } else if(ret < 0) {
                printf("Error, encoding audio frame\n");
                exit(-1);
            }
            
//            printf("newPkt received data: %d", newPkt->data);
            fwrite(newPkt->data, 1, newPkt->size, outfile);
            fflush(outfile);
        }
        
        
//        // pPkt->size = srcLineSize
//        // 16bit --> 2bytes/sample, 32bit --> 1byte/sample
//        memcpy(pOutPkt->data, (void*)ppDest[0], 1024);
//
//        // fwrite is in byte
//        fwrite(pOutPkt->data, 1, pOutPkt->size, outfile);
//        fflush(outfile);
//
//        av_log(NULL, AV_LOG_INFO, "packet %d size is %d(%p), count = %d \n", *(pOutPkt->data), pOutPkt->size, pOutPkt, count);
//        count = count + 1;
    __ERROR:
        
        // unref
        // don't unref the pOutPkt since no idea how to re-init
        av_packet_unref(pInPkt);
//        if(count%2 == 1)
//            memset(ppCache[0], 0, 2048);
//        av_frame_unref(pFrame);
        count = count + 1;
    }
    
    // write out the last part
//    fwrite(pOutPkt->data, 1, pOutPkt->size, outfile);
//    fflush(outfile);

    if(pInPkt && pOutPkt) {
        av_packet_free(&pInPkt);
        av_packet_free(&pOutPkt);
    }

    if(ppSrc && ppDest) {
        av_freep(&ppSrc[0]);
        av_freep(ppSrc);
        av_freep(&ppDest[0]);
        av_freep(ppDest);
        av_freep(&ppCache[0]);
        av_freep(ppCache);
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



