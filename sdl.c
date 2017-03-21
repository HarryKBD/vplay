
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
#include <libswscale/swscale.h>

#include "buf.h"

struct buffer_data {
    uint8_t *ptr;
    size_t size; ///< size left in the buffer
};

static int read_packet2(void *opaque, uint8_t *buf, int buf_size)
{
    struct buffer_data *bd = (struct buffer_data *)opaque;
    buf_size = FFMIN(buf_size, bd->size);

    printf("ptr:%p size:%zu\n", bd->ptr, bd->size);

    /* copy internal buffer data to buf */
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr  += buf_size;
    bd->size -= buf_size;

    return buf_size;
}



static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    struct buffer_data *bd = (struct buffer_data *)opaque;
    int ret;

    /* copy internal buffer data to buf */
    ret = get_data(buf, buf_size);
    //ret = get_frame(buf, buf_size);

    printf("cur_data_sz %d, readbuf size %d read_len %d\n", get_data_cnt(), buf_size, ret);

    return ret;
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int  y;
  
  // Open file
  sprintf(szFilename, "frame%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;
  
  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
  
  // Write pixel data
  for(y=0; y<height; y++)
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
  
  // Close file
  fclose(pFile);
}


void sdl_main(){
    AVFormatContext *fmt_ctx = NULL;
    AVIOContext *avio_ctx = NULL;
    uint8_t *buffer = NULL, *avio_ctx_buffer = NULL;
    size_t buffer_size, avio_ctx_buffer_size = 100000;
    char *input_filename = NULL;
    int ret = 0;
    struct buffer_data bd = { 0 };


  int               i, videoStream;
  AVCodecContext    *pCodecCtxOrig = NULL;
  AVCodecContext    *pCodecCtx = NULL, *c;
  AVCodec           *pCodec = NULL, *codec;
  AVFrame           *pFrame = NULL;
  AVFrame           *pFrameRGB = NULL;
  AVPacket          packet;
  int               frameFinished;
  int               numBytes;
  uint8_t           *buffer2 = NULL;
  struct SwsContext *sws_ctx = NULL;


    /* register codecs and formats and other lavf/lavc components*/
    av_register_all();

    /* fill opaque structure used by the AVIOContext read callback */
    bd.ptr  = (uint8_t *)get_buf_ptr();
    bd.size = (size_t)get_buf_size();

    ///////
    ret = av_file_map("test.h264", &buffer, &buffer_size, 0, NULL);
    if(ret < 0) goto end;

    bd.ptr = buffer;
    bd.size = buffer_size;

    /////


    if (!(fmt_ctx = avformat_alloc_context())) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size,
                                  0, &bd, &read_packet2, NULL, NULL);
    if (!avio_ctx) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    fmt_ctx->pb = avio_ctx;

    ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open input\n");
        goto end;
    }

    codec = avcodec_find_decoder(AV_CODEC_ID_H263);
    if(!codec){
        fprintf(stderr, "Could not open codec\n");
        goto end;
    }

    c = avcodec_alloc_context3(codec);
    if(!c){
        fprintf(stderr, "Could not alloc codec\n");
        goto end;
    }
    fmt_ctx->streams[0]->codec = codec;
   // fmt_ctx->streams[0]->codec->framerate = 5;

    if(avcodec_open2(c, fmt_ctx->streams[0]->codec, NULL) < 0){
        fprintf(stderr, "Could not open avcodec2\n");
        goto end;
    }
    
    /*
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        goto end;
    }
    */

    av_dump_format(fmt_ctx, 0, "test", 0);
    
    printf("DUMP Done\n");


  // Find the first video stream
  videoStream=-1;
  for(i=0; i<fmt_ctx->nb_streams; i++)
    if(fmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
  if(videoStream==-1)
    return; // Didn't find a video stream
  
  // Get a pointer to the codec context for the video stream
  pCodecCtxOrig=fmt_ctx->streams[videoStream]->codec;
  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
  if(pCodec==NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    return; // Codec not found
  }
  // Copy context
  pCodecCtx = avcodec_alloc_context3(pCodec);
  if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
    fprintf(stderr, "Couldn't copy codec context");
    return; // Error copying codec context
  }

  // Open codec
  if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
    return; // Could not open codec
  
  // Allocate video frame
  pFrame=av_frame_alloc();
  
  // Allocate an AVFrame structure
  pFrameRGB=av_frame_alloc();
  if(pFrameRGB==NULL)
    return;

  // Determine required buffer2 size and allocate buffer2
  numBytes=avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
			      pCodecCtx->height);
  buffer2=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
  
  // Assign appropriate parts of buffer2 to image planes in pFrameRGB
  // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
  // of AVPicture
  avpicture_fill((AVPicture *)pFrameRGB, buffer2, AV_PIX_FMT_RGB24,
		 pCodecCtx->width, pCodecCtx->height);
  
  // initialize SWS context for software scaling
  sws_ctx = sws_getContext(pCodecCtx->width,
			   pCodecCtx->height,
			   pCodecCtx->pix_fmt,
			   pCodecCtx->width,
			   pCodecCtx->height,
			   AV_PIX_FMT_RGB24,
			   SWS_BILINEAR,
			   NULL,
			   NULL,
			   NULL
			   );

  // Read frames and save first five frames to disk
  i=0;
  while(av_read_frame(fmt_ctx, &packet)>=0) {
    printf("read pass\n");
    // Is this a packet from the video stream?
    if(packet.stream_index==videoStream) {
      // Decode video frame
      avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
      
      // Did we get a video frame?
      if(frameFinished) {
	// Convert the image from its native format to RGB
	sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
		  pFrame->linesize, 0, pCodecCtx->height,
		  pFrameRGB->data, pFrameRGB->linesize);
	
	// Save the frame to disk
	if(++i<=5){
	  SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
      }
       else{
        printf("skipping saving frame %d\n", i);
      }
    


      }
    
      }
   }
 





end:
    avformat_close_input(&fmt_ctx);
    /* note: the internal buffer could have changed, and be != avio_ctx_buffer */
    if (avio_ctx) {
        av_freep(&avio_ctx->buffer);
        av_freep(&avio_ctx);
    }
    printf("exit sdl main\n");
    return;
}


