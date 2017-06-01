/*
 * @file aa241x_fw_control.cpp
 *
 * Implementation of basic control laws (PID) for constant yaw, roll, pitch, and altitude.
 * Also includes constant heading and constant heading + altitude functions.
 *
 *  @author Elise FOURNIER-BIDOZ		<efb@stanford.edu>
 *  @author Alexandre EL ASSAD          <aelassad@stanford.edu>
 *  @author YOUR NAME			<YOU@EMAIL.COM>
 */

#include <uORB/uORB.h>

// include header file
#include "aa241x_high_control_law.h"
#include "aa241x_high_aux.h"

// needed for variable names
using namespace aa241x_high;

// define global variables (can be seen by all files in aa241x_high directory unless static keyword used)

//SHOULD WE DEFINE THESE FOR ROLL-FOR-HEADING ?
float previous_err_yaw = 0.0f;
float previous_integral_yaw = 0.0f;
float previous_err_pitch = 0.0f;
float previous_integral_pitch = 0.0f;
float previous_err_roll = 0.0f;
float previous_integral_roll = 0.0f;
float previous_err_h = 0.0f;
float previous_integral_h = 0.0f;
float previous_err_th = 0.0f;
float previous_integral_th = 0.0f;
float dt = 1.0/60;

int flight_mode = aah_parameters.flight_mode;
float velocity_desired = 0.0f;
float altitude_desired = 0.0f;
float grndspeed_desired = 0.0f;
float heading_desired = 0.0f;

/**
 * Main function in which your code should be written.
 *
 * This is the only function that is executed at a set interval,
 * feel free to add all the function you'd like, but make sure all
 * the code you'd like executed on a loop is in this function.
 */
in_state_s roll_s;
in_state_s pitch_s;
in_state_s yaw_s;
in_state_s vel_s;
in_state_s alt_s;
in_state_s heading_s;
in_state_s rollForHeading_s;

logger_s data_to_log;


void UpdateInputs(in_state_s & in_roll, \
                  in_state_s & in_pitch, \
                  in_state_s & in_yaw, \
                  in_state_s & in_vel, \
                  in_state_s & in_alt, \
                  in_state_s & in_heading, \
                  in_state_s & in_rollForHeading \
                  ) {
//Update Parameter stuff
    flight_mode = aah_parameters.flight_mode;

    in_roll.kp = aah_parameters.proportional_roll_gain;
    in_roll.kd = aah_parameters.derivative_roll_gain;
    in_roll.ki = aah_parameters.integrator_roll_gain;
    in_roll.current = roll;
    in_roll.desired = roll_desired;

    in_pitch.kp = aah_parameters.proportional_pitch_gain;
    in_pitch.kd = aah_parameters.derivative_pitch_gain;
    in_pitch.ki = aah_parameters.integrator_pitch_gain;
    in_pitch.current = pitch;
    in_pitch.desired = pitch_desired;

    in_yaw.kp = aah_parameters.proportional_yaw_gain;
    in_yaw.kd = aah_parameters.derivative_yaw_gain;
    in_yaw.ki = aah_parameters.integrator_yaw_gain;
    in_yaw.current = yaw;
    in_yaw.desired = yaw_desired;

    in_vel.kp = aah_parameters.proportional_velocity_gain;
    in_vel.kd = aah_parameters.derivative_velocity_gain;
    in_vel.ki = aah_parameters.integrator_velocity_gain;
    in_vel.current = speed_body_u;
    in_vel.desired = velocity_desired;

    in_alt.kp = aah_parameters.proportional_altitude_gain;
    in_alt.kd = aah_parameters.derivative_altitude_gain;
    in_alt.ki = aah_parameters.integrator_altitude_gain;
    in_alt.current = position_D_baro;
    in_alt.desired = altitude_desired;

    in_heading.kp = aah_parameters.proportional_heading_gain;
    in_heading.kd = aah_parameters.derivative_heading_gain;
    in_heading.ki = aah_parameters.integrator_heading_gain;
    in_heading.current = ground_course;
    in_heading.desired = heading_desired;

    in_rollForHeading.kp = aah_parameters.proportional_rollForHeading_gain;
    in_rollForHeading.kd = aah_parameters.derivative_rollForHeading_gain;
    in_rollForHeading.ki = aah_parameters.integrator_rollForHeading_gain;
    in_rollForHeading.current = roll;
    in_rollForHeading.desired = rollForHeading_desired;
}

MazController mazController;
//mazController.GetLogData(data_to_log);

void flight_control() {
    if (hrt_absolute_time() - previous_loop_timestamp > 500000.0f) { // Run if more than 0.5 seconds have passes since last loop,
                                                                 //	should only occur on first engagement since this is 59Hz loop
        yaw_desired = yaw;
        roll_desired = roll;
        pitch_desired = pitch;
        velocity_desired = speed_body_u;
        heading_desired = ground_course;
        altitude_desired = position_D_baro;
        rollForHeading_desired = roll;
        mazController.SetPos(position_N, position_E);
         							// yaw_desired already defined in aa241x_high_aux.h
//    altitude_desired = position_D_baro; 		// altitude_desired needs to be declared outside flight_control() function
	}

	//Set Default Outputs to manual;
	output_s outputs;
	outputs.pitch = man_pitch_in;
	outputs.roll = man_roll_in;
	outputs.yaw = man_yaw_in;
	outputs.throttle = man_throttle_in;
	outputs.rollForHeading = man_roll_in;

	UpdateInputs(roll_s, pitch_s, yaw_s, vel_s, alt_s, heading_s, rollForHeading_s);

    mazController.Controller(flight_mode, outputs, \
                             roll_s, \
                             pitch_s, \
                             yaw_s, \
                             vel_s, \
                             alt_s, \
                             heading_s,\
                             rollForHeading_s \
                             );

    mazController.GetLogData(data_to_log);

    yaw_servo_out = outputs.yaw;
    pitch_servo_out = -outputs.pitch; // Negative for preferred control inversion
    roll_servo_out = outputs.roll;
    throttle_servo_out = outputs.throttle;

}
