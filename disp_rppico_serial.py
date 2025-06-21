#!/usr/env python
import serial.tools.list_ports

def find_pico_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # PicoのVIDとPIDをチェック（2E8A:000A）
        if port.vid == 0x2E8A and port.pid == 0x000A:
            return port.device  # Picoが接続されているポートを返す
    return None

pico_port = find_pico_port()
if pico_port:
    print(f"Raspberry Pi Picoのポートは: {pico_port}")
else:
    print("Raspberry Pi Picoが見つかりませんでした…")
