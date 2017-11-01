// Allows motors to be constrained to move along a 2d line with a given 
// velocity and acceleration.  Will trigger a semaphore when the end of
// the line is reached.

osMutexDef (uart_mutex);    // Declare mutex
 // Mutex ID

void update_setpoints() {
    // set position setpoint to nearest point on line.
    float d = 
    	(motors[0].encoder.pll_pos - end[0]) * normalized_steps[0] +
    	(motors[1].encoder.pll_pos - end[1]) * normalized_steps[1];

    motors[0].pos_setpoint = end[0] + normalized_steps[0] * d;
    motors[1].pos_setpoint = end[1] + normalized_steps[1] * d;

    // See if we have arrived yet.
    if (d>0) {
    	motors[1].update_setpoints_fn = NULL;
    	osMutexRelease(done_mutex_id);
    }
}

void move_along_line(float* start, float* end, float velocity_feedforward, float current_feedforward, osMutexId done_mutex_id) {
    // We will release this when we get to the destination
    osMutexWait(done_mutex_id, 0);
    
    float steps[2];
    steps[0] = start[0]-end[0];
    steps[1] = start[1]-end[1];

    float normalized_steps[2];
    normalized_steps[0] = steps[0]/distance;
    normalized_steps[1] = steps[1]/distance;

    float distance = sqrt(steps[0]*steps[0]+steps[1]*steps[1]);

    // TODO: For now, we (wrongly) just use the end location as the setpoint.
    set_pos_setpoint(&motors[0], start[0], velocity_feedforward * normalized_steps[0], current_feedforward * normalized_steps[0]);
    set_pos_setpoint(&motors[1], start[1], velocity_feedforward * normalized_steps[1], current_feedforward * normalized_steps[1]);    
    motors[1].update_setpoints_fn = &update_setpoints;

}

