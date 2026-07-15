import mysql.connector
from config import DB_HOST, DB_USER, DB_PASSWORD, DB_NAME

def get_db_connection(select_db=True):
    """Establishes temporary MySQL connection on-demand."""
    if select_db:
        return mysql.connector.connect(
            host=DB_HOST,
            user=DB_USER,
            password=DB_PASSWORD,
            database=DB_NAME
        )
    else:
        return mysql.connector.connect(
            host=DB_HOST,
            user=DB_USER,
            password=DB_PASSWORD
        )

def init_database():
    """Builds database and 'records' table silently."""
    conn = None
    cursor = None

    try:
        # Step A: Validate Database exists
        conn = get_db_connection(select_db=False)
        cursor = conn.cursor()
        cursor.execute(f"CREATE DATABASE IF NOT EXISTS {DB_NAME}")
        conn.commit()

        # Step B: Setup records table
        conn = get_db_connection(select_db=True)
        cursor = conn.cursor()
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS records (
                id INT AUTO_INCREMENT PRIMARY KEY,
                time_recorded DATETIME NOT NULL,
                download DOUBLE NOT NULL,
                upload DOUBLE NOT NULL,
                test_ping DOUBLE NOT NULL,
                connection_status VARCHAR(20) NOT NULL,
                system_ping DOUBLE NOT NULL
            )
        ''')
        conn.commit()
    except Exception:
        pass # Silent initialization
    finally:
        if cursor is not None:
            cursor.close()
        if conn is not None:
            conn.close()

def insert_record(time_recorded, download, upload, test_ping, connection_status, system_ping):
    """Inserts a single row of metrics and closes connection instantly to free RAM."""
    conn = None
    cursor = None

    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        query = """
            INSERT INTO records 
            (time_recorded, download, upload, test_ping, connection_status, system_ping) 
            VALUES (%s, %s, %s, %s, %s, %s)
        """
        cursor.execute(query, (time_recorded, download, upload, test_ping, connection_status, system_ping))
        conn.commit()
    except Exception as e:
        print(f"[DATABASE ERROR] Failed to save record: {e}")
    finally:
        if cursor is not None:
            cursor.close()
        if conn is not None:
            conn.close()