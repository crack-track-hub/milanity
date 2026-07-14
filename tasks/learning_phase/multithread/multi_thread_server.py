import socket
import threading

server = socket.socket()

server.bind(
    ("localhost",5000)
)

server.listen()

print("Server Started")

def handle_client(
    client_socket,
    address
):

    print(
        f"{address} Connected"
    )

    while True:

        try:

            data = client_socket.recv(
                1024
            )

            print(
                f"{address}:",
                data.decode()
            )

        except:
            break

    client_socket.close()

while True:

    client_socket, address = server.accept()

    threading.Thread(
        target=handle_client,
        args=(
            client_socket,
            address
        ),
        daemon=True
    ).start()