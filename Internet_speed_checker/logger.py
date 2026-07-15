import csv
import os
from config import LOG_FILE

def init_logger():
    """Creates a backup CSV file with standard, clean headers."""
    if not os.path.exists(LOG_FILE):
        with open(LOG_FILE, mode='w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow([
                "Time Recorded", 
                "Download (Mbps)", 
                "Upload (Mbps)", 
                "Test Ping (ms)", 
                "Connection Status", 
                "System Ping (ms)"
            ])

def write_log(time_recorded, download, upload, test_ping, connection_status, system_ping):
    """Appends records instantly on target intervals."""
    with open(LOG_FILE, mode='a', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow([time_recorded, download, upload, test_ping, connection_status, system_ping])