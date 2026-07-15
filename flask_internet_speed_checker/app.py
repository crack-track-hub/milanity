from flask import Flask, render_template, jsonify
from threading import Thread
from datetime import datetime
import time
import gc

import config
import database
import logger
import monitor

app = Flask(__name__)

# ===========================================
# Initialize Database & Logger
# ===========================================

database.init_database()
logger.init_logger()

# ===========================================
# Background Monitoring Function
# ===========================================

def monitoring_service():

    last_check_time = 0

    while True:

        current_time = time.time()

        current_timestamp = datetime.now().strftime(
            "%Y-%m-%d %H:%M:%S"
        )

        if (current_time - last_check_time) >= config.TEST_INTERVAL:

            status, system_ping = monitor.get_system_latency()

            download, upload, test_ping = monitor.get_bandwidth_speeds()

            if download is not None:

                database.insert_record(

                    current_timestamp,

                    download,

                    upload,

                    test_ping,

                    status,

                    system_ping

                )

                logger.write_log(

                    current_timestamp,

                    download,

                    upload,

                    test_ping,

                    status,

                    system_ping

                )

                print(
                    f"[{current_timestamp}] "
                    f"DL:{download} Mbps | "
                    f"UL:{upload} Mbps | "
                    f"Ping:{test_ping} ms"
                )

            else:

                database.insert_record(

                    current_timestamp,

                    0.0,

                    0.0,

                    0.0,

                    status,

                    system_ping

                )

                logger.write_log(

                    current_timestamp,

                    0.0,

                    0.0,

                    0.0,

                    status,

                    system_ping

                )

            last_check_time = current_time

        gc.collect()

        time.sleep(config.CHECK_DELAY)


# ===========================================
# Flask Routes
# ===========================================

@app.route("/")
def dashboard():

    latest = database.get_latest_record()

    history = database.get_history()

    return render_template(

        "dashboard.html",

        latest=latest,

        history=history

    )


@app.route("/api/latest")
def latest():

    return jsonify(

        database.get_latest_record()

    )


@app.route("/api/history")
def history():

    return jsonify(

        database.get_history()

    )


# ===========================================
# Start Background Thread
# ===========================================

background_thread = Thread(

    target=monitoring_service,

    daemon=True

)

background_thread.start()

# ===========================================
# Run Flask
# ===========================================
if __name__ == "__main__":
    background_thread.start()

    app.run(
        host="0.0.0.0",
        port=5000,
        debug=False
    )