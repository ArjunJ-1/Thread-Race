Write a short C++ console program which performs a simple car traffic simulation on a figure-eight track.

The intersection is controlled by traffic lights.

The program should use multiple threads, each of which is responsible for updating the position of one car.

These should perform the logic and mechanics of the simulation, with no "controlling" thread required beyond the initial setup and basic input/output.

Each car should have a unique acceleration rate and maximum speed which is assigned at startup, but all travel in the same direction.

There are two traffic lanes, to allow overtaking.

Cars should avoid crashing into each other or driving through a red light, but should change lanes when required to pass a slower vehicle.

The simulation should run for five minutes, with the main thread providing a simple "ASCII art" print of the current status every 10 seconds.

The code should be portable.
