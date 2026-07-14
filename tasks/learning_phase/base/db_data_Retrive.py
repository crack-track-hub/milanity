from db_connect import get_connection
conn= get_connection()
cursor=conn.cursor()
cursor.execute("use chat_db")
cursor.execute("SELECT * FROM messages")
bucket = cursor.fetchall()
for rows in bucket:
    print(rows)
