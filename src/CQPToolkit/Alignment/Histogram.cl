/*!
* @file
* @brief Histogram
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18 Sep 2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#define RANGE_SIZE 8192

kernel void histogram(__global uint* data, __constant int dataSize){
    // get the id of this work item
    int wid = get_local_id(0);
    int wSize = get_local_size(0);

    int gid = get_group_id(0);
    int numGroups = get_num_groups(0);

    int rangeStart = gid * RANGE_SIZE / numGroups;
    int rangeEnd = (gid+1) * RANGE_SIZE / numGroups;

    local uint tmp_histogram[RANGE_SIZE];

    uint value;

    for(int i=wid; i< dataSize; i+= wSize){
        value = data[i];
        if(value >= rangeStart && value < rangeEnd){
            atomic_inc(tmp_histogram[value - rangeStart]);
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    //use the local data here
}
