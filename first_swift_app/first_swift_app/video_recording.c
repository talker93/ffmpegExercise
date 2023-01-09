#include "video_recording.h"
#include <string.h>

#define V_WIDTH 640
#define V_HEIGHT 480

static int rec_status = 0;

void set_status(int status) {
    rec_status = status;
}

static AVFormatContext* open_dev() {
    int ret = 0;
    char errors[1024] = {0, };
    
    // ctx
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *options = NULL;
    
    // [video device : audio device]
    // 0: device native camera
    // 1: desktop
    char *device_name = "0";
    
    //get format
    AVInputFormat *iformat = av_find_input_format("avfoundation");
    
    av_dict_set(&options, "video_size", "640x480", 0);
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "pixel_format", "nv12", 0);
    
    // open device
    if((ret = avformat_open_input(&fmt_ctx, device_name, iformat, &options)) < 0) {
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open video device, [%d]%s\n", ret, errors);
        return NULL;
    }
    
    return fmt_ctx;
}

/**
 * @brief xxxx
 * @param[in] width
 * @param[in] height
 * @param[out] enc_ctx
 */
static void open_encoder(int width, int height, AVCodecContext** enc_ctx) {
    int ret = 0;
    AVCodec* codec = NULL;
    
    codec = avcodec_find_encoder_by_name("libx264");
    if(!codec) {
        printf("Codec libx264 not found\n");
        exit(1);
    }
    
    *enc_ctx = avcodec_alloc_context3(codec);
    if(!enc_ctx) {
        printf("Could not allocate video codec context!\n");
        exit(1);
    }
    
    // SPS/PPS
    (*enc_ctx) -> profile = FF_PROFILE_H264_HIGH_444;
    (*enc_ctx) -> level = 50; // level is 5.0
    
    // resolution
    (*enc_ctx) -> width = width; //640
    (*enc_ctx) -> height = height; //480
    
    // GOP
    (*enc_ctx) -> gop_size = 250;
    (*enc_ctx) -> keyint_min = 25; //optional
    
    // B frame setup
    (*enc_ctx) -> max_b_frames = 3; //optional
    (*enc_ctx) -> has_b_frames = 1; //optional
    
    // ref frame number
    (*enc_ctx) -> refs = 3; //optional
    
    // input YUV format
    (*enc_ctx) -> pix_fmt = AV_PIX_FMT_YUV420P;
    
    // bit rate setup
    (*enc_ctx) -> bit_rate = 600000;
    
    // frame rate setup
    (*enc_ctx) -> time_base = (AVRational){1, 25};
    (*enc_ctx) -> framerate = (AVRational){25, 1};
    
    ret = avcodec_open2((*enc_ctx), codec, NULL);
    if(ret < 0) {
        printf("Could not open codec: %s\n", av_err2str(ret));
        exit(1);
    }
    
}

static AVFrame* create_frame(int width, int height) {
    int ret = 0;
    AVFrame* frame = NULL;
    frame = av_frame_alloc();
    
    if(!frame) {
        printf("Error, No memory!\n");
        goto __ERROR;
    }
    
    frame->width = width;
    frame->height = height;
    frame->format = AV_PIX_FMT_YUV420P;
    
    // alloc inner memory
    ret = av_frame_get_buffer(frame, 32);
    if(ret < 0) {
        printf("Error, Failed to alloc buffer for frame!\n");
        goto __ERROR;
    }
    
__ERROR:
    
    return frame;
    
}

static void encode(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* newpkt, FILE* outfile) {
    int ret = 0;
    if(frame) {
        printf("send frame to encoder, pts=%lld", frame->pts);
    }
    
    // send frame to encoder
    ret = avcodec_send_frame(enc_ctx, frame);
    if(ret < 0) {
        printf("Error, Failed to send a frame for encoding\n");
        exit(1);
    }
    
    // receive packet from encoder
    while(ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, newpkt);
        
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            printf("Error, Failed to encode!\n");
            exit(1);
        }
        
        fwrite(newpkt->data, 1, newpkt->size, outfile);
        av_packet_unref(newpkt);
    }
}


void rec_video() {
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext* enc_ctx = NULL;
    
    // set log level
    av_log_set_level(AV_LOG_DEBUG);
    
    // register audio device
    avdevice_register_all();
    
    // start record
    rec_status = 1;
    
    // create and open file
    char *out = "/Users/shanjiang/Desktop/video.yuv";
    FILE *outfile = fopen(out, "wb+");
    if(!outfile) {
        printf("Error, Failed to open file!\n");
        goto __ERROR;
    }
    
    char *out_h264 = "/Users/shanjiang/Desktop/video.h264";
    FILE *outfile_h264 = fopen(out_h264, "wb+");
    if(!outfile_h264) {
        printf("Error, Failed to open file!\n");
        goto __ERROR;
    }

    // open device
    fmt_ctx = open_dev();
    if(!fmt_ctx) {
        printf("Error, failed to open device!\n");
        goto __ERROR;
    }
    
    // open codec
    open_encoder(640, 480, &enc_ctx);
    
    AVFrame* frame = NULL;
    frame = create_frame(640, 480);
    
    AVPacket* newpkt = NULL;
    newpkt = av_packet_alloc();
    if(!newpkt) {
        printf("Error, failed to alloc avpacket!\n");
        goto __ERROR;
    }
    
    AVPacket pkt;
    int ret = 0;
    int base = 0;
    
    while(rec_status) {
        ret = av_read_frame(fmt_ctx, &pkt);
        
        if(ret == -35) {
            sleep(0.01);
            continue;
        } else if(ret < 0) {
            break;
        }
        
        av_log(NULL, AV_LOG_INFO, "packet size is %d(%p)\n", pkt.size, pkt.data);
        
        //YYYYYYYYUVVU NV12
        //YYYYYYYYUUVV YUV420
        // data[0] --> Y
        memcpy(frame->data[0], pkt.data, 307200);
        // data[1][2] --> UV
        for(int i = 0; i < 307200/4; i++) {
            frame->data[1][i] = pkt.data[307200+i*2];
            frame->data[2][i] = pkt.data[307200+i*2+1];
        }
        
        // write to yuv file
        fwrite(frame->data[0], 1, 307200, outfile);
        fwrite(frame->data[1], 1, 307200/4, outfile);
        fwrite(frame->data[2], 1, 307200/4, outfile);
        
        // encode then write to h264 file
        frame->pts = base++;
        encode(enc_ctx, frame, newpkt, outfile_h264);
        
        av_packet_unref(&pkt);
    }
    
    encode(enc_ctx, NULL, newpkt, outfile);
    
__ERROR:
    if(frame) {
        av_frame_free(&frame);
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
    rec_video();
    return 0;
}
#endif



