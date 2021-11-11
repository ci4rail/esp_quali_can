import socket
import sys

ESP_IP = "192.168.7.1"
PORT = 10000

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print("try to connect")
sock.connect((ESP_IP, PORT))
print("connected...")
while True:
  data = sock.recv(255)
  print("msg: ", data.decode())
sock.close()
