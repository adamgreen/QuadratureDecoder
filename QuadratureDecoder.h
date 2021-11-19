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
// Class to use the RP2040's PIO state machines to count quadrature encoder ticks.
#ifndef QUADRATURE_DECODER_H_
#define QUADRATURE_DECODER_H_

#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/dma.h>


class QuadratureDecoder
{
    public:
        // Constructor just sets up object. Need to call init() and addQuadratureEncoder() methods to actually initiliaze
        // the PIO state machines to count quadrature encoder ticks in the background.
        QuadratureDecoder();

        // Call this init() method once to load the assembly language code into the caller specified PIO (pio0 or pio1).
        // The assembly language program used for quadrature decoding contains a 16 element instruction jump table so it
        // needs to be loaded at offset 0 in the PIO and it only leaves a few free instruction slots, 4, after being
        // loaded. Once this method has been called, the addQuadratureEncoder() method can be called for each
        // quadrature encoder that need to be decoded.
        //  pio - The PIO instance to be used, pio0 or pio1.
        // Returns true if everything was initialized successfully.
        // Returns false if the assembly language code fails to load into the specified PIO instance.
        bool init(PIO pio);

        // Call this method to register a set of quadrature encoder pins to be counted in the background. The maximum
        // number of encoders that can be registered will be limited to 4, the number of state machines per PIO
        // instance.
        //  pinBase - The lowest numbered GPIO pin connected to the quadrature encoder. The other signal wire from
        //            the encoder needs to be connected to the next highest GPIO pin (examples: pins 2&3, pins 3&4, etc).
        //  Returns -1 if it failed to allocate the required state machine or DMA resources.
        //  Returns a value >= 0 specifying the index to be used in future calls to the getCount() method.
        int32_t addQuadratureEncoder(uint32_t pinBase);

        // Call this method to get the current count for a quadrature encoder previously registered with the
        // addQuadratureEncoder() method.
        //  index - An index value returned from a previous call to addQuadratureEncoder(). This specifies which of the
        //          registered quadrature encoder counts you want.
        //  Returns the accumulated counts so far for the specified quadrature encoder.
        inline int32_t getCount(int32_t index)
        {
            hard_assert ( index >= 0 && index < (int32_t)(sizeof(m_counters)/sizeof(m_counters[0])) );
            int32_t count = m_counters[index];
            restartDmaBeforeItStops(index);
            return count;
        }

    protected:
        enum { DMA_MAX_TRANSFER_COUNT = 0xFFFFFFFF, DMA_REFRESH_THRESHOLD = 0x80000000 };

        // Can only queue up 0xFFFFFFFF DMA transfers at a time. Every once in awhile we will want to reset the
        // transfer count back to 0xFFFFFFFF so that it doesn't stop pulling the latest encoder counts from the PIO.
        inline void restartDmaBeforeItStops(int32_t index)
        {
            uint32_t dmaChannel = m_dmaChannels[index];
            uint32_t dmaTransfersLeft = dma_channel_hw_addr(dmaChannel)->transfer_count;
            if (dmaTransfersLeft > DMA_REFRESH_THRESHOLD)
            {
                // There are still a lot of transfers left in this DMA operation so just return.
                return;
            }

            // Stopping the DMA channel and starting it again will cause it to use all of the original settings,
            // including the 0xFFFFFFFF transfer count.
            dma_channel_abort(dmaChannel);
            dma_channel_start(dmaChannel);
        }

        PIO                 m_pio;
        // The maximum quadrature encoders to be counted are limited by the number of state machines in the PIO.
        volatile uint32_t   m_counters[NUM_PIO_STATE_MACHINES];
        uint32_t            m_dmaChannels[NUM_PIO_STATE_MACHINES];

};

#endif // QUADRATURE_DECODER_H_
