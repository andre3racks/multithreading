A multithreaded train scheduling system written in C.

Please see the specification for the problem definition.

An exerpt from the specification:

`Each train, which will be simulated by a thread, has the following attributes:
1. Number: an integer uniquely identifying each train.

2. Direction:
• If the direction of a train is Westbound, it starts from the East station and travels to the West station.
• If the direction of a train is Eastbound, it starts from the West station and travels to the East station.

3. Priority: The priority of the station from which it departs.
4. Loading Time: The amount of time that it takes to load it (with goods) before it is ready to depart.
5. Crossing Time: The amount of time that the train takes to cross the main track.
Loading time and crossing time are measured in 10ths of a second. These durations will be simulated by having
your threads, which represent trains, usleep() for the required amount of time.

Step 2: Simulation Rules
The rules enforced by the automated control system are:
1. Only one train is on the main track at any given time.
2. Only loaded trains can cross the main track.
3. If there are multiple loaded trains, the one with the high priority crosses.
4. If two loaded trains have the same priority, then:
(a) If they are both traveling in the same direction, the train which finished loading first gets the clearance
to cross first. If they finished loading at the same time, the one appeared first in the input file gets the
clearance to cross first.
(b) If they are traveling in opposite directions, pick the train which will travel in the direction opposite of
which the last train to cross the main track traveled. If no trains have crossed the main track yet, the
Eastbound train has the priority.
(c) To avoid starvation, if there are three trains in the same direction traveled through the main track back
to back, the trains waiting in the opposite direction get a chance to dispatch one train if any.
 `
