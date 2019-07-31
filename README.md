# bitfield-functor
# Summary

Embedded programing proof of concept: bitfields with functors

 My motivations for conducting this exercise was to experiment with using functors for twiddling bits in a GPIO register. I haven't noticed anyone doing this and I was just interested in seeing how this approach would work out.
# Extended Commentary
Normally hiding bit-twiddling would just mean abstracting such code into one or more functions.  This time I wanted to abstract such code into functors instead of functions.  At the call site both functions and functors would appear indistinguishable from each other.

In addition, this time I wanted to use bit-fields instead of the "raw bits" approach (i.e., bitwise ANDing,ORing and so on.)  NB: I've always understood bit-fields perfectly well, it's just that everyone else almost always chose to go with the "raw bits" technique.  That said, I've always considered  bit-fields to be a more natural looking way to twiddle bits in a register.

 So, in short, I had the following objectives:

1. I wanted to work with functors
2. I wanted to work with partial template specialization
3. I wanted to indulge my preference for using bit-fields instead of the "raw bits" technique
4. I wanted to discover how much extra code, if any, is required to go the generic programming template route.

## The two applications for this template/bitfield scheme
 There are two scenarios I wanted to cover in this exercise:

1. functors for a GPIO register that is part of a custom design (e.g., a custom controller board); and
2. functors for the registers of an off-the-shelf IC (e.g., NEED_IC_ID)
## Regarding this example's GPIO register
This GPIO register controls:
1. two solenoids, each of which operate a valve on a vaccum line; and
2. a lamp's brightness.

The audence should assume the necessary external circuitry is implemented. For example:
1. Assume that the bits associated with controlling the vaccum solenoids are connected to circuity (relay drivers, most likely) suitable for controlling power to a solenoid's coil.
2. Assume that the bits associated with controlling the lights are connected to a DAC, followed by suitable power control circuitry for modulating the power applied to the lamp. 
3. Assume that the lamp's illumination is exactly proportional to power applied (which cannot be expected with incandescent lamps.)
## Regarding the template for the GPIO register

The irony of using a template to operate on a GPIO register hardwired to a custom design can be summed up in two points:

1. The point of templates is to facilitate generic programming;
2. I cannot conceive of a way to abstract bitfield usage in such a way that one template can accomodate all designs for using a GPIO register.

Put another way, the highly hardware implementation specific concerns of register bit-twiddling defeats generic programming's intent of code reuse.  Consequently, the template code for something like a GPIO register will be a one-off, unreusable code.

Unresusable code means that you're not much better off than writing bit manipulation functions in C as compared to templates, at least with respect to the number of lines of code.

This means you cannot expect to reuse functors created for customizeable registers on other, unreleated, designs. Of course this code might be reusable should the design team reuse the hardware design pertaining to the GPIO register, say, on another version of the control board.

## Specializing the templates for the GPIO register's named bitfields 
This example's GPIO register's bitfield is defined:

```
struct genpurpIO_register23
{
     std::uint16_t energize_vac_solenoid2 : 1;    // set to 1 to apply vacuum
     std::uint16_t energize_vac_solenoid3 : 1;    // set to 1 to apply vacuum
     std::uint16_t lamp_pwr               : 3;    // 0 == lights off; 7 == max illumination
     std::uint16_t                        : 11;   // fill to 16 bits
};
```
I will refer to the fields with identifiers (meaning all fields in the bitfield execpt for the _filler_ field) as _named fields_. 

Each functor is implemented via a partial template specialization. Each functor manages exactly one named field. Each functor is defined in its own _partial template specialization_.  Consequently, each named field gets its own partial template specialization.

These partial template specialization only have a single parameter: a unique type only used to select it from the set of available partial template specializations. 

The address of the GPIO register is passed via the functor's constructor (RAII). Afterward the functor's sole parameter is value used to update the field managed by the given functor.  

## About portabilty:
The layout of the field within a bitfield is implementation dependent. Consequently, this code cannot be expected to be portable between C++ compilers.

This is probably OK for the purposes of embedded programming b/c in the varous shops in which I worked portablity wasn't a concern because each shop used only used one particular C++ compiler for the particular microcontroller they were using.


## Annoyances:

1. Each field within the bit-field requires its own partial specialization of the class template, even if there are two fields with exactly the same usage (scenario: each of the two vacuum solenoids had a distinct field.) This annoyed because it meant code duplication, which defeats the recommendation to keep your code DRY.

2. I needed to introduce extra types just to create distinct partial specialization for each named field. The field names themselves were inadequate for this purpose because they were the same type (std::uint16_t) and each partial specialization requires a distinct type.  This annoyed because I had to declare additional, otherwise pointless type just because of the way partial specialization works.

## Tradeoffs: creating a partial specialization of a class template for each field vs creating a class with a method for each field.

1. Implementing this via, say, a class would not help the code duplication concern because the implementation of the methods for the vacuum solenoids would still mean two distinct member functions, one for each vacuum solenoid. (wash)
2. Implementing this via, say, a class would remove the need to create types just for the purpose of creating a distinct partial specialization for each solenoid.  (class wins)
3. Each functors must be individually instantiated while a class need only be instantiated once. (class wins)
4. The call to the functor() appears to be slightly less complex than a call to a class member function. For example
````
   obj.vac_solenoid2(vacuum::ON);
vs
   vac_solenoid2(vacuum::ON);

````
Consequently, a functor call appears to be indistinguishable from calling a C function. (And does anything appears simpler than a C function call?)
(templates win)

Conclusion:

I cannot, with a straight face, make an argument that either approach has a particularly strong advantage.

That said, one important design principle is that it is permissible to make the implementation somewhat more complex in exchange for simplifying the iterface. This consideration becomes more compelling if the usage of a functor occurs many more times that the effort required to setup a functor.

Consequently, I am willing to rationalize that functors are a more preferred implementation because calling functors appears as simple as calling a C function.

## Regarding this example's NEED_IC_ID 

  In the case of an given mass-manufactured device, one that is not
  field programmable so that its registers usage is customizable,
  as in the case of NEED_IC_ID, the said device's register layouts will always
  be the same.  In this sort of case code reuse is practical.


[Need to add commentary regarding NEED_IC_ID]


# Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.
Prerequisites

# What things you need to install the software and how to install them

The repro consists of the template for the example's GPIO register and a unit test.  The build environment is designed around Linux. You will need g++ and gmake installed. 

# Installing

Clone this repo to your local machine using https://github.com/Buto/bitfield-functor

To build and execute the unit test:
````
$ make all
g++ -std=c++17 ut_bitfield.cpp -o ut_bitfield.exe

bitfield UT passed
````
The unit tests provides a number of real-life examples of using the functors.  

The following is an example of instantating and then using a functor to apply vacuum:
````
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

    // if the current state of vaccuum solenoid #2 is energized
    if (vac_solenoid2() == vacuum::ON)
    {
        std::cout << "Works!" << std::endl;
    }
    else // the solenoid was incorrectly not energized
    {
        std::cerr << "Error: functor failed to set the bit for energizing vaccuum solenoid #2." << std::endl;
    }

    return 0;
}
````

# Running the tests

I am using the unit test scheme described in a book by Brian W. Kernighan and Rob Pike, _The Practice of Programming_.  This technique involves:
1. the unit test sending the results of the unit test to stdout; and
2. redirecting this output to a text file; and
3. comparing this output to a file containing the output from a successful run of the unit test 

The makefile both builds the unit test and automatically runs it when 'make all' is issued at the command line. The makefile will emit "bitfield UT FAILED!" if the code under test fails its unit test. 
Otherwise the makefile will emit "bitfield UT passed"

The following shows the results of a successful unit test run:
````
$ ./ut_bitfield.exe
ut00: verifing that the ctor initialized solenoid2 to vacuum:OFF.....................................ok
ut01: verifing that the ctor initialized solenoid3 to vacuum:OFF.....................................ok
ut02: verifing that the ctor initialized lamp to LIGHTS_OUT..........................................ok
ut03: verifing that the solenoid2's functor can return solenoid state prior to the functor call......ok
ut03: verifing that the solenoid2's functor can energize solenoid2...................................ok
ut04: verifing that the solenoid3's functor can return solenoid state prior to the functor call......ok
ut04: verifing that the solenoid3's functor can energize solenoid3...................................ok
ut05: verifing that the solenoid2's functor can return solenoid state prior to the functor call......ok
ut05: verifing that the solenoid2's functor can de-energize solenoid2................................ok
ut06: verifing that the solenoid3's functor can return solenoid state prior to the functor call......ok
ut06: verifing that the solenoid3's functor can de-energize solenoid3................................ok
ut07: verifing that the lamp's functor can return solenoid state prior to the functor call...........ok
ut07: verifing that functor can set the lamp to max power............................................ok
ut08: verifing that the lamp's functor can return lamp state prior to the functor call...............ok
ut08: Walking 1's testing. power bit pattern == 0B100................................................ok
ut09: verifing that the lamp's functor can return lamp state prior to the functor call...............ok
ut09: Walking 1's testing. power bit pattern == 0B010................................................ok
ut10: verifing that the lamp's functor can return lamp state prior to the functor call...............ok
ut10: Walking 1's testing. power bit pattern == 0B001................................................ok
ut11: verifing that the lamp's functor can return lamp state prior to the functor call...............ok
ut11: Verify that functor can remove power from lamp.................................................ok
ut12: Verifing lamp_pwr functor throws 'Out of Range' exception......................................ok
ut12: Verifing 'Out of Range' exception's error message is as expected...............................ok

UNIT TEST passed!
````

# Author

    John Hendrix 

See also the list of contributors who participated in this project.
# License

This project is licensed under the MIT License - see the LICENSE.md file for details
# Acknowledgments

    the code for the rtrim() function was lifted from  https://stackoverflow.com/a/217605
    My thanks to the author for this answer, jotik
