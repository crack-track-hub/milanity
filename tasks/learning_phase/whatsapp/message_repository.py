from db_connect import get_connection


def save_message(
    sender,
    receiver,
    message
):

    conn = get_connection()

    if conn is None:
        return

    cursor = conn.cursor()

    cursor.execute(
        """
        INSERT INTO messages
        (
            sender,
            receiver,
            message
        )
        VALUES
        (
            %s,
            %s,
            %s
        )
        """,
        (
            sender,
            receiver,
            message
        )
    )

    conn.commit()

    cursor.close()
    conn.close()


def get_messages(
    user1,
    user2
):

    conn = get_connection()

    if conn is None:
        return []

    cursor = conn.cursor()

    cursor.execute(
        """
        SELECT
            sender,
            receiver,
            message,
            created_at
        FROM messages

        WHERE

        (sender=%s AND receiver=%s)

        OR

        (sender=%s AND receiver=%s)

        ORDER BY id
        """,
        (
            user1,
            user2,
            user2,
            user1
        )
    )

    records = cursor.fetchall()

    cursor.close()
    conn.close()

    return records