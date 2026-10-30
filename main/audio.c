#include "audio.h"
#include "board.h"
#include <math.h>
#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "esp_check.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

#define RATE 16000
#define BLOCK 256
static const char *TAG="prism_audio";
static esp_codec_dev_handle_t output, input;
static bool input_ready;
static QueueHandle_t sounds;
static atomic_bool muted;
static atomic_uint mic_level, blocks;
typedef struct {int kind, variation;} sound_t;

void app_audio_sound(int kind,int variation)
{
    sound_t sound={kind,variation};
    if(sounds)xQueueSend(sounds,&sound,0);
}
void app_audio_mute(bool value) {atomic_store(&muted,value);}
uint16_t app_audio_level(void) {return atomic_load(&mic_level);}
uint32_t app_audio_blocks(void) {return atomic_load(&blocks);}

static void audio_task(void *unused)
{
    int16_t microphones[BLOCK*4], pcm[BLOCK];
    sound_t sound={0};
    unsigned age=0,duration=0;
    float phase=0;
    while(true) {
        if(input_ready&&esp_codec_dev_read(input,microphones,sizeof(microphones))==ESP_CODEC_DEV_OK) {
            int32_t sum[2]={0};
            for(int i=0;i<BLOCK;++i)for(int c=0;c<2;++c)sum[c]+=microphones[i*4+c];
            uint64_t energy=0;
            for(int i=0;i<BLOCK;++i)for(int c=0;c<2;++c) {
                int32_t value=microphones[i*4+c]-sum[c]/BLOCK;
                energy+=(int64_t)value*value;
            }
            float rms=sqrtf((float)energy/(BLOCK*2));
            // Logarithmic response makes ordinary speech visible without requiring shouting.
            unsigned level=(unsigned)(55.0f*log10f(fmaxf(1.0f,rms/12.0f)));
            if(level>100)level=100;
            unsigned previous=atomic_load(&mic_level);
            atomic_store(&mic_level,level>previous?level:(previous*3+level)/4);
            atomic_fetch_add(&blocks,1);
        } else vTaskDelay(pdMS_TO_TICKS(16));
        if(xQueueReceive(sounds,&sound,0)==pdTRUE) {
            age=0;phase=0;duration=sound.kind==4?RATE*24/100:RATE*12/100;
        }
        for(int i=0;i<BLOCK;++i) {
            float value=0;
            if(age<duration&&!atomic_load(&muted)) {
                float frequency=440+sound.variation*110;
                if(sound.kind==2)frequency=age<duration/2?880:1320;
                if(sound.kind==3)frequency=660;
                if(sound.kind==4)frequency=550+(age/(RATE*8/100))*275;
                float envelope=fminf(1.0f,(float)age/80)*((float)(duration-age)/duration);
                value=sinf(phase)*7000*envelope;
                phase+=6.28318530718f*frequency/RATE;
                if(phase>6.28318530718f)phase-=6.28318530718f;
            }
            pcm[i]=(int16_t)value;
            if(age<duration)++age;
        }
        if(esp_codec_dev_write(output,pcm,sizeof(pcm))!=ESP_CODEC_DEV_OK)
            vTaskDelay(pdMS_TO_TICKS(16));
    }
}

esp_err_t app_audio_init(void)
{
    i2s_chan_handle_t tx=NULL,rx=NULL;
    i2s_chan_config_t channel=I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0,I2S_ROLE_MASTER);
    channel.dma_desc_num=6;channel.dma_frame_num=BLOCK;channel.auto_clear_after_cb=true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&channel,&tx,&rx),TAG,"I2S channels");
    i2s_std_config_t playback={
        .clk_cfg=I2S_STD_CLK_DEFAULT_CONFIG(RATE),
        .slot_cfg=I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,I2S_SLOT_MODE_STEREO),
        .gpio_cfg={.mclk=12,.bclk=13,.ws=14,.dout=16,.din=I2S_GPIO_UNUSED},
    };
    i2s_tdm_config_t recording={
        .clk_cfg=I2S_TDM_CLK_DEFAULT_CONFIG(RATE),
        .slot_cfg=I2S_TDM_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,I2S_SLOT_MODE_STEREO,
            I2S_TDM_SLOT0|I2S_TDM_SLOT1|I2S_TDM_SLOT2|I2S_TDM_SLOT3),
        .gpio_cfg={.mclk=12,.bclk=13,.ws=14,.dout=I2S_GPIO_UNUSED,.din=15},
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx,&playback),TAG,"playback");
    ESP_RETURN_ON_ERROR(i2s_channel_init_tdm_mode(rx,&recording),TAG,"microphones");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx),TAG,"TX enable");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(rx),TAG,"RX enable");
    audio_codec_i2s_cfg_t data_cfg={.port=0,.rx_handle=rx,.tx_handle=tx};
    const audio_codec_data_if_t *data=audio_codec_new_i2s_data(&data_cfg);
    audio_codec_i2c_cfg_t control={.port=0,.addr=ES8311_CODEC_DEFAULT_ADDR,.bus_handle=board_i2c};
    const audio_codec_ctrl_if_t *out_ctrl=audio_codec_new_i2c_ctrl(&control);
    const audio_codec_gpio_if_t *gpio=audio_codec_new_gpio();
    if(!data||!out_ctrl||!gpio)return ESP_ERR_NO_MEM;
    es8311_codec_cfg_t dac={.ctrl_if=out_ctrl,.gpio_if=gpio,.codec_mode=ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin=-1,.use_mclk=true,.hw_gain={.pa_voltage=5.0,.codec_dac_voltage=3.3}};
    const audio_codec_if_t *out_codec=es8311_codec_new(&dac);
    if(!out_codec)return ESP_FAIL;
    esp_codec_dev_cfg_t dev={.dev_type=ESP_CODEC_DEV_TYPE_OUT,.codec_if=out_codec,.data_if=data};
    output=esp_codec_dev_new(&dev);
    esp_codec_dev_sample_info_t fs={.bits_per_sample=16,.channel=1,.sample_rate=RATE};
    if(!output||esp_codec_dev_open(output,&fs)!=ESP_CODEC_DEV_OK)return ESP_FAIL;
    esp_codec_dev_set_out_vol(output,65);

    control.addr=ES7210_CODEC_DEFAULT_ADDR;
    const audio_codec_ctrl_if_t *in_ctrl=audio_codec_new_i2c_ctrl(&control);
    if(in_ctrl) {
        es7210_codec_cfg_t adc={.ctrl_if=in_ctrl,.mic_selected=ES7210_SEL_MIC1|ES7210_SEL_MIC2|ES7210_SEL_MIC3|ES7210_SEL_MIC4};
        const audio_codec_if_t *in_codec=es7210_codec_new(&adc);
        if(in_codec) {
            dev.dev_type=ESP_CODEC_DEV_TYPE_IN;dev.codec_if=in_codec;
            input=esp_codec_dev_new(&dev);
            fs.channel=4;fs.channel_mask=0;
            input_ready=input&&esp_codec_dev_open(input,&fs)==ESP_CODEC_DEV_OK;
            if(input_ready)esp_codec_dev_set_in_gain(input,24);
        }
    }
    sounds=xQueueCreate(8,sizeof(sound_t));
    if(!sounds)return ESP_ERR_NO_MEM;
    ESP_RETURN_ON_ERROR(board_expander_update(1u<<8,1u<<8),TAG,"speaker amplifier");
    if(xTaskCreatePinnedToCore(audio_task,"prism_sound",6144,NULL,4,NULL,1)!=pdPASS) {
        board_expander_update(1u<<8,0);return ESP_ERR_NO_MEM;
    }
    printf("AUDIO_READY speaker=1 microphones=%d rate=%d\n",input_ready,RATE);
    app_audio_sound(4,0);
    return ESP_OK;
}
