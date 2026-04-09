# University Bus Simulation

C implementation of a bus simulation using POSIX threads and semaphores. Simulates students traveling from stops to the university and back, respecting bus capacity and department limits.

## Compilation

### Using gcc

```bash
gcc -pthread bus.c -o bus
```

### Using Makefile
```
make all
```

Remove executable and temporary files using:

```
make clean
```

The program prompts for the number of students. Each student is randomly assigned a department and a study time.

## Notes
* Uses POSIX threads (pthread) for concurrent simulation of students and the bus.
* Semaphores (sem_t) are used for synchronizing access to stops, bus, and print statements.
* Bus has a maximum capacity of 12 students, and each department can only have a maximum of 3 students on the bus at a time.
* Students wait at stops, board the bus when possible, travel to the university, study, then return to Stop B and take the bus home.
* Print statements show the current state of the bus, stops, and university.

---

*This was implemented for the Operating Systems Course*