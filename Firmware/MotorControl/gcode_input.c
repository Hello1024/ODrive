#include "grbl.h"
#include "gcode_input.h"
#include "cmsis_os.h"
#include "control_along_line.h"

osMutexDef (MutexGcode);                                     // Mutex name definition
osMutexId mutex_gcode;

// TODO
float magic_constant_todo_with_mass = 2.f;  //counts/sec^2/A

void update_settings(void) {
    int i;
    for (i=0; i<2; i++) {
        // Maximum velocity in mm/minute
        settings.max_rate[i] = 
                motors[i].vel_limit /* [counts/s] */
                / (float) settings.steps_per_mm[i] * 60.f;

        // Maximum acceleration in mm/min^2. (multiply by 3600!)
        settings.acceleration[i] = 
                motors[i].current_control.current_lim  // Amps
                 * magic_constant_todo_with_mass      //counts/sec^2/A
                  / settings.steps_per_mm[i] * 3600;
    }
}

void consume_output(void) {
    // Gets the current block. Returns NULL if buffer empty
    plan_block_t *pb = plan_get_current_block();

    if (pb) {

        static float start[2];

        float end[2];
        end[0] = start[0] + pb->steps[0] * (pb->direction_bits&1)?1:-1;
        end[1] = start[1] + pb->steps[1] * (pb->direction_bits&2)?1:-1;

        // distance along normalized line (0-1) which we need to switch from 
        // acceleration to deceleration, assuming no constant speed part.
        // It is halfway in time and distance when you remove the difference 
        // between input and output speed.
        // V|
        // E|    .
        // L|   . .
        //  |  .   .
        //  | .     .
        //  |.      |         TIME
        //  ---------------------->


        // TODO

        float vel_limit = 
            plan_compute_profile_nominal_speed(pb) // mm/min
            * settings.steps_per_mm[0] / 60;
        // We begin by accelerating full speed towards the destination!
        // Note: motors[0].current_control.current_lim is for a single motor,
        //    yet the flag is shared across motors with pythagoras, so we 
        //    have spare headroom.
        current_control_along_line(start, end, motors[0].current_control.current_lim,
                                   mutex_gcode, NOTIFY_PAST_END | NOTIFY_OVER_LIMIT, vel_limit);
        osMutexWait(mutex_gcode, osWaitForever);
        osMutexRelease(mutex_gcode);
        if (get_notify_flags() & NOTIFY_OVER_LIMIT) {
            velocity_control_along_line(start, end, vel_limit, 0,
                                        mutex_gcode, NOTIFY_PAST_END);
            osMutexWait(mutex_gcode, osWaitForever);
            osMutexRelease(mutex_gcode);
        }

        // TODO:
        // Downward end of trapezoid velocity profile...

        update_settings();
        plan_discard_current_block();
    }
}



void parse_gcode_line(const char * line, SerialPrintf_t response_interface) {
    static int inited = 0;

    if (!inited) {
        grbl_init();
        update_settings();
        mutex_gcode = osMutexCreate (osMutex (MutexGcode));
        inited=1;
    }

    if (!line[0]) return;

    int status = gc_execute_line((char*)line);

    if (status == STATUS_OK) {
        printf("ok\r\n");
    } else {
        printf("error:%d\r\n", status);       
    }
}