/**
 * \file tictoc.h
 * \brief Simple interval timer for use in benchmarking.
 */

#ifndef TICTOC_H
#define TICTOC_H

/**
 * \brief Start interval timer.
 */
void tic();

/**
 * \brief Stop interval timer and report elapsed time in ms.
 * \return Elapsed time in milliseconds.
 */
double toc();

#endif // TICTOC_H

