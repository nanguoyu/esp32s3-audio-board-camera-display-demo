#include "board.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_rom_sys.h"
#include "led_strip.h"

static const char *TAG = "board";
i2c_master_bus_handle_t board_i2c;
static i2c_master_dev_handle_t expander, touch;
static esp_lcd_panel_handle_t panel;
static led_strip_handle_t ring;
static SemaphoreHandle_t lcd_done;
static uint16_t *stripe;
static uint16_t output_latch;
// Register values specified by the Waveshare 1.69-inch touch LCD example.
static const struct {uint8_t cmd, len, data[14];} lcd_init_commands[] = {
    {0xef,0,{0}}, {0xeb,1,{0x14}}, {0x36,1,{0}}, {0x3a,1,{0x05}},
    {0xb2,5,{0x0b,0x0b,0,0x33,0x35}}, {0xb7,1,{0x11}}, {0xbb,1,{0x35}},
    {0xc0,1,{0x2c}}, {0xc2,1,{1}}, {0xc3,1,{0x0d}}, {0xc4,1,{0x20}},
    {0xc6,1,{0x13}}, {0xd0,2,{0xa4,0xa1}}, {0xd6,1,{0xa1}},
    {0xe0,14,{0xf0,6,0x0b,0x0a,9,0x26,0x29,0x33,0x41,0x18,0x16,0x15,0x29,0x2d}},
    {0xe1,14,{0xf0,4,8,8,7,3,0x28,0x32,0x40,0x3b,0x19,0x18,0x2a,0x2e}},
    {0xe4,3,{0x25,0,0}}, {0x21,0,{0}}, {0x11,0,{0}},
};

static esp_err_t device(uint8_t address, i2c_master_dev_handle_t *handle)
{
    i2c_device_config_t cfg = {.dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address, .scl_speed_hz = 100000};
    return i2c_master_bus_add_device(board_i2c, &cfg, handle);
}

static esp_err_t read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, buf, len, 100);
}

static esp_err_t write16(uint8_t reg, uint16_t value)
{
    uint8_t data[] = {reg, value & 255, value >> 8};
    return i2c_master_transmit(expander, data, sizeof(data), 100);
}

esp_err_t board_expander_update(uint16_t mask, uint16_t value)
{
    uint16_t next = (output_latch & ~mask) | (value & mask);
    esp_err_t err = write16(2, next);
    if (err == ESP_OK) output_latch = next;
    return err;
}

void board_scan(void)
{
    for (int addr = 8; addr < 0x78; ++addr)
        if (i2c_master_probe(board_i2c, addr, 25) == ESP_OK)
            printf("I2C_FOUND 0x%02x\n", addr);
}

esp_err_t board_init(void)
{
    i2c_master_bus_config_t cfg = {.i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_11, .scl_io_num = GPIO_NUM_10,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true};
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&cfg, &board_i2c), TAG, "I2C bus");
    ESP_RETURN_ON_ERROR(device(0x20, &expander), TAG, "expander device");
    uint8_t regs[8];
    // TCA9555 auto-increment wraps within each two-register port pair.
    for (int reg=0; reg<8; reg+=2)
        ESP_RETURN_ON_ERROR(read_reg(expander, reg, &regs[reg], 2), TAG, "expander registers");
    printf("TCA9555 input=%02x%02x output=%02x%02x config=%02x%02x\n",
           regs[1],regs[0],regs[3],regs[2],regs[7],regs[6]);
    output_latch = regs[2] | (regs[3] << 8);
    // PCB EX1/EX2/EX3 correspond to expander bits 0/1/2.
    // The official demos/table identify mux bit 6; the schematic's mux note says 7.
    // Keep both HIGH for camera clock routing. Neither is routed to the user header;
    // the high selection uses 43/44 and preserves the native USB pins 19/20.
    uint16_t mask = 3 | (1u << 5) | (1u << 6) | (1u << 7) | (1u << 8);
    ESP_RETURN_ON_ERROR(board_expander_update(mask, 3 | (1u << 5) | (1u << 6) | (1u << 7)), TAG, "initial levels");
    uint16_t directions = (regs[6] | (regs[7] << 8)) & ~mask;
    directions |= (1u << 2) | (7u << 9); // Touch IRQ and the three buttons are inputs.
    ESP_RETURN_ON_ERROR(write16(6, directions), TAG, "expander directions");
    ESP_RETURN_ON_ERROR(board_expander_update(3, 0), TAG, "display reset low");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_RETURN_ON_ERROR(board_expander_update(3, 3), TAG, "display reset high");
    vTaskDelay(pdMS_TO_TICKS(150));
    led_strip_config_t led = {.strip_gpio_num = 38, .max_leds = 7,
        .led_model = LED_MODEL_WS2812, .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB};
    led_strip_rmt_config_t rmt = {.clk_src = RMT_CLK_SRC_DEFAULT, .resolution_hz = 10000000};
    ESP_RETURN_ON_ERROR(led_strip_new_rmt_device(&led, &rmt, &ring), TAG, "ring");
    board_ring(4, 4, 12);
    return ESP_OK;
}

void board_ring(uint8_t r, uint8_t g, uint8_t b)
{
    if (!ring) return;
    for (int i=0; i<7; ++i) led_strip_set_pixel(ring, i, r, g, b);
    led_strip_refresh(ring);
}

uint16_t board_buttons(void)
{
    uint8_t buf[2];
    if (read_reg(expander, 0, buf, 2) != ESP_OK) return 0;
    return (~(buf[0] | (buf[1] << 8)) >> 9) & 7;
}

esp_err_t board_touch_init(void)
{
    ESP_RETURN_ON_ERROR(device(0x15, &touch), TAG, "touch device");
    uint8_t id[3] = {0};
    ESP_RETURN_ON_ERROR(read_reg(touch, 0xa7, id, 3), TAG, "touch ID");
    printf("TOUCH_ID chip=0x%02x project=0x%02x firmware=0x%02x\n",id[0],id[1],id[2]);
    // CST816 DisAutoSleep: keep the controller available for coordinate polling.
    uint8_t awake[] = {0xfe, 1};
    return i2c_master_transmit(touch, awake, 2, 100);
}

bool board_touch_read(int *x, int *y)
{
    if (!touch) return false;
    uint8_t data[6];
    if (read_reg(touch, 1, data, sizeof(data)) != ESP_OK || !(data[1] & 0x0f)) return false;
    *x = ((data[2] & 0x0f) << 8) | data[3];
    *y = ((data[4] & 0x0f) << 8) | data[5];
    return *x < SCREEN_W && *y < SCREEN_H;
}

static bool lcd_finished(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *event, void *ctx)
{
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(lcd_done, &wake);
    return wake == pdTRUE;
}

static void slow_byte(uint8_t value)
{
    for(int bit=7;bit>=0;--bit) {
        gpio_set_level(4,0);
        gpio_set_level(9,(value>>bit)&1);
        esp_rom_delay_us(1);
        gpio_set_level(4,1);
        esp_rom_delay_us(1);
    }
    gpio_set_level(4,0);
}

static void slow_command(uint8_t cmd, const uint8_t *data, int len)
{
    gpio_set_level(3,0);gpio_set_level(7,0);slow_byte(cmd);
    gpio_set_level(7,1);
    for(int i=0;i<len;++i)slow_byte(data[i]);
    gpio_set_level(3,1);
}

static esp_err_t lcd_gpio_prepare(void)
{
    // Run only before creating the SPI bus. This isolates hardware SPI/DMA setup.
    gpio_config_t cfg={.pin_bit_mask=(1ULL<<3)|(1ULL<<4)|(1ULL<<5)|(1ULL<<7)|(1ULL<<9),
        .mode=GPIO_MODE_OUTPUT};
    ESP_RETURN_ON_ERROR(gpio_config(&cfg),TAG,"GPIO display test");
    gpio_set_level(3,1);gpio_set_level(4,0);gpio_set_level(5,1);
    ESP_RETURN_ON_ERROR(board_expander_update(1,0),TAG,"LCD hardware reset");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_RETURN_ON_ERROR(board_expander_update(1,1),TAG,"LCD release reset");
    vTaskDelay(pdMS_TO_TICKS(150));
    slow_command(0x01,NULL,0);
    vTaskDelay(pdMS_TO_TICKS(150));
    for(size_t i=0;i<sizeof(lcd_init_commands)/sizeof(lcd_init_commands[0]);++i)
        slow_command(lcd_init_commands[i].cmd,lcd_init_commands[i].data,lcd_init_commands[i].len);
    vTaskDelay(pdMS_TO_TICKS(150));
    slow_command(0x29,NULL,0);
    return ESP_OK;
}

esp_err_t board_lcd_gpio_test(void)
{
    ESP_RETURN_ON_ERROR(lcd_gpio_prepare(),TAG,"GPIO LCD startup");
    const uint8_t columns[]={0,0,0,239}, rows[]={0,20,1,43};
    slow_command(0x2a,columns,4);slow_command(0x2b,rows,4);
    gpio_set_level(3,0);gpio_set_level(7,0);slow_byte(0x2c);gpio_set_level(7,1);
    for(int y=0;y<280;++y) {
        for(int x=0;x<240;++x) {
            uint16_t color=x<80?0xf800:x<160?0x07e0:0x001f;
            if(x<4||x>235||y<4||y>275)color=0xffff;
            slow_byte(color>>8);slow_byte(color&255);
        }
        if(y%8==0)vTaskDelay(1);
    }
    gpio_set_level(3,1);
    puts("GPIO_LCD_TEST white border and red green blue bars, holding for 8 seconds");
    vTaskDelay(pdMS_TO_TICKS(8000));
    return ESP_OK;
}

esp_err_t board_lcd_init(void)
{
    // Send the module startup sequence through GPIO, then stream pixels over SPI.
    ESP_RETURN_ON_ERROR(lcd_gpio_prepare(),TAG,"LCD startup");
    lcd_done = xSemaphoreCreateBinary();
    stripe = heap_caps_malloc(SCREEN_W * 20 * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!lcd_done || !stripe) return ESP_ERR_NO_MEM;
    spi_bus_config_t bus = {.sclk_io_num = 4, .mosi_io_num = 9, .miso_io_num = -1,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = SCREEN_W * 20 * 2};
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO), TAG, "SPI");
    esp_lcd_panel_io_spi_config_t io_cfg = {.dc_gpio_num = 7, .cs_gpio_num = 3,
        .pclk_hz = 20000000, .lcd_cmd_bits = 8, .lcd_param_bits = 8,
        .spi_mode = 0, .trans_queue_depth = 1, .on_color_trans_done = lcd_finished,
        .cs_ena_pretrans = 2, .cs_ena_posttrans = 2};
    esp_lcd_panel_io_handle_t io;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_cfg, &io), TAG, "LCD IO");
    esp_lcd_panel_dev_config_t panel_cfg = {.reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB, .bits_per_pixel = 16};
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(io, &panel_cfg, &panel), TAG, "ST7789");
    // Preserve the controller state initialized above instead of resetting it again.
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(panel, 0, 20), TAG, "LCD gap");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel, true), TAG, "LCD inversion");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel, true), TAG, "LCD on");
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_5, 1);
    printf("LCD_READY 240x280 gap=0,20 spi=20000000 gpio_startup=1\n");
    return ESP_OK;
}

esp_err_t board_lcd_draw(const uint16_t *pixels)
{
    for (int y=0; y<SCREEN_H; y+=20) {
        for (int i=0; i<SCREEN_W*20; ++i) stripe[i] = __builtin_bswap16(pixels[y*SCREEN_W+i]);
        ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(panel, 0, y, SCREEN_W, y+20, stripe), TAG, "LCD write");
        if (xSemaphoreTake(lcd_done, pdMS_TO_TICKS(1000)) != pdTRUE) return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t board_camera_init(void)
{
    ESP_RETURN_ON_ERROR(board_expander_update(1u<<5, 0), TAG, "camera power");
    vTaskDelay(pdMS_TO_TICKS(30));
    camera_config_t cfg = {.pin_pwdn=-1, .pin_reset=-1, .pin_xclk=43,
        .pin_sccb_sda=-1, .pin_sccb_scl=-1, .sccb_i2c_port=0,
        .pin_d0=2, .pin_d1=17, .pin_d2=18, .pin_d3=39,
        .pin_d4=45, .pin_d5=46, .pin_d6=47, .pin_d7=48,
        .pin_vsync=21, .pin_href=1, .pin_pclk=44,
        .xclk_freq_hz=20000000, .ledc_timer=LEDC_TIMER_0, .ledc_channel=LEDC_CHANNEL_0,
        .pixel_format=PIXFORMAT_JPEG, .frame_size=FRAMESIZE_240X240, .jpeg_quality=12,
        .fb_count=2, .fb_location=CAMERA_FB_IN_PSRAM, .grab_mode=CAMERA_GRAB_LATEST};
    ESP_RETURN_ON_ERROR(esp_camera_init(&cfg), TAG, "camera init");
    sensor_t *s = esp_camera_sensor_get();
    s->set_hmirror(s,1);
    printf("CAMERA_ID pid=0x%04x ver=0x%02x\n",s->id.PID,s->id.VER);
    return ESP_OK;
}
