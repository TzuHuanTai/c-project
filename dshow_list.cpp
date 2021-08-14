#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
extern "C"
{
#include <libavdevice/avdevice.h>
}

int main()
{
    avdevice_register_all();
    AVInputFormat *iformat = av_find_input_format("dshow");
    printf("========Device Info=============\n");
    AVDeviceInfoList *device_list = NULL;
    AVDictionary *options = NULL;
    //av_dict_set(&options, "list_devices", "true", 0);
    int result = avdevice_list_input_sources(iformat, NULL, options, &device_list);

    if (result < 0)
        // printf("Error Code:%s\n", av_err2str(result)); //Returns -40 AVERROR(ENOSYS)
        printf("result < 0");
    else
        printf("Devices count:%d\n", result);

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    AVDictionary *options2 = NULL;
    av_dict_set(&options2, "list_devices", "true", 0);
    avformat_open_input(&pFormatCtx, NULL, iformat, &options2);
    printf("================================\n");
}