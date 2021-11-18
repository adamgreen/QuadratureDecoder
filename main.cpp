/* Copyright 2021 Adam Green (https://github.com/adamgreen/)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
// Sample code that uses the QuadratureDecoder class to dump the encoder counts encountered each second.
#include <stdio.h>
#include "QuadratureDecoder.h"


int main(void)
{
    QuadratureDecoder decoder;
    int32_t lastCounter = 0;

    stdio_init_all();

    // Configure the pins connected to the encoder to be inputs with no pull-up or pull-down as my encoders already
    // had external pull-ups on the hall effect sensors.
    const uint pinBase = 2;
    gpio_init(pinBase+0);
    gpio_init(pinBase+1);
    gpio_disable_pulls(pinBase+0);
    gpio_disable_pulls(pinBase+1);

    // Initialize the PIO to count the quadrature encoder ticks in the background.
    decoder.init(pio0);
    int32_t index = decoder.addQuadratureEncoder(pinBase);

    // Dump the encoder counts encountered during a fixed interval.
    absolute_time_t lastSampleTime = get_absolute_time();
    while (1)
    {
        absolute_time_t nextSampleTime = delayed_by_us(lastSampleTime, 1000000);
        while (get_absolute_time() < nextSampleTime)
        {
            // Busy Wait.
        }
        lastSampleTime = nextSampleTime;

        int32_t currCounter = decoder.getCount(index);
        int32_t deltaCounter = currCounter - lastCounter;
        lastCounter = currCounter;
        printf("%ld\n", deltaCounter);
    }
}
