#ifndef __CONTROL_ALONG_LINE_H
#define __CONTROL_ALONG_LINE_H

#include "cmsis_os.h"
#include "low_level.h"

/*
Provides the ability to constrain motion to be along a 2D infinite line.

Motion along the line is controlled with a constant velocity regulator.
Motion not along the line is regulated with a position regulator to 
bring the machine head back onto the line.

In the typical case, your machine head will follow the path from start to
end at rate velocity.

By setting done_flags to NOTIFY_PAST_END, you will receive a notification
when it arrives.

Advanced usage:

 * Set "velocity_feedforward" to zero to make a brake/simulated stiff thing.
 * Set NOTIFY_BEFORE_START flag to get a notification if the machine head
   is pushed backwards along the line.
 * Use your notification callback to change the line segment.  This allows
   you to present a curved line, a maze, or anything else fun.
*/
void velocity_control_along_line(
	float* start, float* end, float velocity_feedforward,
	float current_feedforward, osMutexId done_mutex_id, int notify_flags);


/*
Provides the ability to constrain motion to be along a 2D infinite line.

Motion along the line is controlled with a constant current regulator.
Motion not along the line is regulated with a position regulator to 
bring the machine head back onto the line.

In the typical case, your machine head will accelerate along the path
as it sees force in the direction from start to end.  The rate of
acceleration is determined by the mass on the machine head, and is
uncontrolled.

By setting vel_alert_limit, and either NOTIFY_UNDER_LIMIT or 
NOTIFY_OVER_LIMIT notify_flags, you will get notified if the velocity (along the line,
in the direction from start to end) goes past the notification limit you
have set.

Advanced usage:

 * Set "current_feedforward" to zero to allow something to move freely along
   a path.
 * Set NOTIFY_BEFORE_START flag to get a notification if the machine head
   is pushed backwards along the line.
*/
void current_control_along_line(
	float* start, float* end, float current_feedforward, osMutexId done_mutex_id,
	int notify_flags, float vel_alert_limit);


#define NOTIFY_PAST_END (1<<0)
#define NOTIFY_BEFORE_START (1<<1)
#define NOTIFY_OVER_LIMIT (1<<2)
#define NOTIFY_UNDER_LIMIT (1<<3)

// Private/Internal below this line -----------------------------------------


typedef struct {
    float normalized_steps[2];
    float end[2];
    float distance;
    float vel_alert_limit;
    osMutexId done_mutex_id;
    int notify_flags;
    int notify_flag_mask;
 } Control_Along_Line_t;



 #endif