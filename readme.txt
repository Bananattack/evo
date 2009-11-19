evo is an evolutionary algorithm library designed using C and pthreads.

On Windows, it requires pthreads-win32.
evo is only planned to be tested under 32-bit environments.

It was created as a final project for CIS*4450: Parallel Programming at the University of Guelph.
However, it might grow into something more if the project idea seems exceptionally promising.
I've not decided yet.

----

TODO:

* Get a working prototype by the end of the week.
* Add simple built-in mutation/crossover (for certain gene encodings, like strings).
* Add built-in selection operators: tournament selection, roulette/fitness-proportionate selection
* Currently using Windows multi-threaded CRT to do random across all threads.
  However, the problem with this is that each thread needs its own seed,
  and will not the same random sequence as a single-threaded app.
  
  I would like some way to expose random numbers across threads that is consistent with a single-threaded app.
  This way, I have identical results to a single-thread version.
  But, I'd like to do it without mutual exclusion. Hmm.
  
  The thing is, statstically, the randomness would be okay with independent seeds, but the results would not
  be comparable to a sequential app with a fixed seed. Ngggh.
  
