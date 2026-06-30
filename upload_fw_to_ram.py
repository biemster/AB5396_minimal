#!/usr/bin/env python
from bluetrum.cipher import ab_calckey
from bluetrum.utils import *
from bluetrum.dl.uart import UARTDownload

import time
import struct
import argparse
import os
from tqdm import tqdm

class BlCmd:
    IFACE_PARAM     = 0x50
    MEM_READ        = 0x52
    AUTHORIZE       = 0x55
    MEM_WRITE       = 0x57
    SET_CMD_HANDLER = 0x58
    GET_INFO        = 0x5A
    REBOOT          = 0x5E

def main():
    try:
        from serial import Serial
    except ImportError:
        print('pyserial is not installed. Please install it with: pip install pyserial')
        print('by https://github.com/kagaimiq/bluetrum-tools edited by ATC1441 and biemster')
        exit(1)

    ap = argparse.ArgumentParser(description='Tool to upload firmware via UART to run in RAM on Bluetrum chips.')
    ap.add_argument('--port', default='/dev/ttyACM0', help='Serial port to use for UART bootloader (e.g., /dev/ttyAMC0 or COM3)')
    ap.add_argument('--baud', type=int, default=115200, help='Baudrate for data transfer after init')
    ap.add_argument('--firmware', help='Path to the firmware .bin file')
    args = ap.parse_args()

    fw_blob = None
    if args.firmware:
        if not os.path.exists(args.firmware):
            print(f"Error: Firmware file not found at '{args.firmware}'")
            exit(1)
        else:
            try:
                with open(args.firmware, 'rb') as f:
                    fw_blob = f.read()
                print(f"Successfully loaded {len(fw_blob)} bytes from '{args.firmware}'.")
            except Exception as e:
                print(f"Error: Could not read the firmware file '{args.firmware}': {e}")
                exit(1)


    if os.path.exists(args.port):
        with Serial(args.port, args.baud) as port:
            udl = UARTDownload(port)

            print(f'Attempting to synchronize on {args.port}...', end='', flush=True)
            port.reset_input_buffer()
            port.reset_output_buffer()
            time.sleep(0.1)
            port.baudrate = args.baud
            time.sleep(0.1)

            port.timeout = .01
            try:
                done = False
                num = 0
                while not done:
                    if num < 10:
                        udl.port.reset_input_buffer()
                        udl.port.write(UARTDownload.SYNC_TOKEN)
                        while not done:
                            recv = port.read(4)
                            if recv == b'': break
                            if recv == UARTDownload.SYNC_RESP: done = True
                        num += 1
                    else:
                        print('.', end='', flush=True)
                        udl.send_reset(True)
                        num = 0
                if not done:
                    print(" fail!")
                    exit()
            except Exception as e:
                print(f" fail! {e}")
                exit()
            
            print()
            port.timeout = .1
            do_the_stuff(execcmd, udl, fw_blob, 512, port)

        print("\nOperation finished.")
    else:
        print(f'* ERROR: {args.port} not found\n')
        ap.print_help()

def pack_cmd(cmd, arg1=0, arg2=0, arg3=0):
    return struct.pack('>BIBH', cmd, arg1, arg2, arg3)

def execcmd(cb, udl, send=None, recv=None, max_io=512, switch_baud=None):
    # first goes the command block
    udl.send_packet(cb)

    # switch baudrate at that point
    if switch_baud is not None:
        port.baudrate = switch_baud

    # transfer data
    if send is not None:
        # send data blocks
        sent = 0
        while sent < len(send):
            num = min(len(send) - sent, max_io)

            udl.send_packet(send[sent : sent+num])
            sent += num

    elif recv is not None:
        # receive data blocks
        data = b''
        while len(data) < recv:
            num = min(recv - len(data), max_io)

            block = udl.recv_packet()
            data += block

            if len(block) != num:
                break

        return data

def do_the_stuff(execcmd, udl, fw_blob, blocksize, port):
    # Query the information
    resp = execcmd(pack_cmd(BlCmd.GET_INFO, arg1=0x5259414E, arg3=0x67ca), udl, recv=24)
    chipid, loadaddr, commskey, _ = struct.unpack('>12sIII', resp)
    print(f' Chip ID:       {chipid}')
    print(f' Load address:  ${loadaddr:08X}')
    print(f' Init. commkey: ${commskey:08X}')

    # Authorize
    resp = execcmd(pack_cmd(BlCmd.AUTHORIZE, arg1=ab_calckey(commskey)), udl, recv=4)
    commskey, = struct.unpack('>I', resp)
    print(f' New commkey:   ${commskey:08X}')

    # Load blob
    if fw_blob:
        data = bytearray(fw_blob) + b'\x00' * align_by(len(fw_blob), blocksize)
        execcmd(pack_cmd(BlCmd.MEM_WRITE, arg1=loadaddr, arg3=(len(data) // blocksize)), udl, send=data)
        execcmd(pack_cmd(BlCmd.SET_CMD_HANDLER, arg1=loadaddr), udl)

        # start!
        udl.send_packet(pack_cmd(0x00, arg1=int.from_bytes(b'arg1'), arg2=int.from_bytes(b'2'), arg3=int.from_bytes(b'a3')))
        while True:
            try:
                print(udl.recv_packet().decode())
            except KeyboardInterrupt:
                break

if __name__ == '__main__':
    main()
