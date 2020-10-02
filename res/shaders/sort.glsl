#version 430 core

const uint block_sum_size = 256;



layout(std430,binding = 0) buffer d_in{_T_ _input[];};
layout(std430,binding = 1) buffer ki{uint keys_in[];};
layout(std430,binding = 2) buffer d_out{_T_ _output[];};
layout(std430,binding = 3) buffer ko{uint keys_out[];};

layout(std430,binding = 4) buffer blk_sum{uint block_sums[];};
layout(std430,binding = 5) buffer a{uint atomics[];};
layout(std430,binding = 6) buffer d_h{uint histo[];};



uniform int stage;
uniform uint count;
uniform uint nkeys;
uniform uint pass;


const uint numKeys = 2048;
shared uint _keys[numKeys];
shared uint buckets[2048]; // 12 bits
shared uint ids[numKeys];
shared uint local_sums[256];

const uint workSize = 2048 / 256;
void subSort(uint g_id){
    
    // 2048 / 128 = 16
    uint l_id = gl_LocalInvocationID.x;
    uint w_id = gl_WorkGroupID.x;
    uint start = l_id * workSize;
    uint end = start + workSize;

    uint buckStart = l_id * 8;
    uint buckEnd = buckStart + 8;

    uint globalOffset = numKeys * w_id;
    for(uint i = buckStart; i < buckEnd; ++i){
        buckets[i] = 0;
    }

barrier();
    if(end + globalOffset > nkeys){
        end = nkeys - globalOffset;
    }
    for(uint i = start; i < end; ++i){
        // uint k = key(_output[globalOffset + i]);
        uint k = keys_out[globalOffset + i];
        k >>= 5;
        _keys[i] = k;
        ids[i] = globalOffset + i;
        atomicAdd(buckets[k],1);
    }
barrier();

    uint temp = buckets[buckStart];
    buckets[buckStart] = 0;
    for(uint i = buckStart + 1; i < buckEnd; i++){
        uint temp2 = buckets[i];
        buckets[i] = temp;
        temp += temp2;
    }
    local_sums[l_id] = temp;
barrier();
    if(l_id == 0){
        uint temp = globalOffset +  local_sums[0];
        local_sums[0] = globalOffset;
        for(uint i = 1; i < 256; i++){
            uint temp2 = local_sums[i];
            local_sums[i] = temp;
            temp += temp2;
        }
    }
barrier();
    for(uint i = buckStart; i < buckEnd; ++i){
        // uint group_id = gl_WorkGroupID.x;
        buckets[i] += local_sums[l_id];
    }
barrier();

    for(uint i = start; i < end; ++i){
        uint index = atomicAdd(buckets[_keys[i]],1);
        _input[index] = _output[ids[i]];
        keys_in[index] = keys_out[ids[i]];

    }
}


void radix(uint g_id){
    uint index;
    uint temp;
    // first pass
    switch(stage){
        case -1:
            atomicAdd(histo[keys_in[g_id]],1);
            break;
        case 0:
            subSort(g_id);
            break;
        case 1:
            uint start = g_id * block_sum_size;
            uint end = start + block_sum_size;
            
            temp = histo[start];
            histo[start] = 0;
            for(uint i = start + 1; i < end; i++){
                uint temp2 = histo[i];
                histo[i] = temp;
                temp += temp2;
            }
            block_sums[g_id] = temp;
            break;
        case 2:
            temp = block_sums[0];
            block_sums[0] = 0;
            for(uint i = 1; i < block_sum_size; i++){
                uint temp2 = block_sums[i];
                block_sums[i] = temp;
                temp += temp2;
            }
            break;
        case 3:
            histo[g_id] += block_sums[g_id/block_sum_size];
            break;
        case 4:
            _T_ item = _input[g_id]; 
            // index = key(item);
            index = keys_in[g_id]; 
            _output[atomicAdd(histo[index],1)] = item;
            break;
    }
    // // second pass
    // else if(stage == 3){
    //     index = data2[gid].key >> 16;
    //     atomicAdd(counts[index],1);
    // }
    // else if(gid == 0 && stage == 4){
    //     offsets[0] = 0;
    //     for(int i = 1; i < 65536; i++){
    //         offsets[i] = offsets[i - 1] + counts[i - 1];
    //     }
    // }else if(stage == 5){
    //     index = data2[gid].key >> 16;
    //     data1[atomics[0] - atomicAdd(offsets[index],1)] = data2[gid];
    // }
}

uint convertFloatToClamped(float f){
    return uint(f * 32768 + 32768);
}

layout( local_size_x = 256,  local_size_y = 1, local_size_z = 1) in;
void main () {
    uint gid = gl_GlobalInvocationID.x;
    uint index;
    if(gid < count){
        radix(gid);
    }
}