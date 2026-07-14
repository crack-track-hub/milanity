import socket

client = socket.socket()

client.connect(
    ("localhost",5000)
)

while True:

    msg = input(
        "Message: "
    )

    client.send(
        msg.encode()
    )