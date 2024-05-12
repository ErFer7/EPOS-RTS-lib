'''
Testbench.
'''

from subprocess import run, CalledProcessError, DEVNULL, PIPE

TESTS = 100
APPLICATION = 'multicore'

try:
    print('>>> Running "make clean"')
    run(['make', 'clean'], check=True, stdout=DEVNULL)
    print('>>> Running "make veryclean"')
    run(['make', 'veryclean'], check=True, stdout=DEVNULL)
except CalledProcessError as exception:
    print('>>> Error:', exception)
    exit()

for i in range(TESTS):
    print(f'>>> Running test {i + 1}/{TESTS}')

    try:
        print(f'>>> Running "make APPLICATION={APPLICATION} run"')
        result = run(['make', f'APPLICATION={APPLICATION}', 'run'], check=True, stdout=PIPE)

        output = result.stdout.decode()
    except CalledProcessError as exception:
        print('>>> Error:', exception)
        break

    if 'IC:' in output and not 'Hello multicore world!' in output:
        print('>>> Failed')
        with open('output.txt', 'w', encoding='utf-8') as file:
            file.write(output)

        break

    print('>>> Passed')
