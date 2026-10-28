import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import queue
import subprocess
import sys
import tempfile
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
ACCOUNT_ID = 10001
TITLE_ID = "TEST00001"


def make_accept_key(sec_key: str) -> str:
    h = hashlib.sha1((sec_key.strip() + GUID).encode("utf-8")).digest()
    return base64.b64encode(h).decode("ascii")


def make_ws_text_frame(payload: str) -> bytes:
    data = payload.encode("utf-8")
    length = len(data)
    header = bytearray([0x81])
    if length < 126:
        header.append(length)
    elif length <= 0xFFFF:
        header.append(126)
        header.extend(length.to_bytes(2, "big"))
    else:
        header.append(127)
        header.extend(length.to_bytes(8, "big"))
    return bytes(header) + data


class FakeCloudHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass

    def do_GET(self):
        if self.path == "/v1/events":
            sec_key = self.headers.get("Sec-WebSocket-Key")
            if not sec_key:
                self.send_error(400, "Missing Sec-WebSocket-Key")
                return
            accept = make_accept_key(sec_key)
            self.send_response(101, "Switching Protocols")
            self.send_header("Upgrade", "websocket")
            self.send_header("Connection", "Upgrade")
            self.send_header("Sec-WebSocket-Accept", accept)
            self.end_headers()
            welcome = json.dumps({"type": "welcome", "address": "198.18.0.7", "server_time": 5})
            self.wfile.write(make_ws_text_frame(welcome))
            self.wfile.flush()
            try:
                while True:
                    time.sleep(1)
            except Exception:
                pass
            return

        if self.path == "/v1/me":
            payload = json.dumps({
                "id": ACCOUNT_ID,
                "handle": "TestPlayer",
                "email": "tester@anyplay.net",
                "created": 1700000000,
            }).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        if self.path == "/v1/friends":
            payload = json.dumps({"friends": [], "incoming": [], "outgoing": []}).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        if self.path == "/v1/relays":
            payload = json.dumps({"relays": []}).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        payload = json.dumps({"ok": True, "path": self.path}).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def do_POST(self):
        content_length = int(self.headers.get("Content-Length", 0))
        _ = self.rfile.read(content_length)
        if self.path == "/v1/auth/refresh":
            payload = json.dumps({
                "access": "fake-access-token",
                "access_expires": 2500000000,
                "refresh": "fake-refresh-token",
                "device": 1,
            }).encode("utf-8")
        elif self.path == "/v1/sessions":
            payload = json.dumps({
                "id": "ci-session-12345",
                "title_id": TITLE_ID,
                "title": "Test Game",
                "version": "01.000",
                "build": "0.1.3",
                "leader": ACCOUNT_ID,
                "max_members": 4,
                "visibility": "friends",
                "properties": "",
                "members": [
                    {
                        "account": {"id": ACCOUNT_ID, "handle": "TestPlayer"},
                        "endpoint": "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
                        "address": "198.18.0.7",
                        "joined": 1700000000,
                    }
                ],
                "created": 1700000000,
                "revision": 1,
            }).encode("utf-8")
        else:
            payload = json.dumps({"ok": True}).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)


def start_fake_cloud() -> tuple[ThreadingHTTPServer, int]:
    server = ThreadingHTTPServer(("127.0.0.1", 0), FakeCloudHandler)
    port = server.server_address[1]
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server, port


class AnyplayNetProcess:
    def __init__(self, anyplay_net_bin: Path, data_dir: Path):
        self.anyplay_net_bin = anyplay_net_bin
        self.data_dir = data_dir
        self.stderr_path = data_dir / "anyplay-net.stderr"
        self.stderr_file = open(self.stderr_path, "w", encoding="utf-8", errors="replace")
        proc_env = dict(os.environ)
        proc_env["RUST_LOG"] = "info"
        proc_env["ANYPLAY_CLIENT"] = "launcher/0.3.0"
        self.proc = subprocess.Popen(
            [str(self.anyplay_net_bin), "--data", str(self.data_dir), "--file-credentials"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=self.stderr_file,
            env=proc_env,
            text=True,
            bufsize=1,
        )
        self.out_queue = queue.Queue()
        self.reader_thread = threading.Thread(target=self._read_stdout, daemon=True)
        self.reader_thread.start()

    def _read_stdout(self):
        for line in iter(self.proc.stdout.readline, ""):
            self.out_queue.put(line)

    def read_line(self, timeout=15.0) -> str:
        try:
            line = self.out_queue.get(timeout=timeout)
            stripped = line.strip()
            if stripped:
                print(f"<- anyplay-net: {stripped}")
            return stripped
        except queue.Empty:
            if self.proc.poll() is not None:
                self.stderr_file.flush()
                err_content = self.stderr_path.read_text(encoding="utf-8", errors="replace")
                raise RuntimeError(f"anyplay-net terminated with {self.proc.returncode}: {err_content}")
            raise TimeoutError("timed out waiting for line from anyplay-net")

    def send_command(self, cmd_dict):
        line = json.dumps(cmd_dict)
        print(f"-> anyplay-net: {line}")
        self.proc.stdin.write(line + "\n")
        self.proc.stdin.flush()

    def close(self):
        try:
            self.stderr_file.close()
        except Exception:
            pass
        try:
            self.proc.stdin.close()
        except Exception:
            pass
        try:
            self.proc.terminate()
            self.proc.wait(timeout=3)
        except Exception:
            try:
                self.proc.kill()
            except Exception:
                pass


def main(argv=None):
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8", errors="backslashreplace")

    parser = argparse.ArgumentParser(description="run bridge CI end-to-end against anyplay-net")
    parser.add_argument("--anyplay-net", type=Path, required=True, help="path to anyplay-net executable")
    parser.add_argument("--fake-game", type=Path, default=None, help="path to anyplay-fake-game executable")
    parser.add_argument("--test-game", type=Path, required=True, help="path to bridge_e2e_game executable")
    args = parser.parse_args(argv)

    if not args.anyplay_net.is_file():
        parser.error(f"anyplay-net executable does not exist: {args.anyplay_net}")
    if not args.test_game.is_file():
        parser.error(f"test game executable does not exist: {args.test_game}")

    cloud_server, cloud_port = start_fake_cloud()
    cloud_url = f"http://127.0.0.1:{cloud_port}"
    print(f"Fake cloud listening at {cloud_url}")

    try:
        with tempfile.TemporaryDirectory(prefix="anyplay-bridge-signed-out-") as temp_dir:
            data_dir = Path(temp_dir)
            net = AnyplayNetProcess(args.anyplay_net, data_dir)
            try:
                net.send_command({"id": 1, "do": "hello", "launcher": "ci", "cloud": cloud_url})
                deadline = time.time() + 10.0
                while time.time() < deadline:
                    line = net.read_line()
                    try:
                        obj = json.loads(line)
                    except Exception:
                        continue
                    if obj.get("id") == 1 and "ok" in obj:
                        print("anyplay-net answered hello (signed-out phase)")
                        break

                net.send_command({
                    "id": 2,
                    "do": "game_start",
                    "title_id": TITLE_ID,
                    "title": "Test Game",
                    "version": "01.000",
                    "build": "0.1.3",
                })
                env_vars = {}
                deadline = time.time() + 10.0
                while time.time() < deadline and not env_vars:
                    line = net.read_line()
                    try:
                        obj = json.loads(line)
                    except Exception:
                        continue
                    if obj.get("id") == 2 and "ok" in obj:
                        env_vars = obj["ok"].get("env", {})

                if "ANYPS5_NET_ENDPOINT" not in env_vars or "ANYPS5_NET_TOKEN" not in env_vars:
                    raise RuntimeError(f"game_start did not return bridge variables: {env_vars}")

                env = dict(os.environ)
                env["ANYPS5_NET_ENDPOINT"] = env_vars["ANYPS5_NET_ENDPOINT"]
                env["ANYPS5_NET_TOKEN"] = env_vars["ANYPS5_NET_TOKEN"]
                env["ANYPS5_NET_OBSERVE"] = str(data_dir / "observation.jsonl")

                print("Running bridge test game in signed-out mode...")
                res = subprocess.run([str(args.test_game), "--signed-out"], env=env, capture_output=True, text=True, timeout=15)
                if res.returncode != 0:
                    print(res.stdout)
                    print(res.stderr, file=sys.stderr)
                    raise RuntimeError(f"bridge_e2e_game --signed-out failed with code {res.returncode}")
                print(res.stdout.strip())
                net.send_command({"id": 3, "do": "game_end"})
            finally:
                net.close()

        with tempfile.TemporaryDirectory(prefix="anyplay-bridge-signed-in-") as temp_dir:
            data_dir = Path(temp_dir)
            cloud_name = "".join(c if c.isalnum() else "_" for c in cloud_url)[:80]
            signin_file = data_dir / f"signin-{cloud_name}.json"
            signin_file.write_text(json.dumps({"device": 1, "refresh": "fake-refresh-token"}))

            net = AnyplayNetProcess(args.anyplay_net, data_dir)
            try:
                net.send_command({"id": 1, "do": "hello", "launcher": "ci", "cloud": cloud_url})

                signed_in = False
                deadline = time.time() + 20.0
                while time.time() < deadline and not signed_in:
                    line = net.read_line()
                    try:
                        obj = json.loads(line)
                    except Exception:
                        continue
                    if obj.get("id") == 1 and "ok" in obj:
                        print("anyplay-net answered hello (signed-in phase)")
                    if obj.get("event") == "state":
                        account = obj.get("account")
                        if account and account.get("id") == ACCOUNT_ID:
                            signed_in = True
                            print(f"anyplay-net reported signed in as {account.get('handle')}")

                if not signed_in:
                    raise RuntimeError("player did not reach signed in state")

                net.send_command({
                    "id": 2,
                    "do": "game_start",
                    "title_id": TITLE_ID,
                    "title": "Test Game",
                    "version": "01.000",
                    "build": "0.1.3",
                })

                env_vars = {}
                deadline = time.time() + 10.0
                while time.time() < deadline and not env_vars:
                    line = net.read_line()
                    try:
                        obj = json.loads(line)
                    except Exception:
                        continue
                    if obj.get("id") == 2 and "ok" in obj:
                        env_vars = obj["ok"].get("env", {})

                if "ANYPS5_NET_ENDPOINT" not in env_vars or "ANYPS5_NET_TOKEN" not in env_vars:
                    raise RuntimeError(f"game_start did not return bridge variables: {env_vars}")

                env = dict(os.environ)
                env["ANYPS5_NET_ENDPOINT"] = env_vars["ANYPS5_NET_ENDPOINT"]
                env["ANYPS5_NET_TOKEN"] = env_vars["ANYPS5_NET_TOKEN"]
                env["ANYPS5_NET_OBSERVE"] = str(data_dir / "observation.jsonl")

                if args.fake_game and args.fake_game.is_file():
                    print("Running anyplay-fake-game state...")
                    res = subprocess.run([str(args.fake_game), "state"], env=env, capture_output=True, text=True, timeout=10)
                    if res.returncode != 0:
                        raise RuntimeError(f"anyplay-fake-game failed (code {res.returncode}): {res.stderr}")
                    print(f"anyplay-fake-game state: {res.stdout.strip()}")

                print("Running AnyPS5 bridge end-to-end test game...")
                res = subprocess.run([str(args.test_game)], env=env, capture_output=True, text=True, timeout=15)
                if res.returncode != 0:
                    print(res.stdout)
                    print(res.stderr, file=sys.stderr)
                    raise RuntimeError(f"bridge_e2e_game failed with code {res.returncode}")
                print(res.stdout.strip())

                net.send_command({"id": 3, "do": "game_end"})
                print("Bridge CI end-to-end test passed successfully!")
            finally:
                net.close()

    finally:
        cloud_server.shutdown()


if __name__ == "__main__":
    sys.exit(main())
