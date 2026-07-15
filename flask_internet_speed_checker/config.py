# ==============================
# DATABASE CONFIGURATION
# ==============================

DB_HOST = "localhost"
DB_USER = "root"
DB_PASSWORD = "root"
DB_NAME = "networkpulse_db"

# ==============================
# MONITOR CONFIGURATION
# ==============================

INTERVAL_MINUTES = 1

TEST_INTERVAL = INTERVAL_MINUTES * 60

CHECK_DELAY = 2

PING_TARGET = "8.8.8.8"

LOG_FILE = "logs/speed_log.csv"