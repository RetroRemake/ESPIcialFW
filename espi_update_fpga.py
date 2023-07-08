import sys
import serial
import time
import collections
from itertools import islice

def batched(iterable, n):
    "Batch data into tuples of length n. The last batch may be shorter."
    # batched('ABCDEFG', 3) --> ABC DEF G
    if n < 1:
        raise ValueError('n must be at least one')
    it = iter(iterable)
    while batch := tuple(islice(it, n)):
        yield batch

def send_spi_cmd(cmd, arg, response_size):
    arg_size = 0 if arg is None else len(arg)
    total_size = 1 + arg_size + response_size

    payload = [0xAA, total_size >> 8, total_size & 0xff, cmd]
    if arg is not None:
        payload.extend(arg)
    payload.extend([0xff]*response_size)

    ser.write(b''.join(x.to_bytes(1, 'big') for x in payload))
    response = ser.read(total_size)
    return response

def spi_begin():
    ser.write(b'\x96')
    send_spi_cmd(0xAB, None, 4)

def spi_end():
    send_spi_cmd(0xB9, None, 0)
    ser.write(b'\x69')

def spi_dummy():
    ser.write(b'\xff')
    return ser.read()

def flash_reset():
    send_spi_cmd(0x99, None, 0)

def flash_write_enable():
    send_spi_cmd(0x06, None, 0)

def flash_get_id():
    res = send_spi_cmd(0x9f, None, 3)
    return res

def flash_erase_sector(addr):
    addr_bytes = [(addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff]
    res = send_spi_cmd(0x20, addr_bytes, 0)

def flash_chip_erase():
    send_spi_cmd(0x67, None, 0);

def flash_read_status():
    return int.from_bytes(send_spi_cmd(0x05, None, 1), 'big')

def flash_program_page(addr, data):
    arg = [(addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff]
    arg.extend(data)
    send_spi_cmd(0x02, arg, 0)

def flash_read_page(addr):
    addr_bytes = [(addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff]
    page_data = send_spi_cmd(0x03, addr_bytes, 256)
    return page_data[4:]

def flash_busy_wait():
    loop_cnt = 0
    while (flash_read_status() & 0x1) == 1:
        loop_cnt += 1
    return loop_cnt

page_1_fill = [0xC0, 0xED, 0xBA, 0xBE]
page_2_fill = [0x80, 0x08, 0x73, 0x55]

def program_flash_page(page_addr, write_data):
    if (page_addr & 0xfff) == 0:
        print(f"Erasing sector 0x{page_addr:06X}...",end='' )
        if (flash_read_status() & 0x2) == 0:
            flash_write_enable()

        flash_erase_sector(page_addr)
        loop_cnt = flash_busy_wait()
        print(f"Waited {loop_cnt} loops")

    if (flash_read_status() & 0x2) == 0:
        flash_write_enable()

    print(f"Writing page 0x{page_addr:06X}")
    flash_program_page(page_addr, write_data)
    loop_cnt = flash_busy_wait()
    ser.flush()

    ser.reset_input_buffer()

    read_data = flash_read_page(page_addr)
    if not all(map(lambda x: x[0] == x[1], zip(write_data,read_data))):
        print(f"Verification mis-match in page {page_addr}")
        print(write_data)
        print(read_data)
        read_data = None

    return read_data
    
def program_flash(data):
        print("Resetting SPI")
        flash_reset()
        ser.reset_input_buffer()
        time.sleep(0.1)
        res = flash_get_id()
        print("Flash ID: "+res.hex());

        page_addr = 0
        for write_data in batched(data,256):
            verification_data = program_flash_page(page_addr, write_data)
            if verification_data is None:
                break
            page_addr += 256


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Unexpected parameter count: {len(sys.argv)}")
        print(f"Usage: {sys.argv[0]} <com port> <bitstream>")
    else:
        print(f"Flashing from file {sys.argv[1]}")

        with open(sys.argv[2], 'rb') as f:
            data = f.read()
            print(f"Read {len(data)} bytes.")
            global ser
            with serial.Serial(sys.argv[1], 115200) as ser:
                spi_end()
                spi_begin()
                program_flash(data)
                spi_end()
            ser.write(b'\xf0')


