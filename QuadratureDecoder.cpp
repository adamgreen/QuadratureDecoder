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
#include "QuadratureDecoder.h"
#include <string.h>
#include <QuadratureDecoder.pio.h>



QuadratureDecoder::QuadratureDecoder()
{
    m_pio = pio0;
    memset((void*)m_counters, 0, sizeof(m_counters));
    memset(m_dmaChannels, 0, sizeof(m_dmaChannels));
}

bool QuadratureDecoder::init(PIO pio)
{
    if (!pio_can_add_program(pio, &QuadratureDecoder_program))
    {
        return false;
    }

    m_pio = pio;
    pio_add_program(m_pio, &QuadratureDecoder_program);

    return true;
}

int32_t QuadratureDecoder::addQuadratureEncoder(uint32_t pinBase)
{
    // Find an unused state machine in the PIO to run the code for counting this encoder.
    int32_t stateMachine = pio_claim_unused_sm(m_pio, false);
    if (stateMachine < 0)
    {
        // No free state machines so return a failure code.
        return -1;
    }

    // The code is always loaded at offset 0 because of its jump table.
    const uint programOffset = 0;
    pio_sm_config smConfig = QuadratureDecoder_program_get_default_config(programOffset);

    // Configure the state machine to run the quadrature decoder program.
    const bool shiftLeft = false;
    const bool noAutoPush = false;
    const uint threshhold = 32;
    // We want the ISR to shift to right when the pin values are shifted in.
    sm_config_set_in_shift(&smConfig, shiftLeft, noAutoPush, threshhold);
    sm_config_set_in_pins(&smConfig, pinBase);
    // Use the TX FIFO entries for RX since we don't use the TX path. This makes for an 8 element RX FIFO.
    sm_config_set_fifo_join(&smConfig, PIO_FIFO_JOIN_RX);
    pio_sm_init(m_pio, stateMachine, QuadratureDecoder_offset_start, &smConfig);

    int dmaChannel = dma_claim_unused_channel(false);
    if (dmaChannel < 0)
    {
        // No free DMA channels so return a failure code.
        return -1;
    }
    m_dmaChannels[stateMachine] = dmaChannel;

    // Configure DMA to just read the latest count from the state machine's RX FIFO and place it in the m_counters[]
    // element reserved for this encoder.
    dma_channel_config dmaConfig = dma_channel_get_default_config(dmaChannel);
    channel_config_set_read_increment(&dmaConfig, false);
    channel_config_set_write_increment(&dmaConfig, false);
    channel_config_set_dreq(&dmaConfig, pio_get_dreq(m_pio, stateMachine, false));

    volatile uint32_t* pCounter = &m_counters[stateMachine];
    dma_channel_configure(dmaChannel, &dmaConfig,
        pCounter,           // Destination pointer
        &m_pio->rxf[stateMachine],      // Source pointer
        DMA_MAX_TRANSFER_COUNT,         // Largest possible number of transfers
        true                // Start immediately
    );

    // Initialize state machine registers.
    // Initialize the X register to an initial count of 0.
    *pCounter = 0;
    pio_sm_exec(m_pio, stateMachine, pio_encode_set(pio_x, *pCounter));
    // Initialize the Y register to the current value of the pins.
    pio_sm_exec(m_pio, stateMachine, pio_encode_mov(pio_y, pio_pins));

    // Now start the state machine to count quadrature encoder ticks.
    pio_sm_set_enabled(m_pio, stateMachine, true);

    return stateMachine;
}
