import mysql.connector
import config


def get_db_connection(select_db=True):

    if select_db:

        return mysql.connector.connect(
            host=config.DB_HOST,
            user=config.DB_USER,
            password=config.DB_PASSWORD,
            database=config.DB_NAME
        )

    return mysql.connector.connect(
        host=config.DB_HOST,
        user=config.DB_USER,
        password=config.DB_PASSWORD
    )


def init_database():

    connection = get_db_connection(False)

    cursor = connection.cursor()

    cursor.execute(
        f"CREATE DATABASE IF NOT EXISTS {config.DB_NAME}"
    )

    cursor.close()
    connection.close()

    connection = get_db_connection(True)

    cursor = connection.cursor()

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS speed_records(

            id INT AUTO_INCREMENT PRIMARY KEY,

            recorded_time DATETIME,

            download_speed FLOAT,

            upload_speed FLOAT,

            test_ping FLOAT,

            network_status VARCHAR(20),

            system_ping FLOAT

        )
    """)

    connection.commit()

    cursor.close()
    connection.close()


def insert_record(

    recorded_time,
    download,
    upload,
    test_ping,
    status,
    system_ping

):

    connection = get_db_connection(True)

    cursor = connection.cursor()

    query = """
    INSERT INTO speed_records
    (
        recorded_time,
        download_speed,
        upload_speed,
        test_ping,
        network_status,
        system_ping
    )
    VALUES(%s,%s,%s,%s,%s,%s)
    """

    cursor.execute(

        query,

        (
            recorded_time,
            download,
            upload,
            test_ping,
            status,
            system_ping
        )

    )

    connection.commit()

    cursor.close()

    connection.close()


def get_latest_record():

    connection = get_db_connection(True)

    cursor = connection.cursor(dictionary=True)

    cursor.execute("""

        SELECT *

        FROM speed_records

        ORDER BY id DESC

        LIMIT 1

    """)

    data = cursor.fetchone()

    cursor.close()

    connection.close()

    return data


def get_history(limit=100):

    connection = get_db_connection(True)

    cursor = connection.cursor(dictionary=True)

    cursor.execute(f"""

        SELECT *

        FROM speed_records

        ORDER BY id DESC

        LIMIT {limit}

    """)

    data = cursor.fetchall()

    cursor.close()

    connection.close()

    return data