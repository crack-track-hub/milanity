import subprocess
import time
import gc
from config import TARGET_HOST

try:
    import speedtest
except ImportError:
    speedtest = None

def get_system_latency():
    """Runs standard ping utility to capture system connection latency."""
    try:
        # Note: If running on Windows, change "-c" below to "-n"
        cmd = ["ping", "-c", "1", "-W", "2", TARGET_HOST]
        start_time = time.time()
        result = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        latency = round((time.time() - start_time) * 1000, 2)
        
        status = "ONLINE" if result.returncode == 0 else "OFFLINE"
        return status, latency
    except Exception:
        return "ERROR", 0.0

def get_bandwidth_speeds():
    """Executes network speed check and cleans up system variables immediately."""
    if speedtest is None:
        print("[MONITOR ERROR] speedtest module is not installed. Install speedtest-cli to enable bandwidth tests.")
        return None, None, None

    try:
        # HTTP 403 Forbidden-ah bypass panna user-agent-ah standard web-browser-ah mock panrom
        st = speedtest.Speedtest(secure=True) # Secure connection bypasses basic ISP blocks
        st.get_best_server()
        
        download_speed = round(st.download() / 1000000, 2) # Mbps
        upload_speed = round(st.upload() / 1000000, 2)     # Mbps
        server_ping = round(st.results.ping, 2)
        
        # Explicit garbage collection
        del st
        gc.collect()
        
        return download_speed, upload_speed, server_ping
    except Exception as e:
        print(f"[MONITOR ERROR] Speedtest failed to execute: {e}")
        return None, None, None