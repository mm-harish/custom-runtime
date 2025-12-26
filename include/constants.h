#ifndef CONSTANTS_H
#define CONSTANTS_H

/**
 * @file constants.h
 * @brief Defines global constants used throughout the runtime.
 */

#define numframes 32       ///< Number of frames per block in the TaskPool.
#define m 4                ///< Size of the local ready queue.
#define n 65536 * 64       ///< Size of the steal ready queue.

#endif // CONSTANTS_H
