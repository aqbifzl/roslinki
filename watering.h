#ifndef WATERING_H
#define WATERING_H

void watering_init();
void watering_start(int sensor);
void watering_stop(int sensor);
void watering_handle_state();

#endif
