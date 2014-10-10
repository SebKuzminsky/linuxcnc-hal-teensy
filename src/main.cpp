#include "WProgram.h"

int led_state = LOW;


void led_on(void) {
    digitalWriteFast(13, HIGH);
}


void led_off(void) {
    digitalWriteFast(13, LOW);
}


void blink(int count, int ms_delay) {
    led_off();
    delay(ms_delay);
    while (count > 0) {
        led_on();
        delay(ms_delay);
        led_off();
        delay(ms_delay);
        count --;
    }
    digitalWriteFast(13, led_state);
}


void usb_send_packet(int addr, uint16_t data) {
    uint8_t packet[3];

    // remember what we've sent, and dont sent it again
    static bool have_sent_data[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    static uint16_t sent_data[8];

    if ((have_sent_data[addr]) && (sent_data[addr] == data)) {
        return;
    }

    have_sent_data[addr] = 1;
    sent_data[addr] = data;

    packet[0] = 0x00;
    packet[0] |= (addr & 0x07) << 4;
    packet[0] |= (data & 0xf000) >> 12;

    packet[1] = 0x80;
    packet[1] |= (data & 0x0fc0) >> 6;

    packet[2] = 0xc0;
    packet[2] |= data & 0x003f;

    usb_serial_write(packet, 3);
    usb_serial_flush_output();
}


void handle_incoming_packet(uint8_t buf[3]) {
    uint8_t addr;
    uint16_t data;

    addr = (buf[0] >> 4) & 0x07;
    data = ((uint16_t)(buf[0] & 0x0f)) << 12;
    data |= (buf[1] & 0x3f) << 6;
    data |= (buf[2] & 0x3f);

    switch (addr) {
        default:
            // unhandled packet!
            blink(4, 100);
            break;
    }
}


void read_usb(void) {
    // the buffer is valid up to but not including index
    static int index = 0;
    static uint8_t buf[3];

    int new_index;
    uint8_t new_bytes[3];

    int bytes_avail;
    int bytes_wanted;
    int bytes_read;

    bytes_wanted = 3 - index;

    bytes_avail = usb_serial_available();
    if (bytes_avail == 0) {
        return;
    }

    if (bytes_avail < bytes_wanted) {
        bytes_wanted = bytes_avail;
    }

    bytes_read = usb_serial_read(new_bytes, bytes_wanted);
    new_index = 0;

    while (new_index < bytes_read) {
        switch (index) {
            case 0:
                if ((new_bytes[new_index] & 0x80) == 0x00) {
                    buf[index] = new_bytes[new_index];
                    index ++;
                    new_index ++;
                } else {
                    // invalid byte in stream, discard the packet we were assembling
                    index = 0;
                    new_index ++;
                }
                break;

            case 1:
                if ((new_bytes[new_index] & 0xc0) == 0x80) {
                    buf[index] = new_bytes[new_index];
                    index ++;
                    new_index ++;
                } else {
                    // invalid byte in stream, discard the packet we were assembling
                    index = 0;
                }
                break;

            case 2:
                if ((new_bytes[new_index] & 0xc0) == 0xc0) {
                    buf[index] = new_bytes[new_index];
                    // blink(2, 150);
                    handle_incoming_packet(buf);
                    index = 0;
                    new_index ++;
                } else {
                    // invalid byte in stream, discard the packet we were assembling
                    index = 0;
                }
                break;

            default:
                // internal error
                // blink(5, 250);
                index = 0;
                break;
        }
    }
}


extern "C" int main(void) {
    // LED (acts like a digital output)
    pinMode(13, OUTPUT);

    blink(2, 50);

    while (1) {
        read_usb();
        delay(10);
    }
}

