import socket
import threading

from message_repository import (
    save_message
)

server = socket.socket()

server.bind(
    ("localhost",5000)
)

server.listen()

print(
    "Server Started..."
)

clients = {}


def handle_client(
    client_socket,
    username
):

    while True:

        try:

            data = client_socket.recv(
                1024
            )

            if not data:
                break

            payload = data.decode()

            receiver, message = payload.split(
                ":",
                1
            )

            print(
                f"{username} -> {receiver}: {message}"
            )

            save_message(
                username,
                receiver,
                message
            )

            if receiver in clients:

                clients[
                    receiver
                ].send(
                    f"{username}: {message}".encode()
                )

        except Exception as e:

            print(
                e
            )

            break

    if username in clients:
        del clients[username]

    client_socket.close()


while True:

    client_socket, address = server.accept()

    username = client_socket.recv(
        1024
    ).decode()

    clients[
        username
    ] = client_socket

    print(
        f"{username} Connected"
    )

    threading.Thread(
        target=handle_client,
        args=(
            client_socket,
            username
        ),
        daemon=True
    ).start()