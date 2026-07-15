import subprocess
import speedtest
import config


def get_system_latency():
    """
    Check internet connectivity using system ping.
    Returns:
        status (ONLINE/OFFLINE)
        ping_time (ms)
    """

    try:

        command = [
            "ping",
            "-c", "1",
            "-W", "2",
            config.PING_TARGET
        ]

        result = subprocess.run(
            command,
            capture_output=True,
            text=True
        )

        if result.returncode == 0:

            output = result.stdout

            ping_value = None

            for line in output.split("\n"):

                if "time=" in line:

                    ping_value = float(
                        line.split("time=")[1].split()[0]
                    )

                    break

            return "ONLINE", ping_value

        return "OFFLINE", 0.0

    except Exception:

        return "OFFLINE", 0.0


def get_bandwidth_speeds():
    """
    Perform Speed Test.
    Returns:
        Download Mbps
        Upload Mbps
        Server Ping
    """

    try:

        st = speedtest.Speedtest(secure=True)

        st.get_best_server()

        download_speed = round(
            st.download() / 1000000,
            2
        )

        upload_speed = round(
            st.upload() / 1000000,
            2
        )

        ping = round(
            st.results.ping,
            2
        )

        del st

        return (
            download_speed,
            upload_speed,
            ping
        )

    except Exception:

        return (
            None,
            None,
            None
        )