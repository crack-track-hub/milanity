import mysql.connector
from mysql.connector import Error
from datetime import datetime


# ---------- DB CONFIG (unga MySQL details maathikonga) ----------
DB_CONFIG = {
    "host": "localhost",
    "user": "root",
    "password": "milanity",   # unga MySQL password podunga
    "database": "chat_db"               # database irukanum, illana create pannikonga
}


def get_connection():

    try:
        connection = mysql.connector.connect(**DB_CONFIG)
        return connection
    except Error as e:
        print("MySQL Connection Error:", e)
        return None


def store_message(sender, message):
    """
    Oru message ah MySQL 'messages' table la store pandra function.

    Parameters:
        sender  -> "Client" or "Server" (yaaru anuppinaanga nu)
        message -> andha message text
    """
    connection = get_connection()
    if connection is None:
        print("Could not store message: no DB connection")
        return

    try:
        cursor = connection.cursor()
        insert_query = """
        INSERT INTO messages (sender, message, timestamp)
        VALUES (%s, %s, %s)
        """
        values = (sender, message, datetime.now())
        cursor.execute(insert_query, values)
        connection.commit()
    except Error as e:
        print("Insert Error:", e)
    finally:
        cursor.close()
        connection.close()