#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "audio.h"
#include "effects_ui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "jpeg_decoder.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"

static int effect;
static bool frozen,muted;
static int64_t flash_until;

static void change_effect(int step)
{
    effect=(effect+step+EFFECT_COUNT)%EFFECT_COUNT;
    app_audio_sound(1,effect);
    printf("ACTION effect=%d name=%s\n",effect,effect_names[effect]);
}
static void toggle_freeze(void)
{
    frozen=!frozen;
    flash_until=esp_timer_get_time()+180000;
    app_audio_sound(frozen?2:3,0);
    printf("ACTION frozen=%d\n",frozen);
}
static void toggle_mute(void)
{
    muted=!muted;app_audio_mute(muted);
    if(!muted)app_audio_sound(1,effect);
    printf("ACTION muted=%d\n",muted);
}
static void dump_hex(const char *kind,const uint8_t *bytes,size_t size)
{
    static const char hex[]="0123456789abcdef";
    printf("%s_BEGIN %u\n",kind,(unsigned)size);
    for(size_t offset=0;offset<size;offset+=64) {
        char line[129];size_t n=size-offset;if(n>64)n=64;
        for(size_t i=0;i<n;++i) {line[2*i]=hex[bytes[offset+i]>>4];line[2*i+1]=hex[bytes[offset+i]&15];}
        line[2*n]=0;puts(line);
    }
    printf("%s_END\n",kind);fflush(stdout);
}

void app_main(void)
{
    usb_serial_jtag_driver_config_t console={.rx_buffer_size=2048,.tx_buffer_size=8192};
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&console));
    usb_serial_jtag_vfs_use_driver();
    usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_LF);
    setvbuf(stdout,NULL,_IOLBF,0);
    puts("ESP32-S3 AUDIO BOARD CAMERA DISPLAY DEMO");
    ESP_ERROR_CHECK(board_init());
    esp_err_t touch_error=board_touch_init();
    printf("TOUCH_INIT %s\n",esp_err_to_name(touch_error));
    ESP_ERROR_CHECK(board_lcd_init());
    uint16_t *screen=heap_caps_calloc(SCREEN_W*SCREEN_H,2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    // Internal output reduced measured decode time from about 49 ms to 45 ms.
    // This 115,200-byte buffer leaves over 160 KB free with camera/audio active.
    uint16_t *camera=heap_caps_calloc(CAMERA_W*CAMERA_H,2,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
    if(!screen||!camera) {puts("ERROR framebuffer allocation");return;}
    effects_render(screen,camera,0,false,false,false,0,0,false);
    ESP_ERROR_CHECK(board_lcd_draw(screen));
    esp_err_t camera_error=board_camera_init();
    printf("CAMERA_INIT %s\n",esp_err_to_name(camera_error));
    esp_err_t audio_error=app_audio_init();
    printf("AUDIO_INIT %s\n",esp_err_to_name(audio_error));
    bool pressed=false,camera_ok=false,snapshot=false,screen_capture=false;
    int start_x=0,start_y=0,last_x=0,last_y=0,fps=0;
    unsigned frames=0,shown=0,decode_errors=0,capture_errors=0,touches=0;
    uint64_t capture_us=0,decode_us=0,render_us=0,draw_us=0;
    uint16_t previous_buttons=0;
    int64_t press_start=0,last_status=esp_timer_get_time();
    puts("APP_READY touch=effect/freeze swipe=effect hold=mute K1=effect K2=freeze K3=mute");
    while(true) {
        char command;
        while(usb_serial_jtag_read_bytes(&command,1,0)>0) {
            switch(command) {
                case 'n':change_effect(1);break;
                case 'b':change_effect(-1);break;
                case 'f':toggle_freeze();break;
                case 'm':toggle_mute();break;
                case 's':snapshot=true;break;
                case 'p':screen_capture=true;break;
                case 'i':board_scan();break;
                default:break;
            }
        }
        if(camera_error==ESP_OK) {
            int64_t capture_start=esp_timer_get_time();
            camera_fb_t *fb=esp_camera_fb_get();
            capture_us+=esp_timer_get_time()-capture_start;
            if(fb) {
                if(snapshot) {dump_hex("JPEG",fb->buf,fb->len);snapshot=false;}
                if(!frozen||!camera_ok) {
                    esp_jpeg_image_cfg_t cfg={.indata=fb->buf,.indata_size=fb->len,
                        .outbuf=(uint8_t*)camera,.outbuf_size=CAMERA_W*CAMERA_H*2,
                        .out_format=JPEG_IMAGE_FORMAT_RGB565,.out_scale=JPEG_IMAGE_SCALE_0};
                    esp_jpeg_image_output_t info={0};
                    int64_t decode_start=esp_timer_get_time();
                    esp_err_t err=esp_jpeg_decode(&cfg,&info);
                    decode_us+=esp_timer_get_time()-decode_start;
                    if(err==ESP_OK&&info.width==CAMERA_W&&info.height==CAMERA_H) {
                        camera_ok=true;++frames;
                    } else ++decode_errors;
                }
                esp_camera_fb_return(fb);
            } else ++capture_errors;
        }
        int x=0,y=0;
        bool down=touch_error==ESP_OK&&board_touch_read(&x,&y);
        if(down) {
            last_x=x;last_y=y;
            if(!pressed) {
                start_x=x;start_y=y;press_start=esp_timer_get_time();++touches;
                printf("TOUCH x=%d y=%d count=%u\n",x,y,touches);
            }
        } else if(pressed) {
            int dx=last_x-start_x,dy=last_y-start_y;
            if(abs(dx)>45&&abs(dx)>abs(dy))change_effect(dx<0?1:-1);
            else if(esp_timer_get_time()-press_start>850000)toggle_mute();
            else if(last_y>=240&&last_x<120)change_effect(1);
            else toggle_freeze();
        }
        pressed=down;
        uint16_t buttons=board_buttons(),edges=buttons&~previous_buttons;
        if(edges&1)change_effect(1);
        if(edges&2)toggle_freeze();
        if(edges&4)toggle_mute();
        previous_buttons=buttons;
        int level=app_audio_level();
        bool flash=esp_timer_get_time()<flash_until;
        int64_t render_start=esp_timer_get_time();
        effects_render(screen,camera,effect,camera_ok,frozen,muted,level,frozen?0:fps,flash);
        render_us+=esp_timer_get_time()-render_start;
        int64_t draw_start=esp_timer_get_time();
        ESP_ERROR_CHECK(board_lcd_draw(screen));++shown;
        draw_us+=esp_timer_get_time()-draw_start;
        if(screen_capture) {dump_hex("UI",(uint8_t*)screen,SCREEN_W*SCREEN_H*2);screen_capture=false;}
        if(flash)board_ring(45,45,45);
        else {
            int brightness=12+level/3;
            board_ring(effect_colors[effect][0]*brightness/255,
                       effect_colors[effect][1]*brightness/255,
                       effect_colors[effect][2]*brightness/255);
        }
        int64_t now=esp_timer_get_time(),elapsed=now-last_status;
        if(elapsed>=5000000) {
            int fps10=(int)((uint64_t)shown*10000000/elapsed);fps=(fps10+5)/10;
            printf("STATUS frames=%u fps=%d.%d decode_errors=%u capture_errors=%u touches=%u effect=%d frozen=%d muted=%d mic=%d audio_blocks=%u heap=%u\n",
                frames,fps10/10,fps10%10,decode_errors,capture_errors,touches,effect,frozen,muted,level,
                (unsigned)app_audio_blocks(),(unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
            printf("TIMING_US capture=%u decode=%u render=%u lcd=%u\n",
                (unsigned)(capture_us/shown),(unsigned)(decode_us/shown),
                (unsigned)(render_us/shown),(unsigned)(draw_us/shown));
            capture_us=decode_us=render_us=draw_us=0;
            shown=0;last_status=now;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
