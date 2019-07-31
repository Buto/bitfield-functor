#include <iostream>

#include "bitfield.h"

int main(int, char*[])
{
    //-----------------------------------
    //
    //      Setup a named constant storing the GPIO register's address for the
    //      purposes of this demo
    //
    //      In real life hardware registers will exist at an address assigned by
    //      someone on the hardware team. These addresses ought to be specified in
    //      as named constants (eliminating 'magic numbers' elsewhere in the code)
    //      via a project-wide header file. For example:
    //
    //          const unsigned REGISTER_ADDRESS_GPIO23 = 0x12345678;
    //
    //      Afterward instantating a functor would look something like:
    //
    //          gpio_register_23< solenoid2_t > vac_solenoid2{ reinterpret_cast<gpio_reg23_ptr_t>(REGISTER_ADDRESS_GPIO23) };
    //
    //      Now we mock-up a hardware register for this demo
    //
    static struct genpurpIO_register23 mock_reg23;   // This struc is masquerading as GPIO register #23

    // create a named constant for storing the "register's" address
    const gpio_reg23_ptr_t REGISTER_ADDRESS_GPIO23 = &mock_reg23;
    //-----------------------------------

    // create functor for controlling vacuum solenoid #2
    gpio_register_23< solenoid2_t > vac_solenoid2{ reinterpret_cast<gpio_reg23_ptr_t>(REGISTER_ADDRESS_GPIO23) };

    // energize vaccuum solenoid #2, applying vacuum to....something
    vac_solenoid2(vacuum::ON);

    // if the current state of vaccuum solenoid #2 is not energized
    if (vac_solenoid2() == vacuum::ON)
    {
        std::cout << "Works!" << std::endl;
    }
    else
    {
        std::cerr << "Error: functor failed to set the bit for energizing vaccuum solenoid #2." << std::endl;
    }

    return 0;
}

