Since we now load our desired SDL symbols at runtime, this set gutted SDL includes
contains only what is needed for a few types we need for event handling.
All the function declarations have been yanked out, as well as a bunch of other
stuff.
If issues arise at compile time, a fix would probable be to just copy in a full
SDL header.
