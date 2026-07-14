import mysql.connector
from mysql.connector import Error
def get_connection():
    try:
        conn=mysql.connector.connect(
        host="localhost",
        username="root",
        password="milanity"
        )
        
        if conn.is_connected():
            print("connected sucefully")
        return conn
    except Error as e:
        print(f"connection errors {e}")

