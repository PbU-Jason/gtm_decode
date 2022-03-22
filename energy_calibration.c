#include "energy_calibration.h"
#include "match_pattern.h"

void update_energy_from_adc(void)
{
    // no calibration for now
    event_buffer->energy = event_buffer->adc_value;
}
