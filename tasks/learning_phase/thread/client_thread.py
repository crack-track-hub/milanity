import socket
import threading
client=socket.socket()
client.connect(
    ("lochalhost",5000)
)
def messaging():
    while True:
        try:
            message= client.recv()
            print(
                message.encode()
                )
        expect:
        break
    threading.Thread(
    target= messaging,
    daemon=True
    ).start
    




