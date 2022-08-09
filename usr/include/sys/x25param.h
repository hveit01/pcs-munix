/*
 * This file contains paramters for the X.25 driver.
 * They cannot be changed dynamically because
 * arrays of this size must be declared in the X.25 driver.

/*
 * The maximum length of a data/call packet.
 * If this parameter changes, the X.25 controller firmware has to be
 * recompiled too.
 */

#define PACKETSIZE 128

/*
 * The maximum number of channels which can be used.
 * If this parameter changes in the range between 1 and 127,
 * the firmware does not have to be recompiled.
 */

#define MAXCHAN 32
