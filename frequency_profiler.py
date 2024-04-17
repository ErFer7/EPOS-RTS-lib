'''
Profiler para o EPOS.

Requisitos: Python 3.10.x ou mais alto.
'''

from os.path import join
from subprocess import run, CalledProcessError, DEVNULL
from collections import defaultdict

APPLICATION = 'ea_test'
FREQUENCY_STEP = 50  # Passo de 50 Hz
MEASUREMENTS = 3  # Número de medições por frequência
FREQUENCY_RANGE = (100, 10000)  # Hz

MTIMER_INTERRUPT = 'desc=m_timer'
STIMER_INTERRUPT = 'desc=s_timer'
ECALL_INTERRUPT = 'desc=supervisor_ecall'

ALLOWED_SEQUENCE = defaultdict(lambda: [])
ALLOWED_SEQUENCE[MTIMER_INTERRUPT] = [MTIMER_INTERRUPT, STIMER_INTERRUPT]
ALLOWED_SEQUENCE[STIMER_INTERRUPT] = [ECALL_INTERRUPT]
ALLOWED_SEQUENCE[ECALL_INTERRUPT] = [MTIMER_INTERRUPT]

original_frequency = 0

print('>>> EPOS profiler')

higher_bound = FREQUENCY_RANGE[1]
lower_bound = FREQUENCY_RANGE[0]
current_frequency = higher_bound
highest_unsafe_working_frequency = higher_bound

while True:
    print(f'>>> Profiling frequency: {current_frequency} Hz')
    print('>>> Opening sifive_u_traits.h')
    with open(join('include', 'machine', 'riscv', 'sifive_u', 'sifive_u_traits.h'), 'r+', encoding='utf-8') as traits_file:
        lines = traits_file.readlines()

        for i, line in enumerate(lines):
            if line.startswith('    static const long FREQUENCY'):
                original_frequency = int(line.split()[5][:-1])
                lines[i] = f'    static const long FREQUENCY = {current_frequency}; // Hz\n'
                break

        traits_file.seek(0)
        traits_file.writelines(lines)
        traits_file.truncate()
        print('>>> Frequency set on sifive_u_traits.h')

    try:
        errors = 0

        for i in range(MEASUREMENTS):
            print(f'>>> Running measurement {i + 1}/{MEASUREMENTS}')

            print('>>> Running "make clean"')
            run(['make', 'clean'], check=True, stdout=DEVNULL)
            print('>>> Running "make veryclean"')
            run(['make', 'veryclean'], check=True, stdout=DEVNULL)
            print(f'>>> Running "make APPLICATION={APPLICATION} run"')
            run(['make', f'APPLICATION={APPLICATION}', 'run'], check=True, stdout=DEVNULL)

            log: list[str] = []

            print('>>> Opening log file')
            with open(join('img', f'{APPLICATION}.log'), 'r', encoding='utf-8') as log_file:
                log = log_file.readlines()

            previous_call_code = ''
            current_call_code = ''

            for line in log:
                if line.startswith('riscv_cpu_do_interrupt'):
                    current_call_code = line.split()[-1]

                    if previous_call_code == '':
                        previous_call_code = current_call_code
                        continue

                    if current_call_code not in ALLOWED_SEQUENCE[previous_call_code]:
                        print(f'>>> Error detected: {previous_call_code} -> {current_call_code}')
                        errors += 1
                        break

                    previous_call_code = current_call_code

        print(f'>>> Frequency {current_frequency} tested, {errors}/{MEASUREMENTS} timing errors detected')

        if errors > 0:
            if errors < MEASUREMENTS:
                highest_unsafe_working_frequency = current_frequency

            if higher_bound - lower_bound > FREQUENCY_STEP:
                higher_bound = current_frequency
                current_frequency = (current_frequency + lower_bound) // 2
            else:
                current_frequency -= FREQUENCY_STEP

                if current_frequency < FREQUENCY_RANGE[0]:
                    current_frequency = FREQUENCY_RANGE[0]
                    break

                higher_bound = current_frequency
                lower_bound = current_frequency - FREQUENCY_STEP
        else:
            if higher_bound - lower_bound > FREQUENCY_STEP:
                lower_bound = current_frequency
                current_frequency = (higher_bound + lower_bound) // 2

                if current_frequency > FREQUENCY_RANGE[1]:
                    current_frequency = FREQUENCY_RANGE[1]
                    break
            else:
                break
    except CalledProcessError as exception:
        print(f'>>> Exception while running tests: {exception}')
        break

print('>>> Opening sifive_u_traits.h')
with open(join('include', 'machine', 'riscv', 'sifive_u', 'sifive_u_traits.h'), 'r+', encoding='utf-8') as traits_file:
    lines = traits_file.readlines()

    for i, line in enumerate(lines):
        if line.startswith('    static const long FREQUENCY'):
            lines[i] = f'    static const long FREQUENCY = {original_frequency}; // Hz\n'
            break

    traits_file.seek(0)
    traits_file.writelines(lines)
    traits_file.truncate()
    print('>>> Original frequency set on sifive_u_traits.h')

print('\n>>> Results')
print(f'>>> Highest unsafe working frequency: {highest_unsafe_working_frequency} Hz')
print(f'>>> Highest safe working frequency {current_frequency} Hz')

recomended_frequency = ((current_frequency - FREQUENCY_RANGE[0]) //
                        FREQUENCY_STEP) * FREQUENCY_STEP + FREQUENCY_RANGE[0]

print(f'>>> Recommended frequency: {recomended_frequency}')
