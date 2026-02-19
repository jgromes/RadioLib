#!/usr/bin/python3
# -*- encoding: utf-8 -*-

import argparse
import sys
import zmq
import serial
import threading
import queue
import time

from argparse import RawTextHelpFormatter

# default settings
DEFAULT_BAUDRATE = 115200
DEFAULT_SERVER_PORT = 30002

# marker to filter out ADS-B messages from the rest of the serial stream
ADSB_MESSAGE_MARKER = '[ADS-B]'

class SerialToZMQBridge:
    def __init__(self, serial_port, baudrate, zmq_host="0.0.0.0", zmq_port=5555):
        self.serial_port = serial_port
        self.baudrate = baudrate
        self.zmq_host = zmq_host
        self.zmq_port = zmq_port

        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.STREAM)
        self.socket.setsockopt(zmq.LINGER, 0)
        self.socket.bind(f"tcp://{self.zmq_host}:{self.zmq_port}")

        self.serial = serial.Serial(
            port=self.serial_port,
            baudrate=self.baudrate,
            timeout=1
        )

        self.clients = set()
        self.serial_queue = queue.Queue()

        self.running = True

    def serial_reader(self):
        """Continuously read from serial and queue messages."""
        while self.running:
            try:
                line = self.serial.readline()
                if line:
                    line = line.decode(errors='ignore').strip()
                    print(f"[SERIAL RX] {line}")
                    
                    # read the ADS-B frames and add the markers expected by pyModeS
                    if ADSB_MESSAGE_MARKER in line:
                        msg = '*' + line.split(' ')[1].strip() + ';'
                        self.serial_queue.put(msg.encode('utf-8'))
           
            except Exception as e:
                print(f"Serial read error: {e}")
                time.sleep(1)

    def run(self):
        print(f"ZMQ STREAM server listening on tcp://{self.zmq_host}:{self.zmq_port}")
        print(f"Listening to serial port {self.serial_port} @ {self.baudrate}")

        threading.Thread(target=self.serial_reader, daemon=True).start()

        poller = zmq.Poller()
        poller.register(self.socket, zmq.POLLIN)

        try:
            while self.running:
                # Poll ZMQ socket
                events = dict(poller.poll(100))

                if self.socket in events:
                    identity, message = self.socket.recv_multipart()

                    if message == b'':
                        # Connection event
                        print(f"[ZMQ] Client connected/disconnected: {identity}")
                        self.clients.add(identity)
                        continue

                    print(f"[ZMQ RX] {identity}: {message.decode(errors='ignore')}")

                # Send serial data to all connected clients
                while not self.serial_queue.empty():
                    data = self.serial_queue.get()
                    for client_id in list(self.clients):
                        try:
                            self.socket.send_multipart([client_id, data])
                            print(f"[ZMQ TX] Sent to {client_id}")
                        except zmq.ZMQError:
                            self.clients.discard(client_id)

        except KeyboardInterrupt:
            print("Shutting down...")

        finally:
            self.running = False
            self.serial.close()
            self.socket.close()
            self.context.term()


def main():
    parser = argparse.ArgumentParser(formatter_class=RawTextHelpFormatter, description='''
        RadioLib ADS-B Monitor script. Serves as server for "modeslive" live traffic decoder from pyModeS
        (https://github.com/junzis/pyModeS).

        Depends on pyserial and pyModeS, install by:
        'python3 -m pip install pyserial pyModeS'

        Step-by-step guide on how to use the script:
        1. Upload the ADSB_Monitor example to your Arduino board with LR2021 connected.
        2. Run the script with appropriate arguments.
        3. Run "modeslive --source net --connect localhost 30002 raw"
    ''')
    parser.add_argument('port',
        type=str,
        help='COM port to connect to the device')
    parser.add_argument('--speed',
        default=DEFAULT_BAUDRATE,
        type=int,
        help=f'COM port baudrate (defaults to {DEFAULT_BAUDRATE})')
    parser.add_argument('--server-port',
        default=DEFAULT_SERVER_PORT,
        type=int,
        help=f'server port to be used by modeslive (defaults to {DEFAULT_SERVER_PORT})')
    args = parser.parse_args()

    bridge = SerialToZMQBridge(
        serial_port=args.port,
        baudrate=args.speed,
        zmq_port=args.server_port
    )
    bridge.run()

if __name__ == "__main__":
    sys.exit(main())
