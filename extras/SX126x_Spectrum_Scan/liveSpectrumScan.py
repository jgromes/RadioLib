"""
This code is still in development and is not yet ready for production use.
Kevin Leon @ Electronic Cats
  Original Creation Date: Jan 30, 2025
  This code is beerware; if you see me (or any other Electronic Cats
  member) at the local, and you've found our code helpful,
  please buy us a round!
  Distributed as-is; no warranty is given.
"""
import sys
import serial
import threading
import argparse
import platform
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

START_OF_FRAME = "SCAN"
END_OF_FRAME = "END"
FREQ_FRAME_MARK = "FREQ"
DEFAULT_COLOR_MAP = "BuGn"
DEFAULT_RSSI_OFFSET = -11
SCAN_WIDTH = 33
DEFAULT_START_FREQ = 860
DEFAULT_STEP_PER_FREQ = 0.2
DEFAULT_END_FREQ = 928
DEFAULT_BAUDRATE = 115200
LIMIT_COUNT = 2

if platform.system() == "Windows":
    DEFAULT_COMPORT = "COM1"
elif platform.system() == "Darwin":
    DEFAULT_COMPORT = "/dev/tty.usbmodem0001"
else:
    DEFAULT_COMPORT = "/dev/ttyACM0"


def LOG_INFO(message):
    """Function to log information."""
    print(f"[INFO] {message}")


def LOG_ERROR(message):
    """Function to log error."""
    print(f"\x1b[31;1m[ERROR] {message}\x1b[0m")


def LOG_WARNING(message):
    """Function to log warning."""
    print(f"\x1b[33;1m[WARNING] {message}\x1b[0m")


class SpectrumScan:
    def __init__(self):
        self.device_uart = serial.Serial(timeout=2)
        self.recv_running = False
        self.no_bytes_count = 0
        self.fig, self.ax = plt.subplots(figsize=(12, 6))
        self.im = None
        self.recv_worker = None
        self.current_freq = DEFAULT_START_FREQ
        self.start_freq = DEFAULT_START_FREQ
        self.end_freq = DEFAULT_END_FREQ
        self.rssi_offset = DEFAULT_RSSI_OFFSET
        self.delta_freq = 0
        self.data_matrix = np.zeros((SCAN_WIDTH, self.delta_freq))
        self.parser = argparse.ArgumentParser(
            formatter_class=argparse.RawTextHelpFormatter,
            description="""
        RadioLib SX126x_Spectrum_Scan plotter script. Displays output from SX126x_Spectrum_Scan example
        as grayscale and

        Depends on pyserial and matplotlib, install by:
        'python3 -m pip install pyserial matplotlib'

        Step-by-step guide on how to use the script:
        1. Upload the SX126x_Spectrum_Scan example to your Arduino board with SX1262 connected.
        2. Run the script with appropriate arguments.
        3. Once the scan is complete, output files will be saved to out/
    """,
        )
        self.fig.canvas.mpl_connect("close_event", self.on_close)
        self.__load_parser()

    def __load_parser(self):
        self.parser.add_argument(
            "port",
            type=str,
            help="COM port to connect to the device",
            default=DEFAULT_COMPORT,
        )
        self.parser.add_argument(
            "-b",
            "--baudrate",
            type=int,
            help=f"COM port baudrate (defaults to {DEFAULT_BAUDRATE})",
            default=DEFAULT_BAUDRATE,
        )
        self.parser.add_argument(
            "--freqStart",
            type=float,
            help=f"Starting frequency in MHz (Default to {DEFAULT_START_FREQ})",
            default=DEFAULT_START_FREQ,
        )
        self.parser.add_argument(
            "--freqEnd",
            type=float,
            help=f"End frequency in MHz (Default to {DEFAULT_END_FREQ})",
            default=DEFAULT_END_FREQ,
        )
        self.parser.add_argument(
            "--offset",
            type=int,
            help=f"Default RSSI offset in dBm (defaults to {DEFAULT_RSSI_OFFSET})",
            default=DEFAULT_RSSI_OFFSET,
        )

    def __data_dissector(self, plot_data):
        if FREQ_FRAME_MARK in plot_data:
            self.current_freq = float(plot_data.split(" ")[1])
            if (
                self.current_freq >= self.start_freq
                and self.current_freq <= self.end_freq
            ):
                if self.current_freq == self.start_freq:
                    self.data_matrix = np.zeros((SCAN_WIDTH, self.delta_freq))
                return
        if (START_OF_FRAME in plot_data) and (END_OF_FRAME in plot_data):
            if (
                self.current_freq >= self.start_freq
                and self.current_freq <= self.end_freq
            ):
                scan_line = plot_data[len(START_OF_FRAME) : -len(END_OF_FRAME)].split(
                    ","
                )[:-1]
                data = list(map(int, scan_line))
                index = int(
                    (self.current_freq - self.start_freq) / DEFAULT_STEP_PER_FREQ
                )
                self.data_matrix[:, index] = data

    def on_close(self, event):
        self.recv_running = False

    def stop_task(self):
        self.recv_running = False
        if self.device_uart.is_open:
            self.device_uart.close()
        if threading.current_thread() is not self.recv_worker:
            if self.recv_worker and self.recv_worker.is_alive():
                self.recv_worker.join(timeout=2)

    def recv_task(self):
        with self.device_uart as com:
            while self.recv_running:
                if self.recv_worker.is_alive():
                    try:
                        bytestream = com.readline().decode("utf-8").strip()
                        if not self.recv_running:
                            break
                        if bytestream == "":
                            # Board connected but not transmitting any data
                            self.no_bytes_count += 1
                            if self.no_bytes_count > LIMIT_COUNT:
                                self.no_bytes_count = 0
                                LOG_WARNING("No data recived.")
                            continue
                        self.__data_dissector(bytestream)
                    except serial.SerialException as e:
                        LOG_ERROR(e)
                        break
            com.reset_input_buffer()
            com.reset_output_buffer()
            com.close()
        self.stop_task()

    def create_plot(self):
        self.ax.set_ylabel("RSSI [dBm]")
        self.ax.set_xlabel("Frequency (MHz)")
        self.ax.set_aspect("auto")
        self.fig.suptitle(
            f"SX126x Spectral Scan (Frequency range: {self.start_freq}/{self.end_freq} MHz)"
        )
        self.fig.canvas.manager.set_window_title(
            "PWNLabs/ElectroniCats -  Spectral Scan"
        )
        self.im = self.ax.imshow(
            self.data_matrix[:, : self.delta_freq],
            cmap=DEFAULT_COLOR_MAP,
            aspect="auto",
            extent=[
                self.start_freq,
                self.end_freq,
                -4 * (SCAN_WIDTH + 1),
                self.rssi_offset,
            ],
        )
        self.fig.colorbar(self.im)
        manager = plt.get_current_fig_manager()
        try:
            manager.window.attributes("-topmost", 1)
            manager.window.attributes("-topmost", 0)
        except AttributeError:
            pass

    def show_plot(self, i):
        self.im.set_data(self.data_matrix)
        self.ax.relim()
        self.ax.autoscale_view()

    def main(self):
        self.recv_running = True

        args = self.parser.parse_args()
        if args.freqStart < DEFAULT_START_FREQ or args.freqStart > DEFAULT_END_FREQ:
            LOG_WARNING("Frequency start out of range")
            sys.exit(1)

        if args.freqEnd < DEFAULT_START_FREQ or args.freqEnd > DEFAULT_END_FREQ:
            LOG_WARNING("Frequency start out of range")
            sys.exit(1)

        if args.freqStart > args.freqEnd:
            LOG_WARNING("Frequency start is greater than frequency end")
            sys.exit(1)

        if args.offset:
            self.rssi_offset = args.offset

        self.device_uart.port = args.port
        self.current_freq = args.freqStart
        self.start_freq = args.freqStart
        self.end_freq = args.freqEnd

        # Update the initial values with the args values
        self.delta_freq = int((self.end_freq - self.start_freq) / DEFAULT_STEP_PER_FREQ)
        self.data_matrix = np.zeros((SCAN_WIDTH, self.delta_freq))
        # Start the recv task
        self.recv_worker = threading.Thread(target=self.recv_task, daemon=True)
        self.recv_worker.start()
        self.create_plot()
        # Do an animation with the data
        ani = animation.FuncAnimation(
            self.fig, self.show_plot, interval=100, cache_frame_data=False
        )
        plt.show()
        sys.exit(0)


if __name__ == "__main__":
    sc = SpectrumScan()
    try:
        sc.main()
    except KeyboardInterrupt:
        sc.stop_task()
        sys.exit(0)
