import argparse
import subprocess
import time
import random
import shlex
import os
import signal

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server


def run(command, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process


def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()


def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


def main():
    server_command = start_server()
    
    # Запускаем сервер в фоновом процессе
    server_process = run(server_command)
    
    # Ждем, чтобы сервер успел запуститься
    time.sleep(2)
    
    # Запускаем perf record для процесса сервера с записью call stacks
    perf_record_cmd = ['perf', 'record', '-g', '-o', 'perf.data', '-p', str(server_process.pid)]
    perf_process = subprocess.Popen(perf_record_cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    # Выполняем обстрелы
    make_shots()
    
    # Корректно завершаем perf record (отправляем SIGINT)
    perf_process.send_signal(signal.SIGINT)
    perf_process.wait()
    
    # Останавливаем сервер
    stop(server_process)
    
    # Строим флеймграф через двойной пайп
    script_dir = os.path.dirname(os.path.abspath(__file__))
    flamegraph_dir = os.path.join(script_dir, 'FlameGraph')
    
    perf_script = subprocess.Popen(
        ['perf', 'script', '-i', 'perf.data'],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL
    )
    
    stackcollapse = subprocess.Popen(
        [os.path.join(flamegraph_dir, 'stackcollapse-perf.pl')],
        stdin=perf_script.stdout,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL
    )
    
    perf_script.stdout.close()
    
    with open('graph.svg', 'w') as svg_file:
        flamegraph = subprocess.Popen(
            [os.path.join(flamegraph_dir, 'flamegraph.pl')],
            stdin=stackcollapse.stdout,
            stdout=svg_file,
            stderr=subprocess.DEVNULL
        )
        stackcollapse.stdout.close()
        flamegraph.wait()
    
    stackcollapse.wait()
    perf_script.wait()
    
    print('Job done')


if __name__ == '__main__':
    main()

