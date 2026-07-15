# config.py

# ==========================================
# DATABASE SETTINGS
# ==========================================
DB_HOST = "localhost"
DB_USER = "root"
DB_PASSWORD = "Milanity"  # Enter your MySQL server password here
DB_NAME = "internet_speed_check"     # Target DB Name

# ==========================================
# MONITORING INTERVALS & VARIABLES
# ==========================================
# Dynamic minutes variable set by user
INTERVAL_MINUTES = 1 

# Automated conversion to seconds
TEST_INTERVAL = INTERVAL_MINUTES * 60  

# Delay between checks (Seconds)
CHECK_DELAY = 10         
TARGET_HOST = "8.8.8.8"

# Backup File Name
LOG_FILE = "internet_speed_logs.csv"