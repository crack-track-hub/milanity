import time
from datetime import datetime
import gc

import config
import database
import logger
import monitor

def main():
    # ============================================================
    # 1. INITIAL STARTUP HEADER (PRINTS ONLY ONCE AT THE START)
    # ============================================================
    print("="*60)
    print("      SYSTEM MONITORING DAEMON - SPEED TEST STARTED      ")
    print("="*60)
    print(f"[CONFIG] Speed Check Interval : Every {config.INTERVAL_MINUTES} Minute(s)")
    print(f"[CONFIG] System Check Delay   : {config.CHECK_DELAY} Seconds")
    print(f"[CONFIG] Database Target      : {config.DB_NAME} (Table: records)")
    print(f"[CONFIG] Backup Log File      : {config.LOG_FILE}")
    print("="*60)
    print("[SYSTEM] Monitoring is running silently in the background...\n")

    # Silent DB & CSV setups initialization
    database.init_database()
    logger.init_logger()
    
    last_check_time = 0  # Epoch counter to schedule checks
    
    try:
        while True:
            current_time = time.time()
            current_timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            
            # Checks if interval has passed
            if (current_time - last_check_time) >= config.TEST_INTERVAL:
                
                # Task A: Ping Status
                status, system_ping = monitor.get_system_latency()
                
                # Task B: Bandwidth check
                download, upload, test_ping = monitor.get_bandwidth_speeds()
                
                if download is not None:
                    # Silent write to MySQL
                    database.insert_record(
                        current_timestamp, download, upload, test_ping, status, system_ping
                    )
                    
                    # Silent write to CSV
                    logger.write_log(
                        current_timestamp, download, upload, test_ping, status, system_ping
                    )
                    
                    # ONLY PRINT THE EXACT DATA STORED IN TERMINAL
                    print(f"[{current_timestamp}] DL: {download} Mbps | UL: {upload} Mbps | Test Ping: {test_ping} ms | Status: {status} | Sys Ping: {system_ping} ms")
                else:
                    # Fallback log in case speed test fails
                    database.insert_record(
                        current_timestamp, 0.0, 0.0, 0.0, status, system_ping
                    )
                    logger.write_log(
                        current_timestamp, 0.0, 0.0, 0.0, status, system_ping
                    )
                    print(f"[{current_timestamp}] DL: 0.0 Mbps | UL: 0.0 Mbps | Test Ping: 0.0 ms | Status: {status} | Sys Ping: {system_ping} ms")
                
                last_check_time = current_time
            
            # Heap cleaning cycle
            gc.collect()
            time.sleep(config.CHECK_DELAY)
            
    except KeyboardInterrupt:
        print("\n[SYSTEM] Continuous monitoring closed. Exiting...")

if __name__ == "__main__":
    main()