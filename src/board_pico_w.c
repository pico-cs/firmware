// board_pico_w: add to executables only in pico_w case

#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"

#include "board.h"

#define WIFI_TEXT "WiFi SSID " WIFI_SSID " password " WIFI_PASSWORD
#define TCP_TEXT  "TCP port " TCP_PORT_STRING

bool board_init(board_t *board, writer_t *logger) {
    bi_decl(bi_1pin_with_name(CYW43_WL_GPIO_LED_PIN, "CYW43 LED"));

    bi_decl(bi_program_feature(WIFI_TEXT));
    bi_decl(bi_program_feature(TCP_TEXT));

    board_init_common(board, logger);

    if (cyw43_arch_init()) {
        write_event(logger, "wifi: failed to initialize cyw43");
        return false;
    }

    // enable led during bootstrap
    board_set_led(board, true);

    cyw43_arch_enable_sta_mode();

    // MAC adddress
    uint8_t mac[MAC_SIZE_BYTES];
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac);
    uint8_hex_to_string(mac, MAC_SIZE_BYTES, board->mac, ':');

    return true;
}

void board_deinit(board_t *board) {
    board_set_led(board, false);
    cyw43_arch_deinit();
}

void board_set_led(board_t *board, bool v) {cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, v);}

bool board_get_led(board_t *board) {return cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN);}
