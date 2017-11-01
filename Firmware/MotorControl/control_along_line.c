// Allows motors to be constrained to move along a 2d line with a given 
// velocity and acceleration.  Will trigger a semaphore when the end of
// the line is reached.

#include "control_along_line.h"
#include <math.h>


Control_Along_Line_t cal_state;

void update_setpoints_constant_velocity() {
    // set position setpoint to nearest point on line.
    // d = dot(pll_pos - end, normalized_steps)
    float d = 
        (motors[0].encoder.pll_pos - cal_state.end[0]) * cal_state.normalized_steps[0] +
        (motors[1].encoder.pll_pos - cal_state.end[1]) * cal_state.normalized_steps[1];

    motors[0].pos_setpoint = cal_state.end[0] + cal_state.normalized_steps[0] * d;
    motors[1].pos_setpoint = cal_state.end[1] + cal_state.normalized_steps[1] * d;

    // See if we have arrived yet.
    if (d > 0) cal_state.notify_flags |= NOTIFY_PAST_END;
    if (d < -cal_state.distance) cal_state.notify_flags |= NOTIFY_BEFORE_START;

    if (cal_state.notify_flags & cal_state.notify_flag_mask) {
        motors[1].update_setpoints_fn = NULL;
        osMutexRelease(cal_state.done_mutex_id);
        return;
    }
    cal_state.notify_flags = 0;
}

void update_setpoints_constant_current() {

    // Velocity setpoint is set to match velocity component parallel to line.
    float vel_perp = 
        get_pll_vel(&motors[0]) * cal_state.normalized_steps[0] +
        get_pll_vel(&motors[1]) * cal_state.normalized_steps[1];

    motors[0].vel_setpoint = cal_state.normalized_steps[0] * vel_perp;
    motors[1].vel_setpoint = cal_state.normalized_steps[1] * vel_perp;

    // See if we have arrived yet.
    if (vel_perp > cal_state.vel_alert_limit) {
        cal_state.notify_flags |= NOTIFY_OVER_LIMIT;
    } else { 
        cal_state.notify_flags |= NOTIFY_UNDER_LIMIT;
    }  

    update_setpoints_constant_velocity();
}





void line_control(float* start, float* end, float velocity_feedforward, float current_feedforward,
                  void (*update_setpoints_fn)(), osMutexId done_mutex_id, int notify_flags_mask) {

    motors[1].update_setpoints_fn = NULL;

    // We will release this when we get to the destination
    osMutexWait(cal_state.done_mutex_id = done_mutex_id, 0);
    
    float steps[2];
    steps[0] = start[0]-end[0];
    steps[1] = start[1]-end[1];

    cal_state.distance = sqrt(steps[0]*steps[0]+steps[1]*steps[1]);

    cal_state.normalized_steps[0] = steps[0] / cal_state.distance;
    cal_state.normalized_steps[1] = steps[1] / cal_state.distance;
    
    cal_state.notify_flags = 0;
    cal_state.notify_flag_mask = notify_flags_mask;

    set_pos_setpoint(&motors[0], 0, velocity_feedforward * cal_state.normalized_steps[0], 
                     current_feedforward * cal_state.normalized_steps[0]);
    set_pos_setpoint(&motors[1], 0, velocity_feedforward * cal_state.normalized_steps[1],
                     current_feedforward * cal_state.normalized_steps[1]);    
    motors[1].update_setpoints_fn = update_setpoints_fn;

    (*update_setpoints_fn)();
}


void velocity_control_along_line(float* start, float* end, float velocity_feedforward,
                                 float current_feedforward, osMutexId done_mutex_id, int done_flags) {
    line_control(start, end, velocity_feedforward, current_feedforward, update_setpoints_constant_velocity, done_mutex_id, done_flags);
}

void current_control_along_line(float* start, float* end, float current_feedforward, 
                                osMutexId done_mutex_id, int done_flags, float vel_alert_limit) {
    motors[1].update_setpoints_fn = NULL;
    cal_state.vel_alert_limit = vel_alert_limit;
    line_control(start, end, 0.f, current_feedforward, update_setpoints_constant_current, done_mutex_id, done_flags);
}
