import mysql.connector
from mysql.connector import Error


def get_connection():

    try:

        conn = mysql.connector.connect(
            host="localhost",
            user="root",
            password="mlanity",   # change if needed
            database="chat_db"
        )

        return conn

    except Error as e:

        print(
            f"Database Error: {e}"
        )

        return None