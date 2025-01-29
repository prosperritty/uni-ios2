# IOS 2. project
The goal of this project is to practically understand how processes, fork, and semaphores work by implementing a solution to the problem with synchronization.
### Evaluation 15/15

# Task description
There are three processes in this task that have their own goal: __main process__, __skibus__ and __skier__. <br/>

Every skier goes after breakfast to one boarding stop of the ski bus, where it is waiting for the bus to arrive. After the arrival of the bus at the boarding gate
skiers board the stop. If the capacity of the bus is full, the remaining skiers wait for the next arrival. The bus gradually serves all boarding stops and takes skiers to the exit stop at the cable car. If there are other people interested in the ride, the skibus makes another pass.

# Usage
Build:
```console
$ make      
gcc proj2.c -o proj2 -std=gnu99 -Wall -Wextra -Werror -pedantic -pthread -lrt
```
Use:
```console
$ ./proj2 L Z K TL TB
```
Where:
- `L` number of skiers, `L`<20000
- `Z` number of boarding stops, 0<`Z`<=10
- `K` ski bus capacity, 10<=`K`<=100
- `TL` maximum time in microseconds the skier waits before coming to a stop,  0<=`TL`<=10000
- `TB` maximum bus travel time between two stops, 0<=`TB`<=1000

# Processes
### Main process
The main process creates all other processes, one skibus process and `L` skiers processes. The main process will also determine which stop each skier will go to. After the previous actions were successful, it waits for the end of the created processes.

### Skibus process
Skibus process will generate actions and write them out:
- After startup writes `N: BUS: started`
- Goes to the idZ stop, usleep between <0, `TB`> then writes `N: BUS: arrived to idZ`
- It lets all the waiting skiers board the skibus
- Leaves idZ stop writes `N: BUS: leaving idZ`
- If skibus arrives at the final stop writes `N: BUS: arrived to final` and `N: BUS: leaving final` when leaving
- If there are no more skiers waiting writes `N: BUS: finish` and skibus process finishes, otherwise skibus makes another pass

### Skier process
Skier process will generate actions and write them out:
- Each skier is uniquely identified by an `idL` number
- After startup writes `N: L idL: started`
- They finish their breakfast, usleep between <0, `TL`>
- Goes to the assigned `idZ` stop writes `N: L idL: arrived to idZ` and waiting for the arrival of the skibus
- After the arrival of the skibus, they board (if there is free capacity) writes `N: L idL: boarding`
- When the skibus arrives at its final stop writes `N: L idL: going to ski` and skier process finishes

# Example
```console
$ ./proj2 2 2 10 100 100 && cat proj2.out
1: BUS: started
2: L 2: started
3: BUS: arrived to 1
4: BUS: leaving 1
5: L 1: started
6: L 2: arrived to 1
7: BUS: arrived to 2
8: BUS: leaving 2
9: BUS: arrived to final
10: BUS: leaving final
11: L 1: arrived to 2
12: BUS: arrived to 1
13: L 2: boarding
14: BUS: leaving 1
15: BUS: arrived to 2
16: L 1: boarding
17: BUS: leaving 2
18: BUS: arrived to final
19: L 2: going to ski
20: L 1: going to ski
21: BUS: leaving final
22: BUS: finish
```
