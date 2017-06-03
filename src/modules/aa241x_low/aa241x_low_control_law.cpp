/****************************************************************************
 *
 *   Copyright (c) 2013, 2014 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/*
 * @file aa241x_low.cpp
 *
 * Secondary control law file for AA241x.  Contains control/navigation
 * logic to be executed with a lower priority, and "slowly"
 *
 *  @author Adrien Perkins		<adrienp@stanford.edu>
 *  @author YOUR NAME			<YOU@EMAIL.COM>
 */


// include header file
#include "aa241x_low_control_law.h"
#include "aa241x_low_aux.h"

#include <uORB/uORB.h>
#define PI 3.14159265
using namespace aa241x_low;

/**
 * Main function in which your code should be written.
 *
 * This is the only function that is executed at a set interval,
 * feel free to add all the function you'd like, but make sure all
 * the code you'd like executed on a loop is in this function.
 *
 * This loop executes at ~50Hz, but is not guaranteed to be 50Hz every time.
 */

std::vector<float> lat_vals(3);
std::vector<float> lon_vals(3);
int target_idx = 0;
bool new_targets = false;
std::vector<target_s> target_list;

//target_list.reserve(5);

std::vector<target_s> order_targets(std::vector<float> tgt_x_list, std::vector<float> tgt_y_list, float in_x, float in_y, \
                               float in_v_x, float in_v_y) {
    int N = lat_vals.size();
    std::vector<int> idxs(N);
    std::iota(std::begin(idxs), std::end(idxs), 0);
    float min_time = 0;
    float min_dist = 0;
    std::vector<int> best_order(N);
    float radius = 10;
    float v_turn = 15;
    float v_straight = 12;
    while (std::next_permutation(std::begin(idxs), std::end(idxs))) {
        float dist = 0;
        float time = 0;
        float start_x = in_x;
        float start_y = in_y;
        float start_v_x = in_v_x;
        float start_v_y = in_v_y;
        for (int i; i<N; i++) {
            float tgt_x = tgt_x_list[i];
            float tgt_y = tgt_y_list[i];
            float v_tgt_x = tgt_x - start_x;
            float v_tgt_y = tgt_y - start_y;
            float norm_dist = (start_v_x * v_tgt_x + start_v_y * v_tgt_y) / (pow(start_v_x, 2.0f) + pow(start_v_y, 2.0f));
            float int_x = norm_dist * start_v_x + start_x;
            float int_y = norm_dist * start_v_y + start_y;
            float v_int_x = tgt_x - int_x;
            float v_int_y = tgt_y - int_y;
            float mask_x;
            float mask_y;
            if (start_v_x != 0) {
                mask_x = (v_int_x / start_v_x < 0) ? -1.0f : 1.0f;
            } else {
                mask_x = (v_int_y < 0) ? 1.0f : -1.0f;
            }
            if (start_v_y != 0) {
                mask_y = (v_int_y / start_v_y < 0) ? -1.0f : 1.0f;
            } else {
                mask_y = (v_int_x < 0) ? 1.0f : -1.0f;
            }
            float vxc = start_v_y * mask_x;
            float vyc = start_v_x * mask_y;
            float xc = start_x + radius * vxc / sqrt(pow(vxc, 2.0f) + pow(vyc, 2.0f));
            float yc = start_y + radius * vyc / sqrt(pow(vxc, 2.0f) + pow(vyc, 2.0f));

            float xd = tgt_x - xc;
            float yd = tgt_y - yc;
            //roots to find k
            float a_k = 8.0f * xc * tgt_x + 4.0f * pow(yd, 2.0f) - \
                4.0f * (pow(xc,2.0f) + pow(yc, 2.0f) + pow(tgt_y, 2.0f) - 2.0f * yc * tgt_y - pow(radius, 2.0f)) - \
                4.0f * pow(tgt_x, 2.0f);
            float b_k = 8.0f * yd * xd;
            float c_k = 4.0f * pow(xc, 2.0f) - 4.0f * (pow(xc,2.0f) + pow(yc, 2.0f) + pow(tgt_y, 2.0f) - 2.0f * yc * tgt_y - pow(radius, 2.0f));

            float k1 = (-b_k + sqrt(pow(b_k, 2.0f) - 4.0f * a_k * c_k + 1e-7)) / (2.0f * a_k);
            float k2 = (-b_k - sqrt(pow(b_k, 2.0f) - 4.0f * a_k * c_k + 1e-7)) / (2.0f * a_k);

            float a1 = pow(k1, 2.0f) + 1.0f;
            float a2 = pow(k2, 2.0f) + 1.0f;
            float b1 = -2.0f * tgt_x * pow(k1, 2.0f) + 2.0f * yd * k1 - 2.0f * xc;
            float b2 = -2.0f * tgt_x * pow(k2, 2.0f) + 2.0f * yd * k2 - 2.0f * xc;

            float x1 = -b1 / (2.0f * a1);
            float x2 = -b2 / (2.0f * a2);
            float y1 = k1 * (x1 - tgt_x) + tgt_y;
            float y2 = k1 * (x1 - tgt_x) + tgt_y;

            float v1x = tgt_x - x1;
            float v2x = tgt_x - x2;
            float v1y = tgt_y - y1;
            float v2y = tgt_y - y2;
            atan2 (y,x) * 180 / PI;
            float ang1raw = atan2 (y1 - yc,x1 - xc) * 180 / PI;
            float ang2raw = atan2 (y2 - yc,x2 - xc) * 180 / PI;
            float angstart = atan2 (start_y - yc,start_x - xc) * 180 / PI;
            float ang1 = (angstart*mask_x + ang1raw*mask_y) % 360;
            float ang2 = (angstart*mask_x + ang2raw*mask_y) % 360;

            float angmin = (ang1 < ang2) ? ang1 : ang2;
            float xmin = (ang1 < ang2) ? x1 : x2;
            float ymin = (ang1 < ang2) ? y1 : y2;
            float vminx = (ang1 < ang2) ? v1x : v2x;
            float vminy = (ang1 < ang2) ? v1y : v2y;

            float d_turn = angmin * PI / 180 * radius;
            float d_straight = sqrt(pow(tgt_x - xmin, 2.0f) + pow(tgt_y - ymin, 2.0f));
            float t_turn = d_turn / v_turn;
            float t_straight = d_straight / v_straight;
            dist += d_turn;
            dist += d_straight;
            time += t_turn;
            time += t_straight;
            start_x = tgt_x;
            start_y = tgt_y;
            start_v_x = vminx / sqrt(pow(vminx, 2.0f) + pow(vminy, 2.0f));
            start_v_y = vminy / sqrt(pow(vminx, 2.0f) + pow(vminy, 2.0f));

        }

    }


}

bool first_run = true;
void low_loop()
{

	float my_float_variable = 0.0f;		/**< example float variable */
    if (first_run) {
        target_list.reserve(5);
        target_s target1;
        target_s target2;
        target_s target3;

        target1.heading_desired = 0.0;
        target1.turnLeft = true;
        target1.pos_E = 0.0f; //CHANGE THIS!
        target1.pos_N = 0.0f; //CHANGE THIS!
        target_list.push_back(target1);

        target2.heading_desired = 1.5;
        target2.turnLeft = true;
        target2.pos_E = 10.0f; //CHANGE THIS!
        target2.pos_N = 10.0f; //CHANGE THIS!
        target_list.push_back(target2);

        target3.heading_desired = -1.5;
        target3.turnLeft = true;
        target3.pos_E = 10.0f; //CHANGE THIS!
        target3.pos_N = 10.0f; //CHANGE THIS!
        target_list.push_back(target3);

        first_run = false;
        new_targets = true;
    }

	// getting high data value example
	// float my_high_data = high_data.field1;

	// setting low data value example
	low_data.field1 = my_float_variable;


}
