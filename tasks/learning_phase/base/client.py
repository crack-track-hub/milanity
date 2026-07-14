import socket
client = socket.socket()
client.connect(
    ("localhost",5000)
)
print("connected")
while True:
    message= input("enter msg:")
    client.send(message.encode())
    
    reply= client.recv(1024).encode
    print(reply)