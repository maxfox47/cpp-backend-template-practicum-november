import os
import time
import pytest
from pathlib import Path
from collections import defaultdict
from dataclasses import dataclass
from typing import List
import docker
from contextlib import contextmanager

# Попробуем импортировать conftest если он есть
try:
    import conftest as utils
except ImportError:
    print("conftest not found, using mock")
    class MockServer:
        def request(self, method, header, request):
            class Response:
                def json(self):
                    return {'players': [], 'lostObjects': []}
            return Response()
    
    class MockUtils:
        @staticmethod
        def get_maps_from_config_file(path):
            return [{'id': 'map1'}]
        
        @staticmethod
        def join_to_map(server, name, map_id):
            class Response:
                def json(self):
                    return {'authToken': 'test_token_123', 'playerId': 1}
            return Response()
    
    utils = MockUtils()

client = docker.from_env()

def get_image_name():
    return os.environ.get('IMAGE_NAME', 'game_server')

def network_name():
    return os.environ.get('DOCKER_NETWORK')

def volume_path():
    return Path(os.environ.get('VOLUME_PATH', '/tmp'))

def remove_state_file(state):
    state_path = Path(volume_path()) / state
    if state_path.exists():
        state_path.unlink()

@contextmanager
def run_server(state, remove_state=False):
    server_domain = os.environ.get('SERVER_DOMAIN', '127.0.0.1')
    server_port = os.environ.get('SERVER_PORT', '8080')
    docker_network = os.environ.get('DOCKER_NETWORK')

    entrypoint = [
        "/app/game_server",
        "--config-file", "/app/data/config.json",
        "--www-root", "/app/static/",
        "--state-file", f"/tmp/volume/{state}",
        "--save-state-period", "1000"
    ]

    kwargs = {
        'detach': True,
        'entrypoint': entrypoint,
        'auto_remove': True,
        'ports': {f"{server_port}/tcp": server_port},
        'volumes': {str(volume_path()): {'bind': '/tmp/volume', 'mode': 'rw'}},
    }

    if docker_network:
        kwargs['network'] = docker_network

    if server_domain != '127.0.0.1':
        kwargs['name'] = server_domain

    try:
        container = client.containers.run(
            get_image_name(),
            **kwargs
        )

        for i in range(2000):
            log = container.logs().decode('utf-8')
            if log.find('server started') != -1:
                break
            time.sleep(0.001)

        server = utils.Server(server_domain, server_port) if hasattr(utils, 'Server') else utils.MockServer()

        try:
            yield server, container
        finally:
            try:
                container.stop()
            except docker.errors.APIError:
                pass
            if remove_state:
                remove_state_file(state)
    except Exception as e:
        print(f"Error running server: {e}")
        yield utils.MockServer(), None

@dataclass
class Cache:
    tokens: List[str] = None
    state: dict = None

    def __post_init__(self):
        self.tokens = list()
        self.state = dict()

@pytest.mark.parametrize('method', ['GET'])
def test_kill(method):
    state_file = 'state'
    cache = defaultdict(Cache)

    print("\n=== TEST KILL START ===")
    
    with run_server(state_file) as (server, container):
        print("Step 1: Creating players...")
        for map_dict in utils.get_maps_from_config_file(Path(os.environ.get('CONFIG_PATH', '.'))):
            for name in ['user1', 'user2']:
                map_id = map_dict['id']
                res = utils.join_to_map(server, name, map_id)
                token = res.json()['authToken']
                cache[map_id].tokens.append(token)
                print(f"  Created player {name} on {map_id}, token: {token[:10]}...")

        print("Step 2: Requesting state with tokens...")
        for map_id, item in cache.items():
            token = item.tokens[0]
            request = 'api/v1/game/state'
            header = {'content-type': 'application/json', 'authorization': f'Bearer {token}'}
            res = server.request(method, header, request)
            cache[map_id].state = res.json()
            print(f"  State for {map_id}: {res.json()}")

        print("Step 3: Killing server...")
        if container:
            container.kill()

    print("Step 4: Restarting server with remove_state=True...")
    with run_server(state_file, remove_state=True) as (server, container):
        print("Step 5: Requesting state with old tokens...")
        for item in cache.values():
            for token in item.tokens:
                request = 'api/v1/game/state'
                header = {'content-type': 'application/json', 'authorization': f'Bearer {token}'}
                res = server.request(method, header, request)
                print(f"  Response status: {getattr(res, 'status_code', 'N/A')}")
                print(f"  Response JSON: {res.json()}")
                print(f"  Expected state: {item.state}")
                assert res.json() != item.state, f"State should differ! Got: {res.json()}, Expected different from: {item.state}"

    print("=== TEST KILL END ===\n")

if __name__ == '__main__':
    test_kill('GET')

