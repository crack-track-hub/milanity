import socket
import threading

server=socket.socket()
server.bind("localhost", 5000)
print("server connected")
server.listen()
client_socket, address = server.accept()
print("connected address:",address)
def messaging():
    while True:
        try:            
                message=client_socket.recv()
                print(
                    message.encode()
                )
        except:
            break
    threading.Thread(
        target=messaging,
        deamon=True
    ).start()
    
    while True:
        reply= input()
        client_socket.send(
            reply.encode()
        )