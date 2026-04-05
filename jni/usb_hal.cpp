// usb_hal.cpp — USB OTG hardware backend
// Implements the hal_* functions declared in hal/Arduino.h, hal/SPI.h, hal/HardwareSerial.h
// Called from the HAL shim headers; called from Kotlin via JNI for device init.
//
// CH341A device #1  ->  NRF24L01+PA+LNA  (SPI + CE/CSN GPIO)
// CH341A device #2  ->  CC1101 HW-863    (SPI + GDO0/GDO2 GPIO)
// CP2102            ->  GPS GT-U7        (UART 9600 baud)

#include <jni.h>
#include <libusb.h>
#include <cstring>
#include <cstdint>
#include <android/log.h>

#define TAG "HaleHound_USB"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// ── CH341A USB command constants ──────────────────────────────────────────────
#define CH341_VENDOR_IN         0xC0
#define CH341_VENDOR_OUT        0x40
#define CH341_SET_OUTPUT        0xA1
#define CH341_SET_SCL_FREQ      0x9A
#define CH341_SPI_STREAM        0xA8
#define CH341_GPIO_CMD          0xA1
#define CH341_EP_OUT            0x02
#define CH341_EP_IN             0x82
#define CH341_TIMEOUT_MS        500

// ── ESP32 GPIO -> CH341 mapping ────────────────────────────────────────────────
// NRF24 lives on CH341 device index 0
// CC1101 lives on CH341 device index 1
// GPIO bits on CH341: bit0=CS0, bit1=GPIO0(CE/GDO0), bit2=GPIO1(IRQ/GDO2), bit3=GPIO2
#define GPIO_NRF24_CS    (1 << 0)   // CH341 CS0
#define GPIO_NRF24_CE    (1 << 1)   // CH341 GPIO0
#define GPIO_NRF24_IRQ   (1 << 2)   // CH341 GPIO1 (input)
#define GPIO_CC1101_CS   (1 << 0)   // CH341 CS0 on device #2
#define GPIO_CC1101_GDO0 (1 << 1)   // CH341 GPIO2
#define GPIO_CC1101_GDO2 (1 << 2)   // CH341 GPIO3 (input)

static libusb_context*       g_ctx          = nullptr;
static libusb_device_handle* g_ch341_nrf24  = nullptr;  // device #1
static libusb_device_handle* g_ch341_cc1101 = nullptr;  // device #2
static libusb_device_handle* g_cp2102       = nullptr;  // GPS UART

static uint8_t g_gpio_nrf24  = 0xFF;  // current GPIO output state for NRF24 CH341
static uint8_t g_gpio_cc1101 = 0xFF;  // current GPIO output state for CC1101 CH341

// ── Internal helpers ──────────────────────────────────────────────────────────
static libusb_device_handle* ch341_for_pin(uint8_t esp32_pin) {
    switch (esp32_pin) {
        case 4: case 16: case 17: case 18: case 19: case 23:
            return g_ch341_nrf24;
        case 27: case 22: case 35:
            return g_ch341_cc1101;
        default:
            return nullptr;
    }
}

static int ch341_ctrl_out(libusb_device_handle* h, uint8_t req,
                          uint16_t val, uint16_t idx,
                          uint8_t* data, uint16_t len) {
    return libusb_control_transfer(h, CH341_VENDOR_OUT, req, val, idx,
                                   data, len, CH341_TIMEOUT_MS);
}

static void ch341_set_gpio_state(libusb_device_handle* h, uint8_t state, uint8_t* cached) {
    if (!h) return;
    uint8_t buf[3] = { CH341_SET_OUTPUT, state, (uint8_t)(~state) };
    int r = libusb_control_transfer(h, CH341_VENDOR_OUT, CH341_GPIO_CMD,
                                    0, 0, buf, 3, CH341_TIMEOUT_MS);
    if (r >= 0) *cached = state;
}

static void ch341_spi_init(libusb_device_handle* h) {
    if (!h) return;
    // Set SPI mode 0, MSB first, ~4 MHz
    uint8_t cfg[3] = { 0x60, 0x00, 0x00 };
    ch341_ctrl_out(h, CH341_SET_SCL_FREQ, 0, 0, cfg, 3);
    // Assert all CS lines high
    ch341_set_gpio_state(h, 0xFF, &g_gpio_nrf24);
    LOGI("CH341A SPI init complete");
}

// ── JNI init — called from Kotlin HalBridge when USB devices are opened ───────
extern "C" JNIEXPORT void JNICALL
Java_com_halehound_HalBridge_nativeInitCH341NRF24(JNIEnv*, jobject, jint fd) {
    if (!g_ctx) libusb_init(&g_ctx);
    if (libusb_wrap_sys_device(g_ctx, (intptr_t)fd, &g_ch341_nrf24) < 0) {
        LOGE("Failed to wrap CH341 (NRF24) fd=%d", fd);
        return;
    }
    libusb_claim_interface(g_ch341_nrf24, 0);
    ch341_spi_init(g_ch341_nrf24);
    LOGI("CH341A #1 (NRF24) ready, fd=%d", fd);
}

extern "C" JNIEXPORT void JNICALL
Java_com_halehound_HalBridge_nativeInitCH341CC1101(JNIEnv*, jobject, jint fd) {
    if (!g_ctx) libusb_init(&g_ctx);
    if (libusb_wrap_sys_device(g_ctx, (intptr_t)fd, &g_ch341_cc1101) < 0) {
        LOGE("Failed to wrap CH341 (CC1101) fd=%d", fd);
        return;
    }
    libusb_claim_interface(g_ch341_cc1101, 0);
    ch341_spi_init(g_ch341_cc1101);
    g_gpio_cc1101 = 0xFF;
    LOGI("CH341A #2 (CC1101) ready, fd=%d", fd);
}

extern "C" JNIEXPORT void JNICALL
Java_com_halehound_HalBridge_nativeInitCP2102(JNIEnv*, jobject, jint fd) {
    if (!g_ctx) libusb_init(&g_ctx);
    if (libusb_wrap_sys_device(g_ctx, (intptr_t)fd, &g_cp2102) < 0) {
        LOGE("Failed to wrap CP2102 fd=%d", fd);
        return;
    }
    libusb_claim_interface(g_cp2102, 0);
    LOGI("CP2102 (GPS) ready, fd=%d", fd);
}

// ── hal_gpio — called from Arduino.h shim ────────────────────────────────────
extern "C" void hal_digitalWrite(uint8_t pin, uint8_t val) {
    // Map ESP32 GPIO numbers to CH341 GPIO bits and update the correct device
    auto set_bit = [](uint8_t* state, uint8_t bit, uint8_t v) {
        if (v) *state |= bit; else *state &= ~bit;
    };
    switch (pin) {
        case 16: // NRF24 CE
            set_bit(&g_gpio_nrf24, GPIO_NRF24_CE, val);
            ch341_set_gpio_state(g_ch341_nrf24, g_gpio_nrf24, &g_gpio_nrf24);
            break;
        case 4: // NRF24 CSN
            set_bit(&g_gpio_nrf24, GPIO_NRF24_CS, val);
            ch341_set_gpio_state(g_ch341_nrf24, g_gpio_nrf24, &g_gpio_nrf24);
            break;
        case 27: // CC1101 CS
            set_bit(&g_gpio_cc1101, GPIO_CC1101_CS, val);
            ch341_set_gpio_state(g_ch341_cc1101, g_gpio_cc1101, &g_gpio_cc1101);
            break;
        case 22: // CC1101 GDO0
            set_bit(&g_gpio_cc1101, GPIO_CC1101_GDO0, val);
            ch341_set_gpio_state(g_ch341_cc1101, g_gpio_cc1101, &g_gpio_cc1101);
            break;
        default:
            break;
    }
}

extern "C" uint8_t hal_digitalRead(uint8_t pin) {
    // Read GPIO input bits from CH341 status register
    uint8_t buf[1] = {0};
    switch (pin) {
        case 17: { // NRF24 IRQ
            if (g_ch341_nrf24)
                libusb_control_transfer(g_ch341_nrf24, CH341_VENDOR_IN,
                    0xA0, 0, 0, buf, 1, CH341_TIMEOUT_MS);
            return (buf[0] & GPIO_NRF24_IRQ) ? HIGH : LOW;
        }
        case 35: { // CC1101 GDO2
            if (g_ch341_cc1101)
                libusb_control_transfer(g_ch341_cc1101, CH341_VENDOR_IN,
                    0xA0, 0, 0, buf, 1, CH341_TIMEOUT_MS);
            return (buf[0] & GPIO_CC1101_GDO2) ? HIGH : LOW;
        }
        default: return LOW;
    }
}

extern "C" void hal_pinMode(uint8_t pin, uint8_t mode) {
    // CH341A GPIO direction is configured at open time; this is a no-op
    (void)pin; (void)mode;
}

// ── hal_spi — called from SPI.h shim ─────────────────────────────────────────
// The firmware's spi_manager deselects all CS lines before selecting one,
// so we track the currently active CH341 handle.
static libusb_device_handle* g_active_spi = nullptr;

extern "C" void hal_spi_cs_low(uint8_t csPin) {
    g_active_spi = ch341_for_pin(csPin);
    hal_digitalWrite(csPin, LOW);
}

extern "C" void hal_spi_cs_high(uint8_t csPin) {
    hal_digitalWrite(csPin, HIGH);
    if (g_active_spi == ch341_for_pin(csPin)) g_active_spi = nullptr;
}

extern "C" uint8_t hal_spi_transfer(uint8_t data) {
    libusb_device_handle* h = g_active_spi ? g_active_spi : g_ch341_nrf24;
    if (!h) return 0xFF;

    uint8_t cmd[2] = { CH341_SPI_STREAM, data };
    uint8_t rx[1]  = { 0xFF };
    int transferred = 0;
    libusb_bulk_transfer(h, CH341_EP_OUT, cmd, 2, &transferred, CH341_TIMEOUT_MS);
    libusb_bulk_transfer(h, CH341_EP_IN,  rx,  1, &transferred, CH341_TIMEOUT_MS);
    return rx[0];
}

extern "C" void hal_spi_transfer_buf(const uint8_t* tx, uint8_t* rx, size_t len) {
    for (size_t i = 0; i < len; i++) rx[i] = hal_spi_transfer(tx[i]);
}

extern "C" void hal_spi_set_config(uint32_t clockHz, uint8_t mode) {
    // Map clock frequency to CH341A divider — approximate
    // CH341A supports 1.5/3/6/12 MHz steps
    uint8_t div = (clockHz >= 8000000) ? 0 :
                  (clockHz >= 4000000) ? 1 :
                  (clockHz >= 2000000) ? 2 : 3;
    uint8_t cfg[3] = { (uint8_t)(0x60 | (mode & 0x03)), div, 0x00 };
    auto apply = [&](libusb_device_handle* h) {
        if (h) ch341_ctrl_out(h, CH341_SET_SCL_FREQ, 0, 0, cfg, 3);
    };
    apply(g_ch341_nrf24);
    apply(g_ch341_cc1101);
}

// ── hal_uart — CP2102 GPS UART backend ───────────────────────────────────────
// CP2102 EP 0x81 = bulk IN (device->host), EP 0x01 = bulk OUT (host->device)
#define CP2102_EP_IN  0x81
#define CP2102_EP_OUT 0x01
#define CP2102_BAUD_CMD 0x1E  // SET_BAUDRATE vendor request

extern "C" void hal_uart_begin(int port, uint32_t baud) {
    (void)port;
    if (!g_cp2102) { LOGE("CP2102 not initialised"); return; }
    // CP2102 set baud rate via vendor control transfer
    uint32_t b = baud;
    libusb_control_transfer(g_cp2102, 0x40, CP2102_BAUD_CMD,
                            0, 0, (uint8_t*)&b, 4, CH341_TIMEOUT_MS);
    LOGI("CP2102 UART baud=%u", baud);
}

extern "C" void hal_uart_end(int port)     { (void)port; }
extern "C" void hal_uart_flush(int port)   { (void)port; }

static uint8_t uart_buf[256];
static int     uart_buf_len = 0;
static int     uart_buf_pos = 0;

extern "C" int hal_uart_available(int port) {
    (void)port;
    if (!g_cp2102) return 0;
    if (uart_buf_pos < uart_buf_len) return uart_buf_len - uart_buf_pos;
    int transferred = 0;
    int r = libusb_bulk_transfer(g_cp2102, CP2102_EP_IN,
                                 uart_buf, sizeof(uart_buf),
                                 &transferred, 50);
    if (r == 0 && transferred > 0) {
        uart_buf_len = transferred;
        uart_buf_pos = 0;
        return transferred;
    }
    return 0;
}

extern "C" int hal_uart_read(int port) {
    (void)port;
    if (uart_buf_pos >= uart_buf_len) {
        if (hal_uart_available(port) == 0) return -1;
    }
    return uart_buf[uart_buf_pos++];
}

extern "C" void hal_uart_write(int port, uint8_t b) {
    (void)port;
    if (!g_cp2102) return;
    int transferred = 0;
    libusb_bulk_transfer(g_cp2102, CP2102_EP_OUT, &b, 1,
                         &transferred, CH341_TIMEOUT_MS);
}
