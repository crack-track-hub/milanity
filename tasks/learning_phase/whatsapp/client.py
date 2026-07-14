import socket
import threading

client = socket.socket()

client.connect(
    ("localhost",5000)
)

username = input(
    "Enter Username: "
)

client.send(
    username.encode()
)


def receive_messages():

    while True:

        try:

            data = client.recv(
                1024
            )

            print(
                "\n" +
                data.decode()
            )

        except:

            break


threading.Thread(
    target=receive_messages,
    daemon=True
).start()


while True:

    receiver = input(
        "To: "
    )

    message = input(
        "Message: "
    )

    payload = (
        receiver +
        ":" +
        message
    )

    client.send(
        payload.encode()
    )