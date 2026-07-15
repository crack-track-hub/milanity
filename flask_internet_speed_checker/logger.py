import csv
import os
import config


def init_logger():

    folder = os.path.dirname(config.LOG_FILE)

    if folder:

        os.makedirs(folder, exist_ok=True)

    if not os.path.exists(config.LOG_FILE):

        with open(
            config.LOG_FILE,
            "w",
            newline=""
        ) as file:

            writer = csv.writer(file)

            writer.writerow(

                [
                    "Recorded Time",
                    "Download Speed",
                    "Upload Speed",
                    "Test Ping",
                    "Network Status",
                    "System Ping"
                ]

            )


def write_log(

    recorded_time,
    download,
    upload,
    test_ping,
    status,
    system_ping

):

    with open(
        config.LOG_FILE,
        "a",
        newline=""
    ) as file:

        writer = csv.writer(file)

        writer.writerow(

            [

                recorded_time,

                download,

                upload,

                test_ping,

                status,

                system_ping

            ]

        )